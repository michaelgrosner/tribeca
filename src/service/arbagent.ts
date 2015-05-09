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
import Statistics = require("./statistics");
import Active = require("./active-state");
import FairValue = require("./fair-value");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import PositionManagement = require("./position-management");

class GeneratedQuote {
    constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
}

export class EmptyEWMACalculator implements Interfaces.IEwmaCalculator {
    constructor() { }
    latest: number = null;
    Updated = new Utils.Evt<any>();
}

export class EWMACalculator implements Interfaces.IEwmaCalculator {
    private _log: Utils.Logger = Utils.log("tribeca:ewma");

    constructor(private _fv: FairValue.FairValueEngine, private _alpha: number = .095) {
        setInterval(this.onTick, 10 * 1000);
        this.onTick();
    }

    private onTick = () => {
        var fv = this._fv.latestFairValue;

        if (fv === null) {
            this._log("Unable to compute EMWA value");
            return;
        }

        var value = Statistics.computeEwma(fv.price, this._latest, this._alpha);

        this.setLatest(value);
    };

    private _latest: number = null;
    public get latest() { return this._latest; }
    private setLatest = (v: number) => {
        if (Math.abs(v - this._latest) > 1e-3) {
            this._latest = v;
            this.Updated.trigger();

            this._log("New EMWA value: %d", this._latest);
        }
    };

    Updated = new Utils.Evt<any>();
}

export class QuotingEngine {
    private _log: Utils.Logger = Utils.log("tribeca:quotingengine");

    public QuoteChanged = new Utils.Evt<Models.TwoSidedQuote>();

    private _latest: Models.TwoSidedQuote = null;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val: Models.TwoSidedQuote) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this.QuoteChanged.trigger();
        this._quotePublisher.publish(this._latest);
    }

    constructor(private _filteredMarkets: MarketFiltration.MarketFiltration,
        private _fvEngine: FairValue.FairValueEngine,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        private _safetyParams: Safety.SafetySettingsRepository,
        private _quotePublisher: Messaging.IPublish<Models.TwoSidedQuote>,
        private _orderBroker: Interfaces.IOrderBroker,
        private _positionBroker: Interfaces.IPositionBroker,
        private _ewma: Interfaces.IEwmaCalculator,
        private _targetPosition: PositionManagement.TargetBasePositionManager) {
        var recalcWithoutInputTime = () => this.recalcQuote(Utils.date());

        _fvEngine.FairValueChanged.on(() => this.recalcQuote(Utils.timeOrDefault(_fvEngine.latestFairValue)));
        _qlParamRepo.NewParameters.on(recalcWithoutInputTime);
        _safetyParams.NewParameters.on(recalcWithoutInputTime);
        _orderBroker.Trade.on(recalcWithoutInputTime);
        _ewma.Updated.on(recalcWithoutInputTime);
        _quotePublisher.registerSnapshot(() => this.latestQuote === null ? [] : [this.latestQuote]);
        _targetPosition.NewTargetPosition.on(recalcWithoutInputTime);
    }

    private static computeMidQuote(fv: Models.FairValue, params: Models.QuotingParameters) {
        var width = params.width;
        var size = params.size;

        var bidPx = Math.max(fv.price - width, 0);
        var askPx = fv.price + width;

        return new GeneratedQuote(bidPx, size, askPx, size);
    }

    private static getQuoteAtTopOfMarket(filteredMkt: Models.Market): GeneratedQuote {
        var topBid = (filteredMkt.bids[0].size > 0.02 ? filteredMkt.bids[0] : filteredMkt.bids[1]);
        if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
        var bidPx = topBid.price;

        var topAsk = (filteredMkt.asks[0].size > 0.02 ? filteredMkt.asks[0] : filteredMkt.asks[1]);
        if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
        var askPx = topAsk.price;

        return new GeneratedQuote(bidPx, topBid.size, askPx, topAsk.size);
    }

    private static computeTopJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
        var genQt = QuotingEngine.getQuoteAtTopOfMarket(filteredMkt);

        if (params.mode === Models.QuotingMode.Top && genQt.bidSz > .2) {
            genQt.bidPx += .01;
        }

        var minBid = fv.price - params.width / 2.0;
        genQt.bidPx = Math.min(minBid, genQt.bidPx);

        if (params.mode === Models.QuotingMode.Top && genQt.askSz > .2) {
            genQt.askPx -= .01;
        }

        var minAsk = fv.price + params.width / 2.0;
        genQt.askPx = Math.max(minAsk, genQt.askPx);

        genQt.bidSz = params.size;
        genQt.askSz = params.size;

        return genQt;
    }

    private static computeInverseJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
        var genQt = QuotingEngine.getQuoteAtTopOfMarket(filteredMkt);

        var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
        if (mktWidth > params.width) {
            genQt.askPx += params.width;
            genQt.bidPx -= params.width;
        }

        if (params.mode === Models.QuotingMode.InverseTop) {
            if (genQt.bidSz > .2) genQt.bidPx += .01;
            if (genQt.askSz > .2) genQt.askPx -= .01;
        }

        if (mktWidth < (2.0 * params.width / 3.0)) {
            genQt.askPx += params.width / 4.0;
            genQt.bidPx -= params.width / 4.0;
        }

        genQt.bidSz = params.size;
        genQt.askSz = params.size;

        return genQt;
    }

    private static computeQuoteUnrounded(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
        switch (params.mode) {
            case Models.QuotingMode.Mid:
                return QuotingEngine.computeMidQuote(fv, params);
            case Models.QuotingMode.Top:
            case Models.QuotingMode.Join:
                return QuotingEngine.computeTopJoinQuote(filteredMkt, fv, params);
            case Models.QuotingMode.InverseJoin:
            case Models.QuotingMode.InverseTop:
                return QuotingEngine.computeInverseJoinQuote(filteredMkt, fv, params);
        }
    }

    private computeQuote(filteredMkt: Models.Market, fv: Models.FairValue) {
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

        var targetBasePosition = this._targetPosition.latestTargetPosition;
        if (targetBasePosition === null) {
            this._log("cannot compute a quote since no position report exists!");
            return null;
        }

        var latestPosition = this._positionBroker.latestReport;
        var totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        if (totalBasePosition < targetBasePosition - params.positionDivergence) {
            unrounded.askPx += 20; // TODO: revisit! throw away?
        }
        if (totalBasePosition > targetBasePosition + params.positionDivergence) {
            unrounded.bidPx -= 20; // TODO: revisit! throw away?
        }

        unrounded.bidPx = Utils.roundFloat(unrounded.bidPx);
        unrounded.askPx = Utils.roundFloat(unrounded.askPx);

        unrounded.bidPx = Math.max(0, unrounded.bidPx);
        unrounded.askPx = Math.max(unrounded.bidPx + .01, unrounded.askPx);
        
        unrounded.askSz = Math.max(0.01, unrounded.askSz);
        unrounded.bidSz = Math.max(0.01, unrounded.bidSz);

        return unrounded;
    }

    private recalcQuote = (t: Moment) => {
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

    private static quotesAreSame(newQ: Models.Quote, prevTwoSided: Models.TwoSidedQuote, sideGetter: (q: Models.TwoSidedQuote) => Models.Quote): Models.Quote {
        if (prevTwoSided == null) return newQ;
        var previousQ = sideGetter(prevTwoSided);
        if (previousQ == null && newQ != null) return newQ;
        if (Math.abs(newQ.size - previousQ.size) > 5e-3) return newQ;
        return Math.abs(newQ.price - previousQ.price) < .009999 ? previousQ : newQ;
    }
}

