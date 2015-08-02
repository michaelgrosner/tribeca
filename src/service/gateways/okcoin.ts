/// <reference path="../../../typings/tsd.d.ts" />
/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />

import ws = require('ws');
import Q = require("q");
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import Config = require("../config");
import NullGateway = require("./nullgw");
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import _ = require("lodash");
var shortId = require("shortid");

interface OkCoinMessageIncomingMessage {
    channel : string;
    success : string;
    data : any;
    event? : string;
}

interface OkCoinDepthMessage {
    asks : [number, number][];
    bids : [number, number][];
    timestamp : string;
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
    price: number;
    amount: number;
}

interface Cancel extends SignedMessage {
    orderId: string;
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
	subscribe = <T>(channel : string, authenticate: boolean, handler: (newMsg : Models.Timestamped<T>) => void) => {
        var subsReq : any = {event: 'addChannel', channel: channel};
        
        if (authenticate) 
            subsReq.parameters = this._signer.signMessage({});
        
        this._ws.send(JSON.stringify(subsReq));
        this._handlers[channel] = handler;
    }

    private onMessage = (raw : string) => {
        var t = Utils.date();
        try {
            var msg : OkCoinMessageIncomingMessage = JSON.parse(raw)[0];

            if (typeof msg.event !== "undefined" && msg.event == "ping") {
                this._ws.send(this._serializedHeartbeat);
                return;
            }

            if (typeof msg.success !== "undefined") {
                if (msg.success !== "true")
                    this._log("Unsuccessful message %o", msg);
                else
                    this._log("Successfully connected to %s", msg.channel);
                return;
            }

            var handler = this._handlers[msg.channel];

            if (typeof handler === "undefined") {
                this._log("Got message on unknown topic %o", msg);
                return;
            }

            handler(new Models.Timestamped(msg.data, t));
        }
        catch (e) {
            this._log("Error parsing msg %o", raw);
            throw e;
        }
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private _serializedHeartbeat = JSON.stringify({event: "pong"});
    private _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinWebsocket");
    private _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
    private _ws : ws;
    constructor(config : Config.IConfigProvider, private _signer: OkCoinMessageSigner) {
        this._ws = new ws(config.GetString("OkCoinWsUrl"));

        this._ws.on("open", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("close", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
    }
}

class OkCoinMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private onTrade = (trade : Models.Timestamped<[string,string,string,string,string]>) => {
        // [tid, price, amount, time, type]
        var px = parseFloat(trade[1]);
        var amt = parseFloat(trade[2]);
        var side = trade[4] === "ask" ? Models.Side.Ask : Models.Side.Bid; // is this the make side?
        this.MarketTrade.trigger(new Models.GatewayMarketTrade(px, amt, trade.time, false, side));
    };

    MarketData = new Utils.Evt<Models.Market>();
    
    private static GetLevel = (n: [number, number]) : Models.MarketSide => 
        new Models.MarketSide(n[0], n[1]);
        
    private static GetSides = (side: [number, number][]) : Models.MarketSide[] => 
        _(side).first(3).map(OkCoinMarketDataGateway.GetLevel).value();
    
    private onDepth = (depth : Models.Timestamped<OkCoinDepthMessage>) => {
        var msg = depth.data;

        var bids = OkCoinMarketDataGateway.GetSides(msg.bids);
        var asks = OkCoinMarketDataGateway.GetSides(msg.asks);
        var mkt = new Models.Market(bids, asks, depth.time);

        this.MarketData.trigger(mkt);
    };

    constructor(socket : OkCoinWebsocket) {
        socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);
            
            if (cs == Models.ConnectivityStatus.Connected) {
                socket.subscribe("ok_btcusd_depth", false, this.onDepth);
                socket.subscribe("ok_btcusd_trades_v1", false, this.onTrade)
            }
        });
    }
}

class OkCoinOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => {
        return shortId.generate();
    }

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

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var o : Order = {
            symbol: "btc_usd", //btc_usd: bitcoin ltc_usd: litecoin,
            type: OkCoinOrderEntryGateway.GetOrderType(order.side, order.type),
            price: order.price,
            amount: order.quantity};
            
        this._http.post<OrderAck>("trade.do", o).then(ts => {
            var osr : Models.OrderStatusReport = { orderId: order.orderId, time: ts.time };
            
            if (ts.data.result === true) {
                osr.exchangeId = ts.data.order_id.toString();
                osr.orderStatus = Models.OrderStatus.Working;
            } 
            else {
                osr.orderStatus = Models.OrderStatus.Rejected;
            }
            
            this.OrderUpdate.trigger(osr);
        }).done();

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        var c : Cancel = {orderId: cancel.exchangeId };
        this._http.post<OrderAck>("cancel_order.do", c).then(ts => {
            var osr : Models.OrderStatusReport = { orderId: cancel.clientOrderId, time: ts.time };
            
            if (ts.data.result === true) {
                osr.orderStatus = Models.OrderStatus.Cancelled;
            }
            else {
                osr.orderStatus = Models.OrderStatus.Rejected;
                osr.cancelRejected = true;
            }
            
            this.OrderUpdate.trigger(osr);
        });
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
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

    private onMessage = (tsMsg : Models.Timestamped<OkCoinTradeRecord>) => {
        var t = tsMsg.time;
        var msg : OkCoinTradeRecord = tsMsg.data;
        
        var avgPx = parseFloat(msg.averagePrice);
        var lastQty = parseFloat(msg.tradeAmount);
        var lastPx = parseFloat(msg.tradePrice);

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

    _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinOE");
    constructor(private _socket : OkCoinWebsocket, private _http: OkCoinHttp) {
        _socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);
            
            if (cs === Models.ConnectivityStatus.Connected) {
                _socket.subscribe("ok_usd_realtrades", true, this.onMessage);
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
            var key = keys[i];
            if (m.hasOwnProperty(key))
                els.push(key + "=" + m[key]);
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
                    this._log("url: %s, err: %o, body: %o", actionUrl, err, body);
                    d.reject(e);
                }
            }
        });
        
        return d.promise;
    };

    private _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinHTTP");
    private _baseUrl : string;
    constructor(config : Config.IConfigProvider, private _signer: OkCoinMessageSigner) {
        this._baseUrl = config.GetString("OkCoinHttpUrl")
    }
}

class OkCoinPositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private static convertCurrency(name : string) : Models.Currency {
        switch (name.toLowerCase()) {
            case "usd": return Models.Currency.USD;
            case "ltc": return Models.Currency.LTC;
            case "btc": return Models.Currency.BTC;
            default: throw new Error("Unsupported currency " + name);
        }
    }

    private trigger = () => {
        this._http.post("userinfo.do", {}).then(msg => {
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

    constructor(private _http : OkCoinHttp) {
        this.trigger();
        setInterval(this.trigger, 15000);
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
}

export class OkCoin extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var signer = new OkCoinMessageSigner(config);
        var http = new OkCoinHttp(config, signer);
        var socket = new OkCoinWebsocket(config, signer);

        var orderGateway = config.GetString("OkCoinOrderDestination") == "OkCoin"
            ? <Interfaces.IOrderEntryGateway>new OkCoinOrderEntryGateway(socket, http)
            : new NullGateway.NullOrderGateway();

        super(
            new OkCoinMarketDataGateway(socket),
            orderGateway,
            new OkCoinPositionGateway(http),
            new OkCoinBaseGateway());
        }
}