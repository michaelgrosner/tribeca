import Models = require("../share/models");
import Utils = require("./utils");
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
    private _lastQuote: number = new Date().getTime();
    private _latest: Models.TwoSidedQuote = null;
    public latestQuoteAskStatus: Models.QuoteStatus;
    public latestQuoteBidStatus: Models.QuoteStatus;
    public get latestQuote() { return this._latest; }
    public set latestQuote(val: Models.TwoSidedQuote) {
        if (!quotesChanged(this._latest, val, this._minTick))
            return;

        this._latest = val;
        this._evUp('Quote');
    }

    private _registry: QuotingStyleRegistry.QuotingStyleRegistry = null;

    constructor(
      private _fvEngine,
      private _mgFilter,
      private _qpRepo,
      private _positionBroker,
      private _minTick: number,
      private _minSize: number,
      private _ewmaP,
      private _stdevP,
      private _pgTargetBasePos,
      private _pgSideAPR,
      private _safeties,
      private _evOn,
      private _evUp
    ) {
      this._registry = new QuotingStyleRegistry.QuotingStyleRegistry();

      this._evOn('EWMAProtectionCalculator', this.recalcQuote);
      this._evOn('FilteredMarket', this.recalcQuote);
      this._evOn('QuotingParameters', this.recalcQuote);
      this._evOn('OrderTradeBroker', this.recalcQuote);
      this._evOn('TargetPosition', this.recalcQuote);
      this._evOn('Safety', this.recalcQuote);

      setInterval(this.recalcQuote, moment.duration(1, "seconds"));
    }

    private computeQuote(filteredMkt: Models.Market, fv: number) {
        const latestPosition = this._positionBroker();
        if (latestPosition === null) return;
        const targetBasePosition = this._pgTargetBasePos();
        const params = this._qpRepo();
        const widthPing = (params.widthPercentage)
          ? params.widthPingPercentage * fv / 100
          : params.widthPing;
        const widthPong = (params.widthPercentage)
          ? params.widthPongPercentage * fv / 100
          : params.widthPong;
        const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        const totalQuotePosition = (latestPosition.quoteAmount + latestPosition.quoteHeldAmount) / fv;
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

        const unrounded = this._registry.GenerateQuote(new QuoteInput(filteredMkt, fv, widthPing, buySize, sellSize, params.mode, this._minTick));
        if (unrounded === null) return;
        const _unroundedBidSz = unrounded.bidSz;
        const _unroundedAskSz = unrounded.askSz;

        const safety = this._safeties();
        if (safety === null) {
            return null;
        }

        this.latestQuoteAskStatus = Models.QuoteStatus.UnknownHeld;
        this.latestQuoteBidStatus = Models.QuoteStatus.UnknownHeld;

        let sideAPR: string = '';
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
          if (!params.buySizeMax) unrounded.bidSz = Math.min(superTradesMultipliers[1]*buySize, (latestPosition.quoteAmount / fv) / 2);
          if (!params.sellSizeMax) unrounded.askSz = Math.min(superTradesMultipliers[1]*sellSize, latestPosition.baseAmount / 2);
        }

        const ewma = this._ewmaP();
        if (params.quotingEwmaProtection && ewma) {
            unrounded.askPx = Math.max(ewma, unrounded.askPx);
            unrounded.bidPx = Math.min(ewma, unrounded.bidPx);
        }

        if (totalBasePosition < targetBasePosition - pDiv) {
            this.latestQuoteAskStatus = Models.QuoteStatus.TBPHeld;
            unrounded.askPx = null;
            unrounded.askSz = null;
            if (params.aggressivePositionRebalancing !== Models.APR.Off) {
              sideAPR = 'Bid';
              if (!params.buySizeMax) unrounded.bidSz = Math.min(params.aprMultiplier*buySize, targetBasePosition - totalBasePosition, (latestPosition.quoteAmount / fv) / 2);
            }
        }
        else if (totalBasePosition > targetBasePosition + pDiv) {
            this.latestQuoteBidStatus = Models.QuoteStatus.TBPHeld;
            unrounded.bidPx = null;
            unrounded.bidSz = null;
            if (params.aggressivePositionRebalancing !== Models.APR.Off) {
              sideAPR = 'Sell';
              if (!params.sellSizeMax) unrounded.askSz = Math.min(params.aprMultiplier*sellSize, totalBasePosition - targetBasePosition, latestPosition.baseAmount / 2);
            }
        }

        if (params.quotingStdevProtection !== Models.STDEV.Off && this._stdevP() !== null) {
            if (unrounded.askPx && (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTop || sideAPR !== 'Sell'))
              unrounded.askPx = Math.max((params.quotingStdevBollingerBands ? this._stdevP()[
                (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnFVAPROff)
                  ? 'fvMean' : ((params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTopsAPROff)
                    ? 'topsMean' : 'askMean' )
              ]: fv) + this._stdevP()[
                (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnFVAPROff)
                  ? 'fv' : ((params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTopsAPROff)
                    ? 'tops' : 'ask' )
              ], unrounded.askPx);
            if (unrounded.bidPx && (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTop || sideAPR !== 'Bid'))
              unrounded.bidPx = Math.min((params.quotingStdevBollingerBands ? this._stdevP()[
                (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnFVAPROff)
                  ? 'fvMean' : ((params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTopsAPROff)
                    ? 'topsMean' : 'bidMean' )
              ]  : fv) - this._stdevP()[
                (params.quotingStdevProtection === Models.STDEV.OnFV || params.quotingStdevProtection === Models.STDEV.OnFVAPROff)
                  ? 'fv' : ((params.quotingStdevProtection === Models.STDEV.OnTops || params.quotingStdevProtection === Models.STDEV.OnTopsAPROff)
                    ? 'tops' : 'bid' )
              ], unrounded.bidPx);
        }

        this._pgSideAPR(sideAPR);

        if (params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47) {
          if (unrounded.askSz && safety.buyPing && (
            (params.aggressivePositionRebalancing === Models.APR.SizeWidth && sideAPR === 'Sell')
            || params.pongAt == Models.PongAt.ShortPingAggressive
            || params.pongAt == Models.PongAt.LongPingAggressive
            || unrounded.askPx < safety.buyPing + widthPong
          )) unrounded.askPx = safety.buyPing + widthPong;
          if (unrounded.bidSz && safety.sellPong && (
            (params.aggressivePositionRebalancing === Models.APR.SizeWidth && sideAPR === 'Buy')
            || params.pongAt == Models.PongAt.ShortPingAggressive
            || params.pongAt == Models.PongAt.LongPingAggressive
            || unrounded.bidPx > safety.sellPong - widthPong
          )) unrounded.bidPx = safety.sellPong - widthPong;
        }

        if (params.bestWidth) {
          if (unrounded.askPx !== null)
            for (var fai = 0; fai < filteredMkt.asks.length; fai++)
              if (filteredMkt.asks[fai].price > unrounded.askPx) {
                let bestAsk: number = filteredMkt.asks[fai].price - this._minTick;
                if (bestAsk > fv) {
                  unrounded.askPx = bestAsk;
                  break;
                }
              }
          if (unrounded.bidPx !== null)
            for (var fbi = 0; fbi < filteredMkt.bids.length; fbi++)
              if (filteredMkt.bids[fbi].price < unrounded.bidPx) {
                let bestBid: number = filteredMkt.bids[fbi].price + this._minTick;
                if (bestBid < fv) {
                  unrounded.bidPx = bestBid;
                  break;
                }
              }
        }

        if (safety.sell > (params.tradesPerMinute * superTradesMultipliers[0])) {
            this.latestQuoteAskStatus = Models.QuoteStatus.MaxTradesSeconds;
            unrounded.askPx = null;
            unrounded.askSz = null;
        }
        if ((params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47)
            && !safety.buyPing && (params.pingAt === Models.PingAt.StopPings || params.pingAt === Models.PingAt.BidSide || params.pingAt === Models.PingAt.DepletedAskSide
              || (totalQuotePosition>buySize && (params.pingAt === Models.PingAt.DepletedSide || params.pingAt === Models.PingAt.DepletedBidSide))
        )) {
            this.latestQuoteAskStatus = !safety.buyPing
              ? Models.QuoteStatus.WaitingPing
              : Models.QuoteStatus.DepletedFunds;
            unrounded.askPx = null;
            unrounded.askSz = null;
        }

        if (safety.buy > (params.tradesPerMinute * superTradesMultipliers[0])) {
            this.latestQuoteBidStatus = Models.QuoteStatus.MaxTradesSeconds;
            unrounded.bidPx = null;
            unrounded.bidSz = null;
        }
        if ((params.mode === Models.QuotingMode.PingPong || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.AK47)
            && !safety.sellPong && (params.pingAt === Models.PingAt.StopPings || params.pingAt === Models.PingAt.AskSide || params.pingAt === Models.PingAt.DepletedBidSide
              || (totalBasePosition>sellSize && (params.pingAt === Models.PingAt.DepletedSide || params.pingAt === Models.PingAt.DepletedAskSide))
        )) {
            this.latestQuoteBidStatus = !safety.sellPong
              ? Models.QuoteStatus.WaitingPing
              : Models.QuoteStatus.DepletedFunds;
            unrounded.bidPx = null;
            unrounded.bidSz = null;
        }

        if (unrounded.bidPx !== null) {
            unrounded.bidPx = Utils.roundSide(unrounded.bidPx, this._minTick, Models.Side.Bid);
            unrounded.bidPx = Math.max(0, unrounded.bidPx);
        }

        if (unrounded.askPx !== null) {
            unrounded.askPx = Utils.roundSide(unrounded.askPx, this._minTick, Models.Side.Ask);
            unrounded.askPx = Math.max(unrounded.bidPx + this._minTick, unrounded.askPx);
        }

        if (unrounded.askSz !== null) {
            if (unrounded.askSz > totalBasePosition)
              unrounded.askSz = (!_unroundedBidSz || _unroundedBidSz > totalBasePosition)
                ? totalBasePosition : _unroundedBidSz;
            unrounded.askSz = Utils.roundDown(Math.max(this._minSize, unrounded.askSz), 1e-8);
            unrounded.isAskPong = (safety.buyPing && unrounded.askPx && unrounded.askPx >= safety.buyPing + widthPong);
        }

        if (unrounded.bidSz !== null) {
            if (unrounded.bidSz > totalQuotePosition)
              unrounded.bidSz = (!_unroundedAskSz || _unroundedAskSz > totalQuotePosition)
                ? totalQuotePosition : _unroundedAskSz;
            unrounded.bidSz = Utils.roundDown(Math.max(this._minSize, unrounded.bidSz), 1e-8);
            unrounded.isBidPong = (safety.sellPong && unrounded.bidPx && unrounded.bidPx <= safety.sellPong - widthPong);
        }

        return unrounded;
    }

    private recalcQuote = () => {
        this.latestQuoteAskStatus = Models.QuoteStatus.MissingData;
        this.latestQuoteBidStatus = Models.QuoteStatus.MissingData;

        const fv = this._fvEngine();
        if (!fv) {
            this.latestQuote = null;
            return;
        }

        const filteredMkt = this._mgFilter();
        if (filteredMkt == null || !filteredMkt.bids.length || !filteredMkt.asks.length) {
            this.latestQuote = null;
            return;
        }

        const genQt = this.computeQuote(filteredMkt, fv);
        if (genQt === null || typeof genQt == "undefined") {
            this.latestQuote = null;
            return;
        }

        this.latestQuote = new Models.TwoSidedQuote(
            this.quotesAreSame(new Models.Quote(genQt.bidPx, genQt.bidSz, genQt.isBidPong), this.latestQuote, Models.Side.Bid),
            this.quotesAreSame(new Models.Quote(genQt.askPx, genQt.askSz, genQt.isAskPong), this.latestQuote, Models.Side.Ask)
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

        if (Math.abs(newQ.price - previousQ.price) < this._minTick) {
            return previousQ;
        }

        let quoteWasWidened = true;
        if (Models.Side.Bid === side && previousQ.price < newQ.price) quoteWasWidened = false;
        if (Models.Side.Ask === side && previousQ.price > newQ.price) quoteWasWidened = false;

        if (!quoteWasWidened && new Date().getTime() - this._lastQuote < 300) {
            return previousQ;
        }
        this._lastQuote = new Date().getTime();

        return newQ;
    }
}