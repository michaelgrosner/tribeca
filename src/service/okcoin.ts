/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="fix.ts" />
/// <reference path="nullgw.ts" />

import ws = require('ws');
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import Config = require("./config");
import Fix = require("./fix");
import NullGateway = require("./nullgw");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

interface OkCoinMessageIncomingMessage {
    channel : string;
    success : string;
    data : any;
    event? : string;
}

interface OkCoinDepthMessage {
    asks : Array<Array<number>>;
    bids : Array<Array<number>>;
    timestamp : string;
}

interface OkCoinExecutionReport {
    exchangeId : string;
    orderId : string;
    orderStatus : string;
    rejectMessage : string;
    lastQuantity: number;
    lastPrice : number;
    leavesQuantity : number;
    cumQuantity : number;
    averagePrice : number;
}

class OkCoinWebsocket {
    subscribe<T>(channel : string, handler: (newMsg : Models.Timestamped<T>) => void) {
        var subsReq = {event: 'addChannel',
                       channel: channel,
                       parameters: {partner: this._partner,
                                    secretkey: this._secretKey}};
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
    _serializedHeartbeat = JSON.stringify({event: "pong"});
    _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinWebsocket");
    _secretKey : string;
    _partner : string;
    _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
    _ws : ws;
    constructor(config : Config.IConfigProvider) {
        this._partner = config.GetString("OkCoinPartner");
        this._secretKey = config.GetString("OkCoinSecretKey");
        this._ws = new ws(config.GetString("OkCoinWsUrl"));

        this._ws.on("open", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("close", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
    }
}

class OkCoinMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    // TODO
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    private onDepth = (prettyMegan : Models.Timestamped<OkCoinDepthMessage>) => {
        var msg = prettyMegan.data;

        var getLevel = n => new Models.MarketSide(n[0], n[1]);
        var mkt = new Models.Market(msg.bids.map(getLevel), msg.asks.map(getLevel), prettyMegan.time);

        this.MarketData.trigger(mkt);
    };

    constructor(socket : OkCoinWebsocket) {
        socket.ConnectChanged.on(cs => {
            if (cs == Models.ConnectivityStatus.Connected)
                socket.subscribe("ok_btcusd_depth", this.onDepth);
            this.ConnectChanged.trigger(cs);
        });
    }
}

class OkCoinOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    public cancelsByClientOrderId = false;

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var o = {
            id: order.orderId,
            symbol: "BTC/USD",
            side: order.side == Models.Side.Bid ? "buy" : "sell",
            price: order.price,
            tif: Models.TimeInForce[order.timeInForce],
            amount: order.quantity};
        this._socket.sendEvent("New", o);

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private _cancelsWaitingForExchangeOrderId : {[clId : string] : Models.BrokeredCancel} = {};
    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        // race condition! i cannot cancel an order before I get the exchangeId (oid); register it for deletion on the ack
        if (typeof cancel.exchangeId !== "undefined") {
            this.sendCancel(cancel.exchangeId, cancel);
        }
        else {
            this._cancelsWaitingForExchangeOrderId[cancel.clientOrderId] = cancel;
            this._log("Registered %s for late deletion", cancel.clientOrderId);
        }

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private sendCancel = (exchangeId : string, cancel : Models.BrokeredCancel) => {
        var c = {
            symbol: "BTC/USD",
            origOrderId: cancel.clientOrderId,
            origExchOrderId: exchangeId,
            side: cancel.side == Models.Side.Bid ? "buy" : "sell"
        };
        this._socket.sendEvent("Cxl", c);
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    private static getStatus(status: string) : Models.OrderStatus {
        // these are the quickfix tags
        switch (status) {
            case '0':
            case '1':
            case '6': // pending cxl, repl, and new are all working
            case 'A':
            case 'E':
            case '5':
                return Models.OrderStatus.Working;
            case '2':
                return Models.OrderStatus.Complete;
            case '3':
            case '4':
                return Models.OrderStatus.Cancelled;
            case '8':
                return Models.OrderStatus.Rejected;
        }
        return Models.OrderStatus.Other;
    }

    private onMessage = (tsMsg : Models.Timestamped<OkCoinExecutionReport>) => {
        var t = tsMsg.time;
        var msg : OkCoinExecutionReport = tsMsg.data;

        // cancel any open orders waiting for oid
        if (this._cancelsWaitingForExchangeOrderId.hasOwnProperty(msg.orderId)) {
            var cancel = this._cancelsWaitingForExchangeOrderId[msg.orderId];
            this.sendCancel(msg.exchangeId, cancel);
            this._log("Deleting %s late, oid: %s", cancel.clientOrderId, msg.orderId);
            delete this._cancelsWaitingForExchangeOrderId[msg.orderId];
        }

        var orderStatus = OkCoinOrderEntryGateway.getStatus(msg.orderStatus);
        var status : Models.OrderStatusReport = {
            exchangeId: msg.exchangeId,
            orderId: msg.orderId,
            orderStatus: orderStatus,
            time: t,
            lastQuantity: msg.lastQuantity > 0 ? msg.lastQuantity : undefined,
            lastPrice: msg.lastPrice > 0 ? msg.lastPrice : undefined,
            leavesQuantity: orderStatus == Models.OrderStatus.Working ? msg.leavesQuantity : undefined,
            cumQuantity: msg.cumQuantity > 0 ? msg.cumQuantity : undefined,
            averagePrice: msg.averagePrice > 0 ? msg.averagePrice : undefined,
            pendingCancel: msg.orderStatus == "6",
            pendingReplace: msg.orderStatus == "E"
        };

        this.OrderUpdate.trigger(status);
    };

    _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinOE");
    constructor(private _socket : Fix.FixGateway) {
        _socket.subscribe("ExecRpt", this.onMessage);
        _socket.ConnectChanged.on(cs => this.ConnectChanged.trigger(cs));
    }
}

class OkCoinHttp {
    private signMsg = (m : Map<string, string>) => {
        var els : string[] = [];

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
        return crypto.createHash('md5').update(sig).digest("hex").toString().toUpperCase();
    };

    post = <T>(actionUrl: string, msg : any, cb : (m : Models.Timestamped<T>) => void) => {
        msg.partner = this._partner;
        msg.sign = this.signMsg(msg);

        request({
            url: url.resolve(this._baseUrl, actionUrl),
            body: querystring.stringify(msg),
            headers: {"Content-Type": "application/x-www-form-urlencoded"},
            method: "POST"
        }, (err, resp, body) => {
            try {
                var t = Utils.date();
                var data = JSON.parse(body);
                cb(new Models.Timestamped(data, t));
            }
            catch (e) {
                this._log("url: %s, err: %o, body: %o", actionUrl, err, body);
                throw e;
            }
        });
    };

    _log : Utils.Logger = Utils.log("tribeca:gateway:OkCoinHTTP");
    _secretKey : string;
    _partner : string;
    _baseUrl : string;
    constructor(config : Config.IConfigProvider) {
        this._partner = config.GetString("OkCoinPartner");
        this._secretKey = config.GetString("OkCoinSecretKey");
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
        this._http.post("userinfo.do", {}, msg => {
            var funds = (<any>msg.data).info.funds.free;
            var held = (<any>msg.data).info.funds.freezed;

            for (var currencyName in funds) {
                if (!funds.hasOwnProperty(currencyName)) continue;
                var val = funds[currencyName];

                var pos = new Models.CurrencyPosition(parseFloat(val), held, OkCoinPositionGateway.convertCurrency(currencyName));
                this.PositionUpdate.trigger(pos);
            }
        });
    };

    constructor(private _http : OkCoinHttp) {
        this.trigger();
        setInterval(this.trigger, 15000);
    }
}

class OkCoinBaseGateway implements Interfaces.IExchangeDetailsGateway {
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
        var http = new OkCoinHttp(config);
        var socket = new OkCoinWebsocket(config);
        var fix = new Fix.FixGateway();

        var orderGateway = config.GetString("OkCoinOrderDestination") == "OkCoin"
            ? <Interfaces.IOrderEntryGateway>new OkCoinOrderEntryGateway(fix)
            : new NullGateway.NullOrderGateway();

        super(
            new OkCoinMarketDataGateway(socket),
            orderGateway,
            new OkCoinPositionGateway(http),
            new OkCoinBaseGateway());
        }
}
