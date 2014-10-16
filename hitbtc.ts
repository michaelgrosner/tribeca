/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

module HitBtc {

    var crypto = require('crypto');
    var ws = require('ws');
    var request = require('request');

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

    export class HitBtc implements IGateway {
        exchange() : Exchange {
            return Exchange.HitBtc;
        }

        OrderUpdate : Evt<GatewayOrderStatusReport> = new Evt<GatewayOrderStatusReport>();

        cancelOrder = (cancel : BrokeredCancel) => {
            this.sendAuth("OrderCancel", {clientOrderId: cancel.clientOrderId,
                cancelRequestClientOrderId: cancel.requestId,
                symbol: "BTCUSD",
                side: HitBtc.getSide(cancel.side)})
        };

        replaceOrder = (replace : BrokeredReplace) => {
            this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
            this.sendOrder(replace);
        };

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
        _marketDataWs : any;
        _orderEntryWs : any;
        _log : Logger = log("Hudson:Gateway:HitBtc");
        _nonce = 1;
        _lotMultiplier = 100.0;

        private authMsg = <T>(payload : T) : AuthorizedHitBtcMessage<T> => {
            var msg = {nonce: this._nonce, payload: payload};
            this._nonce += 1;

            var signMsg = function (m) : string {
                return crypto.createHmac('sha512', secretkey)
                    .update(JSON.stringify(m))
                    .digest('base64');
            };

            return {apikey: apikey, signature: signMsg(msg), message: msg};
        };

        private sendAuth = <T extends HitBtcPayload>(msgType : string, msg : T) => {
            var v = {};
            v[msgType] = msg;
            var readyMsg = this.authMsg(v);
            this._log("sending authorized message <%s> %j", msgType, readyMsg);
            this._orderEntryWs.send(JSON.stringify(readyMsg));
        };

        private onOpen = (needsLogin : boolean) => {
            if (needsLogin) this.sendAuth("Login", {});
            this.ConnectChanged.trigger(ConnectivityStatus.Connected);
        };

        _lastBook : { [side: string] : { [px: number]: number}} = null;
        private onMarketDataIncrementalRefresh = (msg : MarketDataIncrementalRefresh) => {
            if (msg.symbol != "BTCUSD" || this._lastBook == null) return;

            // todo: they say they send it?...
            var t : Date = msg.timestamp == undefined ? new Date() : new Date(msg.timestamp/1000.0);

            var ordBids = this._applyIncrementals(msg.bid, this._lastBook["bid"], (a, b) => a.price > b.price ? -1 : 1);
            var ordAsks = this._applyIncrementals(msg.ask, this._lastBook["ask"], (a, b) => a.price > b.price ? 1 : -1);

            var getLevel = (n : number) => {
                var bid = new MarketSide(ordBids[n].price, ordBids[n].size);
                var ask = new MarketSide(ordAsks[n].price, ordAsks[n].size);
                return new MarketUpdate(bid, ask, t);
            };

            this.MarketData.trigger(new MarketBook(getLevel(0), getLevel(1), Exchange.HitBtc));
        };

        private _applyIncrementals(incomingUpdates : Update[],
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
                kvps.push(new SideUpdate(parseFloat(px), side[px] / this._lotMultiplier));
            }
            return kvps.sort(cmp);
        }

        private getLevel(msg : MarketDataSnapshotFullRefresh, n : number) : MarketUpdate {
            var bid = new MarketSide(msg.bid[n].price, msg.bid[n].size / this._lotMultiplier);
            var ask = new MarketSide(msg.ask[n].price, msg.ask[n].size / this._lotMultiplier);
            return new MarketUpdate(bid, ask, new Date());
        }

