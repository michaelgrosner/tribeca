/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />
/// <reference path="../interfaces.ts"/>

import Config = require("../config");
import crypto = require('crypto');
import WebSocket from "../websocket";
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import io = require("socket.io-client");
import moment = require("moment");
import util = require("util");
import * as Q from "q";
import log from "../logging";
const shortId = require("shortid");
const SortedMap = require("collections/sorted-map");

const _lotMultiplier = 100.0;

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
    asks : [string, string][];
    bids : [string, string][];
}

interface Update {
    price : string;
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
    execReportType : "new"|"canceled"|"rejected"|"expired"|"trade"|"status";
    orderStatus : "new"|"partiallyFilled"|"filled"|"canceled"|"rejected"|"expired";
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

class SideMarketData {
    private readonly _data : Map<string, Models.MarketSide>;
    private readonly _collator = new Intl.Collator(undefined, {numeric: true, sensitivity: 'base'})

    constructor(side: Models.Side) {
        const compare = side === Models.Side.Bid ? 
            ((a,b) => this._collator.compare(b,a)) : 
            ((a,b) => this._collator.compare(a,b));
        this._data = new SortedMap([], null, compare);
    }

    public update = (k: string, v: Models.MarketSide) : void => {
        if (v.size === 0) {
            this._data.delete(k);
            return;
        }

        const existing = this._data.get(k);
        if (existing) {
            existing.size = v.size;
        }
        else {
            this._data.set(k, v);
        }
    }

    public clear = () : void => this._data.clear();

    public getBest = (n: number) : Models.MarketSide[] => {
        const b = new Array<Models.MarketSide>();
        const it = (<any>(this._data)).iterator();

        while (b.length < n) {
            let x : {done: boolean, value: {key: string, value: Models.MarketSide}} = it.next();
            if (x.done) return b;
            b.push(x.value.value);
        }

        return b;
    };

    public any = () : boolean => (<any>(this._data)).any();
    public min = () : Models.MarketSide => (<any>(this._data)).min();
    public max = () : Models.MarketSide => (<any>(this._data)).max();
}

class HitBtcMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private readonly _marketDataWs : WebSocket;

    private _hasProcessedSnapshot = false;

    private readonly _lastBids = new SideMarketData(Models.Side.Bid);
    private readonly _lastAsks = new SideMarketData(Models.Side.Ask);
    private onMarketDataIncrementalRefresh = (msg : MarketDataIncrementalRefresh, t : Date) => {
        if (msg.symbol !== this._symbolProvider.symbol || !this._hasProcessedSnapshot) return;
        this.onMarketDataUpdate(msg.bid, msg.ask, t);
    };

    private onMarketDataSnapshotFullRefresh = (msg : MarketDataSnapshotFullRefresh, t : Date) => {
        if (msg.symbol !== this._symbolProvider.symbol) return;
        this._lastAsks.clear();
        this._lastBids.clear();
        this.onMarketDataUpdate(msg.bid, msg.ask, t);
        this._hasProcessedSnapshot = true;
    };

    private onMarketDataUpdate = (bids : Update[], asks : Update[], t : Date) => {
        const ordBids = this.applyUpdates(bids, this._lastBids);
        const ordAsks = this.applyUpdates(asks, this._lastAsks);

        this.MarketData.trigger(new Models.Market(ordBids, ordAsks, t));
    };

    private applyUpdates(incomingUpdates : Update[], side : SideMarketData) {
        for (let u of incomingUpdates) {
            const ms = new Models.MarketSide(parseFloat(u.price), u.size / _lotMultiplier);            
            side.update(u.price, ms);
        }

        return side.getBest(25);
    }

