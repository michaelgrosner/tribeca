#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  unsigned int qeT = 0;
  unsigned long qeNextT = 0,
                qeThread = 0;
  mQuote qeQuote;
  mQuoteStatus qeStatus;
  mQuoteState qeBidStatus = mQuoteState::MissingData,
              qeAskStatus = mQuoteState::MissingData;
  typedef mQuote (*qeMode)(double widthPing, double buySize, double sellSize);
  map<mQuotingMode, qeMode> qeQuotingMode;
  map<mSide, mLevel> qeNextQuote;
  bool qeNextIsPong;
  mConnectivity gwQuotingState_ = mConnectivity::Disconnected,
                gwConnectExchange_ = mConnectivity::Disconnected;
  int tStart_ = 0;
  class QE: public Klass {
    protected:
      void load() {
        qeQuotingMode[mQuotingMode::Top] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::Mid] = &calcMidOfMarket;
        qeQuotingMode[mQuotingMode::Join] = &calcJoinMarket;
        qeQuotingMode[mQuotingMode::InverseJoin] = &calcInverseJoinMarket;
        qeQuotingMode[mQuotingMode::InverseTop] = &calcInverseTopOfMarket;
        qeQuotingMode[mQuotingMode::HamelinRat] = &calcColossusOfMarket;
        qeQuotingMode[mQuotingMode::Depth] = &calcDepthOfMarket;
      };
      void waitTime() {
        uv_timer_init(hub.getLoop(), &tStart);
        uv_timer_init(hub.getLoop(), &tCalcs);
        uv_timer_start(&tCalcs, [](uv_timer_t *handle) {
          if (argDebugEvents) FN::log("DEBUG", "EV GW tCalcs timer");
          if (mgFairValue) {
            MG::calcStats();
            PG::calcSafety();
            calcQuote();
          } else FN::logWar("QE", "Unable to calculate quote, missing fair value");
        }, 1e+3, 1e+3);
      };
      void waitData() {
        ev_gwConnectButton = [](mConnectivity k) {
          if (argDebugEvents) FN::log("DEBUG", "EV QE v_gwConnectButton");
          gwQuotingState_ = k;
        };
        ev_gwConnectExchange = [](mConnectivity k) {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_gwConnectExchange");
          gwConnectExchange_ = k;
          send();
        };
        ev_uiQuotingParameters = []() {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_uiQuotingParameters");
          MG::calcFairValue();
          PG::calcTargetBasePos();
          PG::calcSafety();
          calcQuote();
        };
        ev_ogTrade = [](mTrade k) {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_ogTrade");
          PG::addTrade(k);
          PG::calcSafety();
          calcQuote();
        };
        ev_mgEwmaQuoteProtection = []() {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_mgEwmaQuoteProtection");
          calcQuote();
        };
        ev_mgEwmaSMUProtection = []() {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_mgEwmaSMUProtection");
          calcQuote();
        };
        ev_mgLevels = []() {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_mgLevels");
          calcQuote();
        };
        ev_pgTargetBasePosition = []() {
          if (argDebugEvents) FN::log("DEBUG", "EV QE ev_pgTargetBasePosition");
          calcQuote();
        };
      };
      void waitUser() {
        UI::uiSnap(uiTXT::QuoteStatus, &onSnap);
      };
    private:
      static json onSnap() {
        return { qeStatus };
      };
      static void calcQuote() {
        qeBidStatus = mQuoteState::MissingData;
        qeAskStatus = mQuoteState::MissingData;
        if (!mgFairValue or MG::empty()) {
          qeQuote = mQuote();
          return;
        }
        mQuote quote = nextQuote();
        if (!quote.bid.price and !quote.ask.price) {
          qeQuote = mQuote();
          return;
        }
        quote.bid = quotesAreSame(quote.bid.price, quote.bid.size, mSide::Bid);
        quote.ask = quotesAreSame(quote.ask.price, quote.ask.size, mSide::Ask);
        if ((!qeQuote.bid.price and !qeQuote.ask.price and !quote.bid.price and !quote.ask.price) or (
          qeQuote.bid.price and qeQuote.ask.price and quote.bid.price and quote.ask.price
          and abs(qeQuote.bid.price - quote.bid.price) < gw->minTick
          and abs(qeQuote.ask.price - quote.ask.price) < gw->minTick
          and abs(qeQuote.bid.size - quote.bid.size) < gw->minSize
          and abs(qeQuote.ask.size - quote.ask.size) < gw->minSize
        )) return;
        qeQuote = quote;
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote! ") + ((json)qeQuote).dump());
        send();
      };
      static void send() {
        sendQuoteToAPI();
        sendQuoteToUI();
      };
      static void sendQuoteToAPI() {
        if (gwConnectExchange_ == mConnectivity::Disconnected or (!qeQuote.bid.price and !qeQuote.ask.price)) {
          qeAskStatus = mQuoteState::Disconnected;
          qeBidStatus = mQuoteState::Disconnected;
        } else {
          if (gwQuotingState_ == mConnectivity::Disconnected) {
            qeAskStatus = mQuoteState::DisabledQuotes;
            qeBidStatus = mQuoteState::DisabledQuotes;
          } else {
            qeBidStatus = checkCrossedQuotes(mSide::Bid);
            qeAskStatus = checkCrossedQuotes(mSide::Ask);
          }
          if (qeAskStatus == mQuoteState::Live)
            updateQuote(qeQuote.ask, mSide::Ask, qeQuote.isAskPong);
          else stopAllQuotes(mSide::Ask);
          if (qeBidStatus == mQuoteState::Live)
            updateQuote(qeQuote.bid, mSide::Bid, qeQuote.isBidPong);
          else stopAllQuotes(mSide::Bid);
        }
      };
      static void sendQuoteToUI() {
        unsigned int quotesInMemoryNew = 0;
        unsigned int quotesInMemoryWorking = 0;
        unsigned int quotesInMemoryDone = 0;
        bool k = diffCounts(&quotesInMemoryNew, &quotesInMemoryWorking, &quotesInMemoryDone);
        if (!diffStatus() and !k) return;
        qeStatus = mQuoteStatus(qeBidStatus, qeAskStatus, quotesInMemoryNew, quotesInMemoryWorking, quotesInMemoryDone);
        UI::uiSend(uiTXT::QuoteStatus, qeStatus, true);
      };
      static bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        ogMutex.lock();
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          mORS k = (mORS)it->second.orderStatus;
          if (k == mORS::New) ++(*qNew);
          else if (k == mORS::Working) ++(*qWorking);
          else ++(*qDone);
        }
        ogMutex.unlock();
        return diffCounts(*qNew, *qWorking, *qDone);
      };
      static bool diffCounts(unsigned int qNew, unsigned int qWorking, unsigned int qDone) {
        return qNew != qeStatus.quotesInMemoryNew
          or qWorking != qeStatus.quotesInMemoryWorking
          or qDone != qeStatus.quotesInMemoryDone;
      };
      static bool diffStatus() {
        return qeBidStatus != qeStatus.bidStatus
          or qeAskStatus != qeStatus.askStatus;
      };
      static mLevel quotesAreSame(double price, double size, mSide side) {
        mLevel newQuote(price, size);
        if (!price or !size) return mLevel();
        if ((mSide::Bid == side and !qeQuote.bid.price)
          or (mSide::Ask == side and !qeQuote.ask.price)) return newQuote;
        mLevel prevQuote = mSide::Bid == side ? qeQuote.bid : qeQuote.ask;
        if (!prevQuote.price) return newQuote;
        if (abs(size - prevQuote.size) > 5e-3) return newQuote;
        if (abs(price - prevQuote.price) < gw->minTick) return prevQuote;
        bool quoteWasWidened = true;
        if ((mSide::Bid == side and prevQuote.price < price)
          or (mSide::Ask == side and prevQuote.price > price)
        ) quoteWasWidened = false;
        unsigned int now = FN::T();
        if (!quoteWasWidened and now - qeT < 300) return prevQuote;
        qeT = now;
        return newQuote;
      };
      static mQuote nextQuote() {
        if (MG::empty() or PG::empty()) return mQuote();
        pgMutex.lock();
        double value           = pgPos.value,
               baseAmount      = pgPos.baseAmount,
               baseHeldAmount  = pgPos.baseHeldAmount,
               quoteAmount     = pgPos.quoteAmount,
               quoteHeldAmount = pgPos.quoteHeldAmount,
               safetyBuyPing   = pgSafety.buyPing,
               safetySellPong  = pgSafety.sellPong,
               safetyBuy       = pgSafety.buy,
               safetySell      = pgSafety.sell;
        pgMutex.unlock();
        if (safetyBuyPing == -1) return mQuote();
        double widthPing = qp.widthPercentage
          ? qp.widthPingPercentage * mgFairValue / 100
          : qp.widthPing;
        double widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * mgFairValue / 100
          : qp.widthPong;
        double totalBasePosition = baseAmount + baseHeldAmount;
        double totalQuotePosition = (quoteAmount + quoteHeldAmount) / mgFairValue;
        double buySize = qp.percentageValues
          ? qp.buySizePercentage * value / 100
          : qp.buySize;
        double sellSize = qp.percentageValues
          ? qp.sellSizePercentage * value / 100
          : qp.sellSize;
        double pDiv = qp.percentageValues
          ? qp.positionDivergencePercentage * value / 100
          : qp.positionDivergence;
        if (buySize and qp.aggressivePositionRebalancing != mAPR::Off and qp.buySizeMax)
          buySize = fmax(buySize, pgTargetBasePos - totalBasePosition);
        if (sellSize and qp.aggressivePositionRebalancing != mAPR::Off and qp.sellSizeMax)
          sellSize = fmax(sellSize, totalBasePosition - pgTargetBasePos);
        mQuote rawQuote = quote(widthPing, buySize, sellSize);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote? ") +((json)rawQuote).dump());
        if (!rawQuote.bid.price and !rawQuote.ask.price) return mQuote();
        if (rawQuote.bid.price < 0 or rawQuote.ask.price < 0) {
          FN::logWar("QP", "Negative price detected! (widthPing or/and widthPong must be smaller)");
          return mQuote();
        }
        const double rawBidSz = rawQuote.bid.size;
        const double rawAskSz = rawQuote.ask.size;
        qeBidStatus = mQuoteState::UnknownHeld;
        qeAskStatus = mQuoteState::UnknownHeld;
        vector<int> superTradesMultipliers = {1, 1};
        applySuperTrades(&rawQuote, &superTradesMultipliers, widthPing, buySize, sellSize, quoteAmount, baseAmount);
        applyEwmaProtection(&rawQuote);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyEwmaSMUProtection(&rawQuote);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyTotalBasePosition(&rawQuote, totalBasePosition, pDiv, buySize, sellSize, quoteAmount, baseAmount);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyStdevProtection(&rawQuote);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyAggressivePositionRebalancing(&rawQuote, widthPong, safetyBuyPing, safetySellPong);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyBestWidth(&rawQuote);
        applyTradesPerMinute(&rawQuote, superTradesMultipliers, safetyBuy, safetySell);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyDepleted(&rawQuote, totalQuotePosition, totalBasePosition);
        applyWaitingPing(&rawQuote, buySize, sellSize, totalQuotePosition, totalBasePosition, safetyBuyPing, safetySellPong);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyRoundSide(&rawQuote);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        applyRoundDown(&rawQuote, rawBidSz, rawAskSz, widthPong, safetyBuyPing, safetySellPong, totalQuotePosition, totalBasePosition);
        if (argDebugQuotes) FN::log("DEBUG", string("QE quote¿ ") + ((json)rawQuote).dump());
        if (argDebugQuotes) FN::log("DEBUG", string("QE totals ") + "toAsk:" + to_string(totalBasePosition) + " toBid:" + to_string(totalQuotePosition) + " min:" + to_string(gw->minSize));
        return rawQuote;
      };
      static void applyRoundSide(mQuote *rawQuote) {
        if (rawQuote->bid.price) {
          rawQuote->bid.price = FN::roundSide(rawQuote->bid.price, gw->minTick, mSide::Bid);
          rawQuote->bid.price = fmax(0, rawQuote->bid.price);
        }
        if (rawQuote->ask.price) {
          rawQuote->ask.price = FN::roundSide(rawQuote->ask.price, gw->minTick, mSide::Ask);
          rawQuote->ask.price = fmax(rawQuote->bid.price + gw->minTick, rawQuote->ask.price);
        }
      };
      static void applyRoundDown(mQuote *rawQuote, const double rawBidSz, const double rawAskSz, double widthPong, double safetyBuyPing, double safetySellPong, double totalQuotePosition, double totalBasePosition) {
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
      static void applyWaitingPing(mQuote *rawQuote, double buySize, double sellSize, double totalQuotePosition, double totalBasePosition, double safetyBuyPing, double safetySellPong) {
        if ((qp.safety == mQuotingSafety::PingPong or QP::matchPings())
          and !safetyBuyPing and (qp.pingAt == mPingAt::StopPings or qp.pingAt == mPingAt::BidSide or qp.pingAt == mPingAt::DepletedAskSide
            or (totalQuotePosition>buySize and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
        )) {
          qeAskStatus = !safetyBuyPing
            ? mQuoteState::WaitingPing
            : mQuoteState::DepletedFunds;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
        }
        if ((qp.safety == mQuotingSafety::PingPong or QP::matchPings())
          and !safetySellPong and (qp.pingAt == mPingAt::StopPings or qp.pingAt == mPingAt::AskSide or qp.pingAt == mPingAt::DepletedBidSide
            or (totalBasePosition>sellSize and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
        )) {
          qeBidStatus = !safetySellPong
            ? mQuoteState::WaitingPing
            : mQuoteState::DepletedFunds;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
        }
      };
      static void applyDepleted(mQuote *rawQuote, double totalQuotePosition, double totalBasePosition) {
        if (rawQuote->bid.size and rawQuote->bid.size > totalQuotePosition) {
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
          qeBidStatus = mQuoteState::DepletedFunds;
          FN::logWar("QE", string("BUY quote ignored: depleted ") + gw->quote + " balance");
        }
        if (rawQuote->ask.size and rawQuote->ask.size > totalBasePosition) {
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
          qeAskStatus = mQuoteState::DepletedFunds;
          FN::logWar("QE", string("SELL quote ignored: depleted ") + gw->base + " balance");
        }
      };
      static void applyBestWidth(mQuote *rawQuote) {
        if (!qp.bestWidth) return;
        if (rawQuote->ask.price)
          for (vector<mLevel>::iterator it = mgLevelsFilter.asks.begin(); it != mgLevelsFilter.asks.end(); ++it)
            if (it->price > rawQuote->ask.price) {
              double bestAsk = it->price - gw->minTick;
              if (bestAsk > mgFairValue) {
                rawQuote->ask.price = bestAsk;
                break;
              }
            }
        if (rawQuote->bid.price)
          for (vector<mLevel>::iterator it = mgLevelsFilter.bids.begin(); it != mgLevelsFilter.bids.end(); ++it)
            if (it->price < rawQuote->bid.price) {
              double bestBid = it->price + gw->minTick;
              if (bestBid < mgFairValue) {
                rawQuote->bid.price = bestBid;
                break;
              }
            }
      };
      static void applyTotalBasePosition(mQuote *rawQuote, double totalBasePosition, double pDiv, double buySize, double sellSize, double quoteAmount, double baseAmount) {
        if (totalBasePosition < pgTargetBasePos - pDiv) {
          if(qeAskStatus != mQuoteState::UpTrendHeld or qeAskStatus != mQuoteState::DownTrendHeld)
            qeAskStatus = mQuoteState::TBPHeld;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            pgSideAPR = "Buy";
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.aprMultiplier*buySize, fmin(pgTargetBasePos - totalBasePosition, (quoteAmount / mgFairValue) / 2));
          }
        }
        else if (totalBasePosition >= pgTargetBasePos + pDiv) {
          if(qeBidStatus != mQuoteState::UpTrendHeld or qeBidStatus != mQuoteState::DownTrendHeld)
            qeBidStatus = mQuoteState::TBPHeld;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            pgSideAPR = "Sell";
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.aprMultiplier*sellSize, fmin(totalBasePosition - pgTargetBasePos, baseAmount / 2));
          }
        }
        else pgSideAPR = "Off";
      };
      static void applyTradesPerMinute(mQuote *rawQuote, vector<int> superTradesMultipliers, double safetyBuy, double safetySell) {
        if (safetySell > (qp.tradesPerMinute * superTradesMultipliers[0])) {
          qeAskStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
        }
        if (safetyBuy > (qp.tradesPerMinute * superTradesMultipliers[0])) {
          qeBidStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
        }
      };
      static void applyAggressivePositionRebalancing(mQuote *rawQuote, double widthPong, double safetyBuyPing, double safetySellPong) {
        if (qp.safety == mQuotingSafety::Boomerang or qp.safety == mQuotingSafety::AK47 or QP::matchPings()) {
          if (rawQuote->ask.size and safetyBuyPing and (
            (qp.aggressivePositionRebalancing == mAPR::SizeWidth and pgSideAPR == "Sell")
            or qp.pongAt == mPongAt::ShortPingAggressive
            or qp.pongAt == mPongAt::LongPingAggressive
            or rawQuote->ask.price < safetyBuyPing + widthPong
          )) rawQuote->ask.price = safetyBuyPing + widthPong;
          if (rawQuote->bid.size and safetySellPong and (
            (qp.aggressivePositionRebalancing == mAPR::SizeWidth and pgSideAPR == "Buy")
            or qp.pongAt == mPongAt::ShortPingAggressive
            or qp.pongAt == mPongAt::LongPingAggressive
            or rawQuote->bid.price > safetySellPong - widthPong
          )) rawQuote->bid.price = safetySellPong - widthPong;
        }
      };
      static void applySuperTrades(mQuote *rawQuote, vector<int> *superTradesMultipliers, double widthPing, double buySize, double sellSize, double quoteAmount, double baseAmount) {
        if (qp.superTrades != mSOP::Off
          and widthPing * qp.sopWidthMultiplier < mgLevelsFilter.asks.begin()->price - mgLevelsFilter.bids.begin()->price
        ) {
          (*superTradesMultipliers)[0] = qp.superTrades == mSOP::x2trades or qp.superTrades == mSOP::x2tradesSize
            ? 2 : (qp.superTrades == mSOP::x3trades or qp.superTrades == mSOP::x3tradesSize
              ? 3 : 1);
          (*superTradesMultipliers)[1] = qp.superTrades == mSOP::x2Size or qp.superTrades == mSOP::x2tradesSize
            ? 2 : (qp.superTrades == mSOP::x3Size or qp.superTrades == mSOP::x3tradesSize
              ? 3 : 1);
        }
        if ((*superTradesMultipliers)[1] > 1) {
          if (!qp.buySizeMax) rawQuote->bid.size = fmin((*superTradesMultipliers)[1] * buySize, (quoteAmount / mgFairValue) / 2);
          if (!qp.sellSizeMax) rawQuote->ask.size = fmin((*superTradesMultipliers)[1] * sellSize, baseAmount / 2);
        }
      };
      static void applyStdevProtection(mQuote *rawQuote) {
        if (qp.quotingStdevProtection == mSTDEV::Off or !mgStdevFV) return;
        if (rawQuote->ask.price and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or pgSideAPR != "Sell"))
          rawQuote->ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? mgStdevFVMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? mgStdevTopMean : mgStdevAskMean )
              : mgFairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? mgStdevFV : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? mgStdevTop : mgStdevAsk )),
            rawQuote->ask.price
          );
        if (rawQuote->bid.price and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or pgSideAPR != "Buy")) {
          rawQuote->bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? mgStdevFVMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? mgStdevTopMean : mgStdevBidMean )
              : mgFairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? mgStdevFV : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? mgStdevTop : mgStdevBid )),
            rawQuote->bid.price
          );
        }
      };
      static void applyEwmaProtection(mQuote *rawQuote) {
        if (!qp.quotingEwmaProtection or !mgEwmaP) return;
        rawQuote->ask.price = fmax(mgEwmaP, rawQuote->ask.price);
        rawQuote->bid.price = fmin(mgEwmaP, rawQuote->bid.price);
      };
      static void applyEwmaSMUProtection(mQuote *rawQuote) {
        if (!qp.quotingEwmaSMUProtection or !mgEwmaSMUDiff) return;
        if(mgEwmaSMUDiff > qp.quotingEwmaSMUThreshold){
          qeAskStatus = mQuoteState::UpTrendHeld;
          rawQuote->ask.price = 0;
          rawQuote->ask.size = 0;
          if (argDebugQuotes) FN::log("DEBUG", string("QE quote: SMU Protection uptrend ON"));
        }
        else if(mgEwmaSMUDiff < -qp.quotingEwmaSMUThreshold){
          qeBidStatus = mQuoteState::DownTrendHeld;
          rawQuote->bid.price = 0;
          rawQuote->bid.size = 0;
          if (argDebugQuotes) FN::log("DEBUG", string("QE quote: SMU Protection downtrend ON"));
        }
      };
      static mQuote quote(double widthPing, double buySize, double sellSize) {
        if (qeQuotingMode.find(qp.mode) == qeQuotingMode.end()) FN::logExit("QE", "Invalid quoting mode", EXIT_SUCCESS);
        return (*qeQuotingMode[qp.mode])(widthPing, buySize, sellSize);
      };
      static mQuote quoteAtTopOfMarket() {
        mLevel topBid = mgLevelsFilter.bids.begin()->size > gw->minTick
          ? mgLevelsFilter.bids.at(0) : mgLevelsFilter.bids.at(mgLevelsFilter.bids.size()>1?1:0);
        mLevel topAsk = mgLevelsFilter.asks.begin()->size > gw->minTick
          ? mgLevelsFilter.asks.at(0) : mgLevelsFilter.asks.at(mgLevelsFilter.asks.size()>1?1:0);
        return mQuote(topBid, topAsk);
      };
      static mQuote calcJoinMarket(double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = fmin(mgFairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(mgFairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = buySize;
        k.ask.size = sellSize;
        return k;
      };
      static mQuote calcTopOfMarket(double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = k.bid.price + gw->minTick;
        k.ask.price = k.ask.price - gw->minTick;
        k.bid.price = fmin(mgFairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(mgFairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = buySize;
        k.ask.size = sellSize;
        return k;
      };
      static mQuote calcInverseJoinMarket(double widthPing, double buySize, double sellSize) {
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
      static mQuote calcInverseTopOfMarket(double widthPing, double buySize, double sellSize) {
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
      static mQuote calcMidOfMarket(double widthPing, double buySize, double sellSize) {
        return mQuote(
          mLevel(fmax(mgFairValue - widthPing, 0), buySize),
          mLevel(mgFairValue + widthPing, sellSize)
        );
      };
      static mQuote calcColossusOfMarket(double widthPing, double buySize, double sellSize) {
        double bidSz = 0,
               bidPx = 0,
               askSz = 0,
               askPx = 0;
        unsigned int maxLvl = 0;
        for (vector<mLevel>::iterator it = mgLevelsFilter.bids.begin(); it != mgLevelsFilter.bids.end(); ++it) {
          if (bidSz < it->size) {
            bidSz = it->size;
            bidPx = it->price;
          }
          if (++maxLvl==13) break;
        }
        for (vector<mLevel>::iterator it = mgLevelsFilter.asks.begin(); it != mgLevelsFilter.asks.end(); ++it) {
          if (askSz < it->size) {
            askSz = it->size;
            askPx = it->price;
          }
          if (!--maxLvl) break;
        }
        return mQuote(
          mLevel(bidPx + gw->minTick, buySize),
          mLevel(askPx - gw->minTick, sellSize)
        );
      };
      static mQuote calcDepthOfMarket(double depth, double buySize, double sellSize) {
        double bidPx = mgLevelsFilter.bids.begin()->price;
        double bidDepth = 0;
        for (vector<mLevel>::iterator it = mgLevelsFilter.bids.begin(); it != mgLevelsFilter.bids.end(); ++it) {
          bidDepth += it->size;
          if (bidDepth >= depth) break;
          else bidPx = it->price;
        }
        double askPx = mgLevelsFilter.asks.begin()->price;
        double askDepth = 0;
        for (vector<mLevel>::iterator it = mgLevelsFilter.asks.begin(); it != mgLevelsFilter.asks.end(); ++it) {
          askDepth += it->size;
          if (askDepth >= depth) break;
          else askPx = it->price;
        }
        return mQuote(
          mLevel(bidPx, buySize),
          mLevel(askPx, sellSize)
        );
      };
      static mQuoteState checkCrossedQuotes(mSide side) {
        if (side == mSide::Bid and !qeQuote.bid.price) return qeBidStatus;
        else if (side == mSide::Ask and !qeQuote.ask.price) return qeAskStatus;
        if ((side == mSide::Bid and !qeQuote.ask.price)
          or (side == mSide::Ask and !qeQuote.bid.price)) return mQuoteState::Live;
        double price = side == mSide::Bid ? qeQuote.bid.price : qeQuote.ask.price;
        double ecirp = side == mSide::Bid ? qeQuote.ask.price : qeQuote.bid.price;
        if (side == mSide::Bid ? price >= ecirp : price <= ecirp) {
          FN::log("QE", string("Crossing quote detected! new ") + (side == mSide::Bid ? "Bid" : "Ask") + "Side quote at " + to_string(price) + " crossed with " + (side == mSide::Bid ? "Ask" : "Bid") + "Side quote at " + to_string(ecirp));
          return mQuoteState::Crossed;
        }
        return mQuoteState::Live;
     };
      static void updateQuote(mLevel q, mSide side, bool isPong) {
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        bool eq = false;
        for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (it->first == q.price) { eq = true; break; }
        if (qp.safety != mQuotingSafety::AK47) {
          if (orderSide.size()) {
            if (!eq) modify(side, q, isPong);
          } else start(side, q, isPong);
          return;
        }
        if (!eq and orderSide.size() >= (size_t)qp.bullets)
          modify(side, q, isPong);
        else start(side, q, isPong);
      };
      static multimap<double, mOrder> orderCacheSide(mSide side) {
        multimap<double, mOrder> orderSide;
        ogMutex.lock();
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if ((mSide)it->second.side == side)
            orderSide.insert(pair<double, mOrder>(it->second.price, it->second));
        ogMutex.unlock();
        return orderSide;
      };
      static void modify(mSide side, mLevel q, bool isPong) {
        if (qp.safety == mQuotingSafety::Boomerang or qp.safety == mQuotingSafety::AK47)
          stopWorstQuote(side);
        else stopAllQuotes(side);
        start(side, q, isPong);
      };
      static void start(mSide side, mLevel q, bool isPong) {
        if (qp.delayAPI) {
          long nextDiff = (qeNextT + (6e+4 / qp.delayAPI)) - FN::T();
          if (nextDiff > 0) {
            qeNextQuote.clear();
            qeNextQuote[side] = q;
            qeNextIsPong = isPong;
            if (tStart_) {
              uv_timer_stop(&tStart);
              tStart_ = 0;
            }
            uv_timer_start(&tStart, [](uv_timer_t *handle) {
              if (argDebugEvents) FN::log("DEBUG", "EV GW tStart timer");
              start(qeNextQuote.begin()->first, qeNextQuote.begin()->second, qeNextIsPong);
            }, (nextDiff * 1e+3) + 1e+2, 0);
            tStart_ = 1;
            return;
          }
          tStart_ = 0;
          qeNextT = FN::T();
        }
        double price = q.price;
        double range = qp.percentageValues
          ? qp.rangePercentage * pgPos.value / 100
          : qp.range;
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        bool eq = false;
        for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (price == it->first
            or ((qp.safety == mQuotingSafety::Boomerang or qp.safety == mQuotingSafety::AK47)
              and (price + (range - 1e-2)) >= it->first
              and (price - (range - 1e-2)) <= it->first)
          ) { eq = true; break; }
        if (eq) {
          if ((qp.safety == mQuotingSafety::Boomerang or qp.safety == mQuotingSafety::AK47) and orderSide.size()<(size_t)qp.bullets) {
            double incPrice = (range * (side == mSide::Bid ? -1 : 1 ));
            double oldPrice = 0;
            unsigned int len = 0;
            if (side == mSide::Bid)
              calcAK47Increment(orderSide.begin(), orderSide.end(), &price, side, &oldPrice, incPrice, &len);
            else calcAK47Increment(orderSide.rbegin(), orderSide.rend(), &price, side, &oldPrice, incPrice, &len);
            if (len==orderSide.size()) price = incPrice + (side == mSide::Bid
              ? orderSide.rbegin()->second.price
              : orderSide.begin()->second.price);
            eq = false;
            for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
              if (price == it->first
                or ((price + (range - 1e-2)) >= it->first
                  and (price - (range - 1e-2)) <= it->first)
              ) { eq = true; break; }
            if (eq) return;
            stopWorstsQuotes(side, q.price);
            price = FN::roundNearest(price, gw->minTick);
          } else return;
        }
        OG::sendOrder(side, price, q.size, mOrderType::Limit, mTimeInForce::GTC, isPong, true);
      };
      template <typename Iterator> static void calcAK47Increment(Iterator iter, Iterator last, double* price, mSide side, double* oldPrice, double incPrice, unsigned int* len) {
        for (;iter != last; ++iter) {
          if (*oldPrice>0 and (side == mSide::Bid?iter->first<*price:iter->first>*price)) {
            *price = *oldPrice + incPrice;
            if (abs(iter->first - *oldPrice)>incPrice) break;
          }
          *oldPrice = iter->first;
          ++(*len);
        }
      };
      static void stopWorstsQuotes(mSide side, double price) {
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (side == mSide::Bid
            ? price < it->second.price
            : price > it->second.price
          ) OG::cancelOrder(it->second.orderId);
      };
      static void stopWorstQuote(mSide side) {
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        if (orderSide.size())
          OG::cancelOrder(side == mSide::Bid
            ? orderSide.begin()->second.orderId
            : orderSide.rbegin()->second.orderId
          );
      };
      static void stopAllQuotes(mSide side) {
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          OG::cancelOrder(it->second.orderId);
      };
  };
}

#endif
