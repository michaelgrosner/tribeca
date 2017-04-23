import ws = require('ws');
import Q = require("q");
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
var shortId = require("shortid");

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

interface SubscriptionRequest extends SignedMessage { }

class OkCoinWebsocket {
    send = <T>(channel : string, parameters: any) => {
        var subsReq : any = {event: 'addChannel', channel: channel};

        if (parameters !== null)
            subsReq.parameters = parameters;
        this._ws.send(JSON.stringify(subsReq));
    }

    setHandler = <T>(channel : string, handler: (newMsg : Models.Timestamped<T>) => void) => {
        this._handlers[channel] = handler;
    }

    private onMessage = (raw : string) => {
        var t = Utils.date();
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
              errorcode != 10050 /* 10050=Can't cancel more than once */
              && errorcode != 10009 /* 10009=Order does not exist */
              && errorcode != 10010 /* 10010=Insufficient funds */
              && errorcode != 10016 /* 10016=Insufficient coins balance */
              // errorcode != 10001 /* 10001=Request frequency too high */
            ))) this._log.warn("Unsuccessful message %s received.", raw);
            else if (success && (channel == 'addChannel' || channel == 'login'))
              return this._log.info("Successfully connected to %s", channel + (typeof msg.data.channel !== 'undefined' ? ': '+msg.data.channel : ''));
            if (typeof errorcode !== "undefined" && (
              errorcode == 10050
              || errorcode == 10009
              // || errorcode == '10001'
            ))  return;

            var handler = this._handlers[channel];

            if (typeof handler === "undefined") {
                this._log.warn("Got message on unknown topic", msg);
                return;
            }

            handler(new Models.Timestamped(msg.data, t));
        }
        catch (e) {
            this._log.error(e, "Error parsing msg", raw);
            throw e;
        }
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private _serializedHeartping = JSON.stringify({event: "ping"});
    private _serializedHeartbeat = JSON.stringify({event: "pong"});
    private _stillAlive: boolean = true;
    private _log = Utils.log("tribeca:gateway:OkCoinWebsocket");
    private _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
    private _ws : ws;
    constructor(config : Config.IConfigProvider) {
        this._ws = new ws(config.GetString("OkCoinWsUrl"));
        this._ws.on("open", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("close", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
        setInterval(() => {
          if (!this._stillAlive) this._log.info('OkCoin heartbeat lost.');
          this._stillAlive = false;
          this._ws.send(this._serializedHeartping);
        }, 7000);
    }
}

class OkCoinMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private onTrade = (trades : Models.Timestamped<[string,string,string,string,string][]>) => {
        trades.data.forEach(trade => { // [tid, price, amount, time, type]
            var px = parseFloat(trade[1]);
            var amt = parseFloat(trade[2]);
            var side = trade[4] === "ask" ? Models.Side.Ask : Models.Side.Bid;
            var mt = new Models.GatewayMarketTrade(px, amt, trades.time, trades.data.length > 0, side);
            this.MarketTrade.trigger(mt);
        });
    };

    MarketData = new Utils.Evt<Models.Market>();

    private static GetLevel = (n: [any, any]) : Models.MarketSide =>
        new Models.MarketSide(parseFloat(n[0]), parseFloat(n[1]));

    private onDepth = (depth : Models.Timestamped<OkCoinDepthMessage>) => {
        var msg = depth.data;

        var bids = msg.bids.slice(0,13).map(OkCoinMarketDataGateway.GetLevel);
        var asks = msg.asks.reverse().slice(0,13).map(OkCoinMarketDataGateway.GetLevel);
        var mkt = new Models.Market(bids, asks, depth.time);

        this.MarketData.trigger(mkt);
    };

    private _log = Utils.log("tribeca:gateway:OkCoinMD");
    constructor(socket: OkCoinWebsocket, symbolProvider: OkCoinSymbolProvider) {
        var depthChannel = "ok_sub_spot" + symbolProvider.symbolReversed + "_depth_20";
        var tradesChannel = "ok_sub_spot" + symbolProvider.symbolReversed + "_trades";

        socket.setHandler(depthChannel, this.onDepth);
        socket.setHandler(tradesChannel, this.onTrade);

        socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);

            if (cs == Models.ConnectivityStatus.Connected) {
                socket.send(depthChannel, null);
                socket.send(tradesChannel, null);
            }
        });
    }
}

class OkCoinOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => shortId.generate();

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Q.Promise<number> => {
        var d = Q.defer<number>();
        var len = 0;
        this._http.post("order_info.do", <Cancel>{order_id: '-1', symbol: this._symbolProvider.symbol }).then(msg => {
            if (typeof (<any>msg.data).orders == "undefined"
              || typeof (<any>msg.data).orders[0] == "undefined"
              || typeof (<any>msg.data).orders[0].order_id == "undefined") { d.reject(0); return; }
            (<any>msg.data).orders.map((o) => {
                this._http.post("cancel_order.do", <Cancel>{order_id: o.order_id.toString(), symbol: this._symbolProvider.symbol }).then(msg => {
                    if (typeof (<any>msg.data).result == "undefined") return;
                    var osr : Models.OrderStatusReport = { time: msg.time };
                    if ((<any>msg.data).result) {
                        osr.exchangeId = (<any>msg.data).order_id.toString();
                        osr.orderStatus = Models.OrderStatus.Cancelled;
                        osr.leavesQuantity = 0;
                        osr.done = true;
                        this.OrderUpdate.trigger(osr);
                    }
                }).done();
            });
            d.resolve((<any>msg.data).orders.length);
        }).done();
        return d.promise;
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

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var o: Order = {
            symbol: this._symbolProvider.symbol,
            type: OkCoinOrderEntryGateway.GetOrderType(order.side, order.type),
            price: order.price.toString(),
            amount: order.quantity.toString()};

        this._ordersWaitingForAckQueue.push([order.orderId, order.quantity]);

        this._socket.send<OrderAck>("ok_spot" + this._symbolProvider.symbolQuote + "_trade", this._signer.signMessage(o));
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private onOrderAck = (ts: Models.Timestamped<OrderAck>) => {
        var order = this._ordersWaitingForAckQueue.shift();

        var orderId = order[0];
        if (typeof orderId === "undefined") {
            this._log.error("got an order ack when there was no order queued!", util.format(ts.data));
            return;
        }

        var osr : Models.OrderStatusReport = { orderId: orderId, time: ts.time };

        if (typeof ts.data !== "undefined" && ts.data.result) {
            osr.exchangeId = ts.data.order_id.toString();
            osr.orderStatus = Models.OrderStatus.Working;
            osr.leavesQuantity = order[1];
        }
        else {
            osr.orderStatus = Models.OrderStatus.Rejected;
            osr.done = true;
        }

        this.OrderUpdate.trigger(osr);
    };

    cancelOrder = (cancel: Models.BrokeredCancel): Models.OrderGatewayActionReport => {
        this._http.post("cancel_order.do", <Cancel>{order_id: cancel.exchangeId, symbol: this._symbolProvider.symbol }).then(msg => {
            if (typeof (<any>msg.data).result == "undefined") return;

            var osr : Models.OrderStatusReport = { orderId: cancel.clientOrderId, time: msg.time };

            if ((<any>msg.data).result) {
                osr.exchangeId = (<any>msg.data).order_id.toString();
                osr.orderStatus = Models.OrderStatus.Cancelled;
                osr.leavesQuantity = 0;
                osr.done = true;
                this.OrderUpdate.trigger(osr);
            }
            else {
                this._http.post("order_info.do", <Cancel>{order_id: cancel.exchangeId, symbol: this._symbolProvider.symbol }).then(msg => {
                    if (typeof (<any>msg.data).orders == "undefined"
                      || typeof (<any>msg.data).orders[0] == "undefined"
                      || typeof (<any>msg.data).orders[0].order_id == "undefined") return;

                    osr = { orderId: cancel.clientOrderId, time: msg.time };

                    if ((<any>msg.data).result) {
                      osr.orderStatus = OkCoinOrderEntryGateway.getStatus((<any>msg.data).orders[0].status);
                      if (osr.orderStatus == Models.OrderStatus.Working) {
                        osr.exchangeId = (<any>msg.data).orders[0].order_id.toString();
                        var avgPx = parseFloat((<any>msg.data).orders[0].avg_price);
                        var lastQty = parseFloat((<any>msg.data).orders[0].deal_amount);
                        var lastPx = parseFloat((<any>msg.data).orders[0].price);
                        osr.lastQuantity = lastQty > 0 ? lastQty : undefined;
                        osr.lastPrice = lastPx > 0 ? lastPx : undefined;
                        osr.averagePrice = avgPx > 0 ? avgPx : undefined;
                        osr.pendingCancel = (<any>msg.data).orders[0].status === 4;
                        osr.partiallyFilled = (<any>msg.data).orders[0].status === 1;
                        osr.leavesQuantity = (<any>msg.data).orders[0].amount - (<any>msg.data).orders[0].deal_amount;
                      }
                    }
                    if (!osr.leavesQuantity) {
                      osr.orderStatus = Models.OrderStatus.Cancelled;
                      osr.leavesQuantity = 0;
                      osr.done = true;
                    }
                    this.OrderUpdate.trigger(osr);
                }).done();
            }
        }).done();
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    private static getStatus(status: number) : Models.OrderStatus {
        // status: -1: cancelled, 0: pending, 1: partially filled, 2: fully filled, 4: cancel request in process
        switch (status) {
            case -1: return Models.OrderStatus.Cancelled;
            case 0: return Models.OrderStatus.Working;
            case 1: return Models.OrderStatus.Working;
            case 2: return Models.OrderStatus.Complete;
            case 4: return Models.OrderStatus.Working;
            default: return Models.OrderStatus.Other;
        }
    }

    private onTrade = (tsMsg : Models.Timestamped<OkCoinTradeRecord>) => {
        var t = tsMsg.time;
        var msg : OkCoinTradeRecord = tsMsg.data;

        var avgPx = parseFloat(msg.averagePrice);
        var lastQty = parseFloat(msg.sigTradeAmount);
        var lastPx = parseFloat(msg.sigTradePrice);

        var status : Models.OrderStatusReport = {
            exchangeId: msg.orderId.toString(),
            orderStatus: OkCoinOrderEntryGateway.getStatus(msg.status),
            time: t,
            lastQuantity: lastQty > 0 ? lastQty : undefined,
            lastPrice: lastPx > 0 ? lastPx : undefined,
            averagePrice: avgPx > 0 ? avgPx : undefined,
            pendingCancel: msg.status === 4,
            partiallyFilled: msg.status === 1
        };

        this.OrderUpdate.trigger(status);
    };

    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private onPosition = (ts: Models.Timestamped<any>) => {
        var free = (<any>ts.data).info.free;
        var freezed = (<any>ts.data).info.freezed;

        for (var currencyName in free) {
            if (!free.hasOwnProperty(currencyName)) continue;
            var amount = parseFloat(free[currencyName]);
            var held = parseFloat(freezed[currencyName]);

            var pos = new Models.CurrencyPosition(amount, held, OkCoinPositionGateway.convertCurrency(currencyName));
            this.PositionUpdate.trigger(pos);
        }
    }

    private _log = Utils.log("tribeca:gateway:OkCoinOE");
    constructor(private _http : OkCoinHttp,
            private _socket: OkCoinWebsocket,
            private _signer: OkCoinMessageSigner,
            private _symbolProvider: OkCoinSymbolProvider) {
        _socket.setHandler("ok_sub_spot" + _symbolProvider.symbolQuote + "_trades", this.onTrade);
        _socket.setHandler("ok_spot" + _symbolProvider.symbolQuote + "_trade", this.onOrderAck);
        _socket.setHandler("ok_sub_spot" + _symbolProvider.symbolQuote + "_userinfo", this.onPosition);

        _socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);

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

    constructor(config : Config.IConfigProvider) {
        this._api_key = config.GetString("OkCoinApiKey");
        this._secretKey = config.GetString("OkCoinSecretKey");
    }
}

class OkCoinHttp {
    post = <T>(actionUrl: string, msg : SignedMessage) : Q.Promise<Models.Timestamped<T>> => {
        var d = Q.defer<Models.Timestamped<T>>();

        request({
            url: url.resolve(this._baseUrl, actionUrl),
            body: querystring.stringify(this._signer.signMessage(msg)),
            headers: {"Content-Type": "application/x-www-form-urlencoded"},
            method: "POST"
        }, (err, resp, body) => {
            if (err) d.reject(err);
            else {
                try {
                    var t = Utils.date();
                    var data = JSON.parse(body);
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    this._log.error(err, "url: %s, err: %o, body: %o", actionUrl, err, body);
                    d.reject(e);
                }
            }
        });

        return d.promise;
    };

    private _log = Utils.log("tribeca:gateway:OkCoinHTTP");
    private _baseUrl : string;
    constructor(config : Config.IConfigProvider, private _signer: OkCoinMessageSigner) {
        this._baseUrl = config.GetString("OkCoinHttpUrl")
    }
}

class OkCoinPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    public static convertCurrency(name : string) : Models.Currency {
        switch (name.toLowerCase()) {
            case "usd": return Models.Currency.USD;
            case "ltc": return Models.Currency.LTC;
            case "btc": return Models.Currency.BTC;
            case "cny": return Models.Currency.CNY;
            default: throw new Error("Unsupported currency " + name);
        }
    }

    private trigger = () => {
        this._http.post("userinfo.do", {}).then(msg => {
            if (!(<any>msg.data).result)
              this._log.error('Please change the API Key or contact support team of OkCoin, your API Key does not work because was not possible to retrieve your real wallet position; the application will probably crash now.');

            var free = (<any>msg.data).info.funds.free;
            var freezed = (<any>msg.data).info.funds.freezed;

            for (var currencyName in free) {
                if (!free.hasOwnProperty(currencyName)) continue;
                var amount = parseFloat(free[currencyName]);
                var held = parseFloat(freezed[currencyName]);

                var pos = new Models.CurrencyPosition(amount, held, OkCoinPositionGateway.convertCurrency(currencyName));
                this.PositionUpdate.trigger(pos);
            }
        }).done();
    };

    private _log = Utils.log("tribeca:gateway:OkCoinPG");
    constructor(private _http : OkCoinHttp) {
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

    private static AllPairs = [
        new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD),
        new Models.CurrencyPair(Models.Currency.LTC, Models.Currency.USD),
        new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.CNY)
    ];
    public get supportedCurrencyPairs() {
        return OkCoinBaseGateway.AllPairs;
    }
}

