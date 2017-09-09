#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  unsigned int qeT = 0;
  unsigned long qeNextT = 0;
  unsigned long qeThread = 0;
  json qeQuote;
  json qeStatus;
  mQuoteStatus qeBidStatus = mQuoteStatus::MissingData;
  mQuoteStatus qeAskStatus = mQuoteStatus::MissingData;
  typedef json (*qeMode)(double widthPing, double buySize, double sellSize);
  map<mQuotingMode, qeMode> qeQuotingMode;
  map<mSide, json> qeNextQuote;
  bool gwState_ = false;
  mConnectivity gwConn_ = mConnectivity::Disconnected;
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
            } else cout << FN::uiT() << "Unable to calculate quote, missing fair value." << endl;
            this_thread::sleep_for(chrono::seconds(1));
          }
        }).detach();
        EV::on(mEv::ExchangeConnect, [](json k) {
          gwState_ = k.value("state", false);
          gwConn_ = (mConnectivity)k.value("status", 0);
          send();
        });
        EV::on(mEv::QuotingParameters, [](json k) {
          MG::calcFairValue();
          PG::calcTargetBasePos();
          PG::calcSafety();
          calcQuote();
        });
        EV::on(mEv::OrderTradeBroker, [](json k) {
          PG::addTrade(k);
          PG::calcSafety();
          calcQuote();
        });
        EV::on(mEv::EWMAProtectionCalculator, [](json k) {
          calcQuote();
        });
        EV::on(mEv::FilteredMarket, [](json k) {
          calcQuote();
        });
        EV::on(mEv::TargetPosition, [](json k) {
          calcQuote();
        });
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
      static json onSnap(json z) {
        return { qeStatus };
      };
      static void calcQuote() {
        qeBidStatus = mQuoteStatus::MissingData;
        qeAskStatus = mQuoteStatus::MissingData;
        if (!mgFairValue or MG::empty()) {
          qeQuote = {};
          return;
        }
        json quote = nextQuote();
        if (quote.is_null()) {
          qeQuote = {};
          return;
        }
        quote = {
          {"bid", quotesAreSame(quote["bidPx"].get<double>(), quote["bidSz"].get<double>(), quote["isBidPong"].get<bool>(), mSide::Bid)},
          {"ask", quotesAreSame(quote["askPx"].get<double>(), quote["askSz"].get<double>(), quote["isAskPong"].get<bool>(), mSide::Ask)}
        };
        if ((qeQuote.is_null() and quote.is_null()) or (
          abs((qeQuote["bid"].is_null() ? 0 : qeQuote["/bid/price"_json_pointer].get<double>()) - (quote["bid"].is_null() ? 0 : quote["/bid/price"_json_pointer].get<double>())) < gw->minTick
          and abs((qeQuote["ask"].is_null() ? 0 : qeQuote["/ask/price"_json_pointer].get<double>()) - (quote["ask"].is_null() ? 0 : quote["/ask/price"_json_pointer].get<double>())) < gw->minTick
          and abs((qeQuote["bid"].is_null() ? 0 : qeQuote["/bid/size"_json_pointer].get<double>()) - (quote["bid"].is_null() ? 0 : quote["/bid/size"_json_pointer].get<double>())) < gw->minSize
          and abs((qeQuote["ask"].is_null() ? 0 : qeQuote["/ask/size"_json_pointer].get<double>()) - (quote["ask"].is_null() ? 0 : quote["/ask/size"_json_pointer].get<double>())) < gw->minSize
        )) return;
        qeQuote = quote;
        send();
      };
      static void send() {
        sendQuoteToAPI();
        sendQuoteToUI();
      };
      static void sendQuoteToAPI() {
        if (gwConn_ == mConnectivity::Disconnected or qeQuote.is_null()) {
          qeAskStatus = mQuoteStatus::Disconnected;
          qeBidStatus = mQuoteStatus::Disconnected;
        } else {
          if (gwState_) {
            qeBidStatus = checkCrossedQuotes(mSide::Bid);
            qeAskStatus = checkCrossedQuotes(mSide::Ask);
          } else {
            qeAskStatus = mQuoteStatus::DisabledQuotes;
            qeBidStatus = mQuoteStatus::DisabledQuotes;
          }
          if (qeAskStatus == mQuoteStatus::Live) updateQuote(qeQuote["ask"], mSide::Ask);
          else stopAllQuotes(mSide::Ask);
          if (qeBidStatus == mQuoteStatus::Live) updateQuote(qeQuote["bid"], mSide::Bid);
          else stopAllQuotes(mSide::Bid);
        }
      };
      static void sendQuoteToUI() {
        unsigned int quotesInMemoryNew = 0;
        unsigned int quotesInMemoryWorking = 0;
        unsigned int quotesInMemoryDone = 0;
        if (!diffStatus() and !diffCounts(&quotesInMemoryNew, &quotesInMemoryWorking, &quotesInMemoryDone))
          return;
        qeStatus = {
          {"bidStatus", (int)qeBidStatus},
          {"askStatus", (int)qeAskStatus},
          {"quotesInMemoryNew", quotesInMemoryNew},
          {"quotesInMemoryWorking", quotesInMemoryWorking},
          {"quotesInMemoryDone", quotesInMemoryDone}
        };
        UI::uiSend(uiTXT::QuoteStatus, qeStatus, true);
      };
      static bool diffCounts(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        vector<string> toDelete;
        unsigned long T = FN::T();
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          mORS k = (mORS)it->second["orderStatus"].get<int>();
          if (k == mORS::New) {
            if (it->second["exchangeId"].is_null() and T-1e+4>it->second["time"].get<unsigned long>())
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
        return qeStatus.is_null()
          or qNew != qeStatus["quotesInMemoryNew"].get<unsigned int>()
          or qWorking != qeStatus["quotesInMemoryWorking"].get<unsigned int>()
          or qDone != qeStatus["quotesInMemoryDone"].get<unsigned int>();
      };
      static bool diffStatus() {
        return qeStatus.is_null()
          or qeBidStatus != (mQuoteStatus)qeStatus["bidStatus"].get<int>()
          or qeAskStatus != (mQuoteStatus)qeStatus["askStatus"].get<int>();
      };
      static json quotesAreSame(double price, double size, bool isPong, mSide side) {
        json newQuote = {
          {"price", price},
          {"size", size},
          {"isPong", isPong}
        };
        if (!price or !size) return {};
        if (qeQuote.is_null()) return newQuote;
        json prevQuote = mSide::Bid == side ? qeQuote["bid"] : qeQuote["ask"];
        if (prevQuote.is_null()) return newQuote;
        if (abs(size - prevQuote["size"].get<double>()) > 5e-3) return newQuote;
        if (abs(price - prevQuote["size"].get<double>()) < gw->minTick) return prevQuote;
        bool quoteWasWidened = true;
        if ((mSide::Bid == side and prevQuote["price"].get<double>() < price)
          or (mSide::Ask == side and prevQuote["price"].get<double>() > price)
        ) quoteWasWidened = false;
        unsigned int now = FN::T();
        if (!quoteWasWidened and now - qeT < 300) return prevQuote;
        qeT = now;
        return newQuote;
      };
      static json nextQuote() {
        if (MG::empty() or pgPos.is_null()) return {};
        double widthPing = QP::getBool("widthPercentage")
          ? QP::getDouble("widthPingPercentage") * mgFairValue / 100
          : QP::getDouble("widthPing");
        double widthPong = QP::getBool("widthPercentage")
          ? QP::getDouble("widthPongPercentage") * mgFairValue / 100
          : QP::getDouble("widthPong");
        double totalBasePosition = pgPos["baseAmount"].get<double>() + pgPos["baseHeldAmount"].get<double>();
        double totalQuotePosition = (pgPos["quoteAmount"].get<double>() + pgPos["quoteHeldAmount"].get<double>()) / mgFairValue;
        double buySize = QP::getBool("percentageValues")
          ? QP::getDouble("buySizePercentage") * pgPos["value"].get<double>() / 100
          : QP::getDouble("buySize");
        double sellSize = QP::getBool("percentageValues")
          ? QP::getDouble("sellSizePercentage") * pgPos["value"].get<double>() / 100
          : QP::getDouble("sellSize");
        if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off and QP::getBool("buySizeMax"))
          buySize = fmax(buySize, pgTargetBasePos - totalBasePosition);
        if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off and QP::getBool("sellSizeMax"))
          sellSize = fmax(sellSize, totalBasePosition - pgTargetBasePos);
        json rawQuote = quote(widthPing, buySize, sellSize);
        if (rawQuote.is_null()) return {};
        double _rawBidSz = rawQuote["bidSz"].get<double>();
        double _rawAskSz = rawQuote["askSz"].get<double>();
        if (pgSafety.is_null()) return {};
        qeBidStatus = mQuoteStatus::UnknownHeld;
        qeAskStatus = mQuoteStatus::UnknownHeld;
        vector<int> superTradesMultipliers = {1, 1};
        if ((mSOP)QP::getInt("superTrades") != mSOP::Off
          and widthPing * QP::getInt("sopWidthMultiplier") < mGWmktFilter["/asks/0/price"_json_pointer].get<double>() - mGWmktFilter["/bids/0/price"_json_pointer].get<double>()
        ) {
          superTradesMultipliers[0] = (mSOP)QP::getInt("superTrades") == mSOP::x2trades or (mSOP)QP::getInt("superTrades") == mSOP::x2tradesSize
            ? 2 : ((mSOP)QP::getInt("superTrades") == mSOP::x3trades or (mSOP)QP::getInt("superTrades") == mSOP::x3tradesSize
              ? 3 : 1);
          superTradesMultipliers[1] = (mSOP)QP::getInt("superTrades") == mSOP::x2Size or (mSOP)QP::getInt("superTrades") == mSOP::x2tradesSize
            ? 2 : ((mSOP)QP::getInt("superTrades") == mSOP::x3Size or (mSOP)QP::getInt("superTrades") == mSOP::x3tradesSize
              ? 3 : 1);
        }
        double pDiv = QP::getBool("percentageValues")
          ? QP::getDouble("positionDivergencePercentage") * pgPos["value"].get<double>() / 100
          : QP::getDouble("positionDivergence");
        if (superTradesMultipliers[1] > 1) {
          if (!QP::getBool("buySizeMax")) rawQuote["bidSz"] = fmin(superTradesMultipliers[1]*buySize, (pgPos["quoteAmount"].get<double>() / mgFairValue) / 2);
          if (!QP::getBool("sellSizeMax")) rawQuote["askSz"] = fmin(superTradesMultipliers[1]*sellSize, pgPos["baseAmount"].get<double>() / 2);
        }
        if (QP::getBool("quotingEwmaProtection") and mgEwmaP) {
          rawQuote["askPx"] = fmax(mgEwmaP, rawQuote["askPx"].get<double>());
          rawQuote["bidPx"] = fmin(mgEwmaP, rawQuote["bidPx"].get<double>());
        }
        if (totalBasePosition < pgTargetBasePos - pDiv) {
          qeAskStatus = mQuoteStatus::TBPHeld;
          rawQuote["askPx"] = 0;
          rawQuote["askSz"] = 0;
          if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off) {
            pgSideAPR = "Bid";
            if (!QP::getBool("buySizeMax")) rawQuote["bidSz"] = fmin(QP::getInt("aprMultiplier")*buySize, fmin(pgTargetBasePos - totalBasePosition, (pgPos["quoteAmount"].get<double>() / mgFairValue) / 2));
          }
        }
        else if (totalBasePosition > pgTargetBasePos + pDiv) {
          qeBidStatus = mQuoteStatus::TBPHeld;
          rawQuote["bidPx"] = 0;
          rawQuote["bidSz"] = 0;
          if ((mAPR)QP::getInt("aggressivePositionRebalancing") != mAPR::Off) {
            pgSideAPR = "Sell";
            if (!QP::getBool("sellSizeMax")) rawQuote["askSz"] = fmin(QP::getInt("aprMultiplier")*sellSize, fmin(totalBasePosition - pgTargetBasePos, pgPos["baseAmount"].get<double>() / 2));
          }
        }
        if ((mSTDEV)QP::getInt("quotingStdevProtection") != mSTDEV::Off and mgStdevFV) {
          if (rawQuote["askPx"].get<double>() and ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTop or pgSideAPR != "Sell"))
            rawQuote["askPx"] = fmax(
              (QP::getBool("quotingStdevBollingerBands")
                ? ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFVMean : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTopMean : mgStdevAskMean )
                : mgFairValue) + (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFV : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTop : mgStdevAsk )),
              rawQuote["askPx"].get<double>()
            );
          if (rawQuote["bidPx"].get<double>() and ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTop or pgSideAPR != "Bid")) {
            rawQuote["bidPx"] = fmin(
              (QP::getBool("quotingStdevBollingerBands")
                ? ((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFVMean : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTopMean : mgStdevBidMean )
                : mgFairValue) - (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFV or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnFVAPROff)
                  ? mgStdevFV : (((mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTops or (mSTDEV)QP::getInt("quotingStdevProtection") == mSTDEV::OnTopsAPROff)
                    ? mgStdevTop : mgStdevBid )),
              rawQuote["bidPx"].get<double>()
            );
          }
        }
        if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::PingPong or QP::matchPings()) {
          if (rawQuote["askSz"].get<double>() and pgSafety["buyPing"].get<double>() and (
            ((mAPR)QP::getInt("aggressivePositionRebalancing") == mAPR::SizeWidth and pgSideAPR == "Sell")
            or (mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingAggressive
            or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive
            or rawQuote["askPx"].get<double>() < pgSafety["buyPing"].get<double>() + widthPong
          )) rawQuote["askPx"] = pgSafety["buyPing"].get<double>() + widthPong;
          if (rawQuote["bidSz"].get<double>() and pgSafety["sellPong"].get<double>() and (
            ((mAPR)QP::getInt("aggressivePositionRebalancing") == mAPR::SizeWidth and pgSideAPR == "Buy")
            or (mPongAt)QP::getInt("pongAt") == mPongAt::ShortPingAggressive
            or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive
            or rawQuote["bidPx"].get<double>() > pgSafety["sellPong"].get<double>() - widthPong
          )) rawQuote["bidPx"] = pgSafety["sellPong"].get<double>() - widthPong;
        }
        if (QP::getBool("bestWidth")) {
          if (rawQuote["askPx"].get<double>())
            for (json::iterator it = mGWmktFilter["asks"].begin(); it != mGWmktFilter["asks"].end(); ++it)
              if ((*it)["price"].get<double>() > rawQuote["askPx"].get<double>()) {
                double bestAsk = (*it)["price"].get<double>() - gw->minTick;
                if (bestAsk > mgFairValue) {
                  rawQuote["askPx"] = bestAsk;
                  break;
                }
              }
          if (rawQuote["bidPx"].get<double>())
            for (json::iterator it = mGWmktFilter["bids"].begin(); it != mGWmktFilter["bids"].end(); ++it)
              if ((*it)["price"].get<double>() < rawQuote["bidPx"].get<double>()) {
                double bestBid = (*it)["price"].get<double>() + gw->minTick;
                if (bestBid < mgFairValue) {
                  rawQuote["bidPx"] = bestBid;
                  break;
                }
              }
        }
        if (pgSafety["sell"].get<double>() > (QP::getDouble("tradesPerMinute") * superTradesMultipliers[0])) {
          qeAskStatus = mQuoteStatus::MaxTradesSeconds;
          rawQuote["askPx"] = 0;
          rawQuote["askSz"] = 0;
        }
        if (((mQuotingMode)QP::getInt("mode") == mQuotingMode::PingPong or QP::matchPings())
          and !pgSafety["buyPing"].get<double>() and ((mPingAt)QP::getInt("pingAt") == mPingAt::StopPings or (mPingAt)QP::getInt("pingAt") == mPingAt::BidSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedAskSide
            or (totalQuotePosition>buySize and ((mPingAt)QP::getInt("pingAt") == mPingAt::DepletedSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedBidSide))
        )) {
          qeAskStatus = !pgSafety["buyPing"].get<double>()
            ? mQuoteStatus::WaitingPing
            : mQuoteStatus::DepletedFunds;
          rawQuote["askPx"] = 0;
          rawQuote["askSz"] = 0;
        }
        if (pgSafety["buy"].get<double>() > (QP::getDouble("tradesPerMinute") * superTradesMultipliers[0])) {
          qeBidStatus = mQuoteStatus::MaxTradesSeconds;
          rawQuote["bidPx"] = 0;
          rawQuote["bidSz"] = 0;
        }
        if (((mQuotingMode)QP::getInt("mode") == mQuotingMode::PingPong or QP::matchPings())
          and !pgSafety["sellPong"].get<double>() and ((mPingAt)QP::getInt("pingAt") == mPingAt::StopPings or (mPingAt)QP::getInt("pingAt") == mPingAt::AskSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedBidSide
            or (totalBasePosition>sellSize and ((mPingAt)QP::getInt("pingAt") == mPingAt::DepletedSide or (mPingAt)QP::getInt("pingAt") == mPingAt::DepletedAskSide))
        )) {
          qeBidStatus = !pgSafety["sellPong"].get<double>()
            ? mQuoteStatus::WaitingPing
            : mQuoteStatus::DepletedFunds;
          rawQuote["bidPx"] = 0;
          rawQuote["bidSz"] = 0;
        }
        if (rawQuote["bidPx"].get<double>()) {
          rawQuote["bidPx"] = FN::roundSide(rawQuote["bidPx"].get<double>(), gw->minTick, mSide::Bid);
          rawQuote["bidPx"] = fmax(0, rawQuote["bidPx"].get<double>());
        }
        if (rawQuote["askPx"].get<double>()) {
          rawQuote["askPx"] = FN::roundSide(rawQuote["askPx"].get<double>(), gw->minTick, mSide::Ask);
          rawQuote["askPx"] = fmax(rawQuote["bidPx"].get<double>() + gw->minTick, rawQuote["askPx"].get<double>());
        }
        if (rawQuote["askSz"].get<double>()) {
          if (rawQuote["askSz"].get<double>() > totalBasePosition)
            rawQuote["askSz"] = (!_rawBidSz or _rawBidSz > totalBasePosition)
              ? totalBasePosition : _rawBidSz;
          rawQuote["askSz"] = FN::roundDown(fmax(gw->minSize, rawQuote["askSz"].get<double>()), 1e-8);
          rawQuote["isAskPong"] = (pgSafety["buyPing"].get<double>() and rawQuote["askPx"].get<double>() and rawQuote["askPx"].get<double>() >= pgSafety["buyPing"].get<double>() + widthPong);
        } else rawQuote["isAskPong"] = false;
        if (rawQuote["bidSz"].get<double>()) {
          if (rawQuote["bidSz"].get<double>() > totalQuotePosition)
            rawQuote["bidSz"] = (!_rawAskSz or _rawAskSz > totalQuotePosition)
              ? totalQuotePosition : _rawAskSz;
          rawQuote["bidSz"] = FN::roundDown(fmax(gw->minSize, rawQuote["bidSz"].get<double>()), 1e-8);
          rawQuote["isBidPong"] = (pgSafety["sellPong"].get<double>() and rawQuote["bidPx"].get<double>() and rawQuote["bidPx"].get<double>() <= pgSafety["sellPong"].get<double>() - widthPong);
        } else rawQuote["isBidPong"] = false;
        return rawQuote;
      };
      static json quote(double widthPing, double buySize, double sellSize) {
        mQuotingMode k = (mQuotingMode)QP::getInt("mode");
        if (qeQuotingMode.find(k) == qeQuotingMode.end()) { cout << FN::uiT() << "Errrror: Invalid quoting mode." << endl; exit(1); }
        return (*qeQuotingMode[k])(widthPing, buySize, sellSize);
      };
      static json quoteAtTopOfMarket() {
        json topBid = mGWmktFilter["/bids/0/size"_json_pointer].get<double>() > gw->minTick
          ? mGWmktFilter["/bids/0"_json_pointer] : mGWmktFilter["/bids/1"_json_pointer];
        json topAsk = mGWmktFilter["/asks/0/size"_json_pointer].get<double>() > gw->minTick
          ? mGWmktFilter["/asks/0"_json_pointer] : mGWmktFilter["/asks/1"_json_pointer];
        return {
          {"bidPx", topBid["price"].get<double>()},
          {"bidSz", topBid["size"].get<double>()},
          {"askPx", topAsk["price"].get<double>()},
          {"askSz", topAsk["size"].get<double>()}
        };
      };
      static json calcTopOfMarket(double widthPing, double buySize, double sellSize) {
        json k = quoteAtTopOfMarket();
        if ((mQuotingMode)QP::getInt("mode") != mQuotingMode::Join and k["bidSz"].get<double>() > 0.2)
          k["bidPx"] = k["bidPx"].get<double>() + gw->minTick;
        k["bidPx"] = fmin(mgFairValue - widthPing / 2.0, k["bidPx"].get<double>());
        if ((mQuotingMode)QP::getInt("mode") != mQuotingMode::Join and k["askSz"].get<double>() > 0.2)
          k["askPx"] = k["askPx"].get<double>() - gw->minTick;
        k["askPx"] = fmin(mgFairValue + widthPing / 2.0, k["askPx"].get<double>());
        k["bidSz"] = buySize;
        k["askSz"] = sellSize;
        return k;
      };
      static json calcInverseTopOfMarket(double widthPing, double buySize, double sellSize) {
        json k = quoteAtTopOfMarket();
        double mktWidth = abs(k["askPx"].get<double>() - k["bidPx"].get<double>());
        if (mktWidth > widthPing) {
          k["askPx"] = k["askPx"].get<double>() + widthPing;
          k["bidPx"] = k["bidPx"].get<double>() - widthPing;
        }
        if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::InverseTop) {
          if (k["bidSz"].get<double>() > .2) k["bidPx"] = k["bidPx"].get<double>() + gw->minTick;
          if (k["askSz"].get<double>() > .2) k["askPx"] = k["askPx"].get<double>() - gw->minTick;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k["askPx"] = k["askPx"].get<double>() + widthPing / 4.0;
          k["bidPx"] = k["bidPx"].get<double>() - widthPing / 4.0;
        }
        k["bidSz"] = buySize;
        k["askSz"] = sellSize;
        return k;
      };
      static json calcMidOfMarket(double widthPing, double buySize, double sellSize) {
        return {
          {"bidPx", fmax(mgFairValue - widthPing, 0)},
          {"bidSz", buySize},
          {"askPx", mgFairValue + widthPing},
          {"askSz", sellSize}
        };
      };
      static json calcDepthOfMarket(double depth, double buySize, double sellSize) {
        double bidPx = mGWmktFilter["/bids/0/price"_json_pointer].get<double>();
        double bidDepth = 0;
        for (json::iterator it = mGWmktFilter["bids"].begin(); it != mGWmktFilter["bids"].end(); ++it) {
          bidDepth += (*it)["size"].get<double>();
          if (bidDepth >= depth) break;
          else bidPx = (*it)["price"].get<double>();
        }
        double askPx = mGWmktFilter["/asks/0/price"_json_pointer].get<double>();
        double askDepth = 0;
        for (json::iterator it = mGWmktFilter["asks"].begin(); it != mGWmktFilter["asks"].end(); ++it) {
          askDepth += (*it)["size"].get<double>();
          if (askDepth >= depth) break;
          else askPx = (*it)["price"].get<double>();
        }
        return {
          {"bidPx", bidPx},
          {"bidSz", buySize},
          {"askPx", askPx},
          {"askSz", sellSize}
        };
      };
      static mQuoteStatus checkCrossedQuotes(mSide side) {
        bool bidORask = side == mSide::Bid;
        if (qeQuote[bidORask?"bid":"ask"].is_null()) return bidORask ? qeBidStatus : qeAskStatus;
        if (qeQuote[bidORask?"ask":"bid"].is_null()) return mQuoteStatus::Live;
        double price = qeQuote[bidORask?"/bid/price"_json_pointer:"/ask/price"_json_pointer].get<double>();
        double ecirp = qeQuote[bidORask?"/ask/price"_json_pointer:"/bid/price"_json_pointer].get<double>();
        if (bidORask
          ? price >= ecirp
          : price <= ecirp
        ) {
          cout << FN::uiT() << "QE Crossing quote detected! new " << (bidORask ? "Bid" : "Ask") << "Side quote at " << price << " crossed with " << (bidORask ? "Ask" : "Bid") << "Side quote at " << ecirp << "." << endl;
          return mQuoteStatus::Crossed;
        }
        return mQuoteStatus::Live;
     };
      static void updateQuote (json q, mSide side) {
        multimap<double, json> orderSide = orderCacheSide(side);
        bool eq = false;
        for (multimap<double, json>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (it->first == q["price"].get<double>()) { eq = true; break; }
        if ((mQuotingMode)QP::getInt("mode") != mQuotingMode::AK47) {
          if (orderSide.size()) {
            if (!eq) modify(side, q);
          } else start(side, q);
          return;
        }
        if (!eq and orderSide.size() >= (size_t)QP::getInt("bullets"))
          modify(side, q);
        else start(side, q);
      };
      static multimap<double, json> orderCacheSide(mSide side) {
        multimap<double, json> orderSide;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if ((mSide)it->second["side"].get<int>() == side)
            orderSide.insert(pair<double, json>(it->second["price"].get<double>(), it->second));
        return orderSide;
      };
      static void modify(mSide side, json q) {
        if ((mQuotingMode)QP::getInt("mode") == mQuotingMode::AK47)
          stopWorstQuote(side);
        else stopAllQuotes(side);
        start(side, q);
      };
      static void start(mSide side, json q) {
        if (QP::getDouble("delayAPI")) {
          unsigned long nextStart = qeNextT + (6e+4/QP::getDouble("delayAPI"));
          if ((double)nextStart - (double)FN::T() > 0) {
            qeNextQuote.clear();
            qeNextQuote[side] = q;
            thread([&]() {
              unsigned int qeThread_ = ++qeThread;
              unsigned long nextStart_ = nextStart;
              while (qeThread_ == qeThread) {
                if ((double)nextStart_ - (double)FN::T() > 0)
                  this_thread::sleep_for(chrono::milliseconds(100));
                else {
                  start(qeNextQuote.begin()->first, qeNextQuote.begin()->second);
                  break;
                }
              }
            }).detach();
            return;
          }
          qeNextT = FN::T();
        }
        double price = q["price"].get<double>();
        multimap<double, json> orderSide = orderCacheSide(side);
        bool eq = false;
        for (multimap<double, json>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
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
              ? orderSide.rbegin()->second["price"].get<double>()
              : orderSide.begin()->second["price"].get<double>());
            eq = false;
            for (multimap<double, json>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
              if (price == it->first
                or ((price + (QP::getDouble("range") - 1e-2)) >= it->first
                  and (price - (QP::getDouble("range") - 1e-2)) <= it->first)
              ) { eq = true; break; }
            if (eq) return;
            stopWorstsQuotes(side, q["price"].get<double>());
            price = FN::roundNearest(price, gw->minTick);
          } else return;
        }
        OG::sendOrder(side, price, q["size"].get<double>(), mOrderType::Limit, mTimeInForce::GTC, q["isPong"].get<bool>(), true);
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
        multimap<double, json> orderSide = orderCacheSide(side);
        for (multimap<double, json>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          if (side == mSide::Bid
            ? price < it->second["price"].get<double>()
            : price > it->second["price"].get<double>()
          ) OG::cancelOrder(it->second["orderId"].get<string>());
      };
      static void stopWorstQuote(mSide side) {
        multimap<double, json> orderSide = orderCacheSide(side);
        if (orderSide.size())
          OG::cancelOrder(side == mSide::Bid
            ? orderSide.begin()->second["orderId"].get<string>()
            : orderSide.rbegin()->second["orderId"].get<string>()
          );
      };
      static void stopAllQuotes(mSide side) {
        multimap<double, json> orderSide = orderCacheSide(side);
        for (multimap<double, json>::iterator it = orderSide.begin(); it != orderSide.end(); ++it)
          OG::cancelOrder(it->second["orderId"].get<string>());
      };
  };
}

#endif
