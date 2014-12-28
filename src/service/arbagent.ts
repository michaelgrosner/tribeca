/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Aggregators = require("./aggregators");
import Quoter = require("./quoter");

export class ArbAgent {
    private _log : Utils.Logger = Utils.log("tribeca:agent");
    private _maxSize : number;
    private _minProfit : number;

    constructor(private _brokers : Array<Interfaces.IBroker>,
                private _mdAgg : Aggregators.MarketDataAggregator,
                private _orderAgg : Aggregators.OrderBrokerAggregator,
                private _quoter : Quoter.Quoter,
                private config : Config.IConfigProvider) {
        this._maxSize = config.GetNumber("MaxSize");
        this._minProfit = config.GetNumber("MinProfit");
        _mdAgg.MarketData.on(m => this.recalcMarkets(m.update.time));
    }

    Active : boolean = false;
    ActiveChanged = new Utils.Evt<boolean>();

    BestResultChanged = new Utils.Evt<Interfaces.Result>();
    private _lastBestResult = null;
    get LastBestResult() : Interfaces.Result { return this._lastBestResult; }
    set LastBestResult(r : Interfaces.Result) {
        this._lastBestResult = r;
        this.BestResultChanged.trigger(r);
    }

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

    private static hasEnoughPosition(b : Interfaces.IBroker, cur : Models.Currency, minAmt : number) {
        var pos = b.getPosition(cur);
        return pos != null && pos.amount > minAmt;
    }

    private recalcMarkets = (generatedTime : Moment) => {
        var bestResult : Interfaces.Result = null;
        var bestProfit: number = Number.MIN_VALUE;

        for (var i = 0; i < this._brokers.length; i++) {
            var restBroker = this._brokers[i];
            if (!ArbAgent.isBrokerActive(restBroker)) continue;
            var restTop = restBroker.currentBook.update;

            for (var j = 0; j < this._brokers.length; j++) {
                var hideBroker = this._brokers[j];
                if (i == j || !ArbAgent.isBrokerActive(hideBroker)) continue;

                var hideTop = hideBroker.currentBook.update;

                var bidSize = Math.min(this._maxSize, hideTop.bid.size);
                var pBid = bidSize * (-(1 + restBroker.makeFee()) * restTop.bid.price + (1 + hideBroker.takeFee()) * hideTop.bid.price);

                var askSize = Math.min(this._maxSize, hideTop.ask.size);
                var pAsk = askSize * (+(1 + restBroker.makeFee()) * restTop.ask.price - (1 + hideBroker.takeFee()) * hideTop.ask.price);

                if (pBid > bestProfit && pBid > this._minProfit && bidSize >= .01) {
                    if (ArbAgent.hasEnoughPosition(restBroker, Models.Currency.USD, bidSize*restTop.bid.price) &&
                        ArbAgent.hasEnoughPosition(hideBroker, Models.Currency.BTC, bidSize)) {
                        bestProfit = pBid;
                        bestResult = new Interfaces.Result(Models.Side.Bid, restBroker, hideBroker, pBid, restTop.bid, hideTop.bid, bidSize, generatedTime);
                    }
                }

                if (pAsk > bestProfit && pAsk > this._minProfit && askSize >= .01) {
                    if (ArbAgent.hasEnoughPosition(restBroker, Models.Currency.BTC, askSize) &&
                        ArbAgent.hasEnoughPosition(hideBroker, Models.Currency.USD, askSize*hideTop.ask.price)) {
                        bestProfit = pAsk;
                        bestResult = new Interfaces.Result(Models.Side.Ask, restBroker, hideBroker, pAsk, restTop.ask, hideTop.ask, askSize, generatedTime);
                    }
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
                        this.ignore(bestResult);
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
                this.nothing();
            }
        }
        else {
            // ugh, this is only for UI
            this.BestResultChanged.trigger(bestResult);
        }
    };

    private nothing = () => {
        this._log("NOTHING");
        this.LastBestResult = null;
    };

    private ignore = (r : Interfaces.Result) => {
        this._log("IGNORED :: prevExch=%s newExch=%s; prevSide=%s newSide=%s; prevPft=%d newPft=%d",
            Models.Exchange[this.LastBestResult.restBroker.exchange()], Models.Exchange[r.restBroker.exchange()],
            Models.Side[this.LastBestResult.restSide], Models.Side[r.restSide],
            this.LastBestResult.profit, r.profit);
    };

    private noChange = (r : Interfaces.Result) => {
        this._log("NO CHANGE :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Models.Side[r.restSide],
                r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);
        this.LastBestResult = r;
    };

    private modify = (r : Interfaces.Result) => {
        var action = this._quoter.updateQuote(new Quoter.Quote(Quoter.QuoteAction.New, r.restSide,
            r.restBroker.exchange(), r.generatedTime, r.rest.price, r.rest.size));

        var verb  = "MODIFY";
        if (action == Quoter.QuoteSent.First) {
            verb = "START";
            r.restBroker.OrderUpdate.on(this.arbFire);
        }
        else if (action == Quoter.QuoteSent.UnsentDuplicate) {
            verb = "NO CHANGE";
        }

        this._log("%s :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", verb, r.profit, Models.Side[r.restSide],
            r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);

        this.LastBestResult = r;
    };

    private stop = (lr : Interfaces.Result, sendCancel : boolean, t : Moment) => {
        // remove fill notification
        lr.restBroker.OrderUpdate.off(this.arbFire);

        // cancel open order
        this._quoter.updateQuote(new Quoter.Quote(Quoter.QuoteAction.Cancel, lr.restSide, lr.restBroker.exchange(), t));

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
        hideBroker.sendOrder(new Models.SubmitNewOrder(side, o.lastQuantity, o.type, px, Models.TimeInForce.IOC,
            hideBroker.exchange(), o.time, "AF: " + o.orderId));

        this._log("ARBFIRE :: rested %s %d for %d on %s --> pushing %s %d for %d on %s",
            Models.Side[o.side], o.lastQuantity, o.lastPrice, Models.Exchange[this.LastBestResult.restBroker.exchange()],
            Models.Side[side], o.lastQuantity, px, Models.Exchange[this.LastBestResult.hideBroker.exchange()]);

        this.stop(this.LastBestResult, o.orderStatus != Models.OrderStatus.Complete, o.time);
    };
}