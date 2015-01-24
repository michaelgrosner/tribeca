/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("./config");
import crypto = require('crypto');
import ws = require('ws');
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import io = require("socket.io-client");
import moment = require("moment");

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

interface MarketTrade {
    price : number;
    amount : number;
}

class HitBtcMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.MarketSide>();
    _marketDataWs : ws;

    private _hasProcessedSnapshot = false;
    private _lastBids : { [px: number]: number} = {};
    private _lastAsks : { [px: number]: number} = {};
    private onMarketDataIncrementalRefresh = (msg : MarketDataIncrementalRefresh, t : Moment) => {
        if (msg.symbol != "BTCUSD" || !this._hasProcessedSnapshot) return;
        this.onMarketDataUpdate(msg.bid, msg.ask, t);
    };

    private onMarketDataSnapshotFullRefresh = (msg : MarketDataSnapshotFullRefresh, t : Moment) => {
        if (msg.symbol != "BTCUSD") return;
        this.onMarketDataUpdate(msg.bid, msg.ask, t);
        this._hasProcessedSnapshot = true;
    };

    private onMarketDataUpdate = (bids : Update[], asks : Update[], t : Moment) => {
        var ordBids = HitBtcMarketDataGateway._applyIncrementals(bids, this._lastBids, (a, b) => a.price > b.price ? -1 : 1);
        var ordAsks = HitBtcMarketDataGateway._applyIncrementals(asks, this._lastAsks, (a, b) => a.price > b.price ? 1 : -1);

        this.MarketData.trigger(new Models.Market(ordBids, ordAsks, t));
    };

    private static _applyIncrementals(incomingUpdates : Update[],
                                      side : { [px: number]: number},
                                      cmp : (p1 : Models.MarketSide, p2 : Models.MarketSide) => number) {
        for (var i = 0; i < incomingUpdates.length; i++) {
            var u : Update = incomingUpdates[i];
            if (u.size == 0) {
                delete side[u.price];
            }
            else {
                side[u.price] = u.size;
            }
        }

        var kvps : Models.MarketSide[] = [];
        for (var px in side) {
            kvps.push(new Models.MarketSide(parseFloat(px), side[px] / _lotMultiplier));
        }
        return kvps.sort(cmp);
    }

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
    private onConnectionStatusChange = () => {
        if (this._marketDataWs.readyState === ws.OPEN && (<any>this._tradesClient).connected) {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        }
        else {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        }
    };

    _tradesClient : SocketIOClient.Socket;
     _log : Utils.Logger = Utils.log("tribeca:gateway:HitBtcMD");
    constructor(config : Config.IConfigProvider) {
        this._marketDataWs = new ws(config.GetString("HitBtcMarketDataUrl"));
        this._marketDataWs.on('open', this.onConnectionStatusChange);
        this._marketDataWs.on('message', this.onMessage);
        this._marketDataWs.on("error", this.onConnectionStatusChange);

        this._log("socket.io: %s", config.GetString("HitBtcSocketIoUrl") + "/trades/BTCUSD");
        this._tradesClient = io.connect(config.GetString("HitBtcSocketIoUrl") + "/trades/BTCUSD");
        this._tradesClient.on("connect", this.onConnectionStatusChange);
        this._tradesClient.on("trade", (t : MarketTrade) => {
            this.MarketTrade.trigger(new Models.GatewayMarketTrade(t.price, t.amount, Utils.date(), false));
        });
        this._tradesClient.on("disconnect", this.onConnectionStatusChange);

        request.get(
            {url: url.resolve(config.GetString("HitBtcPullUrl"), "/api/1/public/BTCUSD/orderbook")},
            (err, body, resp) => {
                this.onMarketDataSnapshotFullRefresh(resp, Utils.date());
            });

        request.get(
            {url: url.resolve(config.GetString("HitBtcPullUrl"), "/api/1/public/BTCUSD/trades"),
             qs: {from: 0, by: "trade_id", sort: 'desc', start_index: 0, max_results: 100}},
            (err, body, resp) => {
                JSON.parse(body.body).trades.forEach(t => {
                    var price = parseFloat(t[1]);
                    var size = parseFloat(t[2]);
                    var time = moment(t[3]);

                    this.MarketTrade.trigger(new Models.GatewayMarketTrade(price, size, time, true));
                });
            })
    }
}

class HitBtcOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    _orderEntryWs : ws;

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
            price: Utils.roundFloat(order.price),
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
            case "expired":
                return Models.OrderStatus.Cancelled;
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
        try {
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
        }
        catch (e) {
            this._log("exception while processing message %s", raw);
            throw e;
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
        this._orderEntryWs.on("error", this._log);
    }
}

interface HitBtcPositionReport {
    currency_code : string;
    cash : number;
    reserved : number;
}

class HitBtcPositionGateway implements Interfaces.IPositionGateway {
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

class HitBtcBaseGateway implements Interfaces.IExchangeDetailsGateway {
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

export class HitBtc extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var orderGateway = config.GetString("HitBtcOrderDestination") == "HitBtc" ?
            <Interfaces.IOrderEntryGateway>new HitBtcOrderEntryGateway(config)
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