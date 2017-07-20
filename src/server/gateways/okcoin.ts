import ws = require('uws');
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import util = require("util");
import Interfaces = require("../interfaces");

interface OkCoinMessageIncomingMessage {
    channel: string;
    success: boolean;
    data: any;
    event?: string;
    errorcode: number;
    order_id: string;
}

interface OkCoinDepthMessage {
    asks: [number, number][];
    bids: [number, number][];
    timestamp: string;
}

interface OrderAck {
    result: boolean;
    order_id: number;
}

interface SignedMessage {
    api_key?: string;
    sign?: string;
}

interface Order extends SignedMessage {
    symbol: string;
    type: string;
    price: string;
    amount: string;
}

interface Cancel extends SignedMessage {
    order_id: string;
    symbol: string;
}

interface OkCoinTradeRecord {
    averagePrice: string;
    completedTradeAmount: string;
    createdDate: string;
    id: string;
    orderId: string;
    sigTradeAmount: string;
    sigTradePrice: string;
    status: number;
    symbol: string;
    tradeAmount: string;
    tradePrice: string;
    tradeType: string;
    tradeUnitPrice: string;
    unTrade: string;
}

class OkCoinWebsocket {
	send = <T>(channel : string, parameters: any, cb?: () => void) => {
        var subsReq : any = {event: 'addChannel', channel: channel};

        if (parameters !== null)
            subsReq.parameters = parameters;

        this._ws.send(JSON.stringify(subsReq), (e: Error) => {
            if (!e && cb) cb();
        });
    }

    setHandler = <T>(channel : string, handler: (newMsg : Models.Timestamped<T>) => void) => {
        this._handlers[channel] = handler;
    }

    private onMessage = (raw : string) => {
        var t = new Date();
        try {
            var msg : OkCoinMessageIncomingMessage = JSON.parse(raw)[0];
            if (typeof msg === "undefined") msg = JSON.parse(raw);
            if (typeof msg === "undefined") throw new Error("Unkown message from OkCoin socket: " + raw);

            if (typeof msg.event !== "undefined" && msg.event == "ping") {
                this._ws.send(this._serializedHeartbeat);
                return;
            }
            if (typeof msg.event !== "undefined" && msg.event == "pong") {
                this._stillAlive = true;
                return;
            }

            let channel: string = typeof msg.channel !== 'undefined' ? msg.channel : msg.data.channel;
            let success: boolean = typeof msg.success !== 'undefined' ? msg.success : (typeof msg.data !== 'undefined' && typeof msg.data.result !== 'undefined' ? msg.data.result : true);
            let errorcode: number = typeof msg.errorcode !== 'undefined' ? msg.errorcode : msg.data.error_code;

            if (!success && (typeof errorcode === "undefined" || (
              errorcode != 20100 /* 20100=request time out */
              && errorcode != 10002 /* 10002=System error */
              && errorcode != 10050 /* 10050=Can't cancel more than once */
              && errorcode != 10009 /* 10009=Order does not exist */
              && errorcode != 10010 /* 10010=Insufficient funds */
              && errorcode != 10016 /* 10016=Insufficient coins balance */
              // errorcode != 10001 /* 10001=Request frequency too high */
            ))) console.warn(new Date().toISOString().slice(11, -1), 'okcoin', 'Unsuccessful message received:', raw);
            else if (success && (channel == 'addChannel' || channel == 'login'))
              return console.info(new Date().toISOString().slice(11, -1), 'okcoin', 'Successfully connected to', channel + (typeof msg.data.channel !== 'undefined' ? ': '+msg.data.channel : ''));
            if (typeof errorcode !== "undefined" && (
              errorcode == 20100
              || errorcode == 10002
              || errorcode == 10050
              || errorcode == 10009
              // || errorcode == '10001'
            ))  return;

            var handler = this._handlers[channel];

            if (typeof handler === "undefined") {
                console.warn(new Date().toISOString().slice(11, -1), 'okcoin', 'Got message on unknown topic', msg);
                return;
            }

            handler(new Models.Timestamped(msg.data, t));
        }
        catch (e) {
            console.error(new Date().toISOString().slice(11, -1), 'okcoin', e, 'Error parsing msg', raw);
            throw e;
        }
    };

