import Models = require("../share/models");
import Utils = require("./utils");
import Safety = require("./safety");
import _ = require("lodash");
import FairValue = require("./fair-value");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import PositionManagement = require("./position-management");
import Broker = require("./broker");
import Statistics = require("./statistics");
import moment = require('moment');
import QuotingStyleRegistry = require("./quoting-styles/style-registry");
import {QuoteInput} from "./quoting-styles/helpers";

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

export class QuotingEngine {
    public QuoteChanged = new Utils.Evt<Models.TwoSidedQuote>();

    private _latest: Models.TwoSidedQuote = null;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val: Models.TwoSidedQuote) {
        if (!quotesChanged(this._latest, val, this._details.minTickIncrement))
            return;

        this._latest = val;
        this.QuoteChanged.trigger();
    }

    private _registry: QuotingStyleRegistry.QuotingStyleRegistry = null;

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _filteredMarkets: MarketFiltration.MarketFiltration,
        private _fvEngine: FairValue.FairValueEngine,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        private _orderBroker: Broker.OrderBroker,
        private _positionBroker: Broker.PositionBroker,
        private _details: Broker.ExchangeBroker,
        private _ewma: Statistics.ObservableEWMACalculator,
        private _stdev: Statistics.ObservableSTDEVCalculator,
        private _targetPosition: PositionManagement.TargetBasePositionManager,
        private _safeties: Safety.SafetyCalculator) {
        this._registry = new QuotingStyleRegistry.QuotingStyleRegistry();

        _filteredMarkets.FilteredMarketChanged.on(this.recalcQuote);
        _qlParamRepo.NewParameters.on(this.recalcQuote);
        _orderBroker.Trade.on(this.recalcQuote);
        _ewma.Updated.on(() => {
          this.recalcQuote();
          _targetPosition.quoteEWMA = _ewma.latest;
          _targetPosition.widthSTDEV = _stdev.latest;
        });
        _targetPosition.NewTargetPosition.on(() => {
          this._safeties.latestTargetPosition = this._targetPosition.latestTargetPosition;
          this.recalcQuote();
        });
        _safeties.NewValue.on(this.recalcQuote);

        _timeProvider.setInterval(this.recalcQuote, moment.duration(1, "seconds"));
    }

    private computeQuote(filteredMkt: Models.Market, fv: Models.FairValue) {
        if (this._targetPosition.latestTargetPosition === null) return null;
        const targetBasePosition = this._targetPosition.latestTargetPosition.data;

        const params = this._qlParamRepo.latest;
        const minTick = this._details.minTickIncrement;
        const minSize = this._details.minSize;
        const widthPing = (params.widthPercentage)
          ? params.widthPingPercentage * fv.price / 100
          : params.widthPing;
        const widthPong = (params.widthPercentage)
          ? params.widthPongPercentage * fv.price / 100
          : params.widthPong;
        const latestPosition = this._positionBroker.latestReport;
        const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        const totalQuotePosition = (latestPosition.quoteAmount + latestPosition.quoteHeldAmount) / fv.price;
        let buySize: number = params.percentageValues
             ? params.buySizePercentage * latestPosition.value / 100
             : params.buySize;
        if (params.aggressivePositionRebalancing != Models.APR.Off && params.buySizeMax)
          buySize = Math.max(buySize, targetBasePosition - totalBasePosition);
        let sellSize: number = params.percentageValues
            ? params.sellSizePercentage * latestPosition.value / 100
            : params.sellSize;
        if (params.aggressivePositionRebalancing != Models.APR.Off && params.sellSizeMax)
          sellSize = Math.max(sellSize, totalBasePosition - targetBasePosition);

        const unrounded = this._registry.GenerateQuote(new QuoteInput(filteredMkt, fv.price, widthPing, buySize, sellSize, params.mode, minTick));

        if (unrounded === null) return null;
        const _unroundedBidSz = unrounded.bidSz;
        const _unroundedAskSz = unrounded.askSz;

        let sideAPR: string[] = [];
        let superTradesMultipliers = (params.superTrades &&
          widthPing * params.sopWidthMultiplier < filteredMkt.asks[0].price - filteredMkt.bids[0].price
        ) ? [
          (params.superTrades == Models.SOP.x2trades || params.superTrades == Models.SOP.x2tradesSize
            ? 2 : (params.superTrades == Models.SOP.x3trades || params.superTrades == Models.SOP.x3tradesSize
              ? 3 : 1)),
          (params.superTrades == Models.SOP.x2Size || params.superTrades == Models.SOP.x2tradesSize
            ? 2 : (params.superTrades == Models.SOP.x3Size || params.superTrades == Models.SOP.x3tradesSize
              ? 3 : 1))
        ] : [1, 1];

        let pDiv: number  = (params.percentageValues)
          ? params.positionDivergencePercentage * latestPosition.value / 100
          : params.positionDivergence;

        if (superTradesMultipliers[1] > 1) {
          if (!params.buySizeMax) unrounded.bidSz = Math.min(superTradesMultipliers[1]*buySize, (latestPosition.quoteAmount / fv.price) / 2);
          if (!params.sellSizeMax) unrounded.askSz = Math.min(superTradesMultipliers[1]*sellSize, latestPosition.baseAmount / 2);
        }

        if (params.quotingEwmaProtection && this._ewma.latest !== null) {
            unrounded.askPx = Math.max(this._ewma.latest, unrounded.askPx);
            unrounded.bidPx = Math.min(this._ewma.latest, unrounded.bidPx);
        }

        if (totalBasePosition < targetBasePosition - pDiv) {
            unrounded.askPx = null;
            unrounded.askSz = null;
            if (params.aggressivePositionRebalancing !== Models.APR.Off) {
              sideAPR.push('Bid');
              if (!params.buySizeMax) unrounded.bidSz = Math.min(params.aprMultiplier*buySize, targetBasePosition - totalBasePosition, (latestPosition.quoteAmount / fv.price) / 2);
            }
        }
        if (totalBasePosition > targetBasePosition + pDiv) {
            unrounded.bidPx = null;
            unrounded.bidSz = null;
            if (params.aggressivePositionRebalancing !== Models.APR.Off) {
              sideAPR.push('Sell');
              if (!params.sellSizeMax) unrounded.askSz = Math.min(params.aprMultiplier*sellSize, totalBasePosition - targetBasePosition, latestPosition.baseAmount / 2);
            }
        }

        if (params.quotingStdevProtection !== Models.STDEV.Off && this._stdev.latest !== null) {
            if (unrounded.askPx && (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTop || sideAPR.indexOf('Sell')===-1))
              unrounded.askPx = Math.max(fv.price + this._stdev.latest[
                (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnFVAPROff)
                  ? 'fv' : ((params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTopsAPROff)
                    ? 'tops' : 'ask' )
              ], unrounded.askPx);
            if (unrounded.bidPx && (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTop || sideAPR.indexOf('Bid')===-1))
              unrounded.bidPx = Math.min(fv.price - this._stdev.latest[
                (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnFVAPROff)
                  ? 'fv' : ((params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTopsAPROff)
                    ? 'tops' : 'bid' )
              ], unrounded.bidPx);
        }

        this._targetPosition.sideAPR = sideAPR;

        const safety = this._safeties.latest;
        if (safety === null) {
            return null;
        }

        if (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47) {
          if (unrounded.askSz && safety.buyPing && (
            (params.aggressivePositionRebalancing === Models.APR.SizeWidth && sideAPR.indexOf('Sell')>-1)
            || params.pongAt == Models.PongAt.ShortPingAggressive
            || params.pongAt == Models.PongAt.LongPingAggressive
            || unrounded.askPx < safety.buyPing + widthPong
          )) unrounded.askPx = safety.buyPing + widthPong;
          if (unrounded.bidSz && safety.sellPong && (
            (params.aggressivePositionRebalancing === Models.APR.SizeWidth && sideAPR.indexOf('Buy')>-1)
            || params.pongAt == Models.PongAt.ShortPingAggressive
            || params.pongAt == Models.PongAt.LongPingAggressive
            || unrounded.bidPx > safety.sellPong - widthPong
          )) unrounded.bidPx = safety.sellPong - widthPong;
        }

        if (params.bestWidth) {
          if (unrounded.askPx !== null)
            for (var fai = 0; fai < filteredMkt.asks.length; fai++)
              if (filteredMkt.asks[fai].price > unrounded.askPx) {
                let bestAsk: number = filteredMkt.asks[fai].price - minTick;
                if (bestAsk > fv.price) {
                  unrounded.askPx = bestAsk;
                  break;
                }
              }
          if (unrounded.bidPx !== null)
            for (var fbi = 0; fbi < filteredMkt.bids.length; fbi++)
              if (filteredMkt.bids[fbi].price < unrounded.bidPx) {
                let bestBid: number = filteredMkt.bids[fbi].price + minTick;
                if (bestBid < fv.price) {
                  unrounded.bidPx = bestBid;
                  break;
                }
              }
        }

        if (safety.sell > (params.tradesPerMinute * superTradesMultipliers[0]) || (
            (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47)
            && !safety.buyPing && (params.pingAt === Models.PingAt.StopPings || params.pingAt === Models.PingAt.BidSide || params.pingAt === Models.PingAt.DepletedAskSide
              || (totalQuotePosition>buySize && (params.pingAt === Models.PingAt.DepletedSide || params.pingAt === Models.PingAt.DepletedBidSide))
        ))) {
            unrounded.askPx = null;
            unrounded.askSz = null;
        }
        if (safety.buy > (params.tradesPerMinute * superTradesMultipliers[0]) || (
          (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47)
            && !safety.sellPong && (params.pingAt === Models.PingAt.StopPings || params.pingAt === Models.PingAt.AskSide || params.pingAt === Models.PingAt.DepletedBidSide
              || (totalBasePosition>sellSize && (params.pingAt === Models.PingAt.DepletedSide || params.pingAt === Models.PingAt.DepletedAskSide))
        ))) {
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
            if (unrounded.askSz > totalBasePosition)
              unrounded.askSz = (!_unroundedBidSz || _unroundedBidSz > totalBasePosition)
                ? totalBasePosition : _unroundedBidSz;
            unrounded.askSz = Utils.roundDown(Math.max(minSize, unrounded.askSz), 1e-8);
        }

        if (unrounded.bidSz !== null) {
            if (unrounded.bidSz > totalQuotePosition)
              unrounded.bidSz = (!_unroundedAskSz || _unroundedAskSz > totalQuotePosition)
                ? totalQuotePosition : _unroundedAskSz;
            unrounded.bidSz = Utils.roundDown(Math.max(minSize, unrounded.bidSz), 1e-8);
        }

        return unrounded;
    }

    private recalcQuote = () => {
        const fv = this._fvEngine.latestFairValue;
        if (fv == null) {
            this.latestQuote = null;
            return;
        }

        const filteredMkt = this._filteredMarkets.latestFilteredMarket;
        if (filteredMkt == null || !filteredMkt.bids.length || !filteredMkt.asks.length) {
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
            this._timeProvider.utcNow()
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
        if (!quoteWasWidened && Math.abs((new Date()).getTime() - prevTwoSided.time.getTime()) < 300) {
            return previousQ;
        }

        return newQ;
    }
}