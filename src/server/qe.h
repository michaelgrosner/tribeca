#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  class QE: public Klass,
            public Engine { public: QE() { engine = this; };
    private:
      map<mQuotingMode, function<mQuote(const mPrice&, const mAmount&, const mAmount&)>*> quotingMode;
      mQuoteState bidStatus = mQuoteState::MissingData,
                  askStatus = mQuoteState::MissingData;
      mQuoteStatus status;
      unsigned int AK47inc = 0;
    protected:
      void load() {
        quotingMode[mQuotingMode::Top]         = &calcTopOfMarket;
        quotingMode[mQuotingMode::Mid]         = &calcMidOfMarket;
        quotingMode[mQuotingMode::Join]        = &calcJoinMarket;
        quotingMode[mQuotingMode::InverseJoin] = &calcInverseJoinMarket;
        quotingMode[mQuotingMode::InverseTop]  = &calcInverseTopOfMarket;
        quotingMode[mQuotingMode::HamelinRat]  = &calcColossusOfMarket;
        quotingMode[mQuotingMode::Depth]       = &calcDepthOfMarket;
        findMode("loaded");
      };
      void waitWebAdmin() {
        client->welcome(status);
      };
    public:
      void timer_1s() {                                             PRETTY_DEBUG
        calcQuote();
      };
      void calcQuote() {                                            PRETTY_DEBUG
        bidStatus = mQuoteState::MissingData;
        askStatus = mQuoteState::MissingData;
        if (!gw->semaphore.greenGateway) {
          bidStatus = mQuoteState::Disconnected;
          askStatus = mQuoteState::Disconnected;
        } else if (!gw->levels.empty() and !gw->wallet.target.safety.empty()) {
          if (!gw->semaphore.greenButton) {
            bidStatus = mQuoteState::DisabledQuotes;
            askStatus = mQuoteState::DisabledQuotes;
            stopQuotes(mSide::Both);
          } else {
            bidStatus = mQuoteState::UnknownHeld;
            askStatus = mQuoteState::UnknownHeld;
            sendQuoteToAPI();
          }
        }
        sendStatusToUI();
      };
      void calcQuoteAfterSavedParams() {
        findMode("saved");
        gw->levels.calcFairValue(gw->minTick);
        gw->levels.stats.ewma.calcFromHistory();
        gw->wallet.send_ratelimit(gw->levels);
        gw->wallet.target.safety.calc(gw->levels, gw->broker.tradesHistory);
        calcQuote();
      };
    private:
      void findMode(const string &reason) {
        if (quotingMode.find(qp.mode) == quotingMode.end())
          EXIT(screen->error("QE", "Invalid quoting mode "
            + reason + ", consider to remove the database file"));
        calcRawQuoteFromMarket = *quotingMode.at(qp.mode);
      };
      void sendQuoteToAPI() {
        mPrice widthPing = qp.widthPercentage
          ? qp.widthPingPercentage * gw->levels.fairValue / 100
          : qp.widthPing;
        if(qp.protectionEwmaWidthPing and gw->levels.stats.ewma.mgEwmaW)
          widthPing = fmax(widthPing, gw->levels.stats.ewma.mgEwmaW);
         mQuote quote = calcRawQuoteFromMarket(
          widthPing,
          gw->wallet.target.safety.buySize,
          gw->wallet.target.safety.sellSize
        );
        if (quote.bid.price <= 0 or quote.ask.price <= 0) {
          if (quote.bid.price or quote.ask.price)
            screen->logWar("QP", "Negative price detected, widthPing must be smaller");
          return;
        }
        applyQuotingParameters(widthPing, &quote);
        bidStatus = checkCrossedQuotes(mSide::Bid, &quote);
        askStatus = checkCrossedQuotes(mSide::Ask, &quote);
        if (askStatus == mQuoteState::Live) updateQuote(quote.ask, mSide::Ask, quote.isAskPong);
        else stopQuotes(mSide::Ask);
        if (bidStatus == mQuoteState::Live) updateQuote(quote.bid, mSide::Bid, quote.isBidPong);
        else stopQuotes(mSide::Bid);
      };
      void sendStatusToUI() {
        unsigned int quotesInMemoryNew = 0;
        unsigned int quotesInMemoryWorking = 0;
        unsigned int quotesInMemoryDone = 0;
        bool k = diffCounts(&quotesInMemoryNew, &quotesInMemoryWorking, &quotesInMemoryDone);
        if (!diffStatus() and !k) return;
        status.bidStatus = bidStatus;
        status.askStatus = askStatus;
        status.quotesInMemoryNew = quotesInMemoryNew;
        status.quotesInMemoryWorking = quotesInMemoryWorking;
        status.quotesInMemoryDone = quotesInMemoryDone;
        status.send();
      };
      bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        gw->levels.filterBidOrders.clear();
        gw->levels.filterAskOrders.clear();
        vector<mRandId> zombies;
        mClock now = Tstamp;
        for (map<mRandId, mOrder>::value_type &it : gw->broker.orders)
          if (it.second.orderStatus == mStatus::New) {
            if (now-10e+3>it.second.time) zombies.push_back(it.first);
            (*qNew)++;
          } else if (it.second.orderStatus == mStatus::Working) {
            (mSide::Bid == it.second.side
              ? gw->levels.filterBidOrders
              : gw->levels.filterAskOrders
            )[it.second.price] += it.second.quantity;
            (*qWorking)++;
          } else (*qDone)++;
        for (mRandId &it : zombies) gw->broker.erase(it);
        return *qNew != status.quotesInMemoryNew
          or *qWorking != status.quotesInMemoryWorking
          or *qDone != status.quotesInMemoryDone;
      };
      bool diffStatus() {
        return bidStatus != status.bidStatus
          or askStatus != status.askStatus;
      };
      void applyQuotingParameters(const mPrice &widthPing, mQuote *const rawQuote) {
        bool superTradesActive = false;
        DEBUQ("?", bidStatus, askStatus, rawQuote); applySuperTrades(rawQuote, &superTradesActive, widthPing);
        DEBUQ("A", bidStatus, askStatus, rawQuote); applyEwmaProtection(rawQuote);
        DEBUQ("B", bidStatus, askStatus, rawQuote); applyTotalBasePosition(rawQuote);
        DEBUQ("C", bidStatus, askStatus, rawQuote); applyStdevProtection(rawQuote);
        DEBUQ("D", bidStatus, askStatus, rawQuote); applyAggressivePositionRebalancing(rawQuote);
        DEBUQ("E", bidStatus, askStatus, rawQuote); applyAK47Increment(rawQuote);
        DEBUQ("F", bidStatus, askStatus, rawQuote); applyBestWidth(rawQuote);
        DEBUQ("G", bidStatus, askStatus, rawQuote); applyTradesPerMinute(rawQuote, superTradesActive);
        DEBUQ("H", bidStatus, askStatus, rawQuote); applyRoundPrice(rawQuote);
        DEBUQ("I", bidStatus, askStatus, rawQuote); applyRoundSize(rawQuote);
        DEBUQ("J", bidStatus, askStatus, rawQuote); applyDepleted(rawQuote);
        DEBUQ("K", bidStatus, askStatus, rawQuote); applyWaitingPing(rawQuote);
        DEBUQ("L", bidStatus, askStatus, rawQuote); applyEwmaTrendProtection(rawQuote);
        DEBUQ("!", bidStatus, askStatus, rawQuote);
        DEBUG("totals " + ("toAsk: " + to_string(gw->wallet.base.total))
                        + ", toBid: " + to_string(gw->wallet.quote.total / gw->levels.fairValue));
      };
      void applyRoundPrice(mQuote *const rawQuote) {
        if (!rawQuote->bid.empty()) {
          rawQuote->bid.price = floor(rawQuote->bid.price / gw->minTick) * gw->minTick;
          rawQuote->bid.price = fmax(0, rawQuote->bid.price);
        }
        if (!rawQuote->ask.empty()) {
          rawQuote->ask.price = ceil(rawQuote->ask.price / gw->minTick) * gw->minTick;
          rawQuote->ask.price = fmax(rawQuote->bid.price + gw->minTick, rawQuote->ask.price);
        }
      };
      void applyRoundSize(mQuote *const rawQuote) {
        if (!rawQuote->ask.empty())
          rawQuote->ask.size = floor(fmax(
            fmin(
              rawQuote->ask.size,
              gw->wallet.base.total
            ),
            gw->minSize
          ) / 1e-8) * 1e-8;
        if (!rawQuote->bid.empty())
          rawQuote->bid.size = floor(fmax(
            fmin(
              rawQuote->bid.size,
              gw->wallet.quote.total / gw->levels.fairValue
            ),
            gw->minSize
          ) / 1e-8) * 1e-8;
      };
      void applyDepleted(mQuote *const rawQuote) {
        if (rawQuote->bid.size > gw->wallet.quote.total / gw->levels.fairValue) {
          bidStatus = mQuoteState::DepletedFunds;
          rawQuote->bid.clear();
        }
        if (rawQuote->ask.size > gw->wallet.base.total) {
          askStatus = mQuoteState::DepletedFunds;
          rawQuote->ask.clear();
        }
      };
      void applyWaitingPing(mQuote *const rawQuote) {
        if (qp.safety == mQuotingSafety::Off) return;
        if (!rawQuote->isAskPong and (
          (bidStatus != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::BidSide
          or qp.pingAt == mPingAt::DepletedAskSide
        )) {
          askStatus = mQuoteState::WaitingPing;
          rawQuote->ask.clear();
        }
        if (!rawQuote->isBidPong and (
          (askStatus != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::AskSide
          or qp.pingAt == mPingAt::DepletedBidSide
        )) {
          bidStatus = mQuoteState::WaitingPing;
          rawQuote->bid.clear();
        }
      };
      void applyAggressivePositionRebalancing(mQuote *const rawQuote) {
        if (qp.safety == mQuotingSafety::Off) return;
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * gw->levels.fairValue / 100
          : qp.widthPong;
        mPrice &safetyBuyPing = gw->wallet.target.safety.buyPing;
        if (!rawQuote->ask.empty() and safetyBuyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and gw->wallet.target.sideAPR == "Sell")
            or (qp.safety == mQuotingSafety::PingPong
              ? rawQuote->ask.price < safetyBuyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) rawQuote->ask.price = safetyBuyPing + widthPong;
          rawQuote->isAskPong = rawQuote->ask.price >= safetyBuyPing + widthPong;
        }
        mPrice &safetysellPing = gw->wallet.target.safety.sellPing;
        if (!rawQuote->bid.empty() and safetysellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and gw->wallet.target.sideAPR == "Buy")
            or (qp.safety == mQuotingSafety::PingPong
              ? rawQuote->bid.price > safetysellPing - widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) rawQuote->bid.price = safetysellPing - widthPong;
          rawQuote->isBidPong = rawQuote->bid.price <= safetysellPing - widthPong;
        }
      };
      void applyBestWidth(mQuote *const rawQuote) {
        if (!qp.bestWidth) return;
        if (!rawQuote->ask.empty())
          for (mLevel &it : gw->levels.asks)
            if (it.price > rawQuote->ask.price) {
              mPrice bestAsk = it.price - gw->minTick;
              if (bestAsk > gw->levels.fairValue) {
                rawQuote->ask.price = bestAsk;
                break;
              }
            }
        if (!rawQuote->bid.empty())
          for (mLevel &it : gw->levels.bids)
            if (it.price < rawQuote->bid.price) {
              mPrice bestBid = it.price + gw->minTick;
              if (bestBid < gw->levels.fairValue) {
                rawQuote->bid.price = bestBid;
                break;
              }
            }
      };
      void applyTotalBasePosition(mQuote *const rawQuote) {
        if (gw->wallet.base.total < gw->wallet.target.targetBasePosition - gw->wallet.target.positionDivergence) {
          askStatus = mQuoteState::TBPHeld;
          rawQuote->ask.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            gw->wallet.target.sideAPR = "Buy";
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.aprMultiplier*rawQuote->bid.size, fmin(gw->wallet.target.targetBasePosition - gw->wallet.base.total, (gw->wallet.quote.amount / gw->levels.fairValue) / 2));
          }
        }
        else if (gw->wallet.base.total >= gw->wallet.target.targetBasePosition + gw->wallet.target.positionDivergence) {
          bidStatus = mQuoteState::TBPHeld;
          rawQuote->bid.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            gw->wallet.target.sideAPR = "Sell";
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.aprMultiplier*rawQuote->ask.size, fmin(gw->wallet.base.total - gw->wallet.target.targetBasePosition, gw->wallet.base.amount / 2));
          }
        }
        else gw->wallet.target.sideAPR = "Off";
      };
      void applyTradesPerMinute(mQuote *const rawQuote, const bool &superTradesActive) {
        if (gw->wallet.target.safety.sell >= (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          askStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->ask.clear();
        }
        if (gw->wallet.target.safety.buy >= (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          bidStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->bid.clear();
        }
      };
      void applySuperTrades(mQuote *const rawQuote, bool *const superTradesActive, const mPrice &widthPing) {
        if (qp.superTrades != mSOP::Off
          and widthPing * qp.sopWidthMultiplier < gw->levels.spread()
        ) {
          *superTradesActive = (qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize);
          if (qp.superTrades == mSOP::Size or qp.superTrades == mSOP::TradesSize) {
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.sopSizeMultiplier * rawQuote->bid.size, (gw->wallet.quote.amount / gw->levels.fairValue) / 2);
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.sopSizeMultiplier * rawQuote->ask.size, gw->wallet.base.amount / 2);
          }
        }
      };
      void applyAK47Increment(mQuote *const rawQuote) {
        if (qp.safety != mQuotingSafety::AK47) return;
        mPrice range = qp.percentageValues
          ? qp.rangePercentage * gw->wallet.base.value / 100
          : qp.range;
        if (!rawQuote->bid.empty())
          rawQuote->bid.price -= AK47inc * range;
        if (!rawQuote->ask.empty())
          rawQuote->ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyStdevProtection(mQuote *const rawQuote) {
        if (qp.quotingStdevProtection == mSTDEV::Off or !gw->levels.stats.stdev.fair) return;
        if (!rawQuote->ask.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or gw->wallet.target.sideAPR != "Sell"))
          rawQuote->ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? gw->levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? gw->levels.stats.stdev.topMean : gw->levels.stats.stdev.askMean )
              : gw->levels.fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? gw->levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? gw->levels.stats.stdev.top : gw->levels.stats.stdev.ask )),
            rawQuote->ask.price
          );
        if (!rawQuote->bid.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or gw->wallet.target.sideAPR != "Buy"))
          rawQuote->bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? gw->levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? gw->levels.stats.stdev.topMean : gw->levels.stats.stdev.bidMean )
              : gw->levels.fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? gw->levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? gw->levels.stats.stdev.top : gw->levels.stats.stdev.bid )),
            rawQuote->bid.price
          );
      };
      void applyEwmaProtection(mQuote *const rawQuote) {
        if (!qp.protectionEwmaQuotePrice or !gw->levels.stats.ewma.mgEwmaP) return;
        rawQuote->ask.price = fmax(gw->levels.stats.ewma.mgEwmaP, rawQuote->ask.price);
        rawQuote->bid.price = fmin(gw->levels.stats.ewma.mgEwmaP, rawQuote->bid.price);
      };
      void applyEwmaTrendProtection(mQuote *const rawQuote) {
        if (!qp.quotingEwmaTrendProtection or !gw->levels.stats.ewma.mgEwmaTrendDiff) return;
        if (gw->levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold){
          askStatus = mQuoteState::UpTrendHeld;
          rawQuote->ask.clear();
        }
        else if (gw->levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold){
          bidStatus = mQuoteState::DownTrendHeld;
          rawQuote->bid.clear();
        }
      };
      mQuote quoteAtTopOfMarket() {
        mLevel topBid = gw->levels.bids.begin()->size > gw->minTick
          ? gw->levels.bids.at(0) : gw->levels.bids.at(gw->levels.bids.size()>1?1:0);
        mLevel topAsk = gw->levels.asks.begin()->size > gw->minTick
          ? gw->levels.asks.at(0) : gw->levels.asks.at(gw->levels.asks.size()>1?1:0);
        return mQuote(topBid, topAsk);
      };
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcRawQuoteFromMarket;
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcJoinMarket = [&](const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = fmin(gw->levels.fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(gw->levels.fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcTopOfMarket = [&](const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = k.bid.price + gw->minTick;
        k.ask.price = k.ask.price - gw->minTick;
        k.bid.price = fmin(gw->levels.fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(gw->levels.fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcInverseJoinMarket = [&](const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket();
        mPrice mktWidth = abs(k.ask.price - k.bid.price);
        if (mktWidth > widthPing) {
          k.ask.price = k.ask.price + widthPing;
          k.bid.price = k.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k.ask.price = k.ask.price + widthPing / 4.0;
          k.bid.price = k.bid.price - widthPing / 4.0;
        }
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcInverseTopOfMarket = [&](const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket();
        mPrice mktWidth = abs(k.ask.price - k.bid.price);
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
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcMidOfMarket = [&](const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        return mQuote(
          mLevel(fmax(gw->levels.fairValue - widthPing, 0), bidSize),
          mLevel(gw->levels.fairValue + widthPing, askSize)
        );
      };
      function<mQuote(const mPrice&, const mAmount&, const mAmount&)> calcColossusOfMarket = [&](const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.size = 0;
        k.ask.size = 0;
        for (const mLevel &it : gw->levels.bids)
          if (k.bid.size < it.size and it.price <= k.bid.price) {
            k.bid.size = it.size;
            k.bid.price = it.price;
          }
        for (const mLevel &it : gw->levels.asks)
          if (k.ask.size < it.size and it.price >= k.ask.price) {
            k.ask.size = it.size;
            k.ask.price = it.price;
          }
        if (k.bid.size) k.bid.price += gw->minTick;
        if (k.ask.size) k.ask.price -= gw->minTick;
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(const mAmount&, const mAmount&, const mAmount&)> calcDepthOfMarket = [&](const mAmount &depth, const mAmount &bidSize, const mAmount &askSize) {
        mPrice bidPx = gw->levels.bids.begin()->price;
        mAmount bidDepth = 0;
        for (const mLevel &it : gw->levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        mPrice askPx = gw->levels.asks.begin()->price;
        mAmount askDepth = 0;
        for (const mLevel &it : gw->levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        return mQuote(
          mLevel(bidPx, bidSize),
          mLevel(askPx, askSize)
        );
      };
      mQuoteState checkCrossedQuotes(const mSide &side, mQuote *const quote) {
        bool cross = false;
        if (side == mSide::Bid) {
          if (quote->bid.empty()) return bidStatus;
          if (quote->ask.empty()) return mQuoteState::Live;
          cross = quote->bid.price >= quote->ask.price;
        } else if (side == mSide::Ask) {
          if (quote->ask.empty()) return askStatus;
          if (quote->bid.empty()) return mQuoteState::Live;
          cross = quote->ask.price <= quote->bid.price;
        }
        if (cross) {
          screen->logWar("QE", "Cross quote detected");
          return mQuoteState::Crossed;
        } else return mQuoteState::Live;
      };
      void updateQuote(const mLevel &quote, const mSide &side, const bool &isPong) {
        unsigned int n = 0;
        vector<mOrder*> toCancel,
                        keepWorking;
        mClock now = Tstamp;
        for (map<mRandId, mOrder>::value_type &it : gw->broker.orders)
          if (it.second.side != side or !it.second.preferPostOnly) continue;
          else if (abs(it.second.price - quote.price) < gw->minTick) return;
          else if (it.second.orderStatus == mStatus::New) {
            if (qp.safety != mQuotingSafety::AK47 or ++n >= qp.bullets) return;
          } else if (qp.safety != mQuotingSafety::AK47 or (
            side == mSide::Bid
              ? quote.price <= it.second.price
              : quote.price >= it.second.price
          )) {
            if (args.lifetime and it.second.time + args.lifetime > now) return;
            toCancel.push_back(&it.second);
          } else keepWorking.push_back(&it.second);
        if (qp.safety == mQuotingSafety::AK47
          and toCancel.empty()
          and !keepWorking.empty()
        ) toCancel.push_back(side == mSide::Bid ? keepWorking.front() : keepWorking.back());
        mOrder *toReplace = nullptr;
        if (!toCancel.empty()) {
          toReplace = side == mSide::Bid ? toCancel.back() : toCancel.front();
          toCancel.erase(side == mSide::Bid ? toCancel.end()-1 : toCancel.begin());
        }
        sendQuotes(toCancel, toReplace, quote, side, isPong);
        gw->monitor.tick_orders();
      };
      void sendQuotes(const vector<mOrder*> &toCancel, mOrder *const toReplace, const mLevel &quote, const mSide &side, const bool &isPong) {
        for (mOrder *const it : toCancel)
          gw->cancelOrder(it);
        if (toReplace) {
          if (gw->replace)
            return gw->replaceOrder(toReplace, quote.price, isPong);
          if (args.testChamber != 1)
            gw->cancelOrder(toReplace);
        }
        gw->placeOrder(side, quote.price, quote.size, mOrderType::Limit, mTimeInForce::GTC, isPong, true);
        if (args.testChamber == 1 and toReplace)
          gw->cancelOrder(toReplace);
      };
      void stopQuotes(const mSide &side) {
        for (mOrder *const it : gw->broker.working(side))
          gw->cancelOrder(it);
      };
  };
}

#endif