    private onMessage = (raw : Models.Timestamped<string>) => {
        let msg: any;
        try {
            msg = JSON.parse(raw.data);
        }
        catch (e) {
            this._log.error(e, "Error parsing msg", raw);
            throw e;
        }

        if (this._log.debug())
            this._log.debug(msg, "message");

        if (msg.hasOwnProperty("MarketDataIncrementalRefresh")) {
            this.onMarketDataIncrementalRefresh(msg.MarketDataIncrementalRefresh, raw.time);
        }
        else if (msg.hasOwnProperty("MarketDataSnapshotFullRefresh")) {
            this.onMarketDataSnapshotFullRefresh(msg.MarketDataSnapshotFullRefresh, raw.time);
        }
        else {
            this._log.info("unhandled message", msg);
        }
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private onConnectionStatusChange = () => {
        if (this._marketDataWs.isConnected && this._tradesClient.connected) {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        }
        else {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        }
    };
    
    private onTrade = (t: MarketTrade) => {
        let side : Models.Side = Models.Side.Unknown;
        if (this._lastAsks.any() && this._lastBids.any()) {
            const distance_from_bid = Math.abs(this._lastBids.max().price - t.price);
            const distance_from_ask = Math.abs(this._lastAsks.min().price - t.price);
            if (distance_from_bid < distance_from_ask) side = Models.Side.Bid;
            if (distance_from_bid > distance_from_ask) side = Models.Side.Ask;
        }
        
        this.MarketTrade.trigger(new Models.GatewayMarketTrade(t.price, t.amount, new Date(), false, side));
    };

    private readonly _tradesClient : SocketIOClient.Socket;
    private readonly _log = log("tribeca:gateway:HitBtcMD");
    constructor(
            config : Config.IConfigProvider, 
            private readonly _symbolProvider: HitBtcSymbolProvider, 
            private readonly _minTick: number) {

        this._marketDataWs = new WebSocket(
            config.GetString("HitBtcMarketDataUrl"), 5000, 
            this.onMessage, 
            this.onConnectionStatusChange, 
            this.onConnectionStatusChange);
        this._marketDataWs.connect();

        this._tradesClient = io.connect(config.GetString("HitBtcSocketIoUrl") + "/trades/" + this._symbolProvider.symbol);
        this._tradesClient.on("connect", this.onConnectionStatusChange);
        this._tradesClient.on("trade", this.onTrade);
        this._tradesClient.on("disconnect", this.onConnectionStatusChange);

        request.get(
            {url: url.resolve(config.GetString("HitBtcPullUrl"), "/api/1/public/" + this._symbolProvider.symbol + "/orderbook")},
            (err, body, resp) => {
                this.onMarketDataSnapshotFullRefresh(resp, Utils.date());
            });

        request.get(
            {url: url.resolve(config.GetString("HitBtcPullUrl"), "/api/1/public/" + this._symbolProvider.symbol + "/trades"),
             qs: {from: 0, by: "trade_id", sort: 'desc', start_index: 0, max_results: 100}},
            (err, body, resp) => {
                JSON.parse((<any>body).body).trades.forEach(t => {
                    const price = parseFloat(t[1]);
                    const size = parseFloat(t[2]);
                    const time = new Date(t[3]);

                    this.MarketTrade.trigger(new Models.GatewayMarketTrade(price, size, time, true, null));
                });
            })
    }
}

class HitBtcOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    private readonly _orderEntryWs : WebSocket;

    public cancelsByClientOrderId = true;
    
    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Q.Promise<number> => { return Q(0); };

    _nonce = 1;

    cancelOrder = (cancel : Models.OrderStatusReport) => {
        this.sendAuth<OrderCancel>("OrderCancel", {clientOrderId: cancel.orderId,
            cancelRequestClientOrderId: cancel.orderId + "C",
            symbol: this._symbolProvider.symbol,
            side: HitBtcOrderEntryGateway.getSide(cancel.side)}, () => {
                this.OrderUpdate.trigger({
                    orderId: cancel.orderId,
                    computationalLatency: Utils.fastDiff(Utils.date(), cancel.time)
                });
            });
    };