    private connectWS = (cfString) => {
        this._ws = new ws(cfString("OkCoinWsUrl"));
        this._ws.on("open", () => this._evUp('GatewaySocketConnect', Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("close", () => this._evUp('GatewaySocketConnect', Models.ConnectivityStatus.Disconnected));
    };

    private _serializedHeartping = JSON.stringify({event: "ping"});
    private _serializedHeartbeat = JSON.stringify({event: "pong"});
    private _stillAlive: boolean = true;
    private _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
    private _ws : ws;
    constructor(
      private _evUp,
      cfString
    ) {
        this.connectWS(cfString);
        setInterval(() => {
          if (!this._stillAlive) {
            console.warn(new Date().toISOString().slice(11, -1), 'okcoin', 'Heartbeat lost, reconnecting..');
            this._stillAlive = true;
            this.connectWS(cfString);
          } else this._stillAlive = false;
          this._ws.send(this._serializedHeartping);
        }, 21000);
    }
}

class OkCoinMarketDataGateway implements Interfaces.IMarketDataGateway {
    private onTrade = (trades : Models.Timestamped<[string,string,string,string,string][]>) => {
        trades.data.forEach(trade => {
          this._evUp('MarketTradeGateway', new Models.GatewayMarketTrade(
            parseFloat(trade[1]),
            parseFloat(trade[2]),
            trade[4] === "ask" ? Models.Side.Ask : Models.Side.Bid
          ));
        });
    };

    private static GetLevel = (n: [any, any]) : Models.MarketSide =>
        new Models.MarketSide(parseFloat(n[0]), parseFloat(n[1]));

    private onDepth = (depth : Models.Timestamped<OkCoinDepthMessage>) => {
        var msg = depth.data;

        var bids = msg.bids.slice(0,13).map(OkCoinMarketDataGateway.GetLevel);
        var asks = msg.asks.reverse().slice(0,13).map(OkCoinMarketDataGateway.GetLevel);

        this._evUp('MarketDataGateway', new Models.Market(bids, asks));
    };

    constructor(
      private _evOn,
      private _evUp,
      socket: OkCoinWebsocket,
      symbolProvider: OkCoinSymbolProvider
    ) {
        var depthChannel = "ok_sub_spot" + symbolProvider.symbolReversed + "_depth_20";
        var tradesChannel = "ok_sub_spot" + symbolProvider.symbolReversed + "_trades";
        socket.setHandler(depthChannel, this.onDepth);
        socket.setHandler(tradesChannel, this.onTrade);

        this._evOn('GatewaySocketConnect', cs => {
            this._evUp('GatewayMarketConnect', cs);

            if (cs == Models.ConnectivityStatus.Connected) {
                socket.send(depthChannel, null);
                socket.send(tradesChannel, null);
            }
        });
    }
}

class OkCoinOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    private chars: string = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    generateClientOrderId = (): string => {
      let id: string = '';
      for(let i=8;i--;) id += this.chars.charAt(Math.floor(Math.random() * this.chars.length));
      return id;
    };

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => {
      return new Promise<number>((resolve, reject) => {
        this._http.post("order_info.do", <Cancel>{order_id: '-1', symbol: this._symbolProvider.symbol }).then(msg => {
          if (typeof (<any>msg.data).orders == "undefined"
            || typeof (<any>msg.data).orders[0] == "undefined"
            || typeof (<any>msg.data).orders[0].order_id == "undefined") { resolve(0); return; }
          (<any>msg.data).orders.forEach((o) => {
              this._http.post("cancel_order.do", <Cancel>{order_id: o.order_id.toString(), symbol: this._symbolProvider.symbol }).then(msg => {
                  if (typeof (<any>msg.data).result == "undefined") return;
                  if ((<any>msg.data).result) {
                      this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
                        exchangeId: (<any>msg.data).order_id.toString(),
                        leavesQuantity: 0,
                        time: msg.time,
                        orderStatus: Models.OrderStatus.Cancelled
                      });
                  }
              });
          });
          resolve((<any>msg.data).orders.length);
        });
      });
    };

    public cancelsByClientOrderId = false;

    private static GetOrderType(side: Models.Side, type: Models.OrderType) : string {
        if (side === Models.Side.Bid) {
            if (type === Models.OrderType.Limit) return "buy";
            if (type === Models.OrderType.Market) return "buy_market";
        }
        if (side === Models.Side.Ask) {
            if (type === Models.OrderType.Limit) return "sell";
            if (type === Models.OrderType.Market) return "sell_market";
        }
        throw new Error("unable to convert " + Models.Side[side] + " and " + Models.OrderType[type]);
    }

    // let's really hope there's no race conditions on their end -- we're assuming here that orders sent first
    // will be acked first, so we can match up orders and their acks
    private _ordersWaitingForAckQueue = [];

    sendOrder = (order : Models.OrderStatusReport) => {
        var o : Order = {
            symbol: this._symbolProvider.symbol,
            type: OkCoinOrderEntryGateway.GetOrderType(order.side, order.type),
            price: order.price.toString(),
            amount: order.quantity.toString()};

        this._ordersWaitingForAckQueue.push([order.orderId, order.quantity]);

        this._socket.send<OrderAck>("ok_spot" + this._symbolProvider.symbolQuote + "_trade", this._signer.signMessage(o), () => {
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
                orderId: order.orderId,
                computationalLatency: new Date().valueOf() - order.time.valueOf()
            });
        });
    };

    private onOrderAck = (ts: Models.Timestamped<OrderAck>) => {
        var order = this._ordersWaitingForAckQueue.shift();

        var orderId = order[0];
        if (typeof orderId === "undefined") {
            console.error(new Date().toISOString().slice(11, -1), 'okcoin', 'got an order ack when there was no order queued!', util.format(ts.data));
            return;
        }

        var osr : Models.OrderStatusUpdate = { orderId: orderId, time: ts.time };

        if (typeof ts.data !== "undefined" && ts.data.result) {
            osr.exchangeId = ts.data.order_id.toString();
            osr.orderStatus = Models.OrderStatus.Working;
            osr.leavesQuantity = order[1];
        }
        else {
            osr.orderStatus = Models.OrderStatus.Cancelled;
        }

        this._evUp('OrderUpdateGateway', osr);
    };

    cancelOrder = (cancel : Models.OrderStatusReport) => {
        var c : Cancel = {order_id: cancel.exchangeId, symbol: this._symbolProvider.symbol };
        this._socket.send<OrderAck>("ok_spot" + this._symbolProvider.symbolQuote + "_cancel_order", this._signer.signMessage(c), () => {
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
                orderId: cancel.orderId,
                leavesQuantity: 0,
                time: cancel.time,
                orderStatus: Models.OrderStatus.Cancelled
            });
        });
    };

    private onCancel = (ts: Models.Timestamped<OrderAck>) => {
        if (typeof ts.data.order_id == "undefined") return;
        this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
          exchangeId: ts.data.order_id.toString(),
          orderStatus: Models.OrderStatus.Cancelled,
          time: ts.time,
          leavesQuantity: 0
        });
    };

    replaceOrder = (replace : Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private static getStatus(status: number) : Models.OrderStatus {
        // status: -1: cancelled, 0: pending, 1: partially filled, 2: fully filled, 4: cancel request in process
        switch (status) {
            case -1: return Models.OrderStatus.Cancelled;
            case 0: return Models.OrderStatus.Working;
            case 1: return Models.OrderStatus.Working;
            case 2: return Models.OrderStatus.Complete;
            case 4: return Models.OrderStatus.Working;
            default: return Models.OrderStatus.Cancelled;
        }
    }

    private onTrade = (tsMsg : Models.Timestamped<OkCoinTradeRecord>) => {
        var t = tsMsg.time;
        var msg : OkCoinTradeRecord = tsMsg.data;
        var avgPx = parseFloat(msg.averagePrice);
        var lastQty = parseFloat(msg.sigTradeAmount);
        var lastPx = parseFloat(msg.sigTradePrice);

        var status : Models.OrderStatusUpdate = {
            exchangeId: msg.orderId.toString(),
            orderStatus: OkCoinOrderEntryGateway.getStatus(msg.status),
            time: t,
            side: msg.tradeType.indexOf('buy')>-1 ? Models.Side.Bid : Models.Side.Ask,
            lastQuantity: lastQty > 0 ? lastQty : undefined,
            lastPrice: lastPx > 0 ? lastPx : undefined,
            averagePrice: avgPx > 0 ? avgPx : undefined
        };

        this._evUp('OrderUpdateGateway', status);
    };

    private onPosition = (ts: Models.Timestamped<any>) => {
        var free = (<any>ts.data).info.free;
        var freezed = (<any>ts.data).info.freezed;

        for (var currencyName in free) {
            if (!free.hasOwnProperty(currencyName)) continue;
            var amount = parseFloat(free[currencyName]);
            var held = parseFloat(freezed[currencyName]);

            var pos = new Models.CurrencyPosition(amount, held, Models.toCurrency(currencyName));
            this._evUp('PositionGateway', pos);
        }
    }

    constructor(
      private _evOn,
      private _evUp,
      private _http: OkCoinHttp,
      private _socket: OkCoinWebsocket,
      private _signer: OkCoinMessageSigner,
      private _symbolProvider: OkCoinSymbolProvider
    ) {
        _socket.setHandler("ok_sub_spot" + _symbolProvider.symbolQuote + "_trades", this.onTrade);
        _socket.setHandler("ok_spot" + _symbolProvider.symbolQuote + "_trade", this.onOrderAck);
        _socket.setHandler("ok_spot" + _symbolProvider.symbolQuote + "_cancel_order", this.onCancel);
        _socket.setHandler("ok_sub_spot" + _symbolProvider.symbolQuote + "_userinfo", this.onPosition);

        this._evOn('GatewaySocketConnect', cs => {
            this._evUp('GatewayOrderConnect', cs);

            if (cs === Models.ConnectivityStatus.Connected) {
                _socket.send("ok_sub_spot" + _symbolProvider.symbolQuote + "_trades", _signer.signMessage({}));
            }
        });
    }
}

