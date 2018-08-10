#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  class QE: public Klass,
            public Engine { public: QE() { engine = this; };
    private:
      unsigned int AK47inc = 0;
    protected:
      void load() {
        SQLITE_BACKUP
        levels.dummyMM.reset("loaded");
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mConnectivity, {
          if (!semaphore.online(rawdata))
            levels.clear();
        });
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          wallet.reset(rawdata, levels);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          levels.send_reset_filter(rawdata, gw->minTick);
          wallet.send_ratelimit(levels);
          calcQuote();
        });
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          broker.upsert(rawdata, &wallet, levels, &gw->askForFees);
        });
        gw->RAWDATA_ENTRY_POINT(mTrade, {                           PRETTY_DEBUG
          levels.stats.takerTrades.send_push_back(rawdata);
        });
      };
      void waitWebAdmin() {
        CLIENT_WELCOME
        CLIENT_CLICKME
      };
      void waitSysAdmin() {
        SCREEN_PRINTME
        SCREEN_PRESSME
      };
      void run() {                                                  PRETTY_DEBUG
        gw->load_externals();
      };
    public:
      void calcQuote() {                                            PRETTY_DEBUG
        quotes.clear();
        levels.filterBidQuotes.clear();
        levels.filterAskQuotes.clear();
        if (!semaphore.greenGateway) {
          quotes.bidStatus = mQuoteState::Disconnected;
          quotes.askStatus = mQuoteState::Disconnected;
        } else if (!levels.empty() and !wallet.target.safety.empty()) {
          if (!semaphore.greenButton) {
            quotes.bidStatus = mQuoteState::DisabledQuotes;
            quotes.askStatus = mQuoteState::DisabledQuotes;
            cancelOrders();
          } else {
            quotes.bidStatus = mQuoteState::UnknownHeld;
            quotes.askStatus = mQuoteState::UnknownHeld;
            sendQuoteToAPI();
          }
        }
        quotes.status.send_ratelimit();
      };
    private:
      void sendQuoteToAPI() {
        mPrice widthPing = qp.widthPercentage
          ? qp.widthPingPercentage * levels.fairValue / 100
          : qp.widthPing;
        if(qp.protectionEwmaWidthPing and levels.stats.ewma.mgEwmaW)
          widthPing = fmax(widthPing, levels.stats.ewma.mgEwmaW);
         mQuote quote = levels.dummyMM.calcRawQuote(
          gw->minTick,
          widthPing,
          wallet.target.safety.buySize,
          wallet.target.safety.sellSize
        );
        if (quote.bid.price <= 0 or quote.ask.price <= 0) {
          if (quote.bid.price or quote.ask.price)
            screen->logWar("QP", "Negative price detected, widthPing must be smaller");
          return;
        }
        applyQuotingParameters(widthPing, &quote);
        quotes.bidStatus = checkCrossedQuotes(mSide::Bid, &quote);
        quotes.askStatus = checkCrossedQuotes(mSide::Ask, &quote);
        if (quotes.askStatus == mQuoteState::Live) updateQuote(&quote.ask, mSide::Ask, quote.isAskPong);
        else cancelOrders(mSide::Ask);
        if (quotes.bidStatus == mQuoteState::Live) updateQuote(&quote.bid, mSide::Bid, quote.isBidPong);
        else cancelOrders(mSide::Bid);
      };
      void applyQuotingParameters(const mPrice &widthPing, mQuote *const rawQuote) {
        bool superTradesActive = false;
        DEBUQ("?", quotes.bidStatus, quotes.askStatus, rawQuote); applySuperTrades(rawQuote, &superTradesActive, widthPing);
        DEBUQ("A", quotes.bidStatus, quotes.askStatus, rawQuote); applyEwmaProtection(rawQuote);
        DEBUQ("B", quotes.bidStatus, quotes.askStatus, rawQuote); applyTotalBasePosition(rawQuote);
        DEBUQ("C", quotes.bidStatus, quotes.askStatus, rawQuote); applyStdevProtection(rawQuote);
        DEBUQ("D", quotes.bidStatus, quotes.askStatus, rawQuote); applyAggressivePositionRebalancing(rawQuote);
        DEBUQ("E", quotes.bidStatus, quotes.askStatus, rawQuote); applyAK47Increment(rawQuote);
        DEBUQ("F", quotes.bidStatus, quotes.askStatus, rawQuote); applyBestWidth(rawQuote);
        DEBUQ("G", quotes.bidStatus, quotes.askStatus, rawQuote); applyTradesPerMinute(rawQuote, superTradesActive);
        DEBUQ("H", quotes.bidStatus, quotes.askStatus, rawQuote); applyRoundPrice(rawQuote);
        DEBUQ("I", quotes.bidStatus, quotes.askStatus, rawQuote); applyRoundSize(rawQuote);
        DEBUQ("J", quotes.bidStatus, quotes.askStatus, rawQuote); applyDepleted(rawQuote);
        DEBUQ("K", quotes.bidStatus, quotes.askStatus, rawQuote); applyWaitingPing(rawQuote);
        DEBUQ("L", quotes.bidStatus, quotes.askStatus, rawQuote); applyEwmaTrendProtection(rawQuote);
        DEBUQ("!", quotes.bidStatus, quotes.askStatus, rawQuote);
        DEBUG("totals " + ("toAsk: " + to_string(wallet.base.total))
                        + ", toBid: " + to_string(wallet.quote.total / levels.fairValue));
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
              wallet.base.total
            ),
            gw->minSize
          ) / 1e-8) * 1e-8;
        if (!rawQuote->bid.empty())
          rawQuote->bid.size = floor(fmax(
            fmin(
              rawQuote->bid.size,
              wallet.quote.total / levels.fairValue
            ),
            gw->minSize
          ) / 1e-8) * 1e-8;
      };
      void applyDepleted(mQuote *const rawQuote) {
        if (rawQuote->bid.size > wallet.quote.total / levels.fairValue) {
          quotes.bidStatus = mQuoteState::DepletedFunds;
          rawQuote->bid.clear();
        }
        if (rawQuote->ask.size > wallet.base.total) {
          quotes.askStatus = mQuoteState::DepletedFunds;
          rawQuote->ask.clear();
        }
      };
      void applyWaitingPing(mQuote *const rawQuote) {
        if (qp.safety == mQuotingSafety::Off) return;
        if (!rawQuote->isAskPong and (
          (quotes.bidStatus != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::BidSide
          or qp.pingAt == mPingAt::DepletedAskSide
        )) {
          quotes.askStatus = mQuoteState::WaitingPing;
          rawQuote->ask.clear();
        }
        if (!rawQuote->isBidPong and (
          (quotes.askStatus != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::AskSide
          or qp.pingAt == mPingAt::DepletedBidSide
        )) {
          quotes.bidStatus = mQuoteState::WaitingPing;
          rawQuote->bid.clear();
        }
      };
      void applyAggressivePositionRebalancing(mQuote *const rawQuote) {
        if (qp.safety == mQuotingSafety::Off) return;
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * levels.fairValue / 100
          : qp.widthPong;
        mPrice &safetyBuyPing = wallet.target.safety.buyPing;
        if (!rawQuote->ask.empty() and safetyBuyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and wallet.target.sideAPR == "Sell")
            or (qp.safety == mQuotingSafety::PingPong
              ? rawQuote->ask.price < safetyBuyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) rawQuote->ask.price = safetyBuyPing + widthPong;
          rawQuote->isAskPong = rawQuote->ask.price >= safetyBuyPing + widthPong;
        }
        mPrice &safetysellPing = wallet.target.safety.sellPing;
        if (!rawQuote->bid.empty() and safetysellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and wallet.target.sideAPR == "Buy")
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
          for (mLevel &it : levels.asks)
            if (it.price > rawQuote->ask.price) {
              mPrice bestAsk = it.price - gw->minTick;
              if (bestAsk > levels.fairValue) {
                rawQuote->ask.price = bestAsk;
                break;
              }
            }
        if (!rawQuote->bid.empty())
          for (mLevel &it : levels.bids)
            if (it.price < rawQuote->bid.price) {
              mPrice bestBid = it.price + gw->minTick;
              if (bestBid < levels.fairValue) {
                rawQuote->bid.price = bestBid;
                break;
              }
            }
      };
      void applyTotalBasePosition(mQuote *const rawQuote) {
        if (wallet.base.total < wallet.target.targetBasePosition - wallet.target.positionDivergence) {
          quotes.askStatus = mQuoteState::TBPHeld;
          rawQuote->ask.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            wallet.target.sideAPR = "Buy";
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.aprMultiplier*rawQuote->bid.size, fmin(wallet.target.targetBasePosition - wallet.base.total, (wallet.quote.amount / levels.fairValue) / 2));
          }
        }
        else if (wallet.base.total >= wallet.target.targetBasePosition + wallet.target.positionDivergence) {
          quotes.bidStatus = mQuoteState::TBPHeld;
          rawQuote->bid.clear();
          if (qp.aggressivePositionRebalancing != mAPR::Off) {
            wallet.target.sideAPR = "Sell";
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.aprMultiplier*rawQuote->ask.size, fmin(wallet.base.total - wallet.target.targetBasePosition, wallet.base.amount / 2));
          }
        }
        else wallet.target.sideAPR = "Off";
      };
      void applyTradesPerMinute(mQuote *const rawQuote, const bool &superTradesActive) {
        if (wallet.target.safety.sell >= (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          quotes.askStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->ask.clear();
        }
        if (wallet.target.safety.buy >= (qp.tradesPerMinute * (superTradesActive ? qp.sopWidthMultiplier : 1))) {
          quotes.bidStatus = mQuoteState::MaxTradesSeconds;
          rawQuote->bid.clear();
        }
      };
      void applySuperTrades(mQuote *const rawQuote, bool *const superTradesActive, const mPrice &widthPing) {
        if (qp.superTrades != mSOP::Off
          and widthPing * qp.sopWidthMultiplier < levels.spread()
        ) {
          *superTradesActive = (qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize);
          if (qp.superTrades == mSOP::Size or qp.superTrades == mSOP::TradesSize) {
            if (!qp.buySizeMax) rawQuote->bid.size = fmin(qp.sopSizeMultiplier * rawQuote->bid.size, (wallet.quote.amount / levels.fairValue) / 2);
            if (!qp.sellSizeMax) rawQuote->ask.size = fmin(qp.sopSizeMultiplier * rawQuote->ask.size, wallet.base.amount / 2);
          }
        }
      };
      void applyAK47Increment(mQuote *const rawQuote) {
        if (qp.safety != mQuotingSafety::AK47) return;
        mPrice range = qp.percentageValues
          ? qp.rangePercentage * wallet.base.value / 100
          : qp.range;
        if (!rawQuote->bid.empty())
          rawQuote->bid.price -= AK47inc * range;
        if (!rawQuote->ask.empty())
          rawQuote->ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyStdevProtection(mQuote *const rawQuote) {
        if (qp.quotingStdevProtection == mSTDEV::Off or !levels.stats.stdev.fair) return;
        if (!rawQuote->ask.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or wallet.target.sideAPR != "Sell"))
          rawQuote->ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.askMean )
              : levels.fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.ask )),
            rawQuote->ask.price
          );
        if (!rawQuote->bid.empty() and (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTop or wallet.target.sideAPR != "Buy"))
          rawQuote->bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.bidMean )
              : levels.fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.bid )),
            rawQuote->bid.price
          );
      };
      void applyEwmaProtection(mQuote *const rawQuote) {
        if (!qp.protectionEwmaQuotePrice or !levels.stats.ewma.mgEwmaP) return;
        rawQuote->ask.price = fmax(levels.stats.ewma.mgEwmaP, rawQuote->ask.price);
        rawQuote->bid.price = fmin(levels.stats.ewma.mgEwmaP, rawQuote->bid.price);
      };
      void applyEwmaTrendProtection(mQuote *const rawQuote) {
        if (!qp.quotingEwmaTrendProtection or !levels.stats.ewma.mgEwmaTrendDiff) return;
        if (levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold){
          quotes.askStatus = mQuoteState::UpTrendHeld;
          rawQuote->ask.clear();
        }
        else if (levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold){
          quotes.bidStatus = mQuoteState::DownTrendHeld;
          rawQuote->bid.clear();
        }
      };
      mQuoteState checkCrossedQuotes(const mSide &side, mQuote *const quote) {
        bool cross = false;
        if (side == mSide::Bid) {
          if (quote->bid.empty()) return quotes.bidStatus;
          if (quote->ask.empty()) return mQuoteState::Live;
          cross = quote->bid.price >= quote->ask.price;
        } else if (side == mSide::Ask) {
          if (quote->ask.empty()) return quotes.askStatus;
          if (quote->bid.empty()) return mQuoteState::Live;
          cross = quote->ask.price <= quote->bid.price;
        }
        if (cross) {
          screen->logWar("QE", "Cross quote detected");
          return mQuoteState::Crossed;
        } else return mQuoteState::Live;
      };
      void updateQuote(mLevel *const quote, const mSide &side, const bool &isPong) {
        unsigned int n = 0;
        vector<mOrder*> toCancel,
                        keepWorking;
        vector<mRandId> zombies;
        mClock now = Tstamp;
        for (unordered_map<mRandId, mOrder>::value_type &it : broker.orders)
          if (it.second.side != side) continue;
          else {
            if (it.second.orderStatus == mStatus::New) {
              if (now - 10e+3 > it.second.time) {
                zombies.push_back(it.first);
                continue;
              }
              quotes.quotesInMemoryNew++;
            } else if (it.second.orderStatus == mStatus::Working) {
              (mSide::Bid == it.second.side
                ? levels.filterBidQuotes
                : levels.filterAskQuotes
              )[it.second.price] += it.second.quantity;
              quotes.quotesInMemoryWorking++;
            } else quotes.quotesInMemoryDone++;
            if (!it.second.preferPostOnly) continue;
            if (abs(it.second.price - quote->price) < gw->minTick) quote->clear();
            else if (it.second.orderStatus == mStatus::New) {
              if (qp.safety != mQuotingSafety::AK47 or ++n >= qp.bullets) quote->clear();
            } else if (qp.safety != mQuotingSafety::AK47 or (
              side == mSide::Bid
                ? quote->price <= it.second.price
                : quote->price >= it.second.price
            )) {
              if (args.lifetime and it.second.time + args.lifetime > now) quote->clear();
              else toCancel.push_back(&it.second);
            } else keepWorking.push_back(&it.second);
          }
        for (mRandId &it : zombies) broker.erase(it);
        if (quote->empty()) return;
        if (qp.safety == mQuotingSafety::AK47
          and toCancel.empty()
          and !keepWorking.empty()
        ) toCancel.push_back(side == mSide::Bid ? keepWorking.front() : keepWorking.back());
        mOrder *toReplace = nullptr;
        if (!toCancel.empty()) {
          toReplace = side == mSide::Bid ? toCancel.back() : toCancel.front();
          toCancel.erase(side == mSide::Bid ? toCancel.end()-1 : toCancel.begin());
        }
        sendOrders(toCancel, toReplace, quote->price, quote->size, side, isPong);
      };
  };
}

#endif
