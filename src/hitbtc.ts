/// <reference path="../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("./config");
import crypto = require('crypto');
import ws = require('ws');
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("./models");
import Utils = require("./utils");

var _lotMultiplier = 100.0;

interface NoncePayload<T> {
    nonce: number;
    payload: T;
}

interface AuthorizedHitBtcMessage<T> {
    apikey : string;
    signature : string;
    message : NoncePayload<T>;
}

interface HitBtcPayload {
}

interface Login extends HitBtcPayload {
}

interface NewOrder extends HitBtcPayload {
    clientOrderId : string;
    symbol : string;
    side : string;
    quantity : number;
    type : string;
    price : number;
    timeInForce : string;
}

interface OrderCancel extends HitBtcPayload {
    clientOrderId : string;
    cancelRequestClientOrderId : string;
    symbol : string;
    side : string;
}

interface HitBtcOrderBook {
    asks : Array<Array<string>>;
    bids : Array<Array<string>>;
}

interface Update {
    price : number;
    size : number;
    timestamp : number;
}

class SideUpdate {
    constructor(public price: number, public size: number) {}
}

interface MarketDataSnapshotFullRefresh {
    snapshotSeqNo : number;
    symbol : string;
    exchangeStatus : string;
    ask : Array<Update>;
    bid : Array<Update>
}

interface MarketDataIncrementalRefresh {
    seqNo : number;
    timestamp : number;
    symbol : string;
    exchangeStatus : string;
    ask : Array<Update>;
    bid : Array<Update>
    trade : Array<Update>
}

interface ExecutionReport {
    orderId : string;
    clientOrderId : string;
    execReportType : string;
    orderStatus : string;
    orderRejectReason? : string;
    symbol : string;
    side : string;
    timestamp : number;
    price : number;
    quantity : number;
    type : string;
    timeInForce : string;
    tradeId? : string;
    lastQuantity? : number;
    lastPrice? : number;
    leavesQuantity? : number;
    cumQuantity? : number;
    averagePrice? : number;
}

interface CancelReject {
    clientOrderId : string;
    cancelRequestClientOrderId : string;
    rejectReasonCode : string;
    rejectReasonText : string;
    timestamp : number;
}

class HitBtcMarketDataGateway implements Models.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.MarketUpdate>();
    _marketDataWs : any;

    _lastBook : { [side: string] : { [px: number]: number}} = null;
    private onMarketDataIncrementalRefresh = (msg : MarketDataIncrementalRefresh, t : Moment) => {
        if (msg.symbol != "BTCUSD" || this._lastBook == null) return;

        var ordBids = HitBtcMarketDataGateway._applyIncrementals(msg.bid, this._lastBook["bid"], (a, b) => a.price > b.price ? -1 : 1);
        var ordAsks = HitBtcMarketDataGateway._applyIncrementals(msg.ask, this._lastBook["ask"], (a, b) => a.price > b.price ? 1 : -1);

        var getLevel = (n : number) => {
            var bid = new Models.MarketSide(ordBids[n].price, ordBids[n].size);
            var ask = new Models.MarketSide(ordAsks[n].price, ordAsks[n].size);
            return new Models.MarketUpdate(bid, ask, t);
        };

        this.MarketData.trigger(getLevel(0));
    };

    private static _applyIncrementals(incomingUpdates : Update[],
                               side : { [px: number]: number},
                               cmp : (p1 : SideUpdate, p2 : SideUpdate) => number) {
        for (var i = 0; i < incomingUpdates.length; i++) {
            var u : Update = incomingUpdates[i];
            if (u.size == 0) {
                delete side[u.price];
            }
            else {
                side[u.price] = u.size;
            }
        }

        var kvps : SideUpdate[] = [];
        for (var px in side) {
            kvps.push(new SideUpdate(parseFloat(px), side[px] / _lotMultiplier));
        }
        return kvps.sort(cmp);
    }

    private static getLevel(msg : MarketDataSnapshotFullRefresh, n : number, t : Moment) : Models.MarketUpdate {
        var bid = new Models.MarketSide(msg.bid[n].price, msg.bid[n].size / _lotMultiplier);
        var ask = new Models.MarketSide(msg.ask[n].price, msg.ask[n].size / _lotMultiplier);
        return new Models.MarketUpdate(bid, ask, t);
    }

    private onMarketDataSnapshotFullRefresh = (msg : MarketDataSnapshotFullRefresh, t : Moment) => {
        if (msg.symbol != "BTCUSD") return;

        this._lastBook = {bid: {}, ask: {}};

        for (var i = 0; i < msg.ask.length; i++) {
            this._lastBook["ask"][msg.ask[i].price] = msg.ask[i].size;
        }

        for (var i = 0; i < msg.bid.length; i++) {
            this._lastBook["bid"][msg.bid[i].price] = msg.bid[i].size;
        }

        var b = HitBtcMarketDataGateway.getLevel(msg, 0, t);
        this.MarketData.trigger(b);
    };

    private onMessage = (raw : string) => {
        var t = Utils.date();

        try {
            var msg = JSON.parse(raw);
        }
        catch (e) {
            this._log("Error parsing msg %o", raw);
            throw e;
        }

        if (msg.hasOwnProperty("MarketDataIncrementalRefresh")) {
            this.onMarketDataIncrementalRefresh(msg.MarketDataIncrementalRefresh, t);
        }
        else if (msg.hasOwnProperty("MarketDataSnapshotFullRefresh")) {
            this.onMarketDataSnapshotFullRefresh(msg.MarketDataSnapshotFullRefresh, t);
        }
        else {
            this._log("unhandled message", msg);
        }
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private onOpen = () => {
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
    };

     _log : Utils.Logger = Utils.log("tribeca:gateway:HitBtcMD");
    constructor(config : Config.IConfigProvider) {
        this._marketDataWs = new ws(config.GetString("HitBtcMarketDataUrl"));
        this._marketDataWs.on('open', this.onOpen);
        this._marketDataWs.on('message', this.onMessage);
        this._marketDataWs.on("error", this.onMessage);

        request.get(
            {url: url.resolve(config.GetString("HitBtcPullUrl"), "/api/1/public/BTCUSD/orderbook")},
            (err, body, resp) => {
                this.onMarketDataSnapshotFullRefresh(resp, Utils.date());
            });
    }
}

