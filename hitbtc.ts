/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

var crypto = require('crypto');
var ws = require('ws');
var https = require('https');
var Q_lib = require('q');

var apikey = '004ee1065d6c7a6ac556bea221cd6338';
var secretkey = "aa14d615df5d47cb19a13ffe4ea638eb";

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

interface HitBtcOrderBook {
    asks : Array<Array<string>>;
    bids : Array<Array<string>>;
}

function authMsg<T>(payload : T) : AuthorizedHitBtcMessage<T> {
    var msg = {nonce: new Date().getTime(), payload: payload};

    var signMsg = function (m) : string {
        return crypto.createHmac('sha512', secretkey)
            .update(JSON.stringify(m))
            .digest('base64');
    };

    return {apikey: apikey, signature: signMsg(msg), message: msg};
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
    symbol : string;
    side : string;
    timestamp : string;
    price : number;
    quantity : number;
    type : string;
    timeInForce : string;
}

interface CancelReject {
    clientOrderId : string;
    cancelRequestClientOrderId : string;
    rejectReasonCode : string;
    rejectReasonText : string;
    timestamp : number;
}

class HitBtc implements IGateway {
    makeFee() : number {
        return -0.0001;
    }

    takeFee() : number {
        return 0.001;
    }

    name() : string {
        return "HitBtc";
    }

    ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();
    MarketData : Evt<MarketBook> = new Evt<MarketBook>();
    _ws : any;
    _log : Logger = log("HitBtc");

    private sendAuth = <T extends HitBtcPayload>(msgType : string, msg : T) => {
        var v = {};
        v[msgType] = msg;
        var readyMsg = authMsg(v);
        this._log(readyMsg);
        this._ws.send(JSON.stringify(readyMsg));
    };

    private onOpen = () => {
        this.sendAuth("Login", {});
        this.ConnectChanged.trigger(ConnectivityStatus.Connected);
    };

    _lastBook = null;
    private onMarketDataIncrementalRefresh = (msg : MarketDataIncrementalRefresh) => {
        if (msg.symbol != "BTCUSD" || this._lastBook == null) return;
        //this._log("onMarketDataIncrementalRefresh", msg);
    };

    private onMarketDataSnapshotFullRefresh = (msg : MarketDataSnapshotFullRefresh) => {
        if (msg.symbol != "BTCUSD") return;

        function getLevel(n : number) : MarketUpdate {
            return {bidPrice: msg.bid[n].price,
                bidSize: msg.bid[n].size / 100.0,
                askPrice: msg.ask[n].price,
                askSize: msg.ask[n].size / 100.0,
                time: new Date()};
        }

        this._lastBook = {bids: msg.bid.slice(0, 2), asks: msg.ask.slice(0, 2)};
        this.MarketData.trigger({top: getLevel(0), second: getLevel(1), exchangeName: Exchange.HitBtc});
    };

    private onExecutionReport = (msg : ExecutionReport) => {
        this._log("onExecutionReport", msg);
    };

    private onCancelReject = (msg : CancelReject) => {
        this._log("onCancelReject", msg);
    };

    private onMessage = (raw : string) => {
        var msg = JSON.parse(raw);
        if (msg.hasOwnProperty("MarketDataIncrementalRefresh")) {
            this.onMarketDataIncrementalRefresh(msg.MarketDataIncrementalRefresh);
        }
        else if (msg.hasOwnProperty("MarketDataSnapshotFullRefresh")) {
            this.onMarketDataSnapshotFullRefresh(msg.MarketDataSnapshotFullRefresh);
        }
        else if (msg.hasOwnProperty("ExecutionReport")) {
            this.onExecutionReport(msg.ExecutionReport);
        }
        else if (msg.hasOwnProperty("CancelReject")) {
            this.onCancelReject(msg.CancelReject);
        }
        else {
            this._log("unhandled message", msg);
        }
    };

    sendOrder = (order : BrokeredOrder) => {
        var getTif = (tif : TimeInForce) => {
            switch (tif) {
                case TimeInForce.FOK:
                    return "FOK";
                case TimeInForce.GTC:
                    return "GTC";
                case TimeInForce.IOC:
                    return "IOC";
                default:
                    throw new Error("TIF " + TimeInForce[tif] + " not supported in HitBtc");
            }
        };

        var getSide = (side : Side) => {
            switch (side) {
                case Side.Bid:
                    return "buy";
                case Side.Ask:
                    return "sell";
                default:
                    throw new Error("Side " + Side[side] + " not supported in HitBtc");
            }
        };

        var getType = (t : OrderType) => {
            switch (t) {
                case OrderType.Limit:
                    return "limit";
                case OrderType.Market:
                    return "market";
                default:
                    throw new Error("OrderType " + OrderType[t] + " not supported in HitBtc");
            }
        };

        var hitBtcOrder : NewOrder = {
            clientOrderId: order.orderId,
            symbol: "BTCUSD",
            side: getSide(order.side),
            quantity: order.quantity,
            type: getType(order.type),
            price: order.price,
            timeInForce: getTif(order.timeInForce)
        };

        this.sendAuth("NewOrder", hitBtcOrder);
    };

    constructor() {
        this._ws = new ws('ws://api.hitbtc.com:80');
        this._ws.on('open', this.onOpen);
        this._ws.on('message', this.onMessage);
        this._ws.on("error", this.onMessage);
    }
}