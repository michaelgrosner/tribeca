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
  mConnectivity gwQuotingState_ = mConnectivity::Disconnected,
                gwConnectExchange_ = mConnectivity::Disconnected;
  class QE {
    public:
      static void main() {
        load();
        thread([&]() {
          while (true) {
            if (mgFairValue) {
              MG::calcStats();
              PG::calcSafety();
              calcQuote();
            } else cout << FN::uiT() << "QE" << RRED << " Warrrrning:" << BRED << " Unable to calculate quote, missing fair value." << endl;
            this_thread::sleep_for(chrono::seconds(1));
          }
        }).detach();
        ev_gwConnectButton = [](mConnectivity k) {
          gwQuotingState_ = k;
        };
        ev_gwConnectExchange = [](mConnectivity k) {
          gwConnectExchange_ = k;
          send();
        };
        ev_uiQuotingParameters = []() {
          MG::calcFairValue();
          PG::calcTargetBasePos();
          PG::calcSafety();
          calcQuote();
        };
        ev_ogTrade = [](mTradeHydrated k) {
          PG::addTrade(k);
          PG::calcSafety();
          calcQuote();
        };
        ev_mgEwmaQuoteProtection = []() {
          calcQuote();
        };
        ev_mgLevels = []() {
          calcQuote();
        };
        ev_pgTargetBasePosition = []() {
          calcQuote();
        };
        UI::uiSnap(uiTXT::QuoteStatus, &onSnap);
      }
    private:
      static void load() {
        qeQuotingMode[mQuotingMode::Top] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::Mid] = &calcMidOfMarket;
        qeQuotingMode[mQuotingMode::Join] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::InverseJoin] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::InverseTop] = &calcInverseTopOfMarket;
        qeQuotingMode[mQuotingMode::PingPong] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::Boomerang] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::AK47] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::HamelinRat] = &calcTopOfMarket;
        qeQuotingMode[mQuotingMode::Depth] = &calcDepthOfMarket;
      };
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
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote! " << (json)qeQuote << "." << endl;
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
        if (!diffStatus() and !diffCounts(&quotesInMemoryNew, &quotesInMemoryWorking, &quotesInMemoryDone))
          return;
        qeStatus = mQuoteStatus(qeBidStatus, qeAskStatus, quotesInMemoryNew, quotesInMemoryWorking, quotesInMemoryDone);
        UI::uiSend(uiTXT::QuoteStatus, qeStatus, true);
      };
      static bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        vector<string> toDelete;
        unsigned long T = FN::T();
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          mORS k = (mORS)it->second.orderStatus;
          if (k == mORS::New) {
            if (it->second.exchangeId == "" and T-1e+4>it->second.time)
              toDelete.push_back(it->first);
            ++(*qNew);
          } else if (k == mORS::Working) ++(*qWorking);
          else ++(*qDone);
        }
        for (vector<string>::iterator it = toDelete.begin(); it != toDelete.end(); ++it)
          OG::allOrdersDelete(*it, "");
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
        if (abs(price - prevQuote.size) < gw->minTick) return prevQuote;
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
        if (MG::empty() or !pgPos.value) return mQuote();
        double widthPing = QP::getBool("widthPercentage")
          ? QP::getDouble("widthPingPercentage") * mgFairValue / 100
          : QP::getDouble("widthPing");
        double widthPong = QP::getBool("widthPercentage")
          ? QP::getDouble("widthPongPercentage") * mgFairValue / 100
          : QP::getDouble("widthPong");
        double totalBasePosition = pgPos.baseAmount + pgPos.baseHeldAmount;
        double totalQuotePosition = (pgPos.quoteAmount + pgPos.quoteHeldAmount) / mgFairValue;
        double buySize = QP::getBool("percentageValues")
          ? QP::getDouble("buySizePercentage") * pgPos.value / 100
          : QP::getDouble("buySize");
        double sellSize = QP::getBool("percentageValues")
          ? QP::getDouble("sellSizePercentage") * pgPos.value / 100
          : QP::getDouble("sellSize");
        if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off and QP::getBool("buySizeMax"))
          buySize = fmax(buySize, pgTargetBasePos - totalBasePosition);
        if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off and QP::getBool("sellSizeMax"))
          sellSize = fmax(sellSize, totalBasePosition - pgTargetBasePos);
        mQuote rawQuote = quote(widthPing, buySize, sellSize);
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote? " << (json)rawQuote << "." << endl;
        if (!rawQuote.bid.price and !rawQuote.ask.price) return mQuote();
        double _rawBidSz = rawQuote.bid.size;
        double _rawAskSz = rawQuote.ask.size;
        if (pgSafety.buyPing == -1) return mQuote();
        qeBidStatus = mQuoteState::UnknownHeld;
        qeAskStatus = mQuoteState::UnknownHeld;
        vector<int> superTradesMultipliers = {1, 1};
        if ((mSOP)QP::getInt("superTrades") != mSOP::Off
          and widthPing * QP::getInt("sopWidthMultiplier") < mgLevelsFilter.asks.begin()->price - mgLevelsFilter.bids.begin()->price
        ) {
          superTradesMultipliers[0] = (mSOP)QP::getInt("superTrades") == mSOP::x2trades or (mSOP)QP::getInt("superTrades") == mSOP::x2tradesSize
            ? 2 : ((mSOP)QP::getInt("superTrades") == mSOP::x3trades or (mSOP)QP::getInt("superTrades") == mSOP::x3tradesSize
              ? 3 : 1);
          superTradesMultipliers[1] = (mSOP)QP::getInt("superTrades") == mSOP::x2Size or (mSOP)QP::getInt("superTrades") == mSOP::x2tradesSize
            ? 2 : ((mSOP)QP::getInt("superTrades") == mSOP::x3Size or (mSOP)QP::getInt("superTrades") == mSOP::x3tradesSize
              ? 3 : 1);
        }
        double pDiv = QP::getBool("percentageValues")
          ? QP::getDouble("positionDivergencePercentage") * pgPos.value / 100
          : QP::getDouble("positionDivergence");
        if (superTradesMultipliers[1] > 1) {
          if (!QP::getBool("buySizeMax")) rawQuote.bid.size = fmin(superTradesMultipliers[1]*buySize, (pgPos.quoteAmount / mgFairValue) / 2);
          if (!QP::getBool("sellSizeMax")) rawQuote.ask.size = fmin(superTradesMultipliers[1]*sellSize, pgPos.baseAmount / 2);
        }
        if (QP::getBool("quotingEwmaProtection") and mgEwmaP) {
          rawQuote.ask.price = fmax(mgEwmaP, rawQuote.ask.price);
          rawQuote.bid.price = fmin(mgEwmaP, rawQuote.bid.price);
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if (totalBasePosition < pgTargetBasePos - pDiv) {
          qeAskStatus = mQuoteState::TBPHeld;
          rawQuote.ask.price = 0;
          rawQuote.ask.price = 0;
          if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off) {
            pgSideAPR = "Bid";
            if (!QP::getBool("buySizeMax")) rawQuote.bid.size = fmin(QP::getInt("aprMultiplier")*buySize, fmin(pgTargetBasePos - totalBasePosition, (pgPos.quoteAmount / mgFairValue) / 2));
          }
        }
        else if (totalBasePosition > pgTargetBasePos + pDiv) {
          qeBidStatus = mQuoteState::TBPHeld;
          rawQuote.bid.price = 0;
          rawQuote.bid.size = 0;
          if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off) {
            pgSideAPR = "Sell";
            if (!QP::getBool("sellSizeMax")) rawQuote.ask.size = fmin(QP::getInt("aprMultiplier")*sellSize, fmin(totalBasePosition - pgTargetBasePos, pgPos.baseAmount / 2));
          }
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if ((mSTDEV)QP::getInt("quotingStdevProtection") != mSTDEV::Off and mgStdevFV) {
          if (rawQuote.ask.price and ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTop or pgSideAPR != "Sell"))
            rawQuote.ask.price = fmax(
              (QP::getBool("quotingStdevBollingerBands")
                ? ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFVMean : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTopMean : mgStdevAskMean )
                : mgFairValue) + (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFV : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTop : mgStdevAsk )),
              rawQuote.ask.price
            );
          if (rawQuote.bid.price and ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTop or pgSideAPR != "Bid")) {
            rawQuote.bid.price = fmin(
              (QP::getBool("quotingStdevBollingerBands")
                ? ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFVMean : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTopMean : mgStdevBidMean )
                : mgFairValue) - (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFV : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTop : mgStdevBid )),
              rawQuote.bid.price
            );
          }
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::PingPong or QP::matchPings()) {
          if (rawQuote.ask.size and pgSafety.buyPing and (
            ((mAPR)QP::getInt("aggressivePositionRebalancing") == mAPR::SizeWidth and pgSideAPR == "Sell")
            or (mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingAggressive
            or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive
            or rawQuote.ask.price < pgSafety.buyPing + widthPong
          )) rawQuote.ask.price = pgSafety.buyPing + widthPong;
          if (rawQuote.bid.size and pgSafety.sellPong and (
            ((mAPR)QP::getInt("aggressivePositionRebalancing") == mAPR::SizeWidth and pgSideAPR == "Buy")
            or (mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingAggressive
            or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive
            or rawQuote.bid.price > pgSafety.sellPong - widthPong
          )) rawQuote.bid.price = pgSafety.sellPong - widthPong;
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if (QP::getBool("bestWidth")) {
          if (rawQuote.ask.price)
            for (vector<mLevel>::iterator it = mgLevelsFilter.asks.begin(); it != mgLevelsFilter.asks.end(); ++it)
              if (it->price > rawQuote.ask.price) {
                double bestAsk = it->price - gw->minTick;
                if (bestAsk > mgFairValue) {
                  rawQuote.ask.price = bestAsk;
                  break;
                }
              }
          if (rawQuote.bid.price)
            for (vector<mLevel>::iterator it = mgLevelsFilter.bids.begin(); it != mgLevelsFilter.bids.end(); ++it)
              if (it->price < rawQuote.bid.price) {
                double bestBid = it->price + gw->minTick;
                if (bestBid < mgFairValue) {
                  rawQuote.bid.price = bestBid;
                  break;
                }
              }
        }
        if (pgSafety.sell > (QP::getDouble("tradesPerMinute") * superTradesMultipliers[0])) {
          qeAskStatus = mQuoteState::MaxTradesSeconds;
          rawQuote.ask.price = 0;
          rawQuote.ask.size = 0;
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if (((mQuotingMode)QP::getInt("mode") == mQuotingMode::PingPong or QP::matchPings())
          and !pgSafety.buyPing and ((mPingAt)QP::getInt("pingAt") == mPingAt::StopPings or (mPingAt)QP::getInt("pingAt") == mPingAt::BidSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedAskSide
            or (totalQuotePosition>buySize and ((mPingAt)QP::getInt("pingAt") == mPingAt::DepletedSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedBidSide))
        )) {
          qeAskStatus = !pgSafety.buyPing
            ? mQuoteState::WaitingPing
            : mQuoteState::DepletedFunds;
          rawQuote.ask.price = 0;
          rawQuote.ask.size = 0;
        }
        if (pgSafety.buy > (QP::getDouble("tradesPerMinute") * superTradesMultipliers[0])) {
          qeBidStatus = mQuoteState::MaxTradesSeconds;
          rawQuote.bid.price = 0;
          rawQuote.bid.size = 0;
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if (((mQuotingMode)QP::getInt("mode") == mQuotingMode::PingPong or QP::matchPings())
          and !pgSafety.sellPong and ((mPingAt)QP::getInt("pingAt") == mPingAt::StopPings or (mPingAt)QP::getInt("pingAt") == mPingAt::AskSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedBidSide
            or (totalBasePosition>sellSize and ((mPingAt)QP::getInt("pingAt") == mPingAt::DepletedSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedAskSide))
        )) {
          qeBidStatus = !pgSafety.sellPong
            ? mQuoteState::WaitingPing
            : mQuoteState::DepletedFunds;
          rawQuote.bid.price = 0;
          rawQuote.bid.size = 0;
        }
        if (rawQuote.bid.price) {
          rawQuote.bid.price = FN::roundSide(rawQuote.bid.price, gw->minTick, mSide::Bid);
          rawQuote.bid.price = fmax(0, rawQuote.bid.price);
        }
        if (rawQuote.ask.price) {
          rawQuote.ask.price = FN::roundSide(rawQuote.ask.price, gw->minTick, mSide::Ask);
          rawQuote.ask.price = fmax(rawQuote.bid.price + gw->minTick, rawQuote.ask.price);
        }
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW totals " << "toAsk:" << totalBasePosition << " toBid:" << totalQuotePosition << " min:" << gw->minSize << "." << endl;
        if (rawQuote.ask.size) {
          if (rawQuote.ask.size > totalBasePosition)
            rawQuote.ask.size = (!_rawBidSz or _rawBidSz > totalBasePosition)
              ? totalBasePosition : _rawBidSz;
          rawQuote.ask.size = FN::roundDown(fmax(gw->minSize, rawQuote.ask.size), 1e-8);
          rawQuote.isAskPong = (pgSafety.buyPing and rawQuote.ask.price and rawQuote.ask.price >= pgSafety.buyPing + widthPong);
        } else rawQuote.isAskPong = false;
        if (rawQuote.bid.size) {
          if (rawQuote.bid.size > totalQuotePosition)
            rawQuote.bid.size = (!_rawAskSz or _rawAskSz > totalQuotePosition)
              ? totalQuotePosition : _rawAskSz;
          rawQuote.bid.size = FN::roundDown(fmax(gw->minSize, rawQuote.bid.size), 1e-8);
          rawQuote.isBidPong = (pgSafety.sellPong and rawQuote.bid.price and rawQuote.bid.price <= pgSafety.sellPong - widthPong);
        } else rawQuote.isBidPong = false;
        if (argDebugQuotes) cout << FN::uiT() << "DEBUG " << RWHITE << "GW quote¿ " << (json)rawQuote << "." << endl;
        return rawQuote;
      };
      static mQuote quote(double widthPing, double buySize, double sellSize) {
        mQuotingMode k = (mQuotingMode)QP::getInt("mode");
        if (qeQuotingMode.find(k) == qeQuotingMode.end()) { cout << FN::uiT() << "Errrror: Invalid quoting mode." << endl; exit(1); }
        return (*qeQuotingMode[k])(widthPing, buySize, sellSize);
      };
      static mQuote quoteAtTopOfMarket() {
        mLevel topBid = mgLevelsFilter.bids.begin()->size > gw->minTick
          ? mgLevelsFilter.bids.at(0) : mgLevelsFilter.bids.at(mgLevelsFilter.bids.size()>1?1:0);
        mLevel topAsk = mgLevelsFilter.asks.begin()->size > gw->minTick
          ? mgLevelsFilter.asks.at(0) : mgLevelsFilter.asks.at(mgLevelsFilter.asks.size()>1?1:0);
        return mQuote(topBid, topAsk);
      };
      static mQuote calcTopOfMarket(double widthPing, double buySize, double sellSize) {
        mQuote k = quoteAtTopOfMarket();
        if ((mQuotingMode)QP::getInt("mode") != mQuotingMode::Join and k.bid.size > 0.2)
          k.bid.price = k.bid.price + gw->minTick;
        k.bid.price = fmin(mgFairValue - widthPing / 2.0, k.bid.price);
        if ((mQuotingMode)QP::getInt("mode") != mQuotingMode::Join and k.ask.size > 0.2)
          k.ask.price = k.ask.price - gw->minTick;
        k.ask.price = fmin(mgFairValue + widthPing / 2.0, k.ask.price);
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
        if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::InverseTop) {
          if (k.bid.size > .2) k.bid.price = k.bid.price + gw->minTick;
          if (k.ask.size > .2) k.ask.price = k.ask.price - gw->minTick;
        }
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
          cout << FN::uiT() << "QE Crossing quote detected! new " << (side == mSide::Bid ? "Bid" : "Ask") << "Side quote at " << price << " crossed with " << (side == mSide::Bid ? "Ask" : "Bid") << "Side quote at " << ecirp << "." << endl;
          return mQuoteState::Crossed;
        }
        return mQuoteState::Live;
     };
      static void updateQuote(mLevel q, mSide side, bool isPong) {
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        bool eq = false;
        for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (it->first == q.price) { eq = true; break; }
        if ((mQuotingMode)QP::getInt("mode") != mQuotingMode::AK47) {
          if (orderSide.size()) {
            if (!eq) modify(side, q, isPong);
          } else start(side, q, isPong);
          return;
        }
        if (!eq and orderSide.size() >= (size_t)QP::getInt("bullets"))
          modify(side, q, isPong);
        else start(side, q, isPong);
      };
      static multimap<double, mOrder> orderCacheSide(mSide side) {
        multimap<double, mOrder> orderSide;
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if ((mSide)it->second.side == side)
            orderSide.insert(pair<double, mOrder>(it->second.price, it->second));
        return orderSide;
      };
      static void modify(mSide side, mLevel q, bool isPong) {
        if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::AK47)
          stopWorstQuote(side);
        else stopAllQuotes(side);
        start(side, q, isPong);
      };
      static void start(mSide side, mLevel q, bool isPong) {
        if (QP::getDouble("delayAPI")) {
          unsigned long nextStart = qeNextT + (6e+4/QP::getDouble("delayAPI"));
          if ((double)nextStart - (double)FN::T() > 0) {
            qeNextQuote.clear();
            qeNextQuote[side] = q;
            thread([&]() {
              unsigned int qeThread_ = ++qeThread;
              unsigned long nextStart_ = nextStart;
              bool isPong_ = isPong;
              while (qeThread_ == qeThread) {
                if ((double)nextStart_ - (double)FN::T() > 0)
                  this_thread::sleep_for(chrono::milliseconds(100));
                else {
                  start(qeNextQuote.begin()->first, qeNextQuote.begin()->second, isPong_);
                  break;
                }
              }
            }).detach();
            return;
          }
          qeNextT = FN::T();
        }
        double price = q.price;
        multimap<double, mOrder> orderSide = orderCacheSide(side);
        bool eq = false;
        for (multimap<double, mOrder>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (price == it->first
            or ((mQuotingMode)QP::getInt("mode") == mQuotingMode::AK47
              and (price + (QP::getDouble("range") - 1e-2)) >= it->first
              and (price - (QP::getDouble("range") - 1e-2)) <= it->first)
          ) { eq = true; break; }
        if (eq) {
          if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::AK47 and orderSide.size()<(size_t)QP::getInt("bullets")) {
            double incPrice = (QP::getDouble("range") * (side == mSide::Bid ? -1 : 1 ));
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
                or ((price + (QP::getDouble("range") - 1e-2)) >= it->first
                  and (price - (QP::getDouble("range") - 1e-2)) <= it->first)
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