class OkCoinMessageSigner {
    private _secretKey : string;
    private _api_key : string;

    public signMessage = (m : SignedMessage) : SignedMessage => {
        var els : string[] = [];

        if (!m.hasOwnProperty("api_key"))
            m.api_key = this._api_key;

        var keys = [];
        for (var key in m) {
            if (m.hasOwnProperty(key))
                keys.push(key);
        }
        keys.sort();

        for (var i = 0; i < keys.length; i++) {
            const k = keys[i];
            if (m.hasOwnProperty(k))
                els.push(k + "=" + m[k]);
        }

        var sig = els.join("&") + "&secret_key=" + this._secretKey;
        m.sign = crypto.createHash('md5').update(sig).digest("hex").toString().toUpperCase();
        return m;
    };

    constructor(cfString) {
        this._api_key = cfString("OkCoinApiKey");
        this._secretKey = cfString("OkCoinSecretKey");
    }
}

class OkCoinHttp {
    post = <T>(actionUrl: string, msg : SignedMessage) : Promise<Models.Timestamped<T>> => {
      return new Promise<Models.Timestamped<T>>((resolve, reject) => {
        request({
            url: url.resolve(this._baseUrl, actionUrl),
            body: querystring.stringify(this._signer.signMessage(msg)),
            headers: {"Content-Type": "application/x-www-form-urlencoded"},
            method: "POST"
        }, (err, resp, body) => {
            if (err) reject(err);
            else {
                try {
                    var t = new Date();
                    var data = JSON.parse(body);
                    resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    console.error(new Date().toISOString().slice(11, -1), 'okcoin', err, 'url:', actionUrl, 'err:', err, 'body:', body);
                    reject(e);
                }
            }
        });
      });
    };

