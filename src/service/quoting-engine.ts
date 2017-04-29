/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="interfaces.ts"/>
/// <reference path="quoter.ts"/>
/// <reference path="safety.ts"/>
/// <reference path="statistics.ts"/>
/// <reference path="active-state.ts"/>
/// <reference path="fair-value.ts"/>
/// <reference path="market-filtration.ts"/>
/// <reference path="quoting-parameters.ts"/>
/// <reference path="position-management.ts"/>
/// <reference path="./quoting-styles/style-registry.ts"/>

import Config = require("./config");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");
import util = require("util");
import _ = require("lodash");
import Statistics = require("./statistics");
import Active = require("./active-state");
import FairValue = require("./fair-value");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import PositionManagement = require("./position-management");
import moment = require('moment');
import QuotingStyleRegistry = require("./quoting-styles/style-registry");
import {QuoteInput} from "./quoting-styles/helpers";
import log from "./logging";

export class QuotingEngine {
    private _log = log("quotingengine");

    public QuoteChanged = new Utils.Evt<Models.TwoSidedQuote>();

    private _latest: Models.TwoSidedQuote = null;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val: Models.TwoSidedQuote) {
        if (!quotesChanged(this._latest, val, this._details.minTickIncrement)) 
            return;

        this._latest = val;
        this.QuoteChanged.trigger();
        this._quotePublisher.publish(this._latest);
    }

    constructor(
        private _registry: QuotingStyleRegistry.QuotingStyleRegistry,
        private _timeProvider: Utils.ITimeProvider,
        private _filteredMarkets: MarketFiltration.MarketFiltration,
        private _fvEngine: FairValue.FairValueEngine,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        private _quotePublisher: Messaging.IPublish<Models.TwoSidedQuote>,
        private _orderBroker: Interfaces.IOrderBroker,
        private _positionBroker: Interfaces.IPositionBroker,
        private _details: Interfaces.IBroker,
        private _ewma: Interfaces.IEwmaCalculator,
        private _targetPosition: PositionManagement.TargetBasePositionManager,
        private _safeties: Safety.SafetyCalculator) {
        var recalcWithoutInputTime = () => this.recalcQuote(_timeProvider.utcNow());

        _filteredMarkets.FilteredMarketChanged.on(m => this.recalcQuote(Utils.timeOrDefault(m, _timeProvider)));
        _qlParamRepo.NewParameters.on(recalcWithoutInputTime);
        _orderBroker.Trade.on(recalcWithoutInputTime);
        _ewma.Updated.on(recalcWithoutInputTime);
        _quotePublisher.registerSnapshot(() => this.latestQuote === null ? [] : [this.latestQuote]);
        _targetPosition.NewTargetPosition.on(recalcWithoutInputTime);
        _safeties.NewValue.on(recalcWithoutInputTime);
        
        _timeProvider.setInterval(recalcWithoutInputTime, moment.duration(1, "seconds"));
    }

    private computeQuote(filteredMkt: Models.Market, fv: Models.FairValue) {
        const params = this._qlParamRepo.latest;
        const minTick = this._details.minTickIncrement;
        const input = new QuoteInput(filteredMkt, fv, params, minTick);
        const unrounded = this._registry.Get(params.mode).GenerateQuote(input);
        
        if (unrounded === null)
            return null;

        if (params.ewmaProtection && this._ewma.latest !== null) {
            if (this._ewma.latest > unrounded.askPx) {
                unrounded.askPx = Math.max(this._ewma.latest, unrounded.askPx);
            }

            if (this._ewma.latest < unrounded.bidPx) {
                unrounded.bidPx = Math.min(this._ewma.latest, unrounded.bidPx);
            }
        }

        const tbp = this._targetPosition.latestTargetPosition;
        if (tbp === null) {
            this._log.warn("cannot compute a quote since no position report exists!");
            return null;
        }
        const targetBasePosition = tbp.data;
        
        const latestPosition = this._positionBroker.latestReport;
        const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        
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
        
        const safety = this._safeties.latest;
        if (safety === null) {
            return null;
        }
        
        if (params.mode === Models.QuotingMode.PingPong) {
          if (unrounded.askSz && safety.buyPing && unrounded.askPx < safety.buyPing + params.width)
            unrounded.askPx = safety.buyPing + params.width;
          if (unrounded.bidSz && safety.sellPong && unrounded.bidPx > safety.sellPong - params.width)
            unrounded.bidPx = safety.sellPong - params.width;
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
            unrounded.bidPx = Utils.roundSide(unrounded.bidPx, minTick, Models.Side.Bid);
            unrounded.bidPx = Math.max(0, unrounded.bidPx);
        }
        
        if (unrounded.askPx !== null) {
            unrounded.askPx = Utils.roundSide(unrounded.askPx, minTick, Models.Side.Ask);
            unrounded.askPx = Math.max(unrounded.bidPx + minTick, unrounded.askPx);
        }
        
        if (unrounded.askSz !== null) {
            unrounded.askSz = Utils.roundDown(unrounded.askSz, minTick);
            unrounded.askSz = Math.max(minTick, unrounded.askSz);
        }
        
        if (unrounded.bidSz !== null) {
            unrounded.bidSz = Utils.roundDown(unrounded.bidSz, minTick);
            unrounded.bidSz = Math.max(minTick, unrounded.bidSz);
        }

        return unrounded;
    }

    private recalcQuote = (t: Date) => {
        const fv = this._fvEngine.latestFairValue;
        if (fv == null) {
            this.latestQuote = null;
            return;
        }

        const filteredMkt = this._filteredMarkets.latestFilteredMarket;
        if (filteredMkt == null) {
            this.latestQuote = null;
            return;
        }

        const genQt = this.computeQuote(filteredMkt, fv);

        if (genQt === null) {
            this.latestQuote = null;
            return;
        }

        this.latestQuote = new Models.TwoSidedQuote(
            this.quotesAreSame(new Models.Quote(genQt.bidPx, genQt.bidSz), this.latestQuote, Models.Side.Bid),
            this.quotesAreSame(new Models.Quote(genQt.askPx, genQt.askSz), this.latestQuote, Models.Side.Ask),
            t
            );
    };

    private quotesAreSame(
            newQ: Models.Quote, 
            prevTwoSided: Models.TwoSidedQuote, 
            side: Models.Side): Models.Quote {
                
        if (newQ.price === null && newQ.size === null) return null;
        if (prevTwoSided == null) return newQ;
        
        const previousQ = Models.Side.Bid === side ? prevTwoSided.bid : prevTwoSided.ask;
        
        if (previousQ == null && newQ != null) return newQ;
        if (Math.abs(newQ.size - previousQ.size) > 5e-3) return newQ;
        
        if (Math.abs(newQ.price - previousQ.price) < this._details.minTickIncrement) {
            return previousQ;
        }
        
        let quoteWasWidened = true;
        if (Models.Side.Bid === side && previousQ.price < newQ.price) quoteWasWidened = false;
        if (Models.Side.Ask === side && previousQ.price > newQ.price) quoteWasWidened = false;
        
        // prevent flickering
        if (!quoteWasWidened && Math.abs(Utils.fastDiff(new Date(), prevTwoSided.time)) < 300) {
            return previousQ;
        }
        
        return newQ;
    }
}

const quoteChanged = (o: Models.Quote, n: Models.Quote, tick: number) : boolean => { 
    if ((!o && n) || (o && !n)) return true;
    if (!o && !n) return false;

    const oPx = (o && o.price) || 0;
    const nPx = (n && n.price) || 0;
    if (Math.abs(oPx - nPx) > tick) 
        return true;

    const oSz = (o && o.size) || 0;
    const nSz = (n && n.size) || 0;
    return Math.abs(oSz - nSz) > .001;
}

const quotesChanged = (o: Models.TwoSidedQuote, n: Models.TwoSidedQuote, tick: number) : boolean => {
    if ((!o && n) || (o && !n)) return true;
    if (!o && !n) return false;

    if (quoteChanged(o.bid, n.bid, tick)) return true;
    if (quoteChanged(o.ask, n.ask, tick)) return true;
    return false;
}