class HitBtcOrderEntryGateway implements Models.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    _orderEntryWs : any;

    _nonce = 1;

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this.sendAuth("OrderCancel", {clientOrderId: cancel.clientOrderId,
            cancelRequestClientOrderId: cancel.requestId,
            symbol: "BTCUSD",
            side: HitBtcOrderEntryGateway.getSide(cancel.side)});
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var hitBtcOrder : NewOrder = {
            clientOrderId: order.orderId,
            symbol: "BTCUSD",
            side: HitBtcOrderEntryGateway.getSide(order.side),
            quantity: order.quantity * _lotMultiplier,
            type: HitBtcOrderEntryGateway.getType(order.type),
            price: order.price,
            timeInForce: HitBtcOrderEntryGateway.getTif(order.timeInForce)
        };

        this.sendAuth("NewOrder", hitBtcOrder);
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private static getStatus(m : ExecutionReport) : Models.OrderStatus {
        switch (m.execReportType) {
            case "new":
            case "status":
                return Models.OrderStatus.Working;
            case "canceled":
                return Models.OrderStatus.Cancelled;
            case "expired":
                return Models.OrderStatus.Complete;
            case "rejected":
                return Models.OrderStatus.Rejected;
            case "trade":
                if (m.orderStatus == "filled")
                    return Models.OrderStatus.Complete;
                else
                    return Models.OrderStatus.Working;
            default:
                return Models.OrderStatus.Other;
        }
    }

    private static getTif(tif : Models.TimeInForce) {
        switch (tif) {
            case Models.TimeInForce.FOK:
                return "FOK";
            case Models.TimeInForce.GTC:
                return "GTC";
            case Models.TimeInForce.IOC:
                return "IOC";
            default:
                throw new Error("TIF " + Models.TimeInForce[tif] + " not supported in HitBtc");
        }
    }

    private static getSide(side : Models.Side) {
        switch (side) {
            case Models.Side.Bid:
                return "buy";
            case Models.Side.Ask:
                return "sell";
            default:
                throw new Error("Side " + Models.Side[side] + " not supported in HitBtc");
        }
    }

    private static getType(t : Models.OrderType) {
        switch (t) {
            case Models.OrderType.Limit:
                return "limit";
            case Models.OrderType.Market:
                return "market";
            default:
                throw new Error("OrderType " + Models.OrderType[t] + " not supported in HitBtc");
        }
    }

    private onExecutionReport = (tsMsg : Models.Timestamped<ExecutionReport>) => {
        var t = tsMsg.time;
        var msg = tsMsg.data;

        var ordStatus = HitBtcOrderEntryGateway.getStatus(msg);
        var status : Models.OrderStatusReport = {
            exchangeId: msg.orderId,
            orderId: msg.clientOrderId,
            orderStatus: ordStatus,
            time: t,
            rejectMessage: msg.orderRejectReason,
            lastQuantity: msg.lastQuantity > 0 ? msg.lastQuantity / _lotMultiplier : undefined,
            lastPrice: msg.lastQuantity > 0 ? msg.lastPrice : undefined,
            leavesQuantity: ordStatus == Models.OrderStatus.Working ? msg.leavesQuantity / _lotMultiplier : undefined,
            cumQuantity: msg.cumQuantity / _lotMultiplier,
            averagePrice: msg.averagePrice
        };

        this.OrderUpdate.trigger(status);
    };

    private onCancelReject = (tsMsg : Models.Timestamped<CancelReject>) => {
        var msg = tsMsg.data;
        var status : Models.OrderStatusReport = {
            orderId: msg.clientOrderId,
            rejectMessage: msg.rejectReasonText,
            orderStatus: Models.OrderStatus.Rejected,
            cancelRejected: true,
            time: tsMsg.time
        };
        this.OrderUpdate.trigger(status);
    };

    private authMsg = <T>(payload : T) : AuthorizedHitBtcMessage<T> => {
        var msg = {nonce: this._nonce, payload: payload};
        this._nonce += 1;

        var signMsg = m => {
            return crypto.createHmac('sha512', this._secret)
                .update(JSON.stringify(m))
                .digest('base64');
        };

        return {apikey: this._apiKey, signature: signMsg(msg), message: msg};
    };

    private sendAuth = <T extends HitBtcPayload>(msgType : string, msg : T) => {
        var v = {};
        v[msgType] = msg;
        var readyMsg = this.authMsg(v);
        this._orderEntryWs.send(JSON.stringify(readyMsg));
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private onOpen = () => {
        this.sendAuth("Login", {});
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
    };

    private onMessage = (raw : string) => {
        var t = Utils.date();
        var msg = JSON.parse(raw);
        if (msg.hasOwnProperty("ExecutionReport")) {
            this.onExecutionReport(new Models.Timestamped(msg.ExecutionReport, t));
        }
        else if (msg.hasOwnProperty("CancelReject")) {
            this.onCancelReject(new Models.Timestamped(msg.CancelReject, t));
        }
        else {
            this._log("unhandled message", msg);
        }
    };

     _log : Utils.Logger = Utils.log("tribeca:gateway:HitBtcOE");
    private _apiKey : string;
    private _secret : string;
    constructor(config : Config.IConfigProvider) {
        this._apiKey = config.GetString("HitBtcApiKey");
        this._secret = config.GetString("HitBtcSecret");
        this._orderEntryWs = new ws(config.GetString("HitBtcOrderEntryUrl"));
        this._orderEntryWs.on('open', this.onOpen);
        this._orderEntryWs.on('message', this.onMessage);
        this._orderEntryWs.on("error", this.onMessage);
    }
}