    private _baseUrl : string;
    constructor(cfString, private _signer: OkCoinMessageSigner) {
        this._baseUrl = cfString("OkCoinHttpUrl")
    }
}

class OkCoinPositionGateway implements Interfaces.IPositionGateway {
    private trigger = () => {
        this._http.post("userinfo.do", {}).then(msg => {
            if (!(<any>msg.data).result)
              console.error(new Date().toISOString().slice(11, -1), 'okcoin', 'Please change the API Key or contact support team of OkCoin, your API Key does not work because was not possible to retrieve your real wallet position; the application will probably crash now.');

            var free = (<any>msg.data).info.funds.free;
            var freezed = (<any>msg.data).info.funds.freezed;

            for (var currencyName in free) {
                if (!free.hasOwnProperty(currencyName)) continue;
                this._evUp('PositionGateway', new Models.CurrencyPosition(parseFloat(free[currencyName]), parseFloat(freezed[currencyName]), Models.toCurrency(currencyName)));
            }
        });
    };

    constructor(private _evUp, private _http: OkCoinHttp) {
        setInterval(this.trigger, 15000);
        setTimeout(this.trigger, 10);
    }
}

class OkCoinBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    name() : string {
        return "OkCoin";
    }

    makeFee() : number {
        return 0.001;
    }

    takeFee() : number {
        return 0.002;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.OkCoin;
    }

    constructor(public minTickIncrement: number, public minSize: number) {}
}

