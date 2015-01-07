/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");

export class QuotingParametersRepository extends Interfaces.Repository<Models.QuotingParameters> {
    constructor() {
        super("qpr",
            (p : Models.QuotingParameters) => p.size > 0 || p.width > 0,
            (a : Models.QuotingParameters, b : Models.QuotingParameters) => Math.abs(a.width - b.width) > 1e-4 || Math.abs(a.size - b.size) > 1e-4,
            new Models.QuotingParameters(.5, .01));

    }
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
                private _qlParamRepo : QuotingParametersRepository,
                private _safeties : Safety.SafetySettingsManager) {
        _broker.MarketData.on(() => this.recalcMarkets(RecalcRequest.FairValue, _broker.currentBook.time));
        _qlParamRepo.NewParameters.on(() => this.recalcMarkets(RecalcRequest.Quote, Utils.date()));
        _safeties.SafetySettingsViolated.on(() => this.changeActiveStatus(false));
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

    private filterMarket = (mkts : Models.MarketSide[], s : Models.Side) : Models.MarketSide[] => {
        var rgq = this._quoter.quotesSent(s);

        return mkts.filter(m => {
            for (var j = 0; j < rgq.length; j++) {
                var q : Models.Quote = rgq[j].quote;
                if (Math.abs(q.price - m.price) < .01 && Math.abs(q.size - m.size) < .01)  {
                    return false;
                }
            }
            return true;
        });
    };

    private recalcFairValue = (mkt : Models.Market) => {
        var ask = this.filterMarket(mkt.asks, Models.Side.Ask);
        var bid = this.filterMarket(mkt.bids, Models.Side.Bid);

        var weightedMid = n => (ask[n].price*ask[n].size + bid[n].price*bid[n].size) / (ask[n].size + bid[n].size);
        var fv = weightedMid(0); //*.85 + weightedMid(1)*.1 + weightedMid(2)*.05;

        var newFv = new Models.FairValue(fv, mkt);
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

        var buildQuote = (s, px) => new Models.Quote(Models.QuoteAction.New, s, px, size);

        var checkCrossedQuotes = (oppSide : Models.Side, q : Models.Quote) => {
            var doesQuoteCross = oppSide === Models.Side.Bid
                ? (a : Models.Quote, b : Models.Quote) => a.price > b.price
                : (a : Models.Quote, b : Models.Quote) => a.price < b.price;

            var qs = this._quoter.quotesSent(oppSide);
            for (var qi = 0; qi < qs.length; qi++) {
                if (doesQuoteCross(qs[qi].quote, q)) return qs[qi].quote;
            }
            return q;
        };

        var getQuote = (s, px) => {
            var oppoSide = s === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;
            return checkCrossedQuotes(oppoSide, buildQuote(s, px));
        };

        var newBidQuote = getQuote(Models.Side.Bid, bidPx);
        var newAskQuote = getQuote(Models.Side.Ask, askPx);

        this.latestQuote = new Models.TwoSidedQuote(
            QuoteGenerator.quotesAreSame(newBidQuote, this.latestQuote, t => t.bid),
            QuoteGenerator.quotesAreSame(newAskQuote, this.latestQuote, t => t.ask)
        );

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

    private static quotesAreSame(newQ : Models.Quote, prevTwoSided : Models.TwoSidedQuote, sideGetter : (q : Models.TwoSidedQuote) => Models.Quote) : Models.Quote {
        if (prevTwoSided == null) return newQ;
        var previousQ = sideGetter(prevTwoSided);
        if (previousQ == null && newQ != null) return newQ;
        if (Math.abs(newQ.size - previousQ.size) > 5e-3) return newQ;
        return Math.abs(newQ.price - previousQ.price) < .05 ? previousQ : newQ;
    }

    private sendQuote = (t : Moment) : void => {
        var quote = this.latestQuote;

        if (quote === null) {
            return;
        }

        var bidQt : Models.Quote = null;
        var askQt : Models.Quote = null;

        if (this.Active
                && this.isBrokerConnected()
                && this.hasEnoughPosition(this._broker.pair.base, quote.ask.size)
                && this.hasEnoughPosition(this._broker.pair.quote, quote.bid.size*quote.bid.price)) {
            bidQt = quote.bid;
            askQt = quote.ask;
        }
        else {
            bidQt = QuoteGenerator.ConvertToStopQuote(quote.ask);
            askQt = QuoteGenerator.ConvertToStopQuote(quote.bid);
        }

        var askAction = this._quoter.updateQuote(new Models.Timestamped(askQt, t));
        var bidAction = this._quoter.updateQuote(new Models.Timestamped(bidQt, t));

        this.latestDecision = new Models.TradingDecision(bidAction, askAction);
        this.NewTradingDecision.trigger();

        var fv = this.latestFairValue;
        this._log("New trading decision: %s; quote: %s, fv: %d, tAsk0: %s, tBid0: %s, tAsk1: %s, tBid1: %s, tAsk2: %s, tBid2: %s",
            this.latestDecision.toString(), quote.toString(), fv.price,
            fv.mkt.asks[0].toString(), fv.mkt.bids[0].toString(),
            fv.mkt.asks[1].toString(), fv.mkt.bids[1].toString(),
            fv.mkt.asks[2].toString(), fv.mkt.bids[2].toString());
    };

    private static ConvertToStopQuote(q : Models.Quote) {
        return new Models.Quote(Models.QuoteAction.Cancel, q.side);
    }

    private isBrokerConnected = () : boolean => {
        return this._broker.connectStatus == Models.ConnectivityStatus.Connected;
    };

    private hasEnoughPosition = (cur : Models.Currency, minAmt : number) : boolean => {
        var pos = this._broker.getPosition(cur);
        return pos != null && pos.amount > minAmt;
    };
}