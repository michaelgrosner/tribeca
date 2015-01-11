/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");

export class QuotingParametersRepository extends Interfaces.Repository<Models.QuotingParameters> {
    constructor() {
        super("qpr",
            (p : Models.QuotingParameters) => p.size > 0 || p.width > 0,
            (a : Models.QuotingParameters, b : Models.QuotingParameters) => Math.abs(a.width - b.width) > 1e-4 || Math.abs(a.size - b.size) > 1e-4 || a.mode !== b.mode,
            new Models.QuotingParameters(.5, .01, Models.QuotingMode.Top));

    }
}

class GeneratedQuote {
    constructor(public bidPx : number, public bidSz : number, public askPx : number, public askSz : number) {}
}

enum RecalcRequest { FairValue, Quote, TradingDecision }

// computes a quote based off my quoting parameters
export class QuoteGenerator {
    private _log : Utils.Logger = Utils.log("tribeca:qg");
    private _quotePublisher : Messaging.IPublish<Models.TwoSidedQuote>;
    private _fvPublisher : Messaging.IPublish<Models.FairValue>;

    public latestFairValue : Models.FairValue = null;
    public latestQuote : Models.TwoSidedQuote = null;
    public latestDecision : Models.TradingDecision = null;

    constructor(private _quoter : Quoter.Quoter,
                private _broker : Interfaces.IBroker,
                private _qlParamRepo : QuotingParametersRepository,
                private _safeties : Safety.SafetySettingsManager,
                io : any) {
        _broker.MarketData.on(() => this.recalcMarkets(RecalcRequest.FairValue, _broker.currentBook.time));
        _qlParamRepo.NewParameters.on(() => this.recalcMarkets(RecalcRequest.Quote, Utils.date()));
        _safeties.SafetySettingsViolated.on(() => this.changeActiveStatus(false));

        this._quotePublisher = new Messaging.ExchangePairPubSub.ExchangePairPublisher<Models.TwoSidedQuote>(
            _broker.exchange(), _broker.pair, Messaging.Topics.Quote, () => this.latestQuote === null ? [] : [this.latestQuote], io, Utils.log("tribeca:messaging:quote"));

        this._fvPublisher = new Messaging.ExchangePairPubSub.ExchangePairPublisher<Models.FairValue>(
            _broker.exchange(), _broker.pair, Messaging.Topics.FairValue, () => this.latestFairValue === null ? [] : [this.latestFairValue], io, Utils.log("tribeca:messaging:fv"));
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

        var midLevel = n => (ask[n].price*ask[n].size + bid[n].price*bid[n].size) / (ask[n].size + bid[n].size);
        var fv = midLevel(0);

        this.latestFairValue = new Models.FairValue(fv, new Models.Market(bid, ask, mkt.time));
        return true;
    };

    private static computeMidQuote(fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;
        var size = params.size;

        var bidPx = Math.max(fv.price - width, 0);
        var askPx = fv.price + width;

        return new GeneratedQuote(bidPx, size, askPx, size);
    }

    private static computeTopQuote(fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;

        var bidPx = fv.mkt.bids[0].price;
        var minBid = fv.price - width / 2.0;
        bidPx = Math.min(minBid, bidPx);

        var askPx = fv.mkt.asks[0].price;
        var minAsk = fv.price + width / 2.0;
        askPx = Math.max(minAsk, askPx);

        return new GeneratedQuote(bidPx, params.size, askPx, params.size);
    }

    private static computeQuote(fv : Models.FairValue, params : Models.QuotingParameters) {
        switch (params.mode) {
            case Models.QuotingMode.Mid: return QuoteGenerator.computeMidQuote(fv, params);
            case Models.QuotingMode.Top: return QuoteGenerator.computeTopQuote(fv, params);
        }
    }

    private recalcQuote = (t : Moment) => {
        var fv = this.latestFairValue;
        if (fv == null) return false;

        var genQt = QuoteGenerator.computeQuote(fv, this._qlParamRepo.latest);

        var buildQuote = (s, px, sz) => new Models.Quote(Models.QuoteAction.New, s, px, sz);

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

        var getQuote = (s, px, sz) => {
            var oppoSide = s === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;
            return checkCrossedQuotes(oppoSide, buildQuote(s, px, sz));
        };

        var newBidQuote = getQuote(Models.Side.Bid, genQt.bidPx, genQt.bidSz);
        var newAskQuote = getQuote(Models.Side.Ask, genQt.askPx, genQt.askSz);

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

        if (updateFv) this._fvPublisher.publish(this.latestFairValue);
        if (updateQuote) this._quotePublisher.publish(this.latestQuote);
    };

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

    private hasEnoughPosition = (cur : Models.Currency, minAmt : number) : boolean => {
        var pos = this._broker.getPosition(cur);
        return pos != null && pos.amount > minAmt;
    };
}