interface HitBtcPositionReport {
    currency_code : string;
    cash : number;
    reserved : number;
}

class HitBtcPositionGateway implements Models.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:HitBtcPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private getAuth = (uri : string) : any => {
        var nonce : number = new Date().getTime() * 1000; // get rid of *1000 after getting new keys
        var comb = uri + "?" + querystring.stringify({nonce: nonce, apikey: this._apiKey});

        var signature = crypto.createHmac('sha512', this._secret)
                              .update(comb)
                              .digest('hex')
                              .toString()
                              .toLowerCase();

        return {url: url.resolve(this._pullUrl, uri),
                method: "GET",
                headers: {"X-Signature": signature},
                qs: {nonce: nonce.toString(), apikey: this._apiKey}};
    };

    private static convertCurrency(code : string) : Models.Currency {
        switch (code) {
            case "USD": return Models.Currency.USD;
            case "BTC": return Models.Currency.BTC;
            case "LTC": return Models.Currency.LTC;
            default: return null;
        }
    }

    private onTick = () => {
        request.get(
            this.getAuth("/api/1/trading/balance"),
            (err, body, resp) => {
                var rpts : Array<HitBtcPositionReport> = JSON.parse(resp).balance;

                if (typeof rpts === 'undefined' || err) {
                    this._log("Trouble getting positions err: %o body: %o", err, body.body);
                    return;
                }

                rpts.forEach(r => {
                    var currency = HitBtcPositionGateway.convertCurrency(r.currency_code);
                    if (currency == null) return;
                    var position = new Models.CurrencyPosition(r.cash, currency);
                    this.PositionUpdate.trigger(position);
                });
            });
    };

    private _apiKey : string;
    private _secret : string;
    private _pullUrl : string;
    constructor(config : Config.IConfigProvider) {
        this._apiKey = config.GetString("HitBtcApiKey");
        this._secret = config.GetString("HitBtcSecret");
        this._pullUrl = config.GetString("HitBtcPullUrl");
        this.onTick();
        setInterval(this.onTick, 15000);
    }
}

class HitBtcBaseGateway implements Models.IExchangeDetailsGateway {
    exchange() : Models.Exchange {
        return Models.Exchange.HitBtc;
    }

    makeFee() : number {
        return -0.0001;
    }

    takeFee() : number {
        return 0.001;
    }

    name() : string {
        return "HitBtc";
    }
}

export class HitBtc extends Models.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var orderGateway = config.GetString("HitBtcOrderDestination") == "HitBtc" ?
            <Models.IOrderEntryGateway>new HitBtcOrderEntryGateway(config)
            : new NullGateway.NullOrderGateway();

        // Payment actions are not permitted in demo mode -- helpful.
        var positionGateway = config.environment() == Config.Environment.Dev ?
            new NullGateway.NullPositionGateway() :
            new HitBtcPositionGateway(config);

        super(
            new HitBtcMarketDataGateway(config),
            orderGateway,
            positionGateway,
            new HitBtcBaseGateway());
    }
}