export class QuoteSender {
    private _log: Utils.Logger = Utils.log("tribeca:quotesender");

    private _latest = new Models.TwoSidedQuoteStatus(Models.QuoteStatus.Held, Models.QuoteStatus.Held);
    public get latestStatus() { return this._latest; }
    public set latestStatus(val: Models.TwoSidedQuoteStatus) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this._statusPublisher.publish(this._latest);
    }

    constructor(private _quotingEngine: QuotingEngine,
        private _statusPublisher: Messaging.IPublish<Models.TwoSidedQuoteStatus>,
        private _quoter: Quoter.Quoter,
        private _pair: Models.CurrencyPair,
        private _activeRepo: Active.ActiveRepository,
        private _positionBroker: Interfaces.IPositionBroker,
        private _fv: FairValue.FairValueEngine,
        private _broker: Interfaces.IMarketDataBroker,
        private _details: Interfaces.IBroker,
        private _safety: Safety.SafetyCalculator) {
        _activeRepo.NewParameters.on(() => this.sendQuote(Utils.date()));
        _quotingEngine.QuoteChanged.on(() => this.sendQuote(Utils.timeOrDefault(_quotingEngine.latestQuote)));
        _statusPublisher.registerSnapshot(() => this.latestStatus === null ? [] : [this.latestStatus]);
    }

    private checkCrossedQuotes = (side: Models.Side, px: number): boolean => {
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

    private sendQuote = (t: Moment): void => {
        var quote = this._quotingEngine.latestQuote;

        var askStatus = Models.QuoteStatus.Held;
        var bidStatus = Models.QuoteStatus.Held;

        if (quote !== null && this._activeRepo.latest) {
            if (this.hasEnoughPosition(this._pair.base, quote.ask.size) &&
                (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Ask, quote.ask.price))) {
                askStatus = Models.QuoteStatus.Live;
            }

            if (this.hasEnoughPosition(this._pair.quote, quote.bid.size * quote.bid.price) &&
                (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Bid, quote.bid.price))) {
                bidStatus = Models.QuoteStatus.Live;
            }
        }

        var askAction: Models.QuoteSent;
        if (askStatus === Models.QuoteStatus.Live) {
            askAction = this._quoter.updateQuote(new Models.Timestamped(quote.ask, t), Models.Side.Ask);
        }
        else {
            askAction = this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Ask, t));
        }

        var bidAction: Models.QuoteSent;
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

    private fmtQuoteSide(q: Models.TwoSidedQuote) {
        if (q == null) return "[no quote]";
        return util.format("q:[%d %d - %d %d]",
            (q.bid == null ? null : q.bid.size),
            (q.bid == null ? null : q.bid.price),
            (q.ask == null ? null : q.ask.price),
            (q.ask == null ? null : q.ask.size));
    }

    private fmtLevel(n: number, bids: Models.MarketSide[], asks: Models.MarketSide[]) {
        return util.format("mkt%d:[%d %d - %d %d]", n,
            (typeof bids[n] === "undefined" ? null : bids[n].size),
            (typeof bids[n] === "undefined" ? null : bids[n].price),
            (typeof asks[n] === "undefined" ? null : asks[n].price),
            (typeof asks[n] === "undefined" ? null : asks[n].size));
    }

    private shouldLogDescision(a: Models.QuoteSent) {
        return a !== Models.QuoteSent.UnsentDelete && a !== Models.QuoteSent.UnsentDuplicate && a !== Models.QuoteSent.UnableToSend;
    }

    private hasEnoughPosition = (cur: Models.Currency, minAmt: number): boolean => {
        var pos = this._positionBroker.getPosition(cur);
        return pos != null && pos.amount > minAmt * 2;
    };
}
