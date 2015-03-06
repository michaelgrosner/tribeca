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
import _ = require("lodash");

export class QuotingParametersRepository extends Interfaces.Repository<Models.QuotingParameters> {
    constructor(pub : Messaging.IPublish<Models.QuotingParameters>,
                rec : Messaging.IReceive<Models.QuotingParameters>,
                initParam : Models.QuotingParameters) {
        super("qpr",
            (p : Models.QuotingParameters) => p.size > 0 || p.width > 0,
            (a : Models.QuotingParameters, b : Models.QuotingParameters) => !_.isEqual(a, b),
            initParam, rec, pub);

    }
}

export class ActiveRepository extends Interfaces.Repository<boolean> {
    constructor(safeties : Safety.SafetySettingsManager,
                broker : Interfaces.IBroker,
                pub : Messaging.IPublish<boolean>,
                rec : Messaging.IReceive<boolean>) {
        super("active",
            (p : boolean) => safeties.canEnable && broker.connectStatus === Models.ConnectivityStatus.Connected,
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

function timeOrDefault(x : Models.ITimestamped) : Moment {
    if (x === null)
        return Utils.date();

    if (typeof x !== "undefined" && typeof x.time !== "undefined")
        return x.time;
    
    return Utils.date();
}

export class MarketFiltration {
    private _latest : Models.Market = null;
    public FilteredMarketChanged = new Utils.Evt<Models.Market>();

    public get latestFilteredMarket() { return this._latest; }
    public set latestFilteredMarket(val : Models.Market) {
        this._latest = val;
        this.FilteredMarketChanged.trigger();
    }

    constructor(private _quoter : Quoter.Quoter,
                private _broker : Interfaces.IMarketDataBroker) {
        _broker.MarketData.on(this.filterFullMarket);
    }

    private filterFullMarket = () => {
        var mkt = this._broker.currentBook;

        if (mkt == null || mkt.bids.length < 1 || mkt.asks.length < 1)  {
            this.latestFilteredMarket = null;
            return;
        }

        var ask = this.filterMarket(mkt.asks, Models.Side.Ask);
        var bid = this.filterMarket(mkt.bids, Models.Side.Bid);

        this.latestFilteredMarket = new Models.Market(bid, ask, mkt.time);
    };

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
}

export class FairValueEngine {
    public FairValueChanged = new Utils.Evt<Models.FairValue>();

    private _latest : Models.FairValue = null;
    public get latestFairValue() { return this._latest; }
    public set latestFairValue(val : Models.FairValue) {
        if (this._latest != null 
                && val != null
                && Math.abs(this._latest.price - val.price) < 0.02) return;

        this._latest = val;
        this.FairValueChanged.trigger();
        this._fvPublisher.publish(this._latest);
    }

    constructor(private _filtration : MarketFiltration,
                private _qlParamRepo : QuotingParametersRepository, // should not co-mingle these settings
                private _fvPublisher : Messaging.IPublish<Models.FairValue>) {
        _qlParamRepo.NewParameters.on(() => this.recalcFairValue(Utils.date()));
        _filtration.FilteredMarketChanged.on(() => this.recalcFairValue(timeOrDefault(_filtration.latestFilteredMarket)));
        _fvPublisher.registerSnapshot(() => this.latestFairValue === null ? [] : [this.latestFairValue]);
    }

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
        var unrounded = FairValueEngine.ComputeFVUnrounded(ask, bid, model);
        return Utils.roundFloat(unrounded);
    }

    private recalcFairValue = (t : Moment) => {
        var mkt = this._filtration.latestFilteredMarket;

        if (mkt == null) {
            this.latestFairValue = null;
            return;
        }

        var bid = mkt.bids;
        var ask = mkt.asks;

        if (ask.length < 1 || bid.length < 1) {
            this.latestFairValue = null;
            return;
        }

        var fv = new Models.FairValue(FairValueEngine.ComputeFV(ask[0], bid[0], this._qlParamRepo.latest.fvModel), t);
        this.latestFairValue = fv;
    };
}

export class QuotingEngine {
    private _log : Utils.Logger = Utils.log("tribeca:quotingengine");

    public QuoteChanged = new Utils.Evt<Models.TwoSidedQuote>();

    private _latest : Models.TwoSidedQuote = null;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val : Models.TwoSidedQuote) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this.QuoteChanged.trigger();
        this._quotePublisher.publish(this._latest);
    }

