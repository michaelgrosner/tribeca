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
import Winkdex = require("./winkdex");
import util = require("util");

export class QuotingParametersRepository extends Interfaces.Repository<Models.QuotingParameters> {
    constructor(pub : Messaging.IPublish<Models.QuotingParameters>,
                rec : Messaging.IReceive<Models.QuotingParameters>) {
        super("qpr",
            (p : Models.QuotingParameters) => p.size > 0 || p.width > 0,
            (a : Models.QuotingParameters, b : Models.QuotingParameters) => Math.abs(a.width - b.width) > 1e-4 || Math.abs(a.size - b.size) > 1e-4 || a.mode !== b.mode || a.fvModel !== b.fvModel,
            new Models.QuotingParameters(.3, .05, Models.QuotingMode.Top, Models.FairValueModel.BBO), rec, pub);

    }
}

export class ActiveRepository extends Interfaces.Repository<boolean> {
    constructor(safeties : Safety.SafetySettingsManager,
                broker : Interfaces.IBroker,
                pub : Messaging.IPublish<boolean>,
                rec : Messaging.IReceive<boolean>) {
        super("active",
            (p : boolean) => safeties.canEnable,
            (a : boolean, b : boolean) => a !== b,
            false, rec, pub);
        safeties.SafetySettingsViolated.on(() => this.updateParameters(false));
        safeties.SafetyViolationCleared.on(() => this.updateParameters(true));
        broker.ConnectChanged.on(this.onDisconnect);
    }

    private onDisconnect = (cs : Models.ConnectivityStatus) => {
        if (cs === Models.ConnectivityStatus.Disconnected)
            this.updateParameters(false);
    };
}

class GeneratedQuote {
    constructor(public bidPx : number, public bidSz : number, public askPx : number, public askSz : number) {}
}

// computes a quote based off my quoting parameters
export class QuoteGenerator {
    private _log : Utils.Logger = Utils.log("tribeca:qg");
    public latestFairValue : Models.FairValue = null;
    public latestQuote : Models.TwoSidedQuote = null;

    constructor(private _quoter : Quoter.Quoter,
                private _baseBroker : Interfaces.IBroker,
                private _broker : Interfaces.IMarketDataBroker,
                private _qlParamRepo : QuotingParametersRepository,
                private _safeties : Safety.SafetySettingsManager,
                private _quotePublisher : Messaging.IPublish<Models.TwoSidedQuote>,
                private _fvPublisher : Messaging.IPublish<Models.FairValue>,
                private _activeRepo : ActiveRepository,
                private _positionBroker : Interfaces.IPositionBroker,
                private _evs : Winkdex.ExternalValuationSource,
                private _safetyParams : Safety.SafetySettingsRepository) {
        _evs.ValueChanged.on(() => this.recalcMarkets(_evs.Value.time))
        _broker.MarketData.on(() => this.recalcMarkets(_broker.currentBook.time));
        _qlParamRepo.NewParameters.on(() => this.recalcMarkets(Utils.date()));
        _activeRepo.NewParameters.on(() => this.recalcMarkets(Utils.date()));

        _quotePublisher.registerSnapshot(() => this.latestQuote === null ? [] : [this.latestQuote]);
        _fvPublisher.registerSnapshot(() => this.latestFairValue === null ? [] : [this.latestFairValue]);
    }

    private filterMarket = (mkts : Models.MarketSide[], s : Models.Side) : Models.MarketSide[] => {
        var rgq = this._quoter.quotesSent(s);

        var copiedMkts = [];
        for (var i = 0; i < mkts.length; i++) {
            copiedMkts.push(new Models.MarketSide(mkts[i].price, mkts[i].size))
        }

        for (var j = 0; j < rgq.length; j++) {
            var q = rgq[j].quote;

            for (var i = 0; i < copiedMkts.length; i++) {
                var m = copiedMkts[i];

                if (Math.abs(q.price - m.price) < .005) {
                    copiedMkts[i].size = m.size - q.size;
                }
            }
        }

        return copiedMkts.filter(m => m.size > 0.001);
    };

    private static ComputeFVUnrounded(ask : Models.MarketSide, bid : Models.MarketSide, model : Models.FairValueModel) {
        switch (model) {
            case Models.FairValueModel.BBO:
                return (ask.price + bid.price)/2.0;
            case Models.FairValueModel.wBBO:
                return (ask.price*ask.size + bid.price*bid.size) / (ask.size + bid.size);
            default:
                throw new Error(Models.FairValueModel[model]);
        }
    }

