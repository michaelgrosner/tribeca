#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  class QE: public Klass {
    private:
      map<mQuotingMode, function<mQuote(double, double, double)>*> quotingMode;
      mQuoteState bidStatus = mQuoteState::MissingData,
                  askStatus = mQuoteState::MissingData;
      mQuoteStatus status;
      map<mSide, mLevel> nextStart;
      bool nextIsPong = false;
    public:
      mConnectivity gwConnectButton = mConnectivity::Disconnected,
                    gwConnectExchange = mConnectivity::Disconnected;
    protected:
      void load() {
        quotingMode[mQuotingMode::Top] = &calcTopOfMarket;
        quotingMode[mQuotingMode::Mid] = &calcMidOfMarket;
        quotingMode[mQuotingMode::Join] = &calcJoinMarket;
        quotingMode[mQuotingMode::InverseJoin] = &calcInverseJoinMarket;
        quotingMode[mQuotingMode::InverseTop] = &calcInverseTopOfMarket;
        quotingMode[mQuotingMode::HamelinRat] = &calcColossusOfMarket;
        quotingMode[mQuotingMode::Depth] = &calcDepthOfMarket;
      };
      void waitTime() {
        ((EV*)events)->tStart->data = (void*)this;
        ((EV*)events)->tCalcs->data = (void*)this;
        uv_timer_start(((EV*)events)->tCalcs, [](uv_timer_t *handle) {
          QE *k = (QE*)handle->data;
          if (((CF*)k->config)->argDebugEvents) FN::log("DEBUG", "EV GW tCalcs timer");
          if (((MG*)k->market)->fairValue) {
            ((MG*)k->market)->calcStats();
            ((PG*)k->wallet)->calcSafety();
            k->calcQuote();
          } else FN::logWar("QE", "Unable to calculate quote, missing fair value");
        }, 1e+3, 1e+3);
      };
      void waitData() {
        ((EV*)events)->uiQuotingParameters = [&]() {
          if (((CF*)config)->argDebugEvents) FN::log("DEBUG", "EV QE uiQuotingParameters");
          ((MG*)market)->calcFairValue();
          ((PG*)wallet)->calcTargetBasePos();
          ((PG*)wallet)->calcSafety();
          calcQuote();
        };
        ((EV*)events)->ogTrade = [&](mTrade k) {
          if (((CF*)config)->argDebugEvents) FN::log("DEBUG", "EV QE ogTrade");
          ((PG*)wallet)->addTrade(k);
          ((PG*)wallet)->calcSafety();
          calcQuote();
        };
        ((EV*)events)->mgEwmaQuoteProtection = [&]() {
          if (((CF*)config)->argDebugEvents) FN::log("DEBUG", "EV QE mgEwmaQuoteProtection");
          calcQuote();
        };
        ((EV*)events)->mgEwmaSMUProtection = [&]() {
          if (((CF*)config)->argDebugEvents) FN::log("DEBUG", "EV QE mgEwmaSMUProtection");
          calcQuote();
        };
        ((EV*)events)->mgLevels = [&]() {
          if (((CF*)config)->argDebugEvents) FN::log("DEBUG", "EV QE mgLevels");
          calcQuote();
        };
        ((EV*)events)->pgTargetBasePosition = [&]() {
          if (((CF*)config)->argDebugEvents) FN::log("DEBUG", "EV QE pgTargetBasePosition");
          calcQuote();
        };
      };
      void waitUser() {
        ((UI*)client)->welcome(uiTXT::QuoteStatus, &hello);
      };
    private:
      function<json()> hello = [&]() {
        return (json){ status };
      };
      void calcQuote() {
        bidStatus = mQuoteState::MissingData;
        askStatus = mQuoteState::MissingData;
        if (gwConnectExchange == mConnectivity::Disconnected) {
          askStatus = mQuoteState::Disconnected;
          bidStatus = mQuoteState::Disconnected;
        } else if (((MG*)market)->fairValue and !((MG*)market)->empty()) {
          if (gwConnectButton == mConnectivity::Disconnected) {
            askStatus = mQuoteState::DisabledQuotes;
            bidStatus = mQuoteState::DisabledQuotes;
            stopAllQuotes();
          } else {
            bidStatus = mQuoteState::UnknownHeld;
            askStatus = mQuoteState::UnknownHeld;
            sendQuoteToAPI();
          }
        }
        sendStatusToUI();
      };
      void sendQuoteToAPI() {
        mQuote quote = nextQuote();
        bidStatus = checkCrossedQuotes(mSide::Bid, &quote);
        askStatus = checkCrossedQuotes(mSide::Ask, &quote);
        if (askStatus == mQuoteState::Live) updateQuote(quote.ask, mSide::Ask, quote.isAskPong);
        else stopAllQuotes(mSide::Ask);
        if (bidStatus == mQuoteState::Live) updateQuote(quote.bid, mSide::Bid, quote.isBidPong);
        else stopAllQuotes(mSide::Bid);
      };
      void sendStatusToUI() {
        unsigned int quotesInMemoryNew = 0;
        unsigned int quotesInMemoryWorking = 0;
        unsigned int quotesInMemoryDone = 0;
        bool k = diffCounts(&quotesInMemoryNew, &quotesInMemoryWorking, &quotesInMemoryDone);
        if (!diffStatus() and !k) return;
        status = mQuoteStatus(bidStatus, askStatus, quotesInMemoryNew, quotesInMemoryWorking, quotesInMemoryDone);
        ((UI*)client)->send(uiTXT::QuoteStatus, status, true);
      };
      bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        ((OG*)broker)->countOrders(qNew, qWorking, qDone);
        return *qNew != status.quotesInMemoryNew
          or *qWorking != status.quotesInMemoryWorking
          or *qDone != status.quotesInMemoryDone;
      };
      bool diffStatus() {
        return bidStatus != status.bidStatus
          or askStatus != status.askStatus;
      };
      mQuote nextQuote() {
        if (((MG*)market)->empty() or ((PG*)wallet)->empty()) return mQuote();
        ((PG*)wallet)->pgMutex.lock();
        double value           = ((PG*)wallet)->position.value,
               baseAmount      = ((PG*)wallet)->position.baseAmount,
               baseHeldAmount  = ((PG*)wallet)->position.baseHeldAmount,
               quoteAmount     = ((PG*)wallet)->position.quoteAmount,
               quoteHeldAmount = ((PG*)wallet)->position.quoteHeldAmount,
               safetyBuyPing   = ((PG*)wallet)->safety.buyPing,
               safetySellPong  = ((PG*)wallet)->safety.sellPong,
               safetyBuy       = ((PG*)wallet)->safety.buy,
               safetySell      = ((PG*)wallet)->safety.sell;
        ((PG*)wallet)->pgMutex.unlock();
        if (safetyBuyPing == -1) return mQuote();
        double totalBasePosition = baseAmount + baseHeldAmount;
        double totalQuotePosition = (quoteAmount + quoteHeldAmount) / ((MG*)market)->fairValue;
        double pDiv = qp->percentageValues
          ? qp->positionDivergencePercentage * value / 100
          : qp->positionDivergence;
        double buySize = qp->percentageValues
          ? qp->buySizePercentage * value / 100
          : qp->buySize;
        double sellSize = qp->percentageValues
          ? qp->sellSizePercentage * value / 100
          : qp->sellSize;
        double widthPing = qp->widthPercentage
          ? qp->widthPingPercentage * ((MG*)market)->fairValue / 100
          : qp->widthPing;
        double widthPong = qp->widthPercentage
          ? qp->widthPongPercentage * ((MG*)market)->fairValue / 100
          : qp->widthPong;
        if (qp.positionDivergenceMode != mPDiffMode::Off && autoPositionMode != mAutoPositionMode::Manual) pDiv = ((PG*)wallet)->pgDynamicPDiff;
        if (buySize and qp->aggressivePositionRebalancing != mAPR::Off and qp->buySizeMax)
          buySize = fmax(buySize, ((PG*)wallet)->targetBasePosition - totalBasePosition);
        if (sellSize and qp->aggressivePositionRebalancing != mAPR::Off and qp->sellSizeMax)
          sellSize = fmax(sellSize, totalBasePosition - ((PG*)wallet)->targetBasePosition);
        mQuote rawQuote = quote(widthPing, buySize, sellSize);
        if (!rawQuote.bid.price and !rawQuote.ask.price) return mQuote();
        if (rawQuote.bid.price < 0 or rawQuote.ask.price < 0) {
          FN::logWar("QP", "Negative price detected, widthPing or/and widthPong must be smaller");
          return mQuote();
        }
        const double rawBidSz = rawQuote.bid.size;
        const double rawAskSz = rawQuote.ask.size;
        bool superTradesActive = false;
        debug("?", rawQuote); applySuperTrades(&rawQuote, &superTradesActive, widthPing, buySize, sellSize, quoteAmount, baseAmount);
        debug("A", rawQuote); applyEwmaProtection(&rawQuote);
        debug("B", rawQuote); applyEwmaSMUProtection(&rawQuote);
        debug("C", rawQuote); applyTotalBasePosition(&rawQuote, totalBasePosition, pDiv, buySize, sellSize, quoteAmount, baseAmount);
        debug("D", rawQuote); applyStdevProtection(&rawQuote);
        debug("E", rawQuote); applyAggressivePositionRebalancing(&rawQuote, widthPong, safetyBuyPing, safetySellPong);
        debug("F", rawQuote); applyAK47Increment(&rawQuote, value);
        debug("G", rawQuote); applyBestWidth(&rawQuote);
        debug("H", rawQuote); applyTradesPerMinute(&rawQuote, superTradesActive, safetyBuy, safetySell);
        debug("I", rawQuote); applyWaitingPing(&rawQuote, buySize, sellSize, totalQuotePosition, totalBasePosition, safetyBuyPing, safetySellPong);
        debug("J", rawQuote); applyRoundSide(&rawQuote);
        debug("K", rawQuote); applyRoundDown(&rawQuote, rawBidSz, rawAskSz, widthPong, safetyBuyPing, safetySellPong, totalQuotePosition, totalBasePosition);
        debug("L", rawQuote); applyDepleted(&rawQuote, totalQuotePosition, totalBasePosition);
        debug("!", rawQuote);
        if (((CF*)config)->argDebugQuotes) FN::log("DEBUG", string("QE totals ") + "toAsk:" + to_string(totalBasePosition) + " toBid:" + to_string(totalQuotePosition) + " min:" + to_string(gw->minSize));
        return rawQuote;
      };
      void debug(string k, mQuote rawQuote) {
        if (((CF*)config)->argDebugQuotes) FN::log("DEBUG", string("QE quote") + k + " " + to_string((int)bidStatus) + " " + to_string((int)askStatus) + " " + ((json)rawQuote).dump());
      };
      void applyRoundSide(mQuote *rawQuote) {
        if (rawQuote->bid.price) {
          rawQuote->bid.price = FN::roundSide(rawQuote->bid.price, gw->minTick, mSide::Bid);
          rawQuote->bid.price = fmax(0, rawQuote->bid.price);
        }
        if (rawQuote->ask.price) {
          rawQuote->ask.price = FN::roundSide(rawQuote->ask.price, gw->minTick, mSide::Ask);
          rawQuote->ask.price = fmax(rawQuote->bid.price + gw->minTick, rawQuote->ask.price);
        }
      };
      void applyRoundDown(mQuote *rawQuote, const double rawBidSz, const double rawAskSz, double widthPong, double safetyBuyPing, double safetySellPong, double totalQuotePosition, double totalBasePosition) {
        if (rawQuote->ask.size) {
          if (rawQuote->ask.size > totalBasePosition)
            rawQuote->ask.size = (!rawBidSz or rawBidSz > totalBasePosition)
              ? totalBasePosition : rawBidSz;
          rawQuote->ask.size = FN::roundDown(fmax(gw->minSize, rawQuote->ask.size), 1e-8);
          rawQuote->isAskPong = (safetyBuyPing and rawQuote->ask.price and rawQuote->ask.price >= safetyBuyPing + widthPong);
        } else rawQuote->isAskPong = false;
        if (rawQuote->bid.size) {
          if (rawQuote->bid.size > totalQuotePosition)
            rawQuote->bid.size = (!rawAskSz or rawAskSz > totalQuotePosition)
              ? totalQuotePosition : rawAskSz;
          rawQuote->bid.size = FN::roundDown(fmax(gw->minSize, rawQuote->bid.size), 1e-8);
          rawQuote->isBidPong = (safetySellPong and rawQuote->bid.price and rawQuote->bid.price <= safetySellPong - widthPong);
        } else rawQuote->isBidPong = false;
      };
      void applyWaitingPing(mQuote *rawQuote, double buySize, double sellSize, double totalQuotePosition, double totalBasePosition, double safetyBuyPing, double safetySellPong) {
        if ((qp->safety == mQuotingSafety::PingPong or qp->_matchPings)
          and !safetyBuyPing and (qp->pingAt == mPingAt::StopPings or qp->pingAt == mPingAt::BidSide or qp->pingAt == mPingAt::DepletedAskSide
            or (totalQuotePosition>buySize and (qp->pingAt == mPingAt::DepletedSide or qp->pingAt == mPingAt::DepletedBidSide))
        )) {
          askStatus = !safetyBuyPing
            ? mQuoteState::WaitingPing
            : mQuoteState::DepletedFunds;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
        }
        if ((qp->safety == mQuotingSafety::PingPong or qp->_matchPings)
          and !safetySellPong and (qp->pingAt == mPingAt::StopPings or qp->pingAt == mPingAt::AskSide or qp->pingAt == mPingAt::DepletedBidSide
            or (totalBasePosition>sellSize and (qp->pingAt == mPingAt::DepletedSide or qp->pingAt == mPingAt::DepletedAskSide))
        )) {
          bidStatus = !safetySellPong
            ? mQuoteState::WaitingPing
            : mQuoteState::DepletedFunds;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
        }
      };
      void applyDepleted(mQuote *rawQuote, double totalQuotePosition, double totalBasePosition) {
        if (rawQuote->bid.size and rawQuote->bid.size > totalQuotePosition) {
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
          bidStatus = mQuoteState::DepletedFunds;
        }
        if (rawQuote->ask.size and rawQuote->ask.size > totalBasePosition) {
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
          askStatus = mQuoteState::DepletedFunds;
        }
      };
      void applyBestWidth(mQuote *rawQuote) {
        if (!qp->bestWidth) return;
        if (rawQuote->ask.price)
          for (vector<mLevel>::iterator it = ((MG*)market)->levels.asks.begin(); it != ((MG*)market)->levels.asks.end(); ++it)
            if (it->price > rawQuote->ask.price) {
              double bestAsk = it->price - gw->minTick;
              if (bestAsk > ((MG*)market)->fairValue) {
                rawQuote->ask.price = bestAsk;
                break;
              }
            }
        if (rawQuote->bid.price)
          for (vector<mLevel>::iterator it = ((MG*)market)->levels.bids.begin(); it != ((MG*)market)->levels.bids.end(); ++it)
            if (it->price < rawQuote->bid.price) {
              double bestBid = it->price + gw->minTick;
              if (bestBid < ((MG*)market)->fairValue) {
                rawQuote->bid.price = bestBid;
                break;
              }
            }
      };
      void applyTotalBasePosition(mQuote *rawQuote, double totalBasePosition, double pDiv, double buySize, double sellSize, double quoteAmount, double baseAmount) {
        if (totalBasePosition < ((PG*)wallet)->targetBasePosition - pDiv) {
          if(askStatus != mQuoteState::UpTrendHeld or askStatus != mQuoteState::DownTrendHeld)
            askStatus = mQuoteState::TBPHeld;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
          if (qp->aggressivePositionRebalancing != mAPR::Off) {
            ((PG*)wallet)->sideAPR = "Buy";
            if (!qp->buySizeMax) rawQuote->bid.size = fmin(qp->aprMultiplier*buySize, fmin(((PG*)wallet)->targetBasePosition - totalBasePosition, (quoteAmount / ((MG*)market)->fairValue) / 2));
          }
        }
        else if (totalBasePosition >= ((PG*)wallet)->targetBasePosition + pDiv) {
          if(bidStatus != mQuoteState::UpTrendHeld or bidStatus != mQuoteState::DownTrendHeld)
            bidStatus = mQuoteState::TBPHeld;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
          if (qp->aggressivePositionRebalancing != mAPR::Off) {
            ((PG*)wallet)->sideAPR = "Sell";
            if (!qp->sellSizeMax) rawQuote->ask.size = fmin(qp->aprMultiplier*sellSize, fmin(totalBasePosition - ((PG*)wallet)->targetBasePosition, baseAmount / 2));
          }
        }
        else ((PG*)wallet)->sideAPR = "Off";
      };
      void applyTradesPerMinute(mQuote *rawQuote, bool superTradesActive, double safetyBuy, double safetySell) {
        if (safetySell > (qp->tradesPerMinute * (superTradesActive ? qp->sopWidthMultiplier : 1))) {
          askStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
        }
        if (safetyBuy > (qp->tradesPerMinute * (superTradesActive ? qp->sopWidthMultiplier : 1))) {
          bidStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
        }
      };
      void applyAggressivePositionRebalancing(mQuote *rawQuote, double widthPong, double safetyBuyPing, double safetySellPong) {
        if (qp->safety == mQuotingSafety::Boomerang or qp->safety == mQuotingSafety::AK47 or qp->_matchPings) {
          if (rawQuote->ask.size and safetyBuyPing and (
            (qp->aggressivePositionRebalancing == mAPR::SizeWidth and ((PG*)wallet)->sideAPR == "Sell")
            or qp->pongAt == mPongAt::ShortPingAggressive
            or qp->pongAt == mPongAt::LongPingAggressive
            or rawQuote->ask.price < safetyBuyPing + widthPong
          )) rawQuote->ask.price = safetyBuyPing + widthPong;
          if (rawQuote->bid.size and safetySellPong and (
            (qp->aggressivePositionRebalancing == mAPR::SizeWidth and ((PG*)wallet)->sideAPR == "Buy")
            or qp->pongAt == mPongAt::ShortPingAggressive
            or qp->pongAt == mPongAt::LongPingAggressive
            or rawQuote->bid.price > safetySellPong - widthPong
          )) rawQuote->bid.price = safetySellPong - widthPong;
        }
      };
      void applySuperTrades(mQuote *rawQuote, bool *superTradesActive, double widthPing, double buySize, double sellSize, double quoteAmount, double baseAmount) {
        if (qp->superTrades != mSOP::Off
          and widthPing * qp->sopWidthMultiplier < ((MG*)market)->levels.asks.begin()->price - ((MG*)market)->levels.bids.begin()->price
        ) {
          *superTradesActive = (qp->superTrades == mSOP::Trades or qp->superTrades == mSOP::TradesSize);
          if (qp->superTrades == mSOP::Size or qp->superTrades == mSOP::TradesSize) {
            if (!qp->buySizeMax) rawQuote->bid.size = fmin(qp->sopSizeMultiplier * buySize, (quoteAmount / ((MG*)market)->fairValue) / 2);
            if (!qp->sellSizeMax) rawQuote->ask.size = fmin(qp->sopSizeMultiplier * sellSize, baseAmount / 2);
          }
        }
      };
      void applyAK47Increment(mQuote *rawQuote, double value) {
        if (qp->safety == mQuotingSafety::AK47) {
          static int inc = 1;
          double range = qp->percentageValues
            ? qp->rangePercentage * value / 100
            : qp->range;
          rawQuote->bid.size -= inc * range;
          rawQuote->ask.size += inc * range;
          if (++inc > qp->bullets) inc = 1;
        }
      };
      void applyStdevProtection(mQuote *rawQuote) {
        if (qp->quotingStdevProtection == mSTDEV::Off or !((MG*)market)->mgStdevFV) return;
        if (rawQuote->ask.price and (qp->quotingStdevProtection == mSTDEV::OnFV or qp->quotingStdevProtection == mSTDEV::OnTops or qp->quotingStdevProtection == mSTDEV::OnTop or ((PG*)wallet)->sideAPR != "Sell"))
          rawQuote->ask.price = fmax(
            (qp->quotingStdevBollingerBands
              ? (qp->quotingStdevProtection == mSTDEV::OnFV or qp->quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? ((MG*)market)->mgStdevFVMean : ((qp->quotingStdevProtection == mSTDEV::OnTops or qp->quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? ((MG*)market)->mgStdevTopMean : ((MG*)market)->mgStdevAskMean )
              : ((MG*)market)->fairValue) + ((qp->quotingStdevProtection == mSTDEV::OnFV or qp->quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? ((MG*)market)->mgStdevFV : ((qp->quotingStdevProtection == mSTDEV::OnTops or qp->quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? ((MG*)market)->mgStdevTop : ((MG*)market)->mgStdevAsk )),
            rawQuote->ask.price
          );
        if (rawQuote->bid.price and (qp->quotingStdevProtection == mSTDEV::OnFV or qp->quotingStdevProtection == mSTDEV::OnTops or qp->quotingStdevProtection == mSTDEV::OnTop or ((PG*)wallet)->sideAPR != "Buy")) {
          rawQuote->bid.price = fmin(
            (qp->quotingStdevBollingerBands
              ? (qp->quotingStdevProtection == mSTDEV::OnFV or qp->quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? ((MG*)market)->mgStdevFVMean : ((qp->quotingStdevProtection == mSTDEV::OnTops or qp->quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? ((MG*)market)->mgStdevTopMean : ((MG*)market)->mgStdevBidMean )
              : ((MG*)market)->fairValue) - ((qp->quotingStdevProtection == mSTDEV::OnFV or qp->quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? ((MG*)market)->mgStdevFV : ((qp->quotingStdevProtection == mSTDEV::OnTops or qp->quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? ((MG*)market)->mgStdevTop : ((MG*)market)->mgStdevBid )),
            rawQuote->bid.price
          );
        }
      };
      void applyEwmaProtection(mQuote *rawQuote) {
        if (!qp->quotingEwmaProtection or !((MG*)market)->mgEwmaP) return;
        rawQuote->ask.price = fmax(((MG*)market)->mgEwmaP, rawQuote->ask.price);
        rawQuote->bid.price = fmin(((MG*)market)->mgEwmaP, rawQuote->bid.price);
      };
      void applyEwmaSMUProtection(mQuote *rawQuote) {
        if (!qp->quotingEwmaSMUProtection or !((MG*)market)->mgEwmaSMUDiff) return;
        if(((MG*)market)->mgEwmaSMUDiff > qp->quotingEwmaSMUThreshold){
          askStatus = mQuoteState::UpTrendHeld;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
          if (((CF*)config)->argDebugQuotes) FN::log("DEBUG", string("QE quote: SMU Protection uptrend ON"));
        }
        else if(((MG*)market)->mgEwmaSMUDiff < -qp->quotingEwmaSMUThreshold){
          bidStatus = mQuoteState::DownTrendHeld;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
          if (((CF*)config)->argDebugQuotes) FN::log("DEBUG", string("QE quote: SMU Protection downtrend ON"));
        }
      };
      mQuote quote(double widthPing, double buySize, double sellSize) {
        if (quotingMode.find(qp->mode) == quotingMode.end()) FN::logExit("QE", "Invalid quoting mode", EXIT_SUCCESS);
        return (*quotingMode[qp->mode])(widthPing, buySize, sellSize);
      };
      mQuote quoteAtTopOfMarket() {
        mLevel topBid = ((MG*)market)->levels.bids.begin()->size > gw->minTick
          ? ((MG*)market)->levels.bids.at(0) : ((MG*)market)->levels.bids.at(((MG*)market)->levels.bids.size()>1?1:0);
        mLevel topAsk = ((MG*)market)->levels.asks.begin()->size > gw->minTick
          ? ((MG*)market)->levels.asks.at(0) : ((MG*)market)->levels.asks.at(((MG*)market)->levels.asks.size()>1?1:0);
        return mQuote(topBid, topAsk);
      };
      function<mQuote(double, double, double)> calcJoinMarket = [&](double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = fmin(((MG*)market)->fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(((MG*)market)->fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = buySize;
        k.ask.size = sellSize;
        return k;
      };
      function<mQuote(double, double, double)> calcTopOfMarket = [&](double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = k.bid.price + gw->minTick;
        k.ask.price = k.ask.price - gw->minTick;
        k.bid.price = fmin(((MG*)market)->fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(((MG*)market)->fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = buySize;
        k.ask.size = sellSize;
        return k;
      };
      function<mQuote(double, double, double)> calcInverseJoinMarket = [&](double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        double mktWidth = abs(k.ask.price - k.bid.price);
        if (mktWidth > widthPing) {
          k.ask.price = k.ask.price + widthPing;
          k.bid.price = k.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k.ask.price = k.ask.price + widthPing / 4.0;
          k.bid.price = k.bid.price - widthPing / 4.0;
        }
        k.bid.size = buySize;
        k.ask.size = sellSize;
        return k;
      };
      function<mQuote(double, double, double)> calcInverseTopOfMarket = [&](double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        double mktWidth = abs(k.ask.price - k.bid.price);
        if (mktWidth > widthPing) {
          k.ask.price = k.ask.price + widthPing;
          k.bid.price = k.bid.price - widthPing;
        }
        k.bid.price = k.bid.price + gw->minTick;
        k.ask.price = k.ask.price - gw->minTick;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k.ask.price = k.ask.price + widthPing / 4.0;
          k.bid.price = k.bid.price - widthPing / 4.0;
        }
        k.bid.size = buySize;
        k.ask.size = sellSize;
        return k;
      };
      function<mQuote(double, double, double)> calcMidOfMarket = [&](double widthPing, double buySize, double sellSize) {
        return mQuote(
          mLevel(fmax(((MG*)market)->fairValue - widthPing, 0), buySize),
          mLevel(((MG*)market)->fairValue + widthPing, sellSize)
        );
      };
      function<mQuote(double, double, double)> calcColossusOfMarket = [&](double widthPing, double buySize, double sellSize) {
        double bidSz = 0,
               bidPx = 0,
               askSz = 0,
               askPx = 0;
        unsigned int maxLvl = 0;
        for (vector<mLevel>::iterator it = ((MG*)market)->levels.bids.begin(); it != ((MG*)market)->levels.bids.end(); ++it) {
          if (bidSz < it->size && it->price < (((MG*)market)->fairValue - widthPing / 2.0)) {
            bidSz = it->size;
            bidPx = it->price;
          }
          if (++maxLvl==13) break;
        }
        for (vector<mLevel>::iterator it = ((MG*)market)->levels.asks.begin(); it != ((MG*)market)->levels.asks.end(); ++it) {
          if (askSz < it->size && it->price > (((MG*)market)->fairValue + widthPing / 2.0)) {
            askSz = it->size;
            askPx = it->price;
          }
          if (!--maxLvl) break;
        }
        if (bidSz) bidPx = bidPx + gw->minTick;
        if (askSz) askPx = askPx - gw->minTick;
        return mQuote(
          mLevel(bidPx, buySize),
          mLevel(askPx, sellSize)
        );
      };
      function<mQuote(double, double, double)> calcDepthOfMarket = [&](double depth, double buySize, double sellSize) {
        double bidPx = ((MG*)market)->levels.bids.begin()->price;
        double bidDepth = 0;
        for (vector<mLevel>::iterator it = ((MG*)market)->levels.bids.begin(); it != ((MG*)market)->levels.bids.end(); ++it) {
          bidDepth += it->size;
          if (bidDepth >= depth) break;
          else bidPx = it->price;
        }
        double askPx = ((MG*)market)->levels.asks.begin()->price;
        double askDepth = 0;
        for (vector<mLevel>::iterator it = ((MG*)market)->levels.asks.begin(); it != ((MG*)market)->levels.asks.end(); ++it) {
          askDepth += it->size;
          if (askDepth >= depth) break;
          else askPx = it->price;
        }
        return mQuote(
          mLevel(bidPx, buySize),
          mLevel(askPx, sellSize)
        );
      };
      mQuoteState checkCrossedQuotes(mSide side, mQuote *quote) {
        bool cross = false;
        if (side == mSide::Bid) {
          if (!quote->bid.price) return bidStatus;
          if (!quote->ask.price) return mQuoteState::Live;
          cross = quote->bid.price >= quote->ask.price;
        } else if (side == mSide::Ask) {
          if (!quote->ask.price) return askStatus;
          if (!quote->bid.price) return mQuoteState::Live;
          cross = quote->ask.price <= quote->bid.price;
        }
        if (cross) {
          FN::logWar("QE", "Cross quote detected");
          return mQuoteState::Crossed;
        } else return mQuoteState::Live;
      };
      void updateQuote(mLevel q, mSide side, bool isPong) {
        multimap<double, mOrder> ordersSide = ((OG*)broker)->ordersAtSide(side);
        int k = ordersSide.size();
        if (!k) return start(side, q, isPong);
        unsigned long T = FN::T();
        for (multimap<double, mOrder>::iterator it = ordersSide.begin(); it != ordersSide.end(); ++it)
          if (it->first == q.price) { return; }
          else if (it->second.orderStatus == mORS::New) {
            if (T-10e+3>it->second.time) ((OG*)broker)->cleanOrder(it->second.orderId, it->second.exchangeId);
            if (qp->safety != mQuotingSafety::AK47 or (int)k >= qp->bullets) return;
          }
        modify(side, q, isPong);
      };
      void modify(mSide side, mLevel q, bool isPong) {
        if (qp->safety == mQuotingSafety::AK47)
          stopWorstQuote(side);
        else stopAllQuotes(side);
        start(side, q, isPong);
      };
      void start(mSide side, mLevel q, bool isPong) {
        if (qp->delayAPI) {
          static unsigned int tStarted = 0;
          static unsigned long qeNextT = 0;
          long nextDiff = (qeNextT + (6e+4 / qp->delayAPI)) - FN::T();
          if (nextDiff > 0) {
            nextStart.clear();
            nextStart[side] = q;
            nextIsPong = isPong;
            if (tStarted) {
              uv_timer_stop(((EV*)events)->tStart);
              tStarted = 0;
            }
            uv_timer_start(((EV*)events)->tStart, [](uv_timer_t *handle) {
              QE *k = (QE*)handle->data;
              if (((CF*)k->config)->argDebugEvents) FN::log("DEBUG", "EV GW tStart timer");
              k->start(k->nextStart.begin()->first, k->nextStart.begin()->second, k->nextIsPong);
            }, (nextDiff * 1e+3) + 1e+2, 0);
            tStarted = 1;
            return;
          }
          tStarted = 0;
          qeNextT = FN::T();
        }
        ((OG*)broker)->sendOrder(side, q.price, q.size, mOrderType::Limit, mTimeInForce::GTC, isPong, true);
      };
      void stopWorstsQuotes(mSide side, double price) {
        multimap<double, mOrder> ordersSide = ((OG*)broker)->ordersAtSide(side);
        for (multimap<double, mOrder>::iterator it = ordersSide.begin(); it != ordersSide.end(); ++it)
          if (side == mSide::Bid
            ? price < it->second.price
            : price > it->second.price
          ) ((OG*)broker)->cancelOrder(it->second.orderId);
      };
      void stopWorstQuote(mSide side) {
        multimap<double, mOrder> ordersSide = ((OG*)broker)->ordersAtSide(side);
        if (ordersSide.size())
          ((OG*)broker)->cancelOrder(side == mSide::Bid
            ? ordersSide.begin()->second.orderId
            : ordersSide.rbegin()->second.orderId
          );
      };
      void stopAllQuotes(mSide side) {
        multimap<double, mOrder> ordersSide = ((OG*)broker)->ordersAtSide(side);
        for (multimap<double, mOrder>::iterator it = ordersSide.begin(); it != ordersSide.end(); ++it)
          ((OG*)broker)->cancelOrder(it->second.orderId);
      };
      void stopAllQuotes() {
        stopAllQuotes(mSide::Bid);
        stopAllQuotes(mSide::Ask);
      };
  };
}

#endif