    constructor(private _filteredMarkets : MarketFiltration,
                private _fvEngine : FairValueEngine,
                private _qlParamRepo : QuotingParametersRepository,
                private _safetyParams : Safety.SafetySettingsRepository,
                private _quotePublisher : Messaging.IPublish<Models.TwoSidedQuote>,
                private _broker : Interfaces.IMarketDataBroker,
                private _orderBroker : Interfaces.IOrderBroker,
                private _evs : Winkdex.ExternalValuationSource) {
        //_fvEngine.FairValueChanged.on(() => this.recalcQuote(timeOrDefault(_fvEngine.latestFairValue)));  // or should i listen to _broker.MarketData???
        _filteredMarkets.FilteredMarketChanged.on(() => {
            if (this._qlParamRepo.latest.mode === Models.QuotingMode.Join) 
                this.recalcQuote(timeOrDefault(_filteredMarkets.latestFilteredMarket))
        });
        _evs.ValueChanged.on(() => this.recalcQuote(timeOrDefault(_evs.Value)));
        _qlParamRepo.NewParameters.on(() => this.recalcQuote(Utils.date()));
        _safetyParams.NewParameters.on(() => this.recalcQuote(Utils.date()));
        _orderBroker.Trade.on(t => this.recalcQuote(Utils.date()));

        _quotePublisher.registerSnapshot(() => this.latestQuote === null ? [] : [this.latestQuote]);
    }

    private computeMidQuote(fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;
        var size = params.size;

        var bidPx = Math.max(fv.price - width, 0);
        var askPx = fv.price + width;

        return new GeneratedQuote(bidPx, size, askPx, size);
    }

    private computeTopQuote(filteredMkt : Models.Market, fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;

        var topBid = (filteredMkt.bids[0].size > 0.02 ? filteredMkt.bids[0] : filteredMkt.bids[1]);
        if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
        var bidPx = topBid.price;

        if (topBid.size > .2) {
            bidPx += .01;
        }

        var minBid = fv.price - width / 2.0;
        bidPx = Math.min(minBid, bidPx);

        var topAsk = (filteredMkt.asks[0].size > 0.02 ? filteredMkt.asks[0] : filteredMkt.asks[1]);
        if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
        var askPx = topAsk.price;

        if (topAsk.size > .2) {
            askPx -= .01;
        }

        var minAsk = fv.price + width / 2.0;
        askPx = Math.max(minAsk, askPx);

        return new GeneratedQuote(bidPx, params.size, askPx, params.size);
    }

    private computeJoinQuote(filteredMkt : Models.Market, fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;

        var topBid = (filteredMkt.bids[0].size > 0.02 ? filteredMkt.bids[0] : filteredMkt.bids[1]);
        if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
        var bidPx = topBid.price;

        var minBid = fv.price - width / 2.0;
        bidPx = Math.min(minBid, bidPx);

        var topAsk = (filteredMkt.asks[0].size > 0.02 ? filteredMkt.asks[0] : filteredMkt.asks[1]);
        if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
        var askPx = topAsk.price;

        var maxAsk = fv.price + width / 2.0;
        askPx = Math.max(maxAsk, askPx);

        return new GeneratedQuote(bidPx, params.size, askPx, params.size);
    }

    private computeQuoteUnrounded(filteredMkt : Models.Market, fv : Models.FairValue) {
        var params = this._qlParamRepo.latest;
        switch (params.mode) {
            case Models.QuotingMode.Mid: return this.computeMidQuote(fv, params);
            case Models.QuotingMode.Top: return this.computeTopQuote(filteredMkt, fv, params);
            case Models.QuotingMode.Join: return this.computeJoinQuote(filteredMkt, fv, params);
        }
    }

    private computeQuote(filteredMkt : Models.Market, fv : Models.FairValue, extFv : Models.ExternalValuationUpdate) {
        var unrounded = this.computeQuoteUnrounded(filteredMkt, fv);

        var megan = this._safetyParams.latest.maxEvDivergence;

        var eFV = extFv.value;
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
        var fv = this._fvEngine.latestFairValue;
        if (fv == null) {
            this.latestQuote = null;
            return;
        }

        var filteredMkt = this._filteredMarkets.latestFilteredMarket;
        if (filteredMkt == null) {
            this.latestQuote = null;
            return;
        }

        var extVal = this._evs.Value;
        if (extVal == null) {
            this.latestQuote = null;
            return;
        }

        var genQt = this.computeQuote(filteredMkt, fv, extVal);

        if (genQt === null) {
            this.latestQuote = null;
            return;
        }

        this.latestQuote = new Models.TwoSidedQuote(
            QuotingEngine.quotesAreSame(new Models.Quote(genQt.bidPx, genQt.bidSz), this.latestQuote, t => t.bid),
            QuotingEngine.quotesAreSame(new Models.Quote(genQt.askPx, genQt.askSz), this.latestQuote, t => t.ask),
            t
        );
    };

    private static quotesAreSame(newQ : Models.Quote, prevTwoSided : Models.TwoSidedQuote, sideGetter : (q : Models.TwoSidedQuote) => Models.Quote) : Models.Quote {
        if (prevTwoSided == null) return newQ;
        var previousQ = sideGetter(prevTwoSided);
        if (previousQ == null && newQ != null) return newQ;
        if (Math.abs(newQ.size - previousQ.size) > 5e-3) return newQ;
        return Math.abs(newQ.price - previousQ.price) < .009999 ? previousQ : newQ;
    }
}

export class QuoteSender {
    private _log : Utils.Logger = Utils.log("tribeca:quotesender");