    private static ComputeFV(ask : Models.MarketSide, bid : Models.MarketSide, model : Models.FairValueModel) {
        var unrounded = QuoteGenerator.ComputeFVUnrounded(ask, bid, model);
        return Utils.roundFloat(unrounded);
    }

    // i should really stop quoting when false
    private recalcFairValue = (mkt : Models.Market) => {
        if (mkt.bids.length < 1 || mkt.asks.length < 1) 
            return false;

        var ask = this.filterMarket(mkt.asks, Models.Side.Ask);
        var bid = this.filterMarket(mkt.bids, Models.Side.Bid);

        if (ask.length === 0 || bid.length === 0) 
            return false;

        var fv = QuoteGenerator.ComputeFV(ask[0], bid[0], this._qlParamRepo.latest.fvModel);

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

        var topBid = (fv.mkt.bids[0].size > 0.02 ? fv.mkt.bids[0] : fv.mkt.bids[1]);
        var bidPx = topBid.price;

        if (topBid.size > .2) {
            bidPx += .01;
        }

        var minBid = fv.price - width / 2.0;
        bidPx = Math.min(minBid, bidPx);

        var topAsk = (fv.mkt.asks[0].size > 0.02 ? fv.mkt.asks[0] : fv.mkt.asks[1]);
        var askPx = topAsk.price;

        if (topAsk.size > .2) {
            askPx -= .01;
        }

        var minAsk = fv.price + width / 2.0;
        askPx = Math.max(minAsk, askPx);

        return new GeneratedQuote(bidPx, params.size, askPx, params.size);
    }

    private static computeQuoteUnrounded(fv : Models.FairValue, params : Models.QuotingParameters) {
        switch (params.mode) {
            case Models.QuotingMode.Mid: return QuoteGenerator.computeMidQuote(fv, params);
            case Models.QuotingMode.Top: return QuoteGenerator.computeTopQuote(fv, params);
        }
    }

    private computeQuote(fv : Models.FairValue, params : Models.QuotingParameters) {
        var unrounded = QuoteGenerator.computeQuoteUnrounded(fv, params);

        if (this._evs.Value === null) return null;
        var eFV = this._evs.Value.value;
        var megan = this._safetyParams.latest.maxEvDivergence;

        if (unrounded.bidPx > eFV + megan) unrounded.bidPx = eFV + megan;
        if (unrounded.askPx < eFV - megan) unrounded.askPx = eFV - megan;

        // should only make
        var mktBestAsk = this._broker.currentBook.asks[0].price;
        if (unrounded.bidPx + 0.01 >= mktBestAsk) {
            this._log("bid %d would cross with mkt ask %d, backing off", unrounded.bidPx, mktBestAsk);
            unrounded.bidPx = mktBestAsk - .01;
        }

        var mktBestBid = this._broker.currentBook.bids[0].price;
        if (unrounded.askPx - 0.01 <= mktBestBid) {
            this._log("ask %d would cross with mkt bid %d, backing off", unrounded.askPx, mktBestBid);
            unrounded.askPx = mktBestBid + .01;
        }

        unrounded.bidPx = Utils.roundFloat(unrounded.bidPx);
        unrounded.askPx = Utils.roundFloat(unrounded.askPx);

        return unrounded;
    }

    private recalcQuote = (t : Moment) => {
        var fv = this.latestFairValue;
        if (fv == null) return false;

        var genQt;
        try {
            genQt = this.computeQuote(fv, this._qlParamRepo.latest);

            if (genQt === null) return false;

            var newBidQuote = this.getQuote(Models.Side.Bid, genQt.bidPx, genQt.bidSz);
            var newAskQuote = this.getQuote(Models.Side.Ask, genQt.askPx, genQt.askSz);

            this.latestQuote = new Models.TwoSidedQuote(
                QuoteGenerator.quotesAreSame(newBidQuote, this.latestQuote, t => t.bid),
                QuoteGenerator.quotesAreSame(newAskQuote, this.latestQuote, t => t.ask)
            );
        }
        catch (e) {
            this.latestQuote = new Models.TwoSidedQuote(QuoteGenerator._bidStopQuote, QuoteGenerator._askStopQuote);
            this._log("exception while generating quote! stopping until MD can recalc a real quote.", e, e.stack);
        }

        return true;
    };