    replaceOrder = (replace : Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.OrderStatusReport) => {
        const hitBtcOrder : NewOrder = {
            clientOrderId: order.orderId,
            symbol: this._symbolProvider.symbol,
            side: HitBtcOrderEntryGateway.getSide(order.side),
            quantity: order.quantity * _lotMultiplier,
            type: HitBtcOrderEntryGateway.getType(order.type),
            price: order.price,
            timeInForce: HitBtcOrderEntryGateway.getTif(order.timeInForce)
        };

        this.sendAuth<NewOrder>("NewOrder", hitBtcOrder, () => {
            this.OrderUpdate.trigger({
                orderId: order.orderId,
                computationalLatency: Utils.fastDiff(Utils.date(), order.time)
            });
        });
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
        }
    }

    private onExecutionReport = (tsMsg : Models.Timestamped<ExecutionReport>) => {
        const t = tsMsg.time;
        const msg = tsMsg.data;

        const ordStatus = HitBtcOrderEntryGateway.getStatus(msg);

        let lastQuantity : number = undefined;
        let lastPrice : number = undefined;
        

        const status : Models.OrderStatusUpdate = {
            exchangeId: msg.orderId,
            orderId: msg.clientOrderId,
            orderStatus: ordStatus,
            time: t,
        };

        if (msg.lastQuantity > 0 && msg.execReportType === "trade") {
            status.lastQuantity = msg.lastQuantity / _lotMultiplier;
            status.lastPrice = msg.lastPrice;
        }

        if (msg.orderRejectReason)
            status.rejectMessage = msg.orderRejectReason;

        if (status.leavesQuantity)
            status.leavesQuantity = msg.leavesQuantity / _lotMultiplier;
        
        if (msg.cumQuantity)
            status.cumQuantity = msg.cumQuantity / _lotMultiplier;

        if (msg.averagePrice)
            status.averagePrice = msg.averagePrice;

        this.OrderUpdate.trigger(status);
    };

    private onCancelReject = (tsMsg : Models.Timestamped<CancelReject>) => {
        const msg = tsMsg.data;
        const status : Models.OrderStatusUpdate = {
            orderId: msg.clientOrderId,
            rejectMessage: msg.rejectReasonText,
            orderStatus: Models.OrderStatus.Rejected,
            cancelRejected: true,
            time: tsMsg.time
        };
        this.OrderUpdate.trigger(status);
    };

    private authMsg = <T>(payload : T) : AuthorizedHitBtcMessage<T> => {
        const msg = {nonce: this._nonce, payload: payload};
        this._nonce += 1;

        const signMsg = m => {
            return crypto.createHmac('sha512', this._secret)
                .update(JSON.stringify(m))
                .digest('base64');
        };

        return {apikey: this._apiKey, signature: signMsg(msg), message: msg};
    };

    private sendAuth = <T extends HitBtcPayload>(msgType : string, msg : T, cb?: () => void) => {
        const v = {};
        v[msgType] = msg;
        const readyMsg = this.authMsg(v);
        this._orderEntryWs.send(JSON.stringify(readyMsg), cb);
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private onConnectionStatusChange = () => {
        if (this._orderEntryWs.isConnected) {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        }
        else {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        }
    };

    private onOpen = () => {
        this.sendAuth("Login", {});
        this.onConnectionStatusChange();
    };

    private onMessage = (raw : Models.Timestamped<string>) => {
        try {
            const msg = JSON.parse(raw.data);

            if (this._log.debug())
                this._log.debug(msg, "message");

            if (msg.hasOwnProperty("ExecutionReport")) {
                this.onExecutionReport(new Models.Timestamped(msg.ExecutionReport, raw.time));
            }
            else if (msg.hasOwnProperty("CancelReject")) {
                this.onCancelReject(new Models.Timestamped(msg.CancelReject, raw.time));
            }
            else {
                this._log.info("unhandled message", msg);
            }
        }
        catch (e) {
            this._log.error(e, "exception while processing message", raw);
            throw e;
        }
    };

    generateClientOrderId = () => {
        return shortId.generate();
    }

    private readonly _log = log("tribeca:gateway:HitBtcOE");
    private readonly _apiKey : string;
    private readonly _secret : string;
    constructor(config : Config.IConfigProvider, private _symbolProvider: HitBtcSymbolProvider, private _details: HitBtcBaseGateway) {
        this._apiKey = config.GetString("HitBtcApiKey");
        this._secret = config.GetString("HitBtcSecret");
        this._orderEntryWs = new WebSocket(config.GetString("HitBtcOrderEntryUrl"), 5000, 
            this.onMessage, 
            this.onOpen, 
            this.onConnectionStatusChange);
        this._orderEntryWs.connect();
    }
}