class OkCoinSymbolProvider {
    public symbol: string;
    public symbolReversed: string;
    public symbolQuote: string;
    public symbolWithoutUnderscore: string;

    constructor(cfPair) {
        const GetCurrencySymbol = (s: Models.Currency) : string => Models.fromCurrency(s).toLowerCase();
        this.symbol = GetCurrencySymbol(cfPair.base) + "_" + GetCurrencySymbol(cfPair.quote);
        this.symbolReversed = GetCurrencySymbol(cfPair.quote) + "_" + GetCurrencySymbol(cfPair.base);
        this.symbolQuote = GetCurrencySymbol(cfPair.quote);
        this.symbolWithoutUnderscore = GetCurrencySymbol(cfPair.base) + GetCurrencySymbol(cfPair.quote);
    }
}

class OkCoin extends Interfaces.CombinedGateway {
    constructor(
      cfString,
      cfPair,
      _evOn,
      _evUp
    ) {
        var symbol = new OkCoinSymbolProvider(cfPair);
        var signer = new OkCoinMessageSigner(cfString);
        var http = new OkCoinHttp(cfString, signer);
        var socket = new OkCoinWebsocket(_evUp, cfString);

        var orderGateway = cfString("OkCoinOrderDestination") == "OkCoin"
            ? <Interfaces.IOrderEntryGateway>new OkCoinOrderEntryGateway(_evOn, _evUp, http, socket, signer, symbol)
            : new NullGateway.NullOrderGateway(_evUp);

        new OkCoinMarketDataGateway(_evOn, _evUp, socket, symbol);
        new OkCoinPositionGateway(_evUp, http);
        super(
            orderGateway,
            new OkCoinBaseGateway(parseFloat(
              Models.fromCurrency(cfPair.base)
                .replace('BTC', '0.01')
                .replace('LTC', '0.001')
            ) || 0.01, 0.01)
        );
        }
}

export async function createOkCoin(cfString, cfPair, _evOn, _evUp) : Promise<Interfaces.CombinedGateway> {
    return new OkCoin(cfString, cfPair, _evOn, _evUp);
}