    private checkCrossedQuotes = (side : Models.Side, px : number) : boolean => {
        var oppSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;

        var doesQuoteCross = oppSide === Models.Side.Bid
            ? (a, b) => a.price >= b
            : (a, b) => a.price <= b;

        var qs = this._quoter.quotesSent(oppSide);
        for (var qi = 0; qi < qs.length; qi++) {
            if (doesQuoteCross(qs[qi].quote, px)) {
                this._log("crossing quote detected! gen quote at %d would crossed with %s quote at %d",
                    px, Models.Side[oppSide], qs[qi].quote.price);
                return true;
            }
        }
        return false;
    };

    private getQuote = (s, px, sz) : Models.Quote => {
        if (this.checkCrossedQuotes(s, px) || px === null || sz === null) {
            return s === Models.Side.Bid ? QuoteGenerator._bidStopQuote : QuoteGenerator._askStopQuote;
        }
        else {
            return new Models.Quote(Models.QuoteAction.New, s, px, sz);
        }
    };

    private recalcMarkets = (t : Moment) => {
        var mkt = this._broker.currentBook;
        if (mkt == null) return;

        var updateFv = this.recalcFairValue(mkt);
        var updateQuote = (updateFv && this.recalcQuote(t));
        var sentQuote = (updateQuote && this.sendQuote(t));

        if (updateFv) this._fvPublisher.publish(this.latestFairValue);
        if (updateQuote) this._quotePublisher.publish(this.latestQuote);
    };

    private static quotesAreSame(newQ : Models.Quote, prevTwoSided : Models.TwoSidedQuote, sideGetter : (q : Models.TwoSidedQuote) => Models.Quote) : Models.Quote {
        if (prevTwoSided == null) return newQ;
        var previousQ = sideGetter(prevTwoSided);
        if (previousQ == null && newQ != null) return newQ;
        if (Math.abs(newQ.size - previousQ.size) > 5e-3) return newQ;
        return Math.abs(newQ.price - previousQ.price) < .009999 ? previousQ : newQ;
    }

    private static _bidStopQuote = new Models.Quote(Models.QuoteAction.Cancel, Models.Side.Bid);
    private static _askStopQuote = new Models.Quote(Models.QuoteAction.Cancel, Models.Side.Ask);

    private sendQuote = (t : Moment) : void => {
        var quote = this.latestQuote;

        if (quote === null) {
            return;
        }

        var bidQt : Models.Quote = QuoteGenerator._bidStopQuote;
        var askQt : Models.Quote = QuoteGenerator._askStopQuote;

        if (this._activeRepo.latest) {
            if (this.hasEnoughPosition(this._baseBroker.pair.base, quote.ask.size)) {
                askQt = quote.ask;
            }

            if (this.hasEnoughPosition(this._baseBroker.pair.quote, quote.bid.size*quote.bid.price)) {
                bidQt = quote.bid;
            }
        }

        var askAction = this._quoter.updateQuote(new Models.Timestamped(askQt, t));
        var bidAction = this._quoter.updateQuote(new Models.Timestamped(bidQt, t));

        if (QuoteGenerator.shouldLogDescision(askAction) ||
                QuoteGenerator.shouldLogDescision(bidAction)) {
            var fv = this.latestFairValue;
            var lm = this._broker.currentBook;
            this._log("new trading decision bidAction=%s, askAction=%s; fv: %d; q:[%d %d - %d %d] %s %s %s",
                    Models.QuoteSent[bidAction], Models.QuoteSent[askAction], fv.price,
                    quote.bid.size, quote.bid.price, quote.ask.price, quote.ask.size,
                    QuoteGenerator.fmtLevel(0, lm.bids, lm.asks),
                    QuoteGenerator.fmtLevel(1, lm.bids, lm.asks),
                    QuoteGenerator.fmtLevel(2, lm.bids, lm.asks));
        }
    };

    private static fmtLevel(n : number, bids : Models.MarketSide[], asks : Models.MarketSide[]) {
        return util.format("mkt%d:[%d %d - %d %d]", n, bids[n].size, bids[n].price, asks[n].price, asks[n].size);
    }

    private static shouldLogDescision(a : Models.QuoteSent) {
        return a !== Models.QuoteSent.UnsentDelete && a !== Models.QuoteSent.UnsentDuplicate && a !== Models.QuoteSent.UnableToSend;
    }

    private hasEnoughPosition = (cur : Models.Currency, minAmt : number) : boolean => {
        var pos = this._positionBroker.getPosition(cur);
        return pos != null && pos.amount > minAmt * 2;
    };
}