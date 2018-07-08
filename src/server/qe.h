#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  class QE: public Klass,
            public Engine { public: QE() { engine = this; };
    private:
      map<mQuotingMode, function<mQuote(mPrice, mAmount, mAmount)>*> quotingMode;
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
        if (!semaphore.greenGateway) {
          bidStatus = mQuoteState::Disconnected;
          askStatus = mQuoteState::Disconnected;
        } else if (!market->levels.empty()
          and !wallet->position.empty()
          and !wallet->position.safety.empty()
        ) {
          if (!semaphore.greenButton) {
            bidStatus = mQuoteState::DisabledQuotes;
            askStatus = mQuoteState::DisabledQuotes;
            stopAllQuotes(mSide::Both);
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
        market->levels.calcFairValue(gw->minTick);
        market->levels.stats.ewma.calcFromHistory();
        wallet->position.send_ratelimit(market->levels);
        wallet->position.calcSafety(market->levels, broker->orders.tradesHistory);
        calcQuote();
      };
    private:
      void findMode(const string &reason) {
        if (quotingMode.find(qp.mode) == quotingMode.end())
          EXIT(screen->error("QE", "Invalid quoting mode "
            + reason + ", consider to remove the database file"));
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
        status.bidStatus = bidStatus;
        status.askStatus = askStatus;
        status.quotesInMemoryNew = quotesInMemoryNew;
        status.quotesInMemoryWorking = quotesInMemoryWorking;
        status.quotesInMemoryDone = quotesInMemoryDone;
        status.send();
      };
      bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        market->levels.filterBidOrders.clear();
        market->levels.filterAskOrders.clear();
        vector<mRandId> zombies;
        mClock now = Tstamp;
        for (map<mRandId, mOrder>::value_type &it : broker->orders.orders)
          if (it.second.orderStatus == mStatus::New) {
            if (now-10e+3>it.second.time) zombies.push_back(it.first);
            (*qNew)++;
          } else if (it.second.orderStatus == mStatus::Working) {
            (mSide::Bid == it.second.side
              ? market->levels.filterBidOrders
              : market->levels.filterAskOrders
            )[it.second.price] += it.second.quantity;
            (*qWorking)++;
          } else (*qDone)++;
        for (mRandId &it : zombies) broker->cleanOrder(it);
        return *qNew != status.quotesInMemoryNew
          or *qWorking != status.quotesInMemoryWorking
          or *qDone != status.quotesInMemoryDone;
      };
      bool diffStatus() {
        return bidStatus != status.bidStatus
          or askStatus != status.askStatus;
      };
      mQuote nextQuote() {
        mPrice widthPing = qp.widthPercentage
          ? qp.widthPingPercentage * market->levels.fairValue / 100
          : qp.widthPing;
        if(qp.protectionEwmaWidthPing and market->levels.stats.ewma.mgEwmaW)
          widthPing = fmax(widthPing, market->levels.stats.ewma.mgEwmaW);
        mQuote rawQuote = (*quotingMode[qp.mode])(
          widthPing,
          wallet->position.safety.buySize,
          wallet->position.safety.sellSize
        );
        if (rawQuote.bid.price <= 0 or rawQuote.ask.price <= 0) {
          if (rawQuote.bid.price or rawQuote.ask.price)
            screen->logWar("QP", "Negative price detected, widthPing must be smaller");
          return mQuote();
        }
        bool superTradesActive = false;
        DEBUQ("?", bidStatus, askStatus, rawQuote); applySuperTrades(&rawQuote, &superTradesActive, widthPing);
        DEBUQ("A", bidStatus, askStatus, rawQuote); applyEwmaProtection(&rawQuote);
        DEBUQ("B", bidStatus, askStatus, rawQuote); applyTotalBasePosition(&rawQuote);
        DEBUQ("C", bidStatus, askStatus, rawQuote); applyStdevProtection(&rawQuote);
        DEBUQ("D", bidStatus, askStatus, rawQuote); applyAggressivePositionRebalancing(&rawQuote);
        DEBUQ("E", bidStatus, askStatus, rawQuote); applyAK47Increment(&rawQuote);
        DEBUQ("F", bidStatus, askStatus, rawQuote); applyBestWidth(&rawQuote);
        DEBUQ("G", bidStatus, askStatus, rawQuote); applyTradesPerMinute(&rawQuote, superTradesActive);
        DEBUQ("H", bidStatus, askStatus, rawQuote); applyRoundPrice(&rawQuote);
        DEBUQ("I", bidStatus, askStatus, rawQuote); applyRoundSize(&rawQuote);
        DEBUQ("J", bidStatus, askStatus, rawQuote); applyDepleted(&rawQuote);
        DEBUQ("K", bidStatus, askStatus, rawQuote); applyWaitingPing(&rawQuote);
        DEBUQ("L", bidStatus, askStatus, rawQuote); applyEwmaTrendProtection(&rawQuote);
        DEBUQ("!", bidStatus, askStatus, rawQuote);
        DEBUG("totals " + ("toAsk: " + to_string(wallet->position._baseTotal))
                        + ", toBid: " + to_string(wallet->position._quoteTotal));
        return rawQuote;
      };
      void applyRoundPrice(mQuote *rawQuote) {
        if (!rawQuote->bid.empty()) {
          rawQuote->bid.price = floor(rawQuote->bid.price / gw->minTick) * gw->minTick;
          rawQuote->bid.price = fmax(0, rawQuote->bid.price);
        }
        if (!rawQuote->ask.empty()) {
          rawQuote->ask.price = ceil(rawQuote->ask.price / gw->minTick) * gw->minTick;
          rawQuote->ask.price = fmax(rawQuote->bid.price + gw->minTick, rawQuote->ask.price);
        }
      };
      void applyRoundSize(mQuote *rawQuote) {
        if (!rawQuote->ask.empty())
          rawQuote->ask.size = floor(fmax(
            fmin(
              rawQuote->ask.size,
              wallet->position._baseTotal
            ),
            gw->minSize
          ) / 1e-8) * 1e-8;
        if (!rawQuote->bid.empty())
          rawQuote->bid.size = floor(fmax(
            fmin(
              rawQuote->bid.size,
              wallet->position._quoteTotal
            ),
            gw->minSize
          ) / 1e-8) * 1e-8;
      };
      void applyDepleted(mQuote *rawQuote) {
        if (rawQuote->bid.size > wallet->position._quoteTotal) {
          bidStatus = mQuoteState::DepletedFunds;
          rawQuote->bid.clear();
        }
        if (rawQuote->ask.size > wallet->position._baseTotal) {
          askStatus = mQuoteState::DepletedFunds;
          rawQuote->ask.clear();
        }
      };
      void applyWaitingPing(mQuote *rawQuote) {
        if (!qp._matchPings and qp.safety != mQuotingSafety::PingPong) return;
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
      void applyAggressivePositionRebalancing(mQuote *rawQuote) {
        if (qp.safety == mQuotingSafety::Off) return;
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * market->levels.fairValue / 100
          : qp.widthPong;
        mPrice &safetyBuyPing = wallet->position.safety.buyPing;
        if (!rawQuote->ask.empty() and safetyBuyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and wallet->position.target.sideAPR == "Sell")
            or (qp.safety == mQuotingSafety::PingPong
              ? rawQuote->ask.price < safetyBuyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) rawQuote->ask.price = safetyBuyPing + widthPong;
          rawQuote->isAskPong = rawQuote->ask.price >= safetyBuyPing + widthPong;
        }
        mPrice &safetysellPing = wallet->position.safety.sellPing;
        if (!rawQuote->bid.empty() and safetysellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and wallet->position.target.sideAPR == "Buy")
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
      void applyBestWidth(mQuote *rawQuote) {
        if (!qp.bestWidth) return;
        if (!rawQuote->ask.empty())
          for (mLevel &it : market->levels.asks)
            if (it.price > rawQuote->ask.price) {
              mPrice bestAsk = it.price - gw->minTick;
              if (bestAsk > market->levels.fairValue) {
                rawQuote->ask.price = bestAsk;
                break;
              }
            }
        if (!rawQuote->bid.empty())
          for (mLevel &it : market->levels.bids)
            if (it.price < rawQuote->bid.price) {
              mPrice bestBid = it.price + gw->minTick;
              if (bestBid < market->levels.fairValue) {
                rawQuote->bid.price = bestBid;
                break;
              }
            }
      };
      void applyTotalBasePosition(mQuote *rawQuote) {
        if (wallet->position._baseTotal < wallet->position.target.targetBasePosition - wallet->position.target.positionDivergence) {
          askStatus = mQuoteState::TBPHeld;
          rawQuote->ask.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            wallet->position.target.sideAPR = "Buy";
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.aprMultiplier*rawQuote->bid.size, fmin(wallet->position.target.targetBasePosition - wallet->position._baseTotal, wallet->position._quoteAmountValue / 2));
          }
        }
        else if (wallet->position._baseTotal >= wallet->position.target.targetBasePosition + wallet->position.target.positionDivergence) {
          bidStatus = mQuoteState::TBPHeld;
          rawQuote->bid.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            wallet->position.target.sideAPR = "Sell";
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.aprMultiplier*rawQuote->ask.size, fmin(wallet->position._baseTotal - wallet->position.target.targetBasePosition, wallet->position.baseAmount / 2));
          }
        }
        else wallet->position.target.sideAPR = "Off";
      };
      void applyTradesPerMinute(mQuote *rawQuote, bool superTradesActive) {
        if (wallet->position.safety.sell >= (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          askStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->ask.clear();
        }
        if (wallet->position.safety.buy >= (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          bidStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->bid.clear();
        }
      };
      void applySuperTrades(mQuote *rawQuote, bool *superTradesActive, mPrice widthPing) {
        if (qp.superTrades != mSOP::Off
          and widthPing * qp.sopWidthMultiplier < market->levels.spread()
        ) {
          *superTradesActive = (qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize);
          if (qp.superTrades == mSOP::Size or qp.superTrades == mSOP::TradesSize) {
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.sopSizeMultiplier * rawQuote->bid.size, wallet->position._quoteAmountValue / 2);
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.sopSizeMultiplier * rawQuote->ask.size, wallet->position.baseAmount / 2);
          }
        }
      };
      void applyAK47Increment(mQuote *rawQuote) {
        if (qp.safety != mQuotingSafety::AK47) return;
        mPrice range = qp.percentageValues
          ? qp.rangePercentage * wallet->position.baseValue / 100
          : qp.range;
        if (!rawQuote->bid.empty())
          rawQuote->bid.price -= AK47inc * range;
        if (!rawQuote->ask.empty())
          rawQuote->ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyStdevProtection(mQuote *rawQuote) {
        if (qp.quotingStdevProtection == mSTDEV::Off or !market->levels.stats.stdev.fair) return;
        if (!rawQuote->ask.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or wallet->position.target.sideAPR != "Sell"))
          rawQuote->ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->levels.stats.stdev.topMean : market->levels.stats.stdev.askMean )
              : market->levels.fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->levels.stats.stdev.top : market->levels.stats.stdev.ask )),
            rawQuote->ask.price
          );
        if (!rawQuote->bid.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or wallet->position.target.sideAPR != "Buy"))
          rawQuote->bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->levels.stats.stdev.topMean : market->levels.stats.stdev.bidMean )
              : market->levels.fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->levels.stats.stdev.top : market->levels.stats.stdev.bid )),
            rawQuote->bid.price
          );
      };
      void applyEwmaProtection(mQuote *rawQuote) {
        if (!qp.protectionEwmaQuotePrice or !market->levels.stats.ewma.mgEwmaP) return;
        rawQuote->ask.price = fmax(market->levels.stats.ewma.mgEwmaP, rawQuote->ask.price);
        rawQuote->bid.price = fmin(market->levels.stats.ewma.mgEwmaP, rawQuote->bid.price);
      };
      void applyEwmaTrendProtection(mQuote *rawQuote) {
        if (!qp.quotingEwmaTrendProtection or !market->levels.stats.ewma.mgEwmaTrendDiff) return;
        if (market->levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold){
          askStatus = mQuoteState::UpTrendHeld;
          rawQuote->ask.clear();
        }
        else if (market->levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold){
          bidStatus = mQuoteState::DownTrendHeld;
          rawQuote->bid.clear();
        }
      };
      mQuote quoteAtTopOfMarket() {
        mLevel topBid = market->levels.bids.begin()->size > gw->minTick
          ? market->levels.bids.at(0) : market->levels.bids.at(market->levels.bids.size()>1?1:0);
        mLevel topAsk = market->levels.asks.begin()->size > gw->minTick
          ? market->levels.asks.at(0) : market->levels.asks.at(market->levels.asks.size()>1?1:0);
        return mQuote(topBid, topAsk);
      };
      function<mQuote(mPrice, mAmount, mAmount)> calcJoinMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = fmin(market->levels.fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(market->levels.fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(mPrice, mAmount, mAmount)> calcTopOfMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = k.bid.price + gw->minTick;
        k.ask.price = k.ask.price - gw->minTick;
        k.bid.price = fmin(market->levels.fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(market->levels.fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(mPrice, mAmount, mAmount)> calcInverseJoinMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
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
      function<mQuote(mPrice, mAmount, mAmount)> calcInverseTopOfMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
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
      function<mQuote(mPrice, mAmount, mAmount)> calcMidOfMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
        return mQuote(
          mLevel(fmax(market->levels.fairValue - widthPing, 0), bidSize),
          mLevel(market->levels.fairValue + widthPing, askSize)
        );
      };
      function<mQuote(mPrice, mAmount, mAmount)> calcColossusOfMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.size = 0;
        k.ask.size = 0;
        for (mLevel &it : market->levels.bids)
          if (k.bid.size < it.size and it.price <= k.bid.price) {
            k.bid.size = it.size;
            k.bid.price = it.price;
          }
        for (mLevel &it : market->levels.asks)
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
      function<mQuote(mAmount, mAmount, mAmount)> calcDepthOfMarket = [&](mAmount depth, mAmount bidSize, mAmount askSize) {
        mPrice bidPx = market->levels.bids.begin()->price;
        mAmount bidDepth = 0;
        for (mLevel &it : market->levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        mPrice askPx = market->levels.asks.begin()->price;
        mAmount askDepth = 0;
        for (mLevel &it : market->levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        return mQuote(
          mLevel(bidPx, bidSize),
          mLevel(askPx, askSize)
        );
      };
      mQuoteState checkCrossedQuotes(mSide side, mQuote *quote) {
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
      void updateQuote(mLevel q, mSide side, bool isPong) {
        unsigned int n = 0;
        vector<mRandId> toCancel,
                        keepWorking;
        mClock now = Tstamp;
        for (map<mRandId, mOrder>::value_type &it : broker->orders.orders)
          if (it.second.side != side or !it.second.preferPostOnly) continue;
          else if (abs(it.second.price - q.price) < gw->minTick) return;
          else if (it.second.orderStatus == mStatus::New) {
            if (qp.safety != mQuotingSafety::AK47 or ++n >= qp.bullets) return;
          } else if (qp.safety != mQuotingSafety::AK47 or (
            side == mSide::Bid ? q.price <= it.second.price : q.price >= it.second.price
          )) {
            if (args.lifetime and it.second.time + args.lifetime > now) return;
            toCancel.push_back(it.first);
          } else keepWorking.push_back(it.first);
        if (qp.safety == mQuotingSafety::AK47 and toCancel.empty() and !keepWorking.empty())
          toCancel.push_back(side == mSide::Bid ? keepWorking.front() : keepWorking.back());
        broker->sendOrder(toCancel, side, q.price, q.size, mOrderType::Limit, mTimeInForce::GTC, isPong, true);
      };
      void stopAllQuotes(mSide side) {
        vector<mRandId> toCancel;
        for (map<mRandId, mOrder>::value_type &it : broker->orders.orders)
          if (it.second.orderStatus == mStatus::Working and (side == mSide::Both
              or (side == it.second.side and it.second.preferPostOnly)
          )) toCancel.push_back(it.first);
        for (mRandId &it : toCancel) broker->cancelOrder(it);
      };
  };
}

#endif
