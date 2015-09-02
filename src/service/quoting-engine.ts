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
import Web = require("web");
import Statistics = require("./statistics");
import Active = require("./active-state");
import FairValue = require("./fair-value");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import PositionManagement = require("./position-management");
import moment = require('moment');

class GeneratedQuote {
    constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
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

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _filteredMarkets: MarketFiltration.MarketFiltration,
        private _fvEngine: FairValue.FairValueEngine,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        private _quotePublisher: Messaging.IPublish<Models.TwoSidedQuote>,
        private _orderBroker: Interfaces.IOrderBroker,
        private _positionBroker: Interfaces.IPositionBroker,
        private _ewma: Interfaces.IEwmaCalculator,
        private _targetPosition: PositionManagement.TargetBasePositionManager,
        private _safeties: Safety.SafetyCalculator) {
        var recalcWithoutInputTime = () => this.recalcQuote(_timeProvider.utcNow());

        _fvEngine.FairValueChanged.on(() => this.recalcQuote(Utils.timeOrDefault(_fvEngine.latestFairValue, _timeProvider)));
        _qlParamRepo.NewParameters.on(recalcWithoutInputTime);
        _orderBroker.Trade.on(recalcWithoutInputTime);
        _ewma.Updated.on(recalcWithoutInputTime);
        _quotePublisher.registerSnapshot(() => this.latestQuote === null ? [] : [this.latestQuote]);
        _targetPosition.NewTargetPosition.on(recalcWithoutInputTime);
        _safeties.NewValue.on(recalcWithoutInputTime);
        
        _timeProvider.setInterval(recalcWithoutInputTime, moment.duration(1, "seconds"));
    }

    private static computeMidQuote(fv: Models.FairValue, params: Models.QuotingParameters) {
        var width = params.width;
        var size = params.size;

        var bidPx = Math.max(fv.price - width, 0);
        var askPx = fv.price + width;

        return new GeneratedQuote(bidPx, size, askPx, size);
    }

    private static getQuoteAtTopOfMarket(filteredMkt: Models.Market, params: Models.QuotingParameters): GeneratedQuote {
        var topBid = (filteredMkt.bids[0].size > params.stepOverSize ? filteredMkt.bids[0] : filteredMkt.bids[1]);
        if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
        var bidPx = topBid.price;

        var topAsk = (filteredMkt.asks[0].size > params.stepOverSize ? filteredMkt.asks[0] : filteredMkt.asks[1]);
        if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
        var askPx = topAsk.price;

        return new GeneratedQuote(bidPx, topBid.size, askPx, topAsk.size);
    }

    private static computeTopJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
        var genQt = QuotingEngine.getQuoteAtTopOfMarket(filteredMkt, params);

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
        var genQt = QuotingEngine.getQuoteAtTopOfMarket(filteredMkt, params);

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

        var tbp = this._targetPosition.latestTargetPosition;
        if (tbp === null) {
            this._log("cannot compute a quote since no position report exists!");
            return null;
        }
        var targetBasePosition = tbp.data;
        
        var latestPosition = this._positionBroker.latestReport;
        var totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        
        if (totalBasePosition < targetBasePosition - params.positionDivergence) {
            unrounded.askPx = null;
            unrounded.askSz = null;
            if (params.aggressivePositionRebalancing)
                unrounded.bidSz = Math.min(params.aprMultiplier*params.size, targetBasePosition - totalBasePosition);
        }
        
        if (totalBasePosition > targetBasePosition + params.positionDivergence) {
            unrounded.bidPx = null;
            unrounded.bidSz = null;
            if (params.aggressivePositionRebalancing)
                unrounded.askSz = Math.min(params.aprMultiplier*params.size, totalBasePosition - targetBasePosition);
        }
        
        var safety = this._safeties.latest;
        if (safety === null) {
            this._log("cannot compute a quote since trade safety is not yet computed!");
            return null;
        }
        
        if (safety.sell > params.tradesPerMinute) {
            unrounded.askPx = null;
            unrounded.askSz = null;
        }
        if (safety.buy > params.tradesPerMinute) {
            unrounded.bidPx = null;
            unrounded.bidSz = null;
        }
        
        if (unrounded.bidPx !== null) {
            unrounded.bidPx = Utils.roundFloat(unrounded.bidPx);
            unrounded.bidPx = Math.max(0, unrounded.bidPx);
        }
        
        if (unrounded.askPx !== null) {
            unrounded.askPx = Utils.roundFloat(unrounded.askPx);
            unrounded.askPx = Math.max(unrounded.bidPx + .01, unrounded.askPx);
        }
        
        if (unrounded.askSz !== null) {
            unrounded.askSz = Utils.roundFloat(unrounded.askSz);
            unrounded.askSz = Math.max(0.01, unrounded.askSz);
        }
        
        if (unrounded.bidSz !== null) {
            unrounded.bidSz = Utils.roundFloat(unrounded.bidSz);
            unrounded.bidSz = Math.max(0.01, unrounded.bidSz);
        }

        return unrounded;
    }

    private recalcQuote = (t: moment.Moment) => {
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
        if (newQ.price === null && newQ.size === null) return null;
        if (prevTwoSided == null) return newQ;
        var previousQ = sideGetter(prevTwoSided);
        if (previousQ == null && newQ != null) return newQ;
        if (Math.abs(newQ.size - previousQ.size) > 5e-3) return newQ;
        return Math.abs(newQ.price - previousQ.price) < .009999 ? previousQ : newQ;
    }
}