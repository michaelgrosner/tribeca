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
      void waitUser() {
        client->WELCOME(mMatter::QuoteStatus, hello);
      };
      void run() {
        if (args.debugQuotes) return;
        debuq = [&](const string &k, const mQuote &rawQuote) {};
        debug = [&](const string &k) {};
      };
    public:
      void calcQuote() {                                            PRETTY_DEBUG
        bidStatus = mQuoteState::MissingData;
        askStatus = mQuoteState::MissingData;
        if (!greenGateway) {
          bidStatus = mQuoteState::Disconnected;
          askStatus = mQuoteState::Disconnected;
        } else if (market->fairValue
          and !market->levels.empty()
          and !wallet->position.empty()
          and !wallet->safety.empty()
        ) {
          if (!greenButton) {
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
        market->calcFairValue();
        wallet->calcTargetBasePos();
        wallet->calcSafety();
        market->calcEwmaHistory();
        calcQuote();
      };
      void timer_1s() {                                             PRETTY_DEBUG
        if (market->fairValue) {
          market->calcStats();
          wallet->calcSafety();
          calcQuote();
        } else screen->logWar("QE", "Unable to calculate quote, missing market data");
      };
    private:
      void hello(json *const welcome) {
        *welcome = { status };
      };
      void findMode(const string &reason) {
        if (quotingMode.find(qp.mode) == quotingMode.end())
          exit(screen->error("QE", string("Invalid quoting mode ")
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
        status = mQuoteStatus(bidStatus, askStatus, quotesInMemoryNew, quotesInMemoryWorking, quotesInMemoryDone);
        client->send(mMatter::QuoteStatus, status);
      };
      bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        market->filterBidOrders.clear();
        market->filterAskOrders.clear();
        vector<mRandId> zombies;
        mClock now = _Tstamp_;
        for (map<mRandId, mOrder>::value_type &it : broker->orders)
          if (it.second.orderStatus == mStatus::New) {
            if (now-10e+3>it.second.time) zombies.push_back(it.first);
            (*qNew)++;
          } else if (it.second.orderStatus == mStatus::Working) {
            (mSide::Bid == it.second.side
              ? market->filterBidOrders
              : market->filterAskOrders
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
          ? qp.widthPingPercentage * market->fairValue / 100
          : qp.widthPing;
        if(qp.protectionEwmaWidthPing and market->mgEwmaW)
          widthPing = fmax(widthPing, market->mgEwmaW);
        mQuote rawQuote = (*quotingMode[qp.mode])(
          widthPing,
          wallet->safety.buySize,
          wallet->safety.sellSize
        );
        if (rawQuote.bid.price <= 0 or rawQuote.ask.price <= 0) {
          if (rawQuote.bid.price or rawQuote.ask.price)
            screen->logWar("QP", "Negative price detected, widthPing must be smaller");
          return mQuote();
        }
        bool superTradesActive = false;
        debuq("?", rawQuote); applySuperTrades(&rawQuote, &superTradesActive, widthPing);
        debuq("A", rawQuote); applyEwmaProtection(&rawQuote);
        debuq("B", rawQuote); applyTotalBasePosition(&rawQuote);
        debuq("C", rawQuote); applyStdevProtection(&rawQuote);
        debuq("D", rawQuote); applyAggressivePositionRebalancing(&rawQuote);
        debuq("E", rawQuote); applyAK47Increment(&rawQuote);
        debuq("F", rawQuote); applyBestWidth(&rawQuote);
        debuq("G", rawQuote); applyTradesPerMinute(&rawQuote, superTradesActive);
        debuq("H", rawQuote); applyRoundPrice(&rawQuote);
        debuq("I", rawQuote); applyRoundSize(&rawQuote);
        debuq("J", rawQuote); applyDepleted(&rawQuote);
        debuq("K", rawQuote); applyWaitingPing(&rawQuote);
        debuq("L", rawQuote); applyEwmaTrendProtection(&rawQuote);
        debuq("!", rawQuote);
        debug(string("totals ") + "toAsk: " + to_string(wallet->position._baseTotal) + ", "
                                + "toBid: " + to_string(wallet->position._quoteTotal));
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
          rawQuote->ask.size = FN::d8(fmax(
            fmin(
              rawQuote->ask.size,
              wallet->position._baseTotal
            ),
            gw->minSize
          ));
        if (!rawQuote->bid.empty())
          rawQuote->bid.size = FN::d8(fmax(
            fmin(
              rawQuote->bid.size,
              wallet->position._quoteTotal
            ),
            gw->minSize
          ));
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
        if (!qp._matchPings) return;
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * market->fairValue / 100
          : qp.widthPong;
        mPrice safetyBuyPing = wallet->safety.buyPing;
        if (!rawQuote->ask.empty() and safetyBuyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and wallet->sideAPR == "Sell")
            or qp.pongAt == mPongAt::ShortPingAggressive
            or qp.pongAt == mPongAt::AveragePingAggressive
            or qp.pongAt == mPongAt::LongPingAggressive
            or rawQuote->ask.price < safetyBuyPing + widthPong
          ) rawQuote->ask.price = safetyBuyPing + widthPong;
          rawQuote->isAskPong = rawQuote->ask.price >= safetyBuyPing + widthPong;
        }
        mPrice safetysellPing = wallet->safety.sellPing;
        if (!rawQuote->bid.empty() and safetysellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and wallet->sideAPR == "Buy")
            or qp.pongAt == mPongAt::ShortPingAggressive
            or qp.pongAt == mPongAt::AveragePingAggressive
            or qp.pongAt == mPongAt::LongPingAggressive
            or rawQuote->bid.price > safetysellPing - widthPong
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
              if (bestAsk > market->fairValue) {
                rawQuote->ask.price = bestAsk;
                break;
              }
            }
        if (!rawQuote->bid.empty())
          for (mLevel &it : market->levels.bids)
            if (it.price < rawQuote->bid.price) {
              mPrice bestBid = it.price + gw->minTick;
              if (bestBid < market->fairValue) {
                rawQuote->bid.price = bestBid;
                break;
              }
            }
      };
      void applyTotalBasePosition(mQuote *rawQuote) {
        if (wallet->position._baseTotal < wallet->targetBasePosition - wallet->positionDivergence) {
          askStatus = mQuoteState::TBPHeld;
          rawQuote->ask.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            wallet->sideAPR = "Buy";
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.aprMultiplier*rawQuote->bid.size, fmin(wallet->targetBasePosition - wallet->position._baseTotal, wallet->position._quoteAmountValue / 2));
          }
        }
        else if (wallet->position._baseTotal >= wallet->targetBasePosition + wallet->positionDivergence) {
          bidStatus = mQuoteState::TBPHeld;
          rawQuote->bid.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            wallet->sideAPR = "Sell";
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.aprMultiplier*rawQuote->ask.size, fmin(wallet->position._baseTotal - wallet->targetBasePosition, wallet->position.baseAmount / 2));
          }
        }
        else wallet->sideAPR = "Off";
      };
      void applyTradesPerMinute(mQuote *rawQuote, bool superTradesActive) {
        if (wallet->safety.sell > (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          askStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->ask.clear();
        }
        if (wallet->safety.buy > (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
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
        if (qp.quotingStdevProtection == mSTDEV::Off or !market->mgStdevFV) return;
        if (!rawQuote->ask.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or wallet->sideAPR != "Sell"))
          rawQuote->ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->mgStdevFVMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->mgStdevTopMean : market->mgStdevAskMean )
              : market->fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->mgStdevFV : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->mgStdevTop : market->mgStdevAsk )),
            rawQuote->ask.price
          );
        if (!rawQuote->bid.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or wallet->sideAPR != "Buy"))
          rawQuote->bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->mgStdevFVMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->mgStdevTopMean : market->mgStdevBidMean )
              : market->fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? market->mgStdevFV : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? market->mgStdevTop : market->mgStdevBid )),
            rawQuote->bid.price
          );
      };
      void applyEwmaProtection(mQuote *rawQuote) {
        if (!qp.protectionEwmaQuotePrice or !market->mgEwmaP) return;
        rawQuote->ask.price = fmax(market->mgEwmaP, rawQuote->ask.price);
        rawQuote->bid.price = fmin(market->mgEwmaP, rawQuote->bid.price);
      };
      void applyEwmaTrendProtection(mQuote *rawQuote) {
        if (!qp.quotingEwmaTrendProtection or !market->mgEwmaTrendDiff) return;
        if (market->mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold){
          askStatus = mQuoteState::UpTrendHeld;
          rawQuote->ask.clear();
        }
        else if (market->mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold){
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
        k.bid.price = fmin(market->fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(market->fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      function<mQuote(mPrice, mAmount, mAmount)> calcTopOfMarket = [&](mPrice widthPing, mAmount bidSize, mAmount askSize) {
        mQuote k = quoteAtTopOfMarket();
        k.bid.price = k.bid.price + gw->minTick;
        k.ask.price = k.ask.price - gw->minTick;
        k.bid.price = fmin(market->fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(market->fairValue + widthPing / 2.0, k.ask.price);
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
          mLevel(fmax(market->fairValue - widthPing, 0), bidSize),
          mLevel(market->fairValue + widthPing, askSize)
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
        mClock now = _Tstamp_;
        for (map<mRandId, mOrder>::value_type &it : broker->orders)
          if (it.second.side != side) continue;
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
        for (map<mRandId, mOrder>::value_type &it : broker->orders)
          if (it.second.orderStatus == mStatus::Working and (side == mSide::Both or side == it.second.side))
            toCancel.push_back(it.first);
        for (mRandId &it : toCancel) broker->cancelOrder(it);
      };
      function<void(const string&, const mQuote&)> debuq = [&](const string &k, const mQuote &rawQuote) {
        debug("quote" + k + " " + to_string((int)bidStatus) + " " + to_string((int)askStatus) + " " + ((json)rawQuote).dump());
      };
      function<void(const string&)> debug = [&](const string &k) {
        screen->log("DEBUG QE", k);
      };
  };
}

#endif
