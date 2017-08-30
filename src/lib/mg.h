#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  int mgT = 0;
  vector<mGWmt> mGWmt_;
  json mGWmkt;
  json mGWmktFilter;
  double mgFairValue = 0;
  double mgEwmaL = 0;
  double mgEwmaM = 0;
  double mgEwmaS = 0;
  double mgEwmaP = 0;
  vector<double> mgSMA3;
  vector<double> mgStatFV;
  vector<double> mgStatBid;
  vector<double> mgStatAsk;
  vector<double> mgStatTop;
  double mgStdevFV = 0;
  double mgStdevFVMean = 0;
  double mgStdevBid = 0;
  double mgStdevBidMean = 0;
  double mgStdevAsk = 0;
  double mgStdevAskMean = 0;
  double mgStdevTop = 0;
  double mgStdevTopMean = 0;
  double mgTargetPos = 0;
  class MG {
    public:
      static void main(Local<Object> exports) {
        load();
        EV::evOn("MarketTradeGateway", [](json k) {
          tradeUp(k);
        });
        EV::evOn("MarketDataGateway", [](json o) {
          levelUp(o);
        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        UI::uiSnap(uiTXT::EWMAChart, &onSnapEwma);
      };
      static bool empty() {
        return (mGWmktFilter.is_null()
          or mGWmktFilter["bids"].is_null()
          or mGWmktFilter["asks"].is_null()
        );
      };
      static void calcStats() {
        if (++mgT == 60) {
          mgT = 0;
          ewmaPUp();
          ewmaUp();
        }
        stdevPUp();
      };
      static void calcFairValue() {
        if (empty()) return;
        double mgFairValue_ = mgFairValue;
        mgFairValue = FN::roundNearest(
          mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
            ? (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>()) / 2
            : (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() * mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>() * mGWmktFilter["/bids/0/size"_json_pointer].get<double>()) / (mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/size"_json_pointer].get<double>()),
          gw->minTick
        );
        if (!mgFairValue or (mgFairValue_ and abs(mgFairValue - mgFairValue_) < gw->minTick)) return;
        EV::evUp("PositionGateway");
        UI::uiSend(uiTXT::FairValue, {{"price", mgFairValue}}, true);
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::MarketData);
        if (k.size()) {
          for (json::iterator it = k.begin(); it != k.end(); ++it) {
            if ((*it)["time"].is_number() and (*it)["time"].get<unsigned long>()+qpRepo["quotingStdevProtectionPeriods"].get<int>()*1e+3<FN::T()) continue;
            mgStatFV.push_back((*it)["fv"].get<double>());
            mgStatBid.push_back((*it)["bid"].get<double>());
            mgStatAsk.push_back((*it)["ask"].get<double>());
            mgStatTop.push_back((*it)["bid"].get<double>());
            mgStatTop.push_back((*it)["ask"].get<double>());
          }
          calcStdev();
        }
        cout << FN::uiT() << "DB loaded " << mgStatFV.size() << " STDEV Periods." << endl;
        k = DB::load(uiTXT::EWMAChart);
        if (k.size()) {
          if (k["/0/ewmaLong"_json_pointer].is_number() and (!k["/0/time"_json_pointer].is_number() or k["/0/time"_json_pointer].get<unsigned long>()+qpRepo["longEwmaPeriods"].get<int>()*6e+4>FN::T()))
            mgEwmaL = k["/0/ewmaLong"_json_pointer].get<double>();
          if (k["/0/ewmaMedium"_json_pointer].is_number() and (!k["/0/time"_json_pointer].is_number() or k["/0/time"_json_pointer].get<unsigned long>()+qpRepo["mediumEwmaPeriods"].get<int>()*6e+4>FN::T()))
            mgEwmaM = k["/0/ewmaMedium"_json_pointer].get<double>();
          if (k["/0/ewmaShort"_json_pointer].is_number() and (!k["/0/time"_json_pointer].is_number() or k["/0/time"_json_pointer].get<unsigned long>()+qpRepo["shortEwmaPeriods"].get<int>()*6e+4>FN::T()))
            mgEwmaS = k["/0/ewmaShort"_json_pointer].get<double>();
        }
        cout << FN::uiT() << "DB loaded EWMA Long = " << mgEwmaL << "." << endl;
        cout << FN::uiT() << "DB loaded EWMA Medium = " << mgEwmaM << "." << endl;
        cout << FN::uiT() << "DB loaded EWMA Short = " << mgEwmaS << "." << endl;
      };
      static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
          k.push_back(tradeUp(mGWmt_[i]));
        return k;
      };
      static json onSnapFair(json z) {
        return {{{"price", mgFairValue}}};
      };
      static json onSnapEwma(json z) {
        return {{
          {"stdevWidth", {
            {"fv", mgStdevFV},
            {"fvMean", mgStdevFVMean},
            {"tops", mgStdevTop},
            {"topsMean", mgStdevTopMean},
            {"bid", mgStdevBid},
            {"bidMean", mgStdevBidMean},
            {"ask", mgStdevAsk},
            {"askMean", mgStdevAskMean}
          }},
          {"ewmaQuote", mgEwmaP},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"fairValue", mgFairValue}
        }};
      };
      static void stdevPUp() {
        if (empty()) return;
        mgStatFV.push_back(mgFairValue);
        mgStatBid.push_back(mGWmktFilter["/bids/0/price"_json_pointer].get<double>());
        mgStatAsk.push_back(mGWmktFilter["/asks/0/price"_json_pointer].get<double>());
        mgStatTop.push_back(mGWmktFilter["/bids/0/price"_json_pointer].get<double>());
        mgStatTop.push_back(mGWmktFilter["/asks/0/price"_json_pointer].get<double>());
        calcStdev();
        DB::insert(uiTXT::MarketData, {
          {"fv", mgFairValue},
          {"bid", mGWmktFilter["/bids/0/price"_json_pointer].get<double>()},
          {"ask", mGWmktFilter["/bids/0/price"_json_pointer].get<double>()},
          {"time", FN::T()},
        }, false, "NULL", FN::T() - 1e+3 * qpRepo["quotingStdevProtectionPeriods"].get<int>());
      };
      static void tradeUp(json k) {
        mGWmt t(
          gw->exchange,
          gw->base,
          gw->quote,
          k["price"].get<double>(),
          k["size"].get<double>(),
          FN::T(),
          (mSide)k["make_side"].get<int>()
        );
        mGWmt_.push_back(t);
        if (mGWmt_.size()>69) mGWmt_.erase(mGWmt_.begin());
        EV::evUp("MarketTrade");
        UI::uiSend(uiTXT::MarketTrade, tradeUp(t));
      };
      static void levelUp(json k) {
        mGWmkt = k;
        filter();
        UI::uiSend(uiTXT::MarketData, k, true);
      };
      static json tradeUp(mGWmt t) {
        json o = {
          {"exchange", (int)t.exchange},
          {"pair", {{"base", t.base}, {"quote", t.quote}}},
          {"price", t.price},
          {"size", t.size},
          {"time", t.time},
          {"make_size", (int)t.make_side}
        };
        return o;
      };
      static void ewmaUp() {
        calcEwma(&mgEwmaL, qpRepo["longEwmaPeriods"].get<int>());
        calcEwma(&mgEwmaM, qpRepo["mediumEwmaPeriods"].get<int>());
        calcEwma(&mgEwmaS, qpRepo["shortEwmaPeriods"].get<int>());
        calcTargetPos();
        EV::evUp("PositionBroker");
        UI::uiSend(uiTXT::EWMAChart, {
          {"stdevWidth", {
            {"fv", mgStdevFV},
            {"fvMean", mgStdevFVMean},
            {"tops", mgStdevTop},
            {"topsMean", mgStdevTopMean},
            {"bid", mgStdevBid},
            {"bidMean", mgStdevBidMean},
            {"ask", mgStdevAsk},
            {"askMean", mgStdevAskMean}
          }},
          {"ewmaQuote", mgEwmaP},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"fairValue", mgFairValue}
        }, true);
        DB::insert(uiTXT::EWMAChart, {
          {"ewmaLong", mgEwmaL},
          {"ewmaMedium", mgEwmaM},
          {"ewmaShort", mgEwmaS},
          {"time", FN::T()}
        });
      };
      static void ewmaPUp() {
        calcEwma(&mgEwmaP, qpRepo["quotingEwmaProtectionPeriods"].get<int>());
        EV::evUp("EWMAProtectionCalculator");
      };
      static void filter() {
        mGWmktFilter = mGWmkt;
        if (empty()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          filter(mSide::Bid == (mSide)it->second["side"].get<int>() ? "bids" : "asks", it->second);
        if (!empty()) {
          calcFairValue();
          EV::evUp("FilteredMarket");
        }
      };
      static void filter(string k, json o) {
        for (json::iterator it = mGWmktFilter[k].begin(); it != mGWmktFilter[k].end();)
          if (abs((*it)["price"].get<double>() - o["price"].get<double>()) < gw->minTick) {
            (*it)["size"] = (*it)["size"].get<double>() - o["quantity"].get<double>();
            if ((*it)["size"].get<double>() < gw->minTick) mGWmktFilter[k].erase(it);
            break;
          } else ++it;
      };
      static void cleanStdev() {
        size_t periods = (size_t)qpRepo["quotingStdevProtectionPeriods"].get<int>();
        if (mgStatFV.size()>periods) mgStatFV.erase(mgStatFV.begin(), mgStatFV.end()-periods);
        if (mgStatBid.size()>periods) mgStatBid.erase(mgStatBid.begin(), mgStatBid.end()-periods);
        if (mgStatAsk.size()>periods) mgStatAsk.erase(mgStatAsk.begin(), mgStatAsk.end()-periods);
        if (mgStatTop.size()>periods*2) mgStatTop.erase(mgStatTop.begin(), mgStatTop.end()-(periods*2));
      };
      static void calcStdev() {
        cleanStdev();
        if (mgStatFV.size() < 2 or mgStatBid.size() < 2 or mgStatAsk.size() < 2 or mgStatTop.size() < 4) return;
        double k = qpRepo["quotingStdevProtectionFactor"].get<double>();
        mgStdevFV = calcStdev(mgStatFV, k, &mgStdevFVMean);
        mgStdevBid = calcStdev(mgStatBid, k, &mgStdevBidMean);
        mgStdevAsk = calcStdev(mgStatAsk, k, &mgStdevAskMean);
        mgStdevTop = calcStdev(mgStatTop, k, &mgStdevTopMean);
      };
      static double calcStdev(vector<double> a, double f, double *mean) {
        int n = a.size();
        if (n == 0) return 0.0;
        double sum = 0;
        for (int i = 0; i < n; ++i) sum += a[i];
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (int i = 0; i < n; ++i) {
          double diff = a[i] - *mean;
          sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * f;
      };
      static void calcEwma(double *k, int periods) {
        if (*k) {
          double alpha = (double)2 / (periods + 1);
          *k = alpha * mgFairValue + (1 - alpha) * *k;
        } else *k = mgFairValue;
      };
      static void calcTargetPos() {
        mgSMA3.push_back(mgFairValue);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
          SMA3 += *it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / mgEwmaL) - 100);
          double newEwmacrossing = ((mgEwmaS * 100 / mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        } else if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        mgTargetPos = newTargetPosition;
      };
  };
}

#endif