function GetCurrencyEnum(c: string) : Models.Currency {
    switch (c.toLowerCase()) {
        case "usd": return Models.Currency.USD;
        case "ltc": return Models.Currency.LTC;
        case "btc": return Models.Currency.BTC;
        case "cny": return Models.Currency.CNY;
        default: throw new Error("Unsupported currency " + c);
    }
}

function GetCurrencySymbol(c: Models.Currency) : string {
    switch (c) {
        case Models.Currency.USD: return "usd";
        case Models.Currency.LTC: return "ltc";
        case Models.Currency.BTC: return "btc";
        case Models.Currency.CNY: return "cny";
        default: throw new Error("Unsupported currency " + Models.Currency[c]);
    }
}

class OkCoinSymbolProvider {
    public symbol: string;
    public symbolReversed: string;
    public symbolQuote: string;
    public symbolWithoutUnderscore: string;

    constructor(pair: Models.CurrencyPair) {
        this.symbol = GetCurrencySymbol(pair.base) + "_" + GetCurrencySymbol(pair.quote);
        this.symbolReversed = GetCurrencySymbol(pair.quote) + "_" + GetCurrencySymbol(pair.base);
        this.symbolQuote = GetCurrencySymbol(pair.quote);
        this.symbolWithoutUnderscore = GetCurrencySymbol(pair.base) + GetCurrencySymbol(pair.quote);
    }
}

export class OkCoin extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider, pair: Models.CurrencyPair) {
        var symbol = new OkCoinSymbolProvider(pair);
        var signer = new OkCoinMessageSigner(config);
        var http = new OkCoinHttp(config, signer);
        var socket = new OkCoinWebsocket(config);

        var orderGateway = config.GetString("OkCoinOrderDestination") == "OkCoin"
            ? <Interfaces.IOrderEntryGateway>new OkCoinOrderEntryGateway(http, socket, signer, symbol)
            : new NullGateway.NullOrderGateway();

        super(
            new OkCoinMarketDataGateway(socket, symbol),
            orderGateway,
            new OkCoinPositionGateway(http),
            new OkCoinBaseGateway());
        }
}
