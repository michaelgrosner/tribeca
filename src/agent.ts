/// <reference path="models.ts" />
/// <reference path="config.ts" />

import Config = require("./config");
import Models = require("./models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class PositionAggregator {
    PositionUpdate = new Utils.Evt<Models.ExchangeCurrencyPosition>();

    constructor(private _brokers : Array<Interfaces.IBroker>) {
        this._brokers.forEach(b => {
            b.PositionUpdate.on(m => this.PositionUpdate.trigger(m));
        });
    }
}

export class MarketDataAggregator {
    MarketData = new Utils.Evt<Models.Market>();

    constructor(private _brokers : Array<Interfaces.IBroker>) {
        this._brokers.forEach(b => {
            b.MarketData.on(m => this.MarketData.trigger(m));
        });
    }
}

export class OrderBrokerAggregator {
    _log : Utils.Logger = Utils.log("tribeca:brokeraggregator");
    _brokersByExch : { [exchange: number]: Interfaces.IBroker} = {};

    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    constructor(private _brokers : Array<Interfaces.IBroker>) {

        this._brokers.forEach(b => {
            b.OrderUpdate.on(o => this.OrderUpdate.trigger(o));
        });

        for (var i = 0; i < this._brokers.length; i++)
            this._brokersByExch[this._brokers[i].exchange()] = this._brokers[i];
    }

    public submitOrder = (o : Models.SubmitNewOrder) => {
        try {
            this._brokersByExch[o.exchange].sendOrder(o);
        }
        catch (e) {
            this._log("Exception while sending order", o, e, e.stack);
        }
    };

    public cancelReplaceOrder = (o : Models.CancelReplaceOrder) => {
        try {
            this._brokersByExch[o.exchange].replaceOrder(o);
        }
        catch (e) {
            this._log("Exception while cancel/replacing order", o, e, e.stack);
        }
    };

    public cancelOrder = (o : Models.OrderCancel) => {
        try {
            this._brokersByExch[o.exchange].cancelOrder(o);
        }
        catch (e) {
            this._log("Exception while cancelling order", o, e, e.stack);
        }
    };
}

export class Agent {
    private _log : Utils.Logger = Utils.log("tribeca:agent");
    private _maxSize : number;
    private _minProfit : number;

    constructor(private _brokers : Array<Interfaces.IBroker>,
                private _mdAgg : MarketDataAggregator,
                private _orderAgg : OrderBrokerAggregator,
                private config : Config.IConfigProvider) {
        this._maxSize = config.GetNumber("MaxSize");
        this._minProfit = config.GetNumber("MinProfit");
        _mdAgg.MarketData.on(m => this.recalcMarkets(m.update.time));
    }

    Active : boolean = false;
    ActiveChanged = new Utils.Evt<boolean>();

    LastBestResult : Interfaces.Result = null;
    BestResultChanged = new Utils.Evt<Interfaces.Result>();

    private _activeOrderIds : { [ exch : number] : string} = {};

    changeActiveStatus = (to : boolean) => {
        if (this.Active != to) {
            this.Active = to;

            if (this.Active) {
                this.recalcMarkets(Utils.date());
            }
            else if (!this.Active && this.LastBestResult != null) {
                this.stop(this.LastBestResult, true, Utils.date());
            }

            this._log("changing active status to %o", to);
            this.ActiveChanged.trigger(to);
        }
    };

    private static isBrokerActive(b : Interfaces.IBroker) : boolean {
        return b.currentBook != null && b.connectStatus == Models.ConnectivityStatus.Connected;
    }