interface HitBtcPositionReport {
    currency_code : string;
    cash : number;
    reserved : number;
}

class HitBtcPositionGateway implements Interfaces.IPositionGateway {
    private readonly _log = log("tribeca:gateway:HitBtcPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private getAuth = (uri : string) : any => {
        const nonce : number = new Date().getTime() * 1000; // get rid of *1000 after getting new keys
        const comb = uri + "?" + querystring.stringify({nonce: nonce, apikey: this._apiKey});

        const signature = crypto.createHmac('sha512', this._secret)
                              .update(comb)
                              .digest('hex')
                              .toString()
                              .toLowerCase();

        return {url: url.resolve(this._pullUrl, uri),
                method: "GET",
                headers: {"X-Signature": signature},
                qs: {nonce: nonce.toString(), apikey: this._apiKey}};
    };

    private onTick = () => {
        request.get(
            this.getAuth("/api/1/trading/balance"),
            (err, body, resp) => {
                try {
                    const rpts: Array<HitBtcPositionReport> = JSON.parse(resp).balance;
                    if (typeof rpts === 'undefined' || err) {
                        this._log.warn(err, "Trouble getting positions", body.body);
                        return;
                    }

                    rpts.forEach(r => {
                        let currency: Models.Currency;
                        try {
                            currency = Models.toCurrency(r.currency_code);
                        }
                        catch (e) {
                            return;
                        }
                        if (currency == null) return;
                        const position = new Models.CurrencyPosition(r.cash, r.reserved, currency);
                        this.PositionUpdate.trigger(position);
                    });
                }
                catch (e) {
                    this._log.error(e, "Error processing JSON response ", resp);
                }
            });
    };

    private readonly _apiKey : string;
    private readonly _secret : string;
    private readonly _pullUrl : string;
    constructor(config : Config.IConfigProvider) {
        this._apiKey = config.GetString("HitBtcApiKey");
        this._secret = config.GetString("HitBtcSecret");
        this._pullUrl = config.GetString("HitBtcPullUrl");
        this.onTick();
        setInterval(this.onTick, 15000);
    }
}

class HitBtcBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

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

    constructor(public minTickIncrement: number) {}
}

class HitBtcSymbolProvider {
    public readonly symbol : string;
    
    constructor(pair: Models.CurrencyPair) {
        this.symbol = Models.fromCurrency(pair.base) + Models.fromCurrency(pair.quote);
    }
}

class HitBtc extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider, symbolProvider: HitBtcSymbolProvider, step: number, pair: Models.CurrencyPair) {
        const details = new HitBtcBaseGateway(step);
        const orderGateway = config.GetString("HitBtcOrderDestination") == "HitBtc" ?
            <Interfaces.IOrderEntryGateway>new HitBtcOrderEntryGateway(config, symbolProvider, details)
            : new NullGateway.NullOrderGateway();

        // Payment actions are not permitted in demo mode -- helpful.
        let positionGateway : Interfaces.IPositionGateway = new HitBtcPositionGateway(config);
        if (config.GetString("HitBtcPullUrl").indexOf("demo") > -1) {
            positionGateway = new NullGateway.NullPositionGateway(pair);
        }

        super(
            new HitBtcMarketDataGateway(config, symbolProvider, step),
            orderGateway,
            positionGateway,
            details);
    }
}

interface HitBtcSymbol {
    symbol: string,
    step: string,
    lot: string,
    currency: string,
    commodity: string,
    takeLiquidityRate: string,
    provideLiquidityRate: string
}

export async function createHitBtc(config : Config.IConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    const symbolsUrl = config.GetString("HitBtcPullUrl") + "/api/1/public/symbols";
    const symbols = await Utils.getJSON<{symbols: HitBtcSymbol[]}>(symbolsUrl);
    const symbolProvider = new HitBtcSymbolProvider(pair);

    for (let s of symbols.symbols) {
        if (s.symbol === symbolProvider.symbol) 
            return new HitBtc(config, symbolProvider, parseFloat(s.step), pair);
    }

    throw new Error("unable to match pair to a hitbtc symbol " + pair.toString());
}