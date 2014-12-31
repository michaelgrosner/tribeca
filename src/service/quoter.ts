/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Aggregators = require("./aggregators");

class QuoteOrder {
    constructor(public quote : Models.Quote, public orderId : string) {}
}

// aggregator for quoting
export class Quoter {
    private _quotersByExchange : { [ exch : number] : ExchangeQuoter } = {};

    constructor(exchQuoters : ExchangeQuoter[]) {
        exchQuoters.forEach(q => this._quotersByExchange[q.exchange] = q);
    }

    public updateQuote = (q : Models.Quote) : Models.QuoteSent => {
        return this._quotersByExchange[q.exchange].updateQuote(q);
    };
}

// idea: single EQ per side, switch side in Quoter above
// wraps a single broker to make orders behave like quotes
export class ExchangeQuoter {
    private _activeQuotes : { [ side : number] : QuoteOrder } = {};

    constructor(private _broker : Interfaces.IBroker) {
        this._broker.OrderUpdate.on(this.handleOrderUpdate);
    }

    public get exchange() {
        return this._broker.exchange();
    }

    private handleOrderUpdate = (o : Models.OrderStatusReport) => {
        switch (o.orderStatus) {
            case Models.OrderStatus.Cancelled:
            case Models.OrderStatus.Complete:
            case Models.OrderStatus.Rejected:
                var bySide = this._activeQuotes[o.side];
                if (typeof bySide !== "undefined" && bySide.orderId === o.orderId) {
                    delete this._activeQuotes[o.side];
                }
        }
    };

    public updateQuote = (q : Models.Quote) : Models.QuoteSent => {
        if (this._broker.connectStatus !== Models.ConnectivityStatus.Connected)
            return Models.QuoteSent.UnableToSend;

        switch (q.type) {
            case Models.QuoteAction.New:
                if (typeof this._activeQuotes[q.side] !== "undefined") {
                    return this.modify(q);
                }
                else {
                    return this.start(q);
                }
            case Models.QuoteAction.Cancel:
                return this.stop(q);
            default:
                throw new Error("Unknown QuoteAction " + Models.QuoteAction[q.type]);
        }
    };

    private modify = (q : Models.Quote) : Models.QuoteSent => {
        this.stop(q);
        this.start(q);
        return Models.QuoteSent.Modify;
    };

    private start = (q : Models.Quote) : Models.QuoteSent => {
        var existing = this._activeQuotes[q.side];
        if (typeof existing !== "undefined" && existing.quote.equals(q)) {
            return Models.QuoteSent.UnsentDuplicate;
        }

        var newOrder = new Models.SubmitNewOrder(q.side, q.size, Models.OrderType.Limit, q.price, Models.TimeInForce.GTC, q.exchange, q.time);
        var sent = this._broker.sendOrder(newOrder);
        this._activeQuotes[q.side] = new QuoteOrder(q, sent.sentOrderClientId);
        return Models.QuoteSent.First;
    };

    private stop = (q : Models.Quote) : Models.QuoteSent => {
        if (typeof this._activeQuotes[q.side] === "undefined") {
            return Models.QuoteSent.UnsentDuplicate;
        }

        var cxl = new Models.OrderCancel(this._activeQuotes[q.side].orderId, q.exchange, q.time);
        this._broker.cancelOrder(cxl);
        delete this._activeQuotes[q.side];
        return Models.QuoteSent.Delete;
    };
}