    private _latest = new Models.TwoSidedQuoteStatus(Models.QuoteStatus.Held, Models.QuoteStatus.Held);
    public get latestStatus() { return this._latest; }
    public set latestStatus(val : Models.TwoSidedQuoteStatus) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this._statusPublisher.publish(this._latest);
    }
    
    constructor(private _quotingEngine : QuotingEngine, 
                private _statusPublisher : Messaging.IPublish<Models.TwoSidedQuoteStatus>,
                private _quoter : Quoter.Quoter,
                private _pair : Models.CurrencyPair,
                private _activeRepo : ActiveRepository,
                private _positionBroker : Interfaces.IPositionBroker,
                private _fv : FairValueEngine,
                private _broker : Interfaces.IMarketDataBroker) {
        _activeRepo.NewParameters.on(() => this.sendQuote(Utils.date()));
        _quotingEngine.QuoteChanged.on(() => this.sendQuote(timeOrDefault(_quotingEngine.latestQuote)));

        _statusPublisher.registerSnapshot(() => this.latestStatus === null ? [] : [this.latestStatus]);
    }

    private checkCrossedQuotes = (side : Models.Side, px : number) : boolean => {
        var oppSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;

        var doesQuoteCross = oppSide === Models.Side.Bid
            ? (a, b) => a.price >= b
            : (a, b) => a.price <= b;

        var qs = this._quoter.quotesSent(oppSide);
        for (var qi = 0; qi < qs.length; qi++) {
            if (doesQuoteCross(qs[qi].quote, px)) {
                this._log("crossing quote detected! gen quote at %d would crossed with %s quote at %j",
                    px, Models.Side[oppSide], qs[qi]);
                return true;
            }
        }
        return false;
    };

    private sendQuote = (t : Moment) : void => {
        var quote = this._quotingEngine.latestQuote;

        var askStatus = Models.QuoteStatus.Held;
        var bidStatus = Models.QuoteStatus.Held;

        if (quote !== null && this._activeRepo.latest) {
            if (this.hasEnoughPosition(this._pair.base, quote.ask.size) && !this.checkCrossedQuotes(Models.Side.Ask, quote.ask.price)) {
                askStatus = Models.QuoteStatus.Live;
            }

            if (this.hasEnoughPosition(this._pair.quote, quote.bid.size*quote.bid.price) && !this.checkCrossedQuotes(Models.Side.Bid, quote.bid.price)) {
                bidStatus = Models.QuoteStatus.Live;
            }
        }

        var askAction : Models.QuoteSent;
        if (askStatus === Models.QuoteStatus.Live) {
            askAction = this._quoter.updateQuote(new Models.Timestamped(quote.ask, t), Models.Side.Ask);
        }
        else {
            askAction = this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Ask, t));
        }

        var bidAction : Models.QuoteSent;
        if (bidStatus === Models.QuoteStatus.Live) {
            bidAction = this._quoter.updateQuote(new Models.Timestamped(quote.bid, t), Models.Side.Bid);
        }
        else {
            bidAction = this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Bid, t));
        }

        this.latestStatus = new Models.TwoSidedQuoteStatus(bidStatus, askStatus);

        if (this.shouldLogDescision(askAction) || this.shouldLogDescision(bidAction)) {
            var fv = this._fv.latestFairValue;
            var lm = this._broker.currentBook;
            this._log("new trading decision bidAction=%s, askAction=%s; fv: %d; q:%s %s %s %s",
                    Models.QuoteSent[bidAction], Models.QuoteSent[askAction], 
                    (fv == null ? null : fv.price),
                    this.fmtQuoteSide(quote),
                    this.fmtLevel(0, lm.bids, lm.asks),
                    this.fmtLevel(1, lm.bids, lm.asks),
                    this.fmtLevel(2, lm.bids, lm.asks));
        }
    };

    private fmtQuoteSide(q : Models.TwoSidedQuote) {
        if (q == null) return "[no quote]";
        return util.format("q:[%d %d - %d %d]", 
            (q.bid == null ? null : q.bid.size), 
            (q.bid == null ? null : q.bid.price), 
            (q.ask == null ? null : q.ask.price), 
            (q.ask == null ? null : q.ask.size));
    }

    private fmtLevel(n : number, bids : Models.MarketSide[], asks : Models.MarketSide[]) {
        return util.format("mkt%d:[%d %d - %d %d]", n, 
            (typeof bids[n] === "undefined" ? null : bids[n].size), 
            (typeof bids[n] === "undefined" ? null : bids[n].price), 
            (typeof asks[n] === "undefined" ? null : asks[n].price),
            (typeof asks[n] === "undefined" ? null : asks[n].size));
    }

    private shouldLogDescision(a : Models.QuoteSent) {
        return a !== Models.QuoteSent.UnsentDelete && a !== Models.QuoteSent.UnsentDuplicate && a !== Models.QuoteSent.UnableToSend;
    }

    private hasEnoughPosition = (cur : Models.Currency, minAmt : number) : boolean => {
        var pos = this._positionBroker.getPosition(cur);
        return pos != null && pos.amount > minAmt * 2;
    };
}