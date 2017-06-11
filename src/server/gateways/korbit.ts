import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import Config = require("../config");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import Utils = require("../utils");
import util = require("util");
import Interfaces = require("../interfaces");
import moment = require("moment");
import * as Promises from '../promises';

function getJSON<T>(url: string, qs?: any) : Promise<T> {
    return new Promise((resolve, reject) => {
        request({url: url, qs: qs}, (err: Error, resp, body) => {
            if (err) {
                reject(err);
            }
            else {
                try {
                    resolve(JSON.parse(body));
                }
                catch (e) {
                    reject(e);
                }
            }
        });
    });
}

interface KorbitMessageIncomingMessage {
    channel: string;
    success: boolean;
    data: any;
    event?: string;
    errorcode: number;
    order_id: string;
}

interface KorbitDepthMessage {
    asks: [number, number][];
    bids: [number, number][];
    timestamp: string;
}

interface OrderAck {
    result: boolean;
    order_id: number;
}

interface SignedMessage {
    client_id?: string;
    client_secret?: string;
    refresh_token?: string;
    price?: string;
    nonce?: number;
    type?: string;
    coin_amount?: string;
    grant_type?: string;
    username?: string;
    password?: string;
    currency_pair?: string;
    category?: string;
    time?: string;
}

interface TokenMessage {
    access_token: string;
    refresh_token: string;
}

interface Order extends SignedMessage {
    symbol: string;
    type: string;
    price: string;
    amount: string;
}

interface Cancel extends SignedMessage {
    id?: string;
    currency_pair?: string;
}

interface SubscriptionRequest extends SignedMessage { }

class KorbitMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private triggerMarketTrades = () => {
        this._http.get("transactions", {currency_pair: this._symbolProvider.symbol, time: 'minute'}, true).then(msg => {
            if (!(<any>msg.data).length) return;
            for (let i = (<any>msg.data).length;i--;) {
              var px = parseFloat((<any>msg.data)[i].price);
              var amt = parseFloat((<any>msg.data)[i].amount);
              var side = Models.Side.Ask;
              var mt = new Models.GatewayMarketTrade(px, amt, msg.time, false, side);
              this.MarketTrade.trigger(mt);
            }
        });
    };

    MarketData = new Utils.Evt<Models.Market>();

    private static GetLevel = (n: [any, any]) : Models.MarketSide =>
        new Models.MarketSide(parseFloat(n[0]), parseFloat(n[1]));

    private triggerOrderBook = () => {
        this._http.get("orderbook", {currency_pair: this._symbolProvider.symbol, category: 'all'}, true).then(msg => {
            if (!(<any>msg.data).timestamp) return;
            this.MarketData.trigger(new Models.Market(
              (<any>msg.data).bids.slice(0,13).map(KorbitMarketDataGateway.GetLevel),
              (<any>msg.data).asks.slice(0,13).map(KorbitMarketDataGateway.GetLevel),
              msg.time
            ));
        });
    };

    constructor(private _http : KorbitHttp, private _symbolProvider: KorbitSymbolProvider) {
        setInterval(this.triggerMarketTrades, 60000);
        setInterval(this.triggerOrderBook, 1000);
        setTimeout(()=>this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 10);
    }
}

class KorbitOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = (): string => new Date().valueOf().toString().substr(-9);

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => {
        var d = Promises.defer<number>();
        this._http.get("user/orders/open", <Cancel>{currency_pair: this._symbolProvider.symbol }).then(msg => {
          if (!(<any>msg.data).length) { d.resolve(0); return; }
          (<any>msg.data).map((o) => {
              this._http.post("user/orders/cancel", <Cancel>{id: o.id, currency_pair: this._symbolProvider.symbol, nonce: new Date().getTime() }).then(msg => {
                  if (!(<any>msg.data).length) return;
                  this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
                    exchangeId: (<any>msg.data).order_id.toString(),
                    leavesQuantity: 0,
                    time: msg.time,
                    orderStatus: Models.OrderStatus.Cancelled
                  });
              });
          });
          d.resolve((<any>msg.data).orders.length);
        });
        return d.promise;
    };

    public cancelsByClientOrderId = false;

    sendOrder = (order : Models.OrderStatusReport) => {
        this._http.post('user/orders/'+(order.side==Models.Side.Bid?'buy':'sell'), {
            type: order.type === Models.OrderType.Market ? 'market' : 'limit',
            price: order.price.toString(),
            coin_amount: order.quantity.toString(),
            currency_pair: this._symbolProvider.symbol,
            nonce: new Date().getTime()
        }).then(msg => {
          if (!(<any>msg.data).orderId || ((<any>msg.data).status && (<any>msg.data).status != 'success')) {
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              orderId: order.orderId,
              leavesQuantity: 0,
              time: (<any>msg.data).time,
              orderStatus: Models.OrderStatus.Cancelled
            });
            if ((<any>msg.data).status)
              console.info(new Date().toISOString().slice(11, -1), 'korbit', 'order error:', (<any>msg.data).status);
          }
          else
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              orderId: order.orderId,
              computationalLatency: new Date().valueOf() - (<any>msg.time).valueOf(),
              time: (<any>msg.data).time,
              exchangeId: (<any>msg.data).orderId,
              orderStatus: Models.OrderStatus.Working,
              leavesQuantity: order[1]
            });
        });
    };

    cancelOrder = (cancel : Models.OrderStatusReport) => {
        this._http.post('user/orders/cancel', <Cancel>{id: cancel.exchangeId, currency_pair: this._symbolProvider.symbol, nonce: new Date().getTime() }).then(msg => {
            if (!(<any>msg.data).length!) return;
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              orderId: cancel.orderId,
              leavesQuantity: 0,
              time: cancel.time,
              orderStatus: Models.OrderStatus.Cancelled
            });
        });
    };

    replaceOrder = (replace : Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private triggerUserTades = () => {
        this._http.get('user/transactions', {currency_pair: this._symbolProvider.symbol, category: 'fills'}).then(msg => {
            if (!(<any>msg.data).length) return;
            for (let i = (<any>msg.data).length;i--;) {
              var px = parseFloat((<any>msg.data)[i].price);
              var trade : any = (<any>msg.data)[i];
              var status : Models.OrderStatusUpdate = {
                  exchangeId: trade.orderId.toString(),
                  orderStatus: Models.OrderStatus.Complete,
                  time: new Date(trade.timestamp),
                  side: trade.type.indexOf('buy')>-1 ? Models.Side.Bid : Models.Side.Ask,
                  lastQuantity: trade.fillsDetail.amount.value,
                  lastPrice: trade.fillsDetail.price.value,
                  averagePrice: trade.fillsDetail.price.value
              };
              this.OrderUpdate.trigger(status);
            }
        });
    };

    constructor(
            private _http: KorbitHttp,
            private _symbolProvider: KorbitSymbolProvider) {
        setInterval(this.triggerUserTades, 10000);
        setTimeout(()=>this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 10);
    }
}

class KorbitMessageSigner {
    private _client_id : string;
    private _secretKey : string;
    private _user : string;
    private _pass : string;
    public token : string;
    private _token_refresh : string;
    private _token_time : number = 0;

    public signMessage = async (_http: KorbitHttp, m : SignedMessage) : Promise<SignedMessage> => {
        if (!this._token_time) {
          var d = new Promise((resolve, reject) => {
            _http.post('oauth2/access_token', {
              client_id: this._client_id,
              client_secret: this._secretKey,
              username: this._user,
              password: this._pass,
              grant_type: 'password'
            }, false, true).then(newToken => {
              resolve(newToken);
            });
          });
          const newToken: any = await d;
          this._token_time = (parseFloat(newToken.data.expires_in) * 1000 ) + new Date().getTime();
          this.token = newToken.data.access_token;
          this._token_refresh = newToken.data.refresh_token;
          console.info(new Date().toISOString().slice(11, -1), 'korbit', 'Authentication successful, new token expires at '+(new Date(this._token_time).toISOString().slice(11, -1))+'.');
        } else if (new Date().getTime()+60>this._token_time) {
          d = new Promise((resolve, reject) => {
            _http.post('oauth2/access_token', {
              client_id: this._client_id,
              client_secret: this._secretKey,
              refresh_token: this._token_refresh,
              grant_type: 'refresh_token'
            }, false, true).then(refreshToken => {
              resolve(refreshToken);
            });
          });
          const refreshToken: any = await d;
          this._token_time = (parseFloat(refreshToken.data.expires_in) * 1000 ) + new Date().getTime();
          this.token = refreshToken.data.access_token;
          this._token_refresh = refreshToken.data.refresh_token;
          console.info(new Date().toISOString().slice(11, -1), 'korbit', 'Authentication refresh successful, new token expires at '+(new Date(this._token_time).toISOString().slice(11, -1))+'.');
        }

        return m;
    };

    public toQueryString = (msg: SignedMessage) : string => {
        var els : string[] = [];
        var keys = [];
        for (var key in msg) {
            if (msg.hasOwnProperty(key))
                keys.push(key);
        }
        for (var i = 0; i < keys.length; i++) {
            const k = keys[i];
            if (msg.hasOwnProperty(k))
                els.push(k + "=" + msg[k]);
        }
        return els.join("&");
    }

    constructor(config : Config.ConfigProvider) {
        this._client_id = config.GetString("KorbitApiKey");
        this._secretKey = config.GetString("KorbitSecretKey");
        this._user = config.GetString("KorbitUsername");
        this._pass = config.GetString("KorbitPassword");
    }
}

