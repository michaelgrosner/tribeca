/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="interfaces.ts"/>
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

export class QuotingEngine {
    private _log = Utils.log("quotingengine");

    public QuoteChanged = new Utils.Evt<Models.TwoSidedQuote>();

    private _latest: Models.TwoSidedQuote = null;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val: Models.TwoSidedQuote) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this.QuoteChanged.trigger();
    }

    constructor(
        private _registry: QuotingStyleRegistry.QuotingStyleRegistry,
        private _timeProvider: Utils.ITimeProvider,
        private _filteredMarkets: MarketFiltration.MarketFiltration,
        private _fvEngine: FairValue.FairValueEngine,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        private _orderBroker: Interfaces.IOrderBroker,
        private _positionBroker: Interfaces.IPositionBroker,
        private _ewma: Interfaces.IEwmaCalculator,
        private _targetPosition: PositionManagement.TargetBasePositionManager,
        private _safeties: Safety.SafetyCalculator) {
        var recalcWithoutInputTime = () => this.recalcQuote(_timeProvider.utcNow());

        _filteredMarkets.FilteredMarketChanged.on(m => this.recalcQuote(Utils.timeOrDefault(m, _timeProvider)));
        _qlParamRepo.NewParameters.on(recalcWithoutInputTime);
        _orderBroker.Trade.on(recalcWithoutInputTime);
        _ewma.Updated.on(recalcWithoutInputTime);
        _targetPosition.NewTargetPosition.on(recalcWithoutInputTime);
        _safeties.NewValue.on(recalcWithoutInputTime);

        _timeProvider.setInterval(recalcWithoutInputTime, moment.duration(1, "seconds"));
    }

    private computeQuote(filteredMkt: Models.Market, fv: Models.FairValue) {
        var params = this._qlParamRepo.latest;
        var unrounded = this._registry.Get(params.mode).GenerateQuote(filteredMkt, fv, params);

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

        var tbp = this._targetPosition.latestTargetPosition;
        if (tbp === null) {
            // this._log.warn("cannot compute a quote since no position report exists!");
            return null;
        }
        var targetBasePosition = tbp.data;

        var latestPosition = this._positionBroker.latestReport;
        var totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        var totalQuotePosition = (latestPosition.quoteAmount + latestPosition.quoteHeldAmount) / fv.price;

        if (totalBasePosition < targetBasePosition - params.positionDivergence) {
            unrounded.askPx = null;
            unrounded.askSz = null;
            if (params.aggressivePositionRebalancing)
                unrounded.bidSz = Math.min(params.aprMultiplier*params.buySize, targetBasePosition - totalBasePosition, (latestPosition.quoteAmount / fv.price) / 2);
        }

        if (totalBasePosition > targetBasePosition + params.positionDivergence) {
            unrounded.bidPx = null;
            unrounded.bidSz = null;
            if (params.aggressivePositionRebalancing)
                unrounded.askSz = Math.min(params.aprMultiplier*params.sellSize, totalBasePosition - targetBasePosition, latestPosition.baseAmount / 2);
        }

        var safety = this._safeties.latest;
        if (safety === null) {
            this._log.warn("cannot compute a quote since trade safety is not yet computed!");
            return null;
        }

        if (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47) {
          if (unrounded.askSz && safety.buyPing && (params.pongAt == Models.PongAt.ShortPingAggressive || params.pongAt == Models.PongAt.LongPingAggressive || unrounded.askPx < safety.buyPing + params.width))
            unrounded.askPx = safety.buyPing + params.width;
          if (unrounded.bidSz && safety.sellPong && (params.pongAt == Models.PongAt.ShortPingAggressive || params.pongAt == Models.PongAt.LongPingAggressive || unrounded.bidPx > safety.sellPong - params.width))
            unrounded.bidPx = safety.sellPong - params.width;
        }

        if (unrounded.askPx !== null)
          for (var fai = 0; fai < filteredMkt.asks.length; fai++)
            if (filteredMkt.asks[fai].price > unrounded.askPx) {
              let bestAsk: number = filteredMkt.asks[fai].price - 1e-2;
              if (bestAsk > fv.price) {
                unrounded.askPx = bestAsk;
                break;
              }
            }
        if (unrounded.bidPx !== null)
          for (var fbi = 0; fbi < filteredMkt.bids.length; fbi++)
            if (filteredMkt.bids[fbi].price < unrounded.bidPx) {
              let bestBid: number = filteredMkt.bids[fbi].price + 1e-2;
              if (bestBid < fv.price) {
                unrounded.bidPx = bestBid;
                break;
              }
            }

        if (safety.sell > params.tradesPerMinute || (
            (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47)
            && !safety.buyPing && (params.pingAt === Models.PingAt.StopPings || params.pingAt === Models.PingAt.BidSide || params.pingAt === Models.PingAt.DepletedAskSide
              || (totalQuotePosition>params.buySize && (params.pingAt === Models.PingAt.DepletedSide || params.pingAt === Models.PingAt.DepletedBidSide))
        ))) {
            unrounded.askPx = null;
            unrounded.askSz = null;
        }
        if (safety.buy > params.tradesPerMinute || (
          (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47)
            && !safety.sellPong && (params.pingAt === Models.PingAt.StopPings || params.pingAt === Models.PingAt.AskSide || params.pingAt === Models.PingAt.DepletedBidSide
              || (totalBasePosition>params.sellSize && (params.pingAt === Models.PingAt.DepletedSide || params.pingAt === Models.PingAt.DepletedAskSide))
        ))) {
            unrounded.bidPx = null;
            unrounded.bidSz = null;
        }

        if (unrounded.bidPx !== null) {
            unrounded.bidPx = Utils.roundFloat(unrounded.bidPx);
            unrounded.bidPx = Math.max(0, unrounded.bidPx);
        }

        if (unrounded.askPx !== null) {
            unrounded.askPx = Utils.roundFloat(unrounded.askPx);
            unrounded.askPx = Math.max(unrounded.bidPx + 1e-2, unrounded.askPx);
        }

        if (unrounded.askSz !== null) {
            unrounded.askSz = Utils.roundFloat(unrounded.askSz);
            unrounded.askSz = Math.max(1e-2, unrounded.askSz);
        }

        if (unrounded.bidSz !== null) {
            unrounded.bidSz = Utils.roundFloat(unrounded.bidSz);
            unrounded.bidSz = Math.max(1e-2, unrounded.bidSz);
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
