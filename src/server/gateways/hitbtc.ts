import WebSocket = require('uws');
import request = require('request');
import url = require("url");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import Interfaces = require("../interfaces");
import moment = require("moment");
import util = require("util");
const SortedMap = require("collections/sorted-map");
import querystring = require("querystring");
import crypto = require('crypto');

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
    private _data : Map<string, Models.MarketSide>;
    private _collator = new Intl.Collator(undefined, {numeric: true, sensitivity: 'base'})

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
    private _marketDataWs : WebSocket;

    private _hasProcessedSnapshot = false;

    private _lastBids = new SideMarketData(Models.Side.Bid);
    private _lastAsks = new SideMarketData(Models.Side.Ask);
    private onMarketDataIncrementalRefresh = (msg : MarketDataIncrementalRefresh, t : Date) => {
        if (msg.symbol !== this._gwSymbol || !this._hasProcessedSnapshot) return;
        this.onMarketDataUpdate(msg.bid, msg.ask, t);
    };

    private onMarketDataSnapshotFullRefresh = (msg : MarketDataSnapshotFullRefresh, t : Date) => {
        if (msg.symbol !== this._gwSymbol) return;
        this._lastAsks.clear();
        this._lastBids.clear();
        this.onMarketDataUpdate(msg.bid, msg.ask, t);
        this._hasProcessedSnapshot = true;
    };

    private onMarketDataUpdate = (bids : Update[], asks : Update[], t : Date) => {
        const ordBids = this.applyUpdates(bids, this._lastBids);
        const ordAsks = this.applyUpdates(asks, this._lastAsks);

        this._evUp('MarketDataGateway', new Models.Market(ordBids, ordAsks));
    };

    private applyUpdates(incomingUpdates : Update[], side : SideMarketData) {
        for (let u of incomingUpdates) {
            const ms = new Models.MarketSide(parseFloat(u.price), u.size * this._lotMultiplier);
            side.update(u.price, ms);
        }

        return side.getBest(13);
    }

    private onMessage = (raw: string) => {
        let msg: any;
        try {
            msg = JSON.parse(raw);
        }
        catch (e) {
            console.error('hitbtc', e, 'Error parsing msg', raw);
            throw e;
        }


        if (msg.hasOwnProperty("MarketDataIncrementalRefresh")) {
            this.onMarketDataIncrementalRefresh(msg.MarketDataIncrementalRefresh, new Date());
        }
        else if (msg.hasOwnProperty("MarketDataSnapshotFullRefresh")) {
            this.onMarketDataSnapshotFullRefresh(msg.MarketDataSnapshotFullRefresh, new Date());
        }
        else {
            console.info('hitbtc', 'unhandled message', msg);
        }
    };

    private onConnectionStatusChange = () => {
        if (this._marketDataWs.OPEN) {
            this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Connected);
        }
        else {
            this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Disconnected);
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

        this._evUp('MarketTradeGateway', new Models.GatewayMarketTrade(t.price, t.amount, side));
    };

    constructor(private _evUp, cfString, private _gwSymbol, private _lotMultiplier) {
        this._marketDataWs = new WebSocket(cfString("HitBtcMarketDataUrl"));
        this._marketDataWs.on('open', this.onConnectionStatusChange);
        this._marketDataWs.on('message', this.onMessage);
        this._marketDataWs.on("close", (code, msg) => {
            this.onConnectionStatusChange();
            console.warn('hitbtc', 'close code=', code, 'msg=', msg);
        });
        this._marketDataWs.on("error", err => {
            this.onConnectionStatusChange();
            console.error('hitbtc', err);
            throw err;
        });

        request.get(
            {url: url.resolve(cfString("HitBtcPullUrl"), "/api/1/public/" + this._gwSymbol + "/orderbook")},
            (err, body, resp) => {
                this.onMarketDataSnapshotFullRefresh(resp, new Date());
            });

        request.get(
            {url: url.resolve(cfString("HitBtcPullUrl"), "/api/1/public/" + this._gwSymbol + "/trades"),
             qs: {from: 0, by: "trade_id", sort: 'desc', start_index: 0, max_results: 100}},
            (err, body, resp) => {
                JSON.parse((<any>body).body).trades.forEach(t => {
                    const price = parseFloat(t[1]);
                    const size = parseFloat(t[2]);

                    this._evUp('MarketTradeGateway', new Models.GatewayMarketTrade(price, size, null));
                });
            })
    }
}

class HitBtcOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    private _orderEntryWs : WebSocket;

    public cancelsByClientOrderId = true;

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => {
      return new Promise<number>((resolve, reject) => {
        request(this.getAuth("/api/1/trading/orders/active", {symbols: this._gwSymbol}), (err, body, resp) => {
          var msg: any = JSON.parse(resp);
          if (!msg.orders || !msg.orders.length) { resolve(0); return; }
          msg.orders.forEach((o) => {
              request(this.getAuth("/api/1/trading/cancel_order", {clientOrderId: o.clientOrderId,
                cancelRequestClientOrderId: o.clientOrderId + "C",
                symbol: this._gwSymbol,
                side: o.side}), (err, body, resp) => {this.onMessage(resp);});
          });
          resolve(msg.orders.length);
        });
      });
    };

    _nonce = 1;

    cancelOrder = (cancel : Models.OrderStatusReport) => {
      request(this.getAuth("/api/1/trading/cancel_order", {clientOrderId: cancel.orderId,
            cancelRequestClientOrderId: cancel.orderId + "C",
            symbol: this._gwSymbol,
            side: HitBtcOrderEntryGateway.getSide(cancel.side)}), (err, body, resp) => {this.onMessage(resp);});
        // this.sendAuth<OrderCancel>("OrderCancel", , () => {
                // this._evUp('OrderUpdateGateway', {
                    // orderId: cancel.orderId,
                    // computationalLatency: new Date().valueOf() - cancel.time
                // });
            // });
    };

    replaceOrder = (replace : Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.OrderStatusReport) => {
        const hitBtcOrder : NewOrder = {
            clientOrderId: order.orderId,
            symbol: this._gwSymbol,
            side: HitBtcOrderEntryGateway.getSide(order.side),
            quantity: order.quantity / this._lotMultiplier,
            type: HitBtcOrderEntryGateway.getType(order.type),
            price: order.price,
            timeInForce: HitBtcOrderEntryGateway.getTif(order.timeInForce)
        };
      request(this.getAuth("/api/1/trading/new_order", hitBtcOrder), (err, body, resp) => {this.onMessage(resp);});

        // this.sendAuth<NewOrder>("NewOrder", hitBtcOrder, () => {
            // this._evUp('OrderUpdateGateway', {
                // orderId: order.orderId,
                // computationalLatency: new Date().valueOf() - order.time
            // });
        // });
    };

    private static getStatus(m : ExecutionReport) : Models.OrderStatus {
        switch (m.execReportType) {
            case "new":
            case "status":
                return Models.OrderStatus.Working;
            case "canceled":
            case "expired":
            case "rejected":
                return Models.OrderStatus.Cancelled;
            case "trade":
                if (m.orderStatus == "filled")
                    return Models.OrderStatus.Complete;
                else
                    return Models.OrderStatus.Working;
            default:
                return Models.OrderStatus.Cancelled;
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
        const msg = tsMsg.data;

        const status : Models.OrderStatusUpdate = {
            exchangeId: msg.orderId,
            orderId: msg.clientOrderId,
            orderStatus: HitBtcOrderEntryGateway.getStatus(msg),
            time: tsMsg.time.getTime()
        };

        if (msg.lastQuantity > 0 && msg.execReportType === "trade") {
            status.lastQuantity = msg.lastQuantity * this._lotMultiplier;
            status.lastPrice = msg.lastPrice;
        }

        if (msg.orderRejectReason)
            status.rejectMessage = msg.orderRejectReason;

        if (status.leavesQuantity)
            status.leavesQuantity = msg.leavesQuantity * this._lotMultiplier;

        if (msg.cumQuantity)
            status.cumQuantity = msg.cumQuantity * this._lotMultiplier;

        if (msg.averagePrice)
            status.averagePrice = msg.averagePrice;

        this._evUp('OrderUpdateGateway', status);
    };

    private onCancelReject = (tsMsg : Models.Timestamped<CancelReject>) => {
        const msg = tsMsg.data;
        const status : Models.OrderStatusUpdate = {
            orderId: msg.clientOrderId,
            rejectMessage: msg.rejectReasonText,
            orderStatus: Models.OrderStatus.Cancelled,
            time: tsMsg.time.getTime()
        };
        this._evUp('OrderUpdateGateway', status);
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

    private onConnectionStatusChange = () => {
        if (this._orderEntryWs.OPEN) {
            this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected);
        }
        else {
            this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Disconnected);
        }
    };

    private onOpen = () => {
      this.sendAuth("Login", {});
      this.onConnectionStatusChange();
    };

    private onClosed = (code, msg) => {
        this.onConnectionStatusChange();
        console.warn('hitbtc', 'close code=', code, 'msg=', msg);
    };

    private onError = (err : Error) => {
        this.onConnectionStatusChange();
        console.log('hitbtc', err);
        throw err;
    };

    private onMessage = (raw) => {
        try {
            var t = new Date();
            var msg = JSON.parse(raw);
            if (msg.hasOwnProperty("ExecutionReport")) {
                this.onExecutionReport(new Models.Timestamped(msg.ExecutionReport, new Date()));
            }
            else if (msg.hasOwnProperty("CancelReject")) {
                this.onCancelReject(new Models.Timestamped(msg.CancelReject, new Date()));
            }
            else {
                console.info('hitbtc', 'unhandled message', msg);
            }
        }
        catch (e) {
            console.error('hitbtc', e, 'exception while processing message', raw);
            throw e;
        }
    };

    private chars: string = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    generateClientOrderId = (): string => {
      let id: string = '';
      for(let i=16;i--;) id += this.chars.charAt(Math.floor(Math.random() * this.chars.length));
      return id;
    };

    private getAuth = (uri : string, param: any) : any => {
        const nonce : number = new Date().getTime() * 1000; // get rid of *1000 after getting new keys
        const comb = uri + "?" + querystring.stringify({nonce: nonce, apikey: this._apiKey}) + querystring.stringify(param);

        const signature = crypto.createHmac('sha512', this._secret)
                              .update(comb)
                              .digest('hex')
                              .toString()
                              .toLowerCase();

        return {url: this._pullUrl + uri + "?" + querystring.stringify({nonce: nonce, apikey: this._apiKey}),
                method: "POST",
                headers: {"Content-Type": "application/x-www-form-urlencoded", "X-Signature": signature},
                body: querystring.stringify(param)};
    };

    private _apiKey : string;
    private _secret : string;
    private _pullUrl: string;
    constructor(private _evUp, cfString, private _gwSymbol, private _lotMultiplier) {
        this._apiKey = cfString("HitBtcApiKey");
        this._secret = cfString("HitBtcSecret");
        this._pullUrl = cfString("HitBtcPullUrl");
        setTimeout(() => {this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected)}, 3000);
        // this._orderEntryWs = new WebSocket(cfString("HitBtcOrderEntryUrl"));
        // this._orderEntryWs.on('open', this.onOpen);
        // this._orderEntryWs.on('message', this.onMessage);
        // this._orderEntryWs.on('close', this.onConnectionStatusChange);
        // this._orderEntryWs.on("error", this.onError);
    }
}

export class HitBtc extends Interfaces.CombinedGateway {
    constructor(
      gwSymbol,
      cfString,
      _evOn,
      _evUp,
      lot
    ) {
        const orderGateway = cfString("HitBtcOrderDestination") == "HitBtc" ?
            <Interfaces.IOrderEntryGateway>new HitBtcOrderEntryGateway(_evUp, cfString, gwSymbol, lot)
            : new NullGateway.NullOrderGateway(_evUp);

        new HitBtcMarketDataGateway(_evUp, cfString, gwSymbol, lot);

        super(
          orderGateway
        );
    }
}