class KorbitHttp {
    post = async <T>(
      actionUrl: string,
      msg : SignedMessage,
      publicApi?: boolean,
      forceUnsigned?: boolean
    ) : Promise<Models.Timestamped<T>> => {
        var d = Promises.defer<Models.Timestamped<T>>();
        request({
            url: url.resolve(this._baseUrl+'/', actionUrl),
            body: this._signer.toQueryString((publicApi || forceUnsigned) ? msg : await this._signer.signMessage(this, msg)),
            headers: Object.assign(publicApi?{}:{"Content-Type": "application/x-www-form-urlencoded"}, (publicApi || !this._signer.token)?{}:{'Authorization': 'Bearer '+this._signer.token}),
            method: 'POST'
        }, (err, resp, body) => {
            if (err) d.reject(err);
            else {
                try {
                    var t = new Date();
                    var data = body ? JSON.parse(body) : {};
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    console.error(new Date().toISOString().slice(11, -1), 'korbit', err, 'url:', actionUrl, 'err:', err, 'body:', body);
                    d.reject(e);
                }
            }
        });

        return d.promise;
    };

    get = async <T>(
      actionUrl: string,
      msg : SignedMessage,
      publicApi?: boolean
    ) : Promise<Models.Timestamped<T>> => {
        var d = Promises.defer<Models.Timestamped<T>>();
        request({
            url: url.resolve(this._baseUrl+'/', actionUrl+'?'+ this._signer.toQueryString(publicApi ? msg : await this._signer.signMessage(this, msg))),
            headers: (publicApi || !this._signer.token)?{}:{'Authorization': 'Bearer '+this._signer.token},
            method: 'GET'
        }, (err, resp, body) => {
            if (err) d.reject(err);
            else {
                try {
                    var t = new Date();
                    var data = body ? JSON.parse(body) : {};
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    console.error(new Date().toISOString().slice(11, -1), 'korbit', err, 'url:', actionUrl, 'err:', err, 'body:', body);
                    d.reject(e);
                }
            }
        });

        return d.promise;
    };

    private _baseUrl : string;
    constructor(config : Config.ConfigProvider, private _signer: KorbitMessageSigner) {
        this._baseUrl = config.GetString("KorbitHttpUrl")
    }
}

class KorbitPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private trigger = () => {
        this._http.get("user/wallet", {currency_pair: this._symbolProvider.symbol}).then(msg => {
            if (!(<any>msg.data).balance) return;

            var free = (<any>msg.data).available;
            var freezed = (<any>msg.data).pendingOrders;
            var wallet = [];
            free.forEach(x => { wallet[x.currency] = [x.currency, x.value]; });
            freezed.forEach(x => { wallet[x.currency].push(x.value); });
            for (let x in wallet) {
                var amount = parseFloat(wallet[x][1]);
                var held = parseFloat(wallet[x][2]);

                var pos = new Models.CurrencyPosition(amount, held, Models.toCurrency(wallet[x][0]));
                this.PositionUpdate.trigger(pos);
            }
        });
    };

    constructor(
      private _http : KorbitHttp,
      private _symbolProvider: KorbitSymbolProvider
    ) {
        setInterval(this.trigger, 15000);
        setTimeout(this.trigger, 3000);
    }
}

class KorbitBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    name() : string {
        return "Korbit";
    }

    makeFee() : number {
        return 0.001;
    }

    takeFee() : number {
        return 0.002;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.Korbit;
    }

    constructor(public minTickIncrement: number, public minSize: number) {}
}

class KorbitSymbolProvider {
    public symbol: string;
    public symbolReversed: string;
    public symbolQuote: string;

    constructor(pair: Models.CurrencyPair) {
        const GetCurrencySymbol = (s: Models.Currency) : string => Models.fromCurrency(s).toLowerCase();
        this.symbol = GetCurrencySymbol(pair.base) + "_" + GetCurrencySymbol(pair.quote);
        this.symbolReversed = GetCurrencySymbol(pair.quote) + "_" + GetCurrencySymbol(pair.base);
        this.symbolQuote = GetCurrencySymbol(pair.quote);
    }
}

class Korbit extends Interfaces.CombinedGateway {
    constructor(config : Config.ConfigProvider, pair: Models.CurrencyPair, minTick: number, minSize: number) {
        var symbol = new KorbitSymbolProvider(pair);
        var http = new KorbitHttp(config, new KorbitMessageSigner(config));

        var orderGateway = config.GetString("KorbitOrderDestination") == "Korbit"
            ? <Interfaces.IOrderEntryGateway>new KorbitOrderEntryGateway(http, symbol)
            : new NullGateway.NullOrderGateway();

        super(
            new KorbitMarketDataGateway(http, symbol),
            orderGateway,
            new KorbitPositionGateway(http, symbol),
            new KorbitBaseGateway(minTick, minSize)
        );
    }
}

export async function createKorbit(config : Config.ConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    const constants = await getJSON<any[]>(config.GetString("KorbitHttpUrl")+"/constants");
    let minTick = 500;
    let minSize = 0.015;
    for (let constant in constants)
      if (constant.toUpperCase()==Models.fromCurrency(pair.base)+'TICKSIZE')
          minTick = parseFloat(constants[constant]);
      // else if (constant.toUpperCase()=='MIN'+Models.fromCurrency(pair.base)+'ORDER')
          // minSize = parseFloat(constants[constant]);

    return new Korbit(config, pair, minTick, minSize);
}
