import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import util = require("util");
import Interfaces = require("../interfaces");

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

class KorbitOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    generateClientOrderId = (): string => new Date().valueOf().toString().substr(-9);

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => {
      return new Promise<number>((resolve, reject) => {
        this._http.get("user/orders/open", <Cancel>{currency_pair: this._gwSymbol }).then(msg => {
          if (!(<any>msg.data).length) { resolve(0); return; }
          (<any>msg.data).forEach((o) => {
              this._http.post("user/orders/cancel", <Cancel>{id: o.id, currency_pair: this._gwSymbol, nonce: new Date().getTime() }).then(msg => {
                  if (!(<any>msg.data).length) return;
                  this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
                    exchangeId: (<any>msg.data).order_id.toString(),
                    leavesQuantity: 0,
                    time: msg.time.getTime(),
                    orderStatus: Models.OrderStatus.Cancelled
                  });
              });
          });
          resolve((<any>msg.data).orders.length);
        });
      });
    };

    public cancelsByClientOrderId = false;

    sendOrder = (order : Models.OrderStatusReport) => {
        this._http.post('user/orders/'+(order.side==Models.Side.Bid?'buy':'sell'), {
            type: order.type === Models.OrderType.Market ? 'market' : 'limit',
            price: order.price.toString(),
            coin_amount: order.quantity.toString(),
            currency_pair: this._gwSymbol,
            nonce: new Date().getTime()
        }).then(msg => {
          if (!(<any>msg.data).orderId || ((<any>msg.data).status && (<any>msg.data).status != 'success')) {
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
              orderId: order.orderId,
              leavesQuantity: 0,
              time: new Date().getTime(),
              orderStatus: Models.OrderStatus.Cancelled
            });
            if ((<any>msg.data).status)
              console.info(new Date().toISOString().slice(11, -1), 'korbit', 'order error:', (<any>msg.data).status);
          }
          else
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
              orderId: order.orderId,
              time: new Date().getTime(),
              exchangeId: (<any>msg.data).orderId,
              orderStatus: Models.OrderStatus.Working,
              leavesQuantity: order[1]
            });
        });
    };

    cancelOrder = (cancel : Models.OrderStatusReport) => {
        this._http.post('user/orders/cancel', <Cancel>{id: cancel.exchangeId, currency_pair: this._gwSymbol, nonce: new Date().getTime() }).then(msg => {
            if (!(<any>msg.data).length!) return;
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
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
        this._http.get('user/transactions', {currency_pair: this._gwSymbol, category: 'fills'}).then(msg => {
            if (!(<any>msg.data).length) return;
            for (let i = (<any>msg.data).length;i--;) {
              var px = parseFloat((<any>msg.data)[i].price);
              var trade : any = (<any>msg.data)[i];
              if (['buy','sell'].indexOf(trade.type)===-1) continue;
              var status : Models.OrderStatusUpdate = {
                  exchangeId: trade.fillsDetail.orderId,
                  orderStatus: Models.OrderStatus.Complete,
                  time: new Date(trade.timestamp).getTime(),
                  side: trade.type.indexOf('buy')>-1 ? Models.Side.Bid : Models.Side.Ask,
                  lastQuantity: trade.fillsDetail.amount.value,
                  lastPrice: trade.fillsDetail.price.value,
                  averagePrice: trade.fillsDetail.price.value
              };
              this._evUp('OrderUpdateGateway', status);
            }
        });
    };

    constructor(
      private _evUp,
      private _http: KorbitHttp,
      private _gwSymbol
    ) {
        setInterval(this.triggerUserTades, 10000);
        setTimeout(()=>this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected), 10);
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

    public signMessage = async (_http: KorbitHttp, m: SignedMessage): Promise<SignedMessage> => {
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

        return Promise.resolve(m);
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

    constructor(cfString) {
        this._client_id = cfString("KorbitApiKey");
        this._secretKey = cfString("KorbitSecretKey");
        this._user = cfString("KorbitUsername");
        this._pass = cfString("KorbitPassword");
    }
}

class KorbitHttp {
    post = async <T>(
      actionUrl: string,
      msg : SignedMessage,
      publicApi?: boolean,
      forceUnsigned?: boolean
    ) : Promise<Models.Timestamped<T>> => {
      return new Promise<Models.Timestamped<T>>(async (resolve, reject) => {
        request({
            url: url.resolve(this._baseUrl+'/', actionUrl),
            body: this._signer.toQueryString((publicApi || forceUnsigned) ? msg : await this._signer.signMessage(this, msg)),
            headers: Object.assign(publicApi?{}:{"Content-Type": "application/x-www-form-urlencoded"}, (publicApi || !this._signer.token)?{}:{'Authorization': 'Bearer '+this._signer.token}),
            method: 'POST'
        }, (err, resp, body) => {
            if (err) reject(err);
            else {
                try {
                    var t = new Date();
                    var data = body ? JSON.parse(body) : {};
                    resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    console.error(new Date().toISOString().slice(11, -1), 'korbit', err, 'url:', actionUrl, 'err:', err, 'body:', body);
                    reject(e);
                }
            }
        });
      });
    };

    get = async <T>(
      actionUrl: string,
      msg : SignedMessage,
      publicApi?: boolean
    ) : Promise<Models.Timestamped<T>> => {
      return new Promise<Models.Timestamped<T>>(async (resolve, reject) => {
        request({
            url: url.resolve(this._baseUrl+'/', actionUrl+'?'+ this._signer.toQueryString(publicApi ? msg : await this._signer.signMessage(this, msg))),
            headers: (publicApi || !this._signer.token)?{}:{'Authorization': 'Bearer '+this._signer.token},
            method: 'GET'
        }, (err, resp, body) => {
            if (err) reject(err);
            else {
                try {
                    var t = new Date();
                    var data = body ? JSON.parse(body) : {};
                    resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    console.error(new Date().toISOString().slice(11, -1), 'korbit', err, 'url:', actionUrl, 'err:', err, 'body:', body);
                    reject(e);
                }
            }
        });
      });
    };

    private _baseUrl : string;
    constructor(cfString, private _signer: KorbitMessageSigner) {
        this._baseUrl = cfString("KorbitHttpUrl")
    }
}

export class Korbit extends Interfaces.CombinedGateway {
    constructor(
      gwSymbol,
      cfString,
      _evOn,
      _evUp
    ) {
        var http = new KorbitHttp(cfString, new KorbitMessageSigner(cfString));

        var orderGateway = cfString("KorbitOrderDestination") == "Korbit"
            ? <Interfaces.IOrderEntryGateway>new KorbitOrderEntryGateway(_evUp, http, gwSymbol)
            : new NullGateway.NullOrderGateway(_evUp);

        super(
            orderGateway
        );
    }
}
