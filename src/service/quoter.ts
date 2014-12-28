/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Aggregators = require("./aggregators");

export enum QuoteAction { New, Cancel }
export enum QuoteSent { First, Modify, UnsentDuplicate, Delete }

export class Quote {
    constructor(public type : QuoteAction,
                public side : Models.Side,
                public exchange : Models.Exchange,
                public time : Moment,
                public price : number = null,
                public size : number = null) {}

    public equals(other : Quote, tol : number = 1e-3) {
        return this.type == other.type
            && this.side == other.side
            && this.exchange == other.exchange
            && Math.abs(this.price - other.price) < tol
            && Math.abs(this.size - other.size) < tol;
    }
}

class QuoteOrder {
    constructor(public quote : Quote, public orderId : string) {}
}

export class Quoter {
    private _activeQuotes : { [ exch : number] : { [ side : number] : QuoteOrder } } = {};

    constructor(private _orderAgg : Aggregators.OrderBrokerAggregator) {
        this._orderAgg.OrderUpdate.on(this.handleOrderUpdate);
    }

    private handleOrderUpdate = (o : Models.OrderStatusReport) => {
        switch (o.orderStatus) {
            case Models.OrderStatus.Cancelled:
            case Models.OrderStatus.Complete:
            case Models.OrderStatus.Rejected:
                var byExch = this._activeQuotes[o.exchange];
                if (typeof byExch !== "undefined") {
                    var bySide = byExch[o.side];
                    if (typeof bySide !== "undefined") {
                        if (this._activeQuotes[o.exchange][o.side].orderId === o.orderId) {
                            delete this._activeQuotes[o.exchange][o.side];
                        }
                    }
                }
        }
    };

    public updateQuote = (q : Quote) : QuoteSent => {
        switch (q.type) {
            case QuoteAction.New:
                if (typeof this._activeQuotes[q.exchange] !== "undefined"
                    && typeof this._activeQuotes[q.exchange][q.side] !== "undefined") {
                    return this.modify(q);
                }
                else {
                    return this.start(q);
                }
            case QuoteAction.Cancel:
                return this.stop(q);
            default:
                throw new Error("Unknown QuoteAction " + QuoteAction[q.type]);
        }
    };

    private modify = (q : Quote) : QuoteSent => {
        this.stop(q);
        this.start(q);
        return QuoteSent.Modify;
    };

    private start = (q : Quote) : QuoteSent => {
        if (typeof this._activeQuotes[q.exchange] === "undefined") {
            this._activeQuotes[q.exchange] = {};
        }
        else {
            var existing = this._activeQuotes[q.exchange][q.side];
            if (typeof existing !== "undefined" && existing.quote.equals(q)) {
                return QuoteSent.UnsentDuplicate;
            }
        }

        var newOrder = new Models.SubmitNewOrder(q.side, q.size, Models.OrderType.Limit, q.price, Models.TimeInForce.GTC, q.exchange, q.time);
        var sent = this._orderAgg.submitOrder(newOrder);
        this._activeQuotes[q.exchange][q.side] = new QuoteOrder(q, sent.sentOrderClientId);
        return QuoteSent.First;
    };

    private stop = (q : Quote) : QuoteSent => {
        var cxl = new Models.OrderCancel(this._activeQuotes[q.exchange], q.exchange, q.time);
        this._orderAgg.cancelOrder(cxl);

        if (typeof this._activeQuotes[q.exchange] === "undefined") {
            this._activeQuotes[q.exchange] = {};
        }

        delete this._activeQuotes[q.exchange][q.side];
        return QuoteSent.Delete;
    };
}