        private onMarketDataSnapshotFullRefresh = (msg : MarketDataSnapshotFullRefresh) => {
            if (msg.symbol != "BTCUSD") return;

            this._lastBook = {bid: {}, ask: {}};

            for (var i = 0; i < msg.ask.length; i++) {
                this._lastBook["ask"][msg.ask[i].price] = msg.ask[i].size;
            }

            for (var i = 0; i < msg.bid.length; i++) {
                this._lastBook["bid"][msg.bid[i].price] = msg.bid[i].size;
            }

            this.MarketData.trigger({top: this.getLevel(msg, 0),
                second: this.getLevel(msg, 1),
                exchangeName: Exchange.HitBtc});
        };

        private static getStatus(m : ExecutionReport) : OrderStatus {
            if (m.execReportType == "new") return OrderStatus.Working;
            if (m.execReportType == "canceled") return OrderStatus.Cancelled;
            if (m.execReportType == "rejected") return OrderStatus.Rejected;
            if (m.execReportType == "expired") return OrderStatus.Cancelled;
            if (m.orderStatus == "partiallyFilled") return OrderStatus.PartialFill;
            if (m.orderStatus == "filled") return OrderStatus.Filled;
            return OrderStatus.Other;
        }

        private onExecutionReport = (msg : ExecutionReport) => {
            var status : GatewayOrderStatusReport = {
                exchangeId: msg.orderId,
                orderId: msg.clientOrderId,
                orderStatus: HitBtc.getStatus(msg),
                time: new Date(msg.timestamp),
                rejectMessage: msg.orderRejectReason,
                lastQuantity: msg.lastQuantity / this._lotMultiplier,
                lastPrice: msg.lastPrice,
                leavesQuantity: msg.leavesQuantity / this._lotMultiplier,
                cumQuantity: msg.cumQuantity / this._lotMultiplier,
                averagePrice: msg.averagePrice
            };

            this.OrderUpdate.trigger(status);
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
                this._log(msg);
                this.onExecutionReport(msg.ExecutionReport);
            }
            else if (msg.hasOwnProperty("CancelReject")) {
                this._log(msg);
                this.onCancelReject(msg.CancelReject);
            }
            else {
                this._log("unhandled message", msg);
            }
        };

        private static getTif(tif : TimeInForce) {
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
        }

        private static getSide(side : Side) {
            switch (side) {
                case Side.Bid:
                    return "buy";
                case Side.Ask:
                    return "sell";
                default:
                    throw new Error("Side " + Side[side] + " not supported in HitBtc");
            }
        }

        private static getType(t : OrderType) {
            switch (t) {
                case OrderType.Limit:
                    return "limit";
                case OrderType.Market:
                    return "market";
                default:
                    throw new Error("OrderType " + OrderType[t] + " not supported in HitBtc");
            }
        }

        sendOrder = (order : BrokeredOrder) => {
            var hitBtcOrder : NewOrder = {
                clientOrderId: order.orderId,
                symbol: "BTCUSD",
                side: HitBtc.getSide(order.side),
                quantity: order.quantity * this._lotMultiplier,
                type: HitBtc.getType(order.type),
                price: order.price,
                timeInForce: HitBtc.getTif(order.timeInForce)
            };

            this.sendAuth("NewOrder", hitBtcOrder);

            var rpt : GatewayOrderStatusReport = {
                orderId: order.orderId,
                orderStatus: OrderStatus.New,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        };

        constructor() {
            this._marketDataWs = new ws('ws://demo-api.hitbtc.com:80');
            this._marketDataWs.on('open', () => this.onOpen(false));
            this._marketDataWs.on('message', this.onMessage);
            this._marketDataWs.on("error", this.onMessage);

            this._orderEntryWs = new ws("ws://demo-api.hitbtc.com:8080");
            this._orderEntryWs.on('open', () => this.onOpen(true));
            this._orderEntryWs.on('message', this.onMessage);
            this._orderEntryWs.on("error", this.onMessage);

            request.get(
                {url: "https://api.hitbtc.com/api/1/public/BTCUSD/orderbook"},
                (err, body, resp) => {
                    this.onMarketDataSnapshotFullRefresh(resp);
                });
        }
    }
}