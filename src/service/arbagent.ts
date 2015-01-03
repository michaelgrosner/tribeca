/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");

export class QuotingParametersRepository {
    private _log : Utils.Logger = Utils.log("tribeca:qpr");
    NewParameters = new Utils.Evt();

    private _latest = new Models.QuotingParameters(.2, .01);
    public get latest() : Models.QuotingParameters {
        return this._latest;
    }

    public updateParameters = (newParams : Models.QuotingParameters) => {
        if (newParams.size <= 0 || newParams.width <= 0) return;
        if (Math.abs(this.latest.width - newParams.width) > 1e-4 || Math.abs(this.latest.size - newParams.size) > 1e-4) {
            this._latest = newParams;
            this._log("Changed parameters width=%d size=%d", this.latest.width, this.latest.size);
            this.NewParameters.trigger();
        }
    };
}

enum RecalcRequest { FairValue, Quote, TradingDecision }

// computes a quote based off my quoting parameters
export class QuoteGenerator {
    private _log : Utils.Logger = Utils.log("tribeca:qg");

    public NewValue = new Utils.Evt();
    public latestFairValue : Models.FairValue = null;

    public NewQuote = new Utils.Evt();
    public latestQuote : Models.TwoSidedQuote = null;

    public NewTradingDecision = new Utils.Evt();
    public latestDecision : Models.TradingDecision = null;

    constructor(private _quoter : Quoter.Quoter,
                private _broker : Interfaces.IBroker,
                private _qlParamRepo : QuotingParametersRepository) {
        _broker.MarketData.on(() => this.recalcMarkets(RecalcRequest.FairValue, _broker.currentBook.time));
        _qlParamRepo.NewParameters.on(() => this.recalcMarkets(RecalcRequest.Quote, Utils.date()));
    }

    public Active : boolean = false;
    public ActiveChanged = new Utils.Evt();
    changeActiveStatus = (to : boolean) => {
        if (this.Active != to) {
            this.Active = to;
            this._log("changing active status to ", to);

            this.recalcMarkets(RecalcRequest.TradingDecision, Utils.date());
            this.ActiveChanged.trigger(this.Active);
        }
    };

    private getFirstNonQuoteMarket = (mkts : Models.MarketSide[], s : Models.Side) : Models.MarketSide => {
        var rgq = this._quoter.quotesSent(s);

        for (var i = 0; i < mkts.length; i++) {
            var m : Models.MarketSide = mkts[i];

            var anyMatch = false;
            for (var j = 0; !anyMatch && j < rgq.length; j++) {
                var q : Models.Quote = rgq[j].quote;
                if (Math.abs(q.price - m.price) < .01 && Math.abs(q.size - m.size) < .01)  {
                    anyMatch = true;
                }
            }

            if (!anyMatch)
                return m;
        }
    };

    private recalcFairValue = (mkt : Models.Market) => {
        var ask = this.getFirstNonQuoteMarket(mkt.asks, Models.Side.Ask);
        var bid = this.getFirstNonQuoteMarket(mkt.bids, Models.Side.Bid);
        var mid = (ask.price + bid.price) / 2.0;

        var newFv = new Models.FairValue(mid, mkt);
        var previousFv = this.latestFairValue;
        if (!QuoteGenerator.fairValuesAreSame(newFv, previousFv)) {
            this.latestFairValue = newFv;
            return true;
        }
        return false;
    };

    private recalcQuote = (t : Moment) => {
        if (this.latestFairValue == null) return false;
        var params = this._qlParamRepo.latest;
        var width = params.width;
        var size = params.size;

        var bidPx = Math.max(this.latestFairValue.price - width, 0);
        var askPx = this.latestFairValue.price + width;

        var bidQuote = new Models.Quote(Models.QuoteAction.New, Models.Side.Bid, t, bidPx, size);
        var askQuote = new Models.Quote(Models.QuoteAction.New, Models.Side.Ask, t, askPx, size);

        this.latestQuote = new Models.TwoSidedQuote(bidQuote, askQuote);
        return true;
    };

    private recalcMarkets = (req : RecalcRequest, t : Moment) => {
        var mkt = this._broker.currentBook;
        if (mkt == null) return;

        var updateFv = req > RecalcRequest.FairValue || this.recalcFairValue(mkt);
        var updateQuote = req > RecalcRequest.Quote || (updateFv && this.recalcQuote(t));
        var sentQuote = req > RecalcRequest.TradingDecision || (updateQuote && this.sendQuote(t));

        if (updateFv) this.NewValue.trigger();
        if (updateQuote) this.NewQuote.trigger();
        if (sentQuote) this.NewTradingDecision.trigger();
    };

    private static fairValuesAreSame(newFv : Models.FairValue, previousFv : Models.FairValue) {
        if (previousFv == null && newFv != null) return false;
        return Math.abs(newFv.price - previousFv.price) < 1e-3;
    }

    private sendQuote = (t : Moment) : void => {
        var quote = this.latestQuote;

        if (quote === null) {
            return;
        }

        var bidQt : Models.Quote = null;
        var askQt : Models.Quote = null;

        if (this.Active && this.isBrokerActive()) {
            bidQt = quote.bid;
            askQt = quote.ask;
        }
        else {
            bidQt = QuoteGenerator.ConvertToStopQuote(quote.ask);
            askQt = QuoteGenerator.ConvertToStopQuote(quote.bid);
        }

        var askAction = this._quoter.updateQuote(new Models.Timestamped(askQt, t));
        var bidAction = this._quoter.updateQuote(new Models.Timestamped(bidQt, t));

        var decision = new Models.TradingDecision(bidAction, askAction);
        this.latestDecision = decision;
        this.NewTradingDecision.trigger(decision);

        var fv = this.latestFairValue;
        this._log("New trading decision: %s; quote: %s, fv: %d, tAsk0: %s, tBid0: %s, tAsk1: %s, tBid1: %s, tAsk2: %s, tBid2: %s",
            decision.toString(), quote.toString(), fv.price,
            fv.mkt.asks[0].toString(), fv.mkt.bids[0].toString(),
            fv.mkt.asks[1].toString(), fv.mkt.bids[1].toString(),
            fv.mkt.asks[2].toString(), fv.mkt.bids[2].toString());
    };

    private static ConvertToStopQuote(q : Models.Quote) {
        return new Models.Quote(Models.QuoteAction.Cancel, q.side, q.time);
    }

    private isBrokerActive = () : boolean => {
        return this._broker.connectStatus == Models.ConnectivityStatus.Connected;
    };

    private hasEnoughPosition = (b : Interfaces.IBroker, cur : Models.Currency, minAmt : number) : boolean => {
        var pos = b.getPosition(cur);
        return pos != null && pos.amount > minAmt;
    };
}