    private recalcMarkets = (generatedTime : Moment) => {
        var bestResult : Interfaces.Result = null;
        var bestProfit: number = Number.MIN_VALUE;

        for (var i = 0; i < this._brokers.length; i++) {
            var restBroker = this._brokers[i];
            if (!Agent.isBrokerActive(restBroker)) continue;
            var restTop = restBroker.currentBook.update;

            for (var j = 0; j < this._brokers.length; j++) {
                var hideBroker = this._brokers[j];
                if (i == j || !Agent.isBrokerActive(hideBroker)) continue;

                var hideTop = hideBroker.currentBook.update;

                var bidSize = Math.min(this._maxSize, hideTop.bid.size);
                var pBid = bidSize * (-(1 + restBroker.makeFee()) * restTop.bid.price + (1 + hideBroker.takeFee()) * hideTop.bid.price);

                var askSize = Math.min(this._maxSize, hideTop.ask.size);
                var pAsk = askSize * (+(1 + restBroker.makeFee()) * restTop.ask.price - (1 + hideBroker.takeFee()) * hideTop.ask.price);

                if (pBid > bestProfit && pBid > this._minProfit && bidSize >= .01) {
                    bestProfit = pBid;
                    bestResult = new Interfaces.Result(Models.Side.Bid, restBroker, hideBroker, pBid, restTop.bid, hideTop.bid, bidSize, generatedTime);
                }

                if (pAsk > bestProfit && pAsk > this._minProfit && askSize >= .01) {
                    bestProfit = pAsk;
                    bestResult = new Interfaces.Result(Models.Side.Ask, restBroker, hideBroker, pAsk, restTop.ask, hideTop.ask, askSize, generatedTime);
                }
            }
        }

        if (this.Active) {
            if (bestResult == null && this.LastBestResult !== null) {
                this.stop(this.LastBestResult, true, generatedTime);
            }
            else if (bestResult !== null && this.LastBestResult == null) {
                this.start(bestResult);
            }
            else if (bestResult !== null && this.LastBestResult !== null) {
                if (bestResult.restBroker.exchange() != this.LastBestResult.restBroker.exchange()
                        || bestResult.restSide != this.LastBestResult.restSide) {
                    // don't flicker
                    if (Math.abs(bestResult.profit - this.LastBestResult.profit) < this._minProfit) {
                        this.noChange(bestResult);
                    }
                    else {
                        this.stop(this.LastBestResult, true, generatedTime);
                        this.start(bestResult);
                    }
                }
                else if (Math.abs(bestResult.rest.price - this.LastBestResult.rest.price) > 1e-3) {
                    this.modify(bestResult);
                }
                else {
                    this.noChange(bestResult);
                }
            }
            else {
                this._log("NOTHING");
            }
        }

        this.BestResultChanged.trigger(bestResult)
    };

    private noChange = (r : Interfaces.Result) => {
        this._log("NO CHANGE :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Models.Side[r.restSide],
                r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);
    };

    private modify = (r : Interfaces.Result) => {
        var restExch = r.restBroker.exchange();
        // cxl-rpl live order -- need to rethink cxl-rpl
        var cxl = new Models.OrderCancel(this._activeOrderIds[restExch], restExch, r.generatedTime);
        r.restBroker.cancelOrder(cxl);
        var newOrder = new Models.SubmitNewOrder(r.restSide, r.size, Models.OrderType.Limit, r.rest.price, Models.TimeInForce.GTC, restExch, r.generatedTime);
        this._activeOrderIds[restExch] = r.restBroker.sendOrder(newOrder).sentOrderClientId;

        this._log("MODIFY :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Models.Side[r.restSide],
            r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);

        this.LastBestResult = r;
    };

    private start = (r : Interfaces.Result) => {
        var restExch = r.restBroker.exchange();
        // set up fill notification
        r.restBroker.OrderUpdate.on(this.arbFire);

        // send an order
        var sent = r.restBroker.sendOrder(new Models.SubmitNewOrder(r.restSide, r.size, Models.OrderType.Limit, r.rest.price, Models.TimeInForce.GTC, restExch, r.generatedTime));
        this._activeOrderIds[restExch] = sent.sentOrderClientId;

        this._log("START :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Models.Side[r.restSide],
            r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);

        this.LastBestResult = r;
    };

    private stop = (lr : Interfaces.Result, sendCancel : boolean, t : Moment) => {
        // remove fill notification
        lr.restBroker.OrderUpdate.off(this.arbFire);

        // cancel open order
        var restExch = lr.restBroker.exchange();
        if (sendCancel) lr.restBroker.cancelOrder(new Models.OrderCancel(this._activeOrderIds[restExch], restExch, t));
        delete this._activeOrderIds[restExch];

        this._log("STOP :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", lr.profit,
            Models.Side[lr.restSide], lr.restBroker.name(),
            lr.rest.price, lr.hideBroker.name(), lr.hide.price);

        this.LastBestResult = null;
    };

    private arbFire = (o : Models.OrderStatusReport) => {
        if (!(o.lastQuantity > 0))
            return;

        var hideBroker = this.LastBestResult.hideBroker;
        var px = o.side == Models.Side.Ask
            ? hideBroker.currentBook.update.ask.price
            : hideBroker.currentBook.update.bid.price;
        var side = o.side == Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;
        hideBroker.sendOrder(new Models.SubmitNewOrder(side, o.lastQuantity, o.type, px, Models.TimeInForce.IOC, hideBroker.exchange(), o.time));

        this._log("ARBFIRE :: rested %s %d for %d on %s --> pushing %s %d for %d on %s",
            Models.Side[o.side], o.lastQuantity, o.lastPrice, Models.Exchange[this.LastBestResult.restBroker.exchange()],
            Models.Side[side], o.lastQuantity, px, Models.Exchange[this.LastBestResult.hideBroker.exchange()]);

        this.stop(this.LastBestResult, o.orderStatus != Models.OrderStatus.Complete, o.time);
    };
}