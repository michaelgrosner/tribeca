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
import util = require("util");
import _ = require("lodash");
import Persister = require("./persister");
import Web = require("web");

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
    private _savedQuotingMode : boolean;

    constructor(startQuoting : boolean,
                safeties : Safety.SafetySettingsManager,
                broker : Interfaces.IBroker,
                pub : Messaging.IPublish<boolean>,
                rec : Messaging.IReceive<boolean>) {
        super("active",
              (p : boolean) => safeties.canEnable && broker.connectStatus === Models.ConnectivityStatus.Connected,
              (a : boolean, b : boolean) => a !== b,
              startQuoting, rec, pub);
        this._savedQuotingMode = this.latest;

        safeties.SafetySettingsViolated.on(() => this.updateParameters(false));
        safeties.SafetyViolationCleared.on(() => this.updateParameters(true));
        broker.ConnectChanged.on(this.onConnectChanged);
    }

    private onConnectChanged = (cs : Models.ConnectivityStatus) => {
        this._savedQuotingMode = this.latest;
        if (cs === Models.ConnectivityStatus.Disconnected)
            this.updateParameters(false);
        if (this._savedQuotingMode && cs === Models.ConnectivityStatus.Connected)
            this.updateParameters(true);
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

        if (this._latest !== null)
            this._fvPersister.persist(this._latest);
    }

    constructor(private _filtration : MarketFiltration,
                private _qlParamRepo : QuotingParametersRepository, // should not co-mingle these settings
                private _fvPublisher : Messaging.IPublish<Models.FairValue>,
                private _fvHttpPublisher : Web.StandaloneHttpPublisher<Models.FairValue>,
                private _fvPersister : Persister.FairValuePersister) {
        _qlParamRepo.NewParameters.on(() => this.recalcFairValue(Utils.date()));
        _filtration.FilteredMarketChanged.on(() => this.recalcFairValue(timeOrDefault(_filtration.latestFilteredMarket)));
        _fvPublisher.registerSnapshot(() => this.latestFairValue === null ? [] : [this.latestFairValue]);
        _fvHttpPublisher.registerSnapshot(this._fvPersister.loadAll);
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

export class EmptyEWMACalculator implements Interfaces.IEwmaCalculator {
    constructor() {}
    latest : number = null;
    Updated = new Utils.Evt<any>();
}

export class EWMACalculator implements Interfaces.IEwmaCalculator {
    private _log : Utils.Logger = Utils.log("tribeca:ewma");

    constructor(private _fv : FairValueEngine, private _alpha : number = .095) {
        setInterval(this.onTick, 10*1000);
        this.onTick();
    }

    private onTick = () => {
        var fv = this._fv.latestFairValue;

        if (fv === null) {
            this._log("Unable to compute EMWA value");
            return;
        }

        var value : number = fv.price;
        if (this._latest === null) {
            this.setLatest(value);
        }
        else {
            var newVal = this._alpha * value + (1 - this._alpha) * this._latest;
            this.setLatest(newVal);
        }
    };

    private _latest : number = null;
    public get latest() { return this._latest; }
    private setLatest = (v : number) => {
        if (Math.abs(v - this._latest) > 1e-3) {
            this._latest = v;
            this.Updated.trigger();

            this._log("New EMWA value: %d", this._latest);
        }
    };

    Updated = new Utils.Evt<any>();
}

export class QuotingEngine {
    public QuoteChanged = new Utils.Evt<Models.TwoSidedQuote>();

    private _latest : Models.TwoSidedQuote = null;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val : Models.TwoSidedQuote) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this.QuoteChanged.trigger();
        this._quotePublisher.publish(this._latest);
    }

    constructor(private _pair : Models.CurrencyPair,
                private _filteredMarkets : MarketFiltration,
                private _fvEngine : FairValueEngine,
                private _qlParamRepo : QuotingParametersRepository,
                private _safetyParams : Safety.SafetySettingsRepository,
                private _quotePublisher : Messaging.IPublish<Models.TwoSidedQuote>,
                private _broker : Interfaces.IMarketDataBroker,
                private _orderBroker : Interfaces.IOrderBroker,
                private _positionBroker : Interfaces.IPositionBroker,
                private _ewma : Interfaces.IEwmaCalculator) {
        _fvEngine.FairValueChanged.on(() => this.recalcQuote(timeOrDefault(_fvEngine.latestFairValue)));  // or should i listen to _broker.MarketData???
        _qlParamRepo.NewParameters.on(() => this.recalcQuote(Utils.date()));
        _safetyParams.NewParameters.on(() => this.recalcQuote(Utils.date()));
        _orderBroker.Trade.on(t => this.recalcQuote(Utils.date()));
        _ewma.Updated.on(() => this.recalcQuote(Utils.date()));
        _quotePublisher.registerSnapshot(() => this.latestQuote === null ? [] : [this.latestQuote]);
    }

    private static computeMidQuote(fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;
        var size = params.size;

        var bidPx = Math.max(fv.price - width, 0);
        var askPx = fv.price + width;

        return new GeneratedQuote(bidPx, size, askPx, size);
    }

    private static getQuoteAtTopOfMarket(filteredMkt : Models.Market) : GeneratedQuote {
        var topBid = (filteredMkt.bids[0].size > 0.02 ? filteredMkt.bids[0] : filteredMkt.bids[1]);
        if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
        var bidPx = topBid.price;

        var topAsk = (filteredMkt.asks[0].size > 0.02 ? filteredMkt.asks[0] : filteredMkt.asks[1]);
        if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
        var askPx = topAsk.price;

        return new GeneratedQuote(bidPx, topBid.size, askPx, topAsk.size);
    }

    private static computeTopJoinQuote(filteredMkt : Models.Market, fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;

        var genQt = QuotingEngine.getQuoteAtTopOfMarket(filteredMkt);

        if (params.mode === Models.QuotingMode.Top && genQt.bidSz > .2) {
            genQt.bidPx += .01;
        }

        var minBid = fv.price - width / 2.0;
        genQt.bidPx = Math.min(minBid, genQt.bidPx);

        if (params.mode === Models.QuotingMode.Top && genQt.askSz > .2) {
            genQt.askPx -= .01;
        }

        var minAsk = fv.price + width / 2.0;
        genQt.askPx = Math.max(minAsk, genQt.askPx);

        return genQt;
    }

    private static computeInverseJoinQuote(filteredMkt : Models.Market, fv : Models.FairValue, params : Models.QuotingParameters) {
        var width = params.width;

        var genQt = QuotingEngine.getQuoteAtTopOfMarket(filteredMkt);

        var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
        if (mktWidth > width) {
            genQt.askPx += params.width;
            genQt.bidPx -= params.width;
        }
        else if (width/2.0 > mktWidth) {
            genQt.askPx = fv.price + width / 2.0;
            genQt.bidPx = fv.price - width / 2.0;
        }

        return new GeneratedQuote(genQt.bidPx, params.size, genQt.askPx, params.size);
    }

    private static computeQuoteUnrounded(filteredMkt : Models.Market, fv : Models.FairValue, params : Models.QuotingParameters) {
        switch (params.mode) {
            case Models.QuotingMode.Mid:
                return QuotingEngine.computeMidQuote(fv, params);
            case Models.QuotingMode.Top:
            case Models.QuotingMode.Join:
                return QuotingEngine.computeTopJoinQuote(filteredMkt, fv, params);
            case Models.QuotingMode.InverseJoin:
                return QuotingEngine.computeInverseJoinQuote(filteredMkt, fv, params);
        }
    }

    private computeQuote(filteredMkt : Models.Market, fv : Models.FairValue) {
        var params = this._qlParamRepo.latest;
        var unrounded = QuotingEngine.computeQuoteUnrounded(filteredMkt, fv, params);

        if (params.ewmaProtection && this._ewma.latest !== null) {
            if (this._ewma.latest > unrounded.askPx) {
                unrounded.askPx = Math.max(this._ewma.latest, unrounded.askPx);
            }

            if (this._ewma.latest < unrounded.bidPx) {
                unrounded.bidPx = Math.min(this._ewma.latest, unrounded.bidPx);
            }
        }

        var latestPosition = this._positionBroker.getPosition(this._pair.base);
        if (typeof latestPosition === "undefined") return null;
        var tPos = latestPosition.heldAmount + latestPosition.amount;
        if (tPos < params.targetBasePosition - params.positionDivergence) {
            unrounded.askPx += 20; // TODO: revisit! throw away?
        }
        if (tPos > params.targetBasePosition + params.positionDivergence) {
            unrounded.bidPx -= 20; // TODO: revisit! throw away?
        }

        unrounded.bidPx = Utils.roundFloat(unrounded.bidPx);
        unrounded.askPx = Utils.roundFloat(unrounded.askPx);

        unrounded.bidPx = Math.max(0, unrounded.bidPx);
        unrounded.askPx = Math.max(unrounded.bidPx + .01, unrounded.askPx);

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

        var genQt = this.computeQuote(filteredMkt, fv);

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
