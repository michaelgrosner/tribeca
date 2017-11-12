#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  mLevels mgLevelsFilter;
  vector<mTrade> mgTrades;
  double mgFairValue = 0;
  double mgEwmaL = 0;
  double mgEwmaM = 0;
  double mgEwmaS = 0;
  double mgEwmaP = 0;
  double mgEwmaSMUDiff = 0;
  double mgEwmaSM = 0;
  double mgEwmaSU = 0;
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
  class MG: public Klass {
    protected:
      void load() {
        json k = ((DB*)evDB)->load(uiTXT::MarketData);
        if (k.size()) {
          for (json::reverse_iterator it = k.rbegin(); it != k.rend(); ++it) {
            if (it->value("time", (unsigned long)0)+qp.quotingStdevProtectionPeriods*1e+3<FN::T()) continue;
            mgStatFV.push_back(it->value("fv", 0.0));
            mgStatBid.push_back(it->value("bid", 0.0));
            mgStatAsk.push_back(it->value("ask", 0.0));
            mgStatTop.push_back(it->value("bid", 0.0));
            mgStatTop.push_back(it->value("ask", 0.0));
          }
          calcStdev();
        }
        FN::log("DB", string("loaded ") + to_string(mgStatFV.size()) + " STDEV Periods");
        if (argEwmaLong) mgEwmaL = argEwmaLong;
        if (argEwmaMedium) mgEwmaM = argEwmaMedium;
        if (argEwmaShort) mgEwmaS = argEwmaShort;
        k = ((DB*)evDB)->load(uiTXT::EWMAChart);
        if (k.size()) {
          k = k.at(0);
          if (!mgEwmaL and k.value("time", (unsigned long)0)+qp.longEwmaPeriods*6e+4>FN::T())
            mgEwmaL = k.value("ewmaLong", 0.0);
          if (!mgEwmaM and k.value("time", (unsigned long)0)+qp.mediumEwmaPeriods*6e+4>FN::T())
            mgEwmaM = k.value("ewmaMedium", 0.0);
          if (!mgEwmaS and k.value("time", (unsigned long)0)+qp.shortEwmaPeriods*6e+4>FN::T())
            mgEwmaS = k.value("ewmaShort", 0.0);
        }
        FN::log(argEwmaLong ? "ARG" : "DB", string("loaded EWMA Long = ") + to_string(mgEwmaL));
        FN::log(argEwmaMedium ? "ARG" : "DB", string("loaded EWMA Medium = ") + to_string(mgEwmaM));
        FN::log(argEwmaShort ? "ARG" : "DB", string("loaded EWMA Short = ") + to_string(mgEwmaS));
      };
      void waitData() {
        gw->evDataTrade = [&](mTrade k) {
          if (argDebugEvents) FN::log("DEBUG", "EV MG evDataTrade");
          tradeUp(k);
        };
        gw->evDataLevels = [&](mLevels k) {
          if (argDebugEvents) FN::log("DEBUG", "EV MG evDataLevels");
          levelUp(k);
        };
      };
      void waitUser() {
        ((UI*)evUI)->evSnap(uiTXT::MarketTrade, &onSnapTrade);
        ((UI*)evUI)->evSnap(uiTXT::FairValue, &onSnapFair);
        ((UI*)evUI)->evSnap(uiTXT::EWMAChart, &onSnapEwma);
      };
    public:
      bool empty() {
        return (!mgLevelsFilter.bids.size() or !mgLevelsFilter.asks.size());
      };
      void calcStats() {
        static int mgT = 0;
        if (++mgT == 60) {
          mgT = 0;
          ewmaPUp();
          ewmaSMUUp();
          ewmaUp();
        }
        stdevPUp();
      };
      void calcFairValue() {
        if (empty()) return;
        double mgFairValue_ = mgFairValue;
        double topAskPrice = mgLevelsFilter.asks.begin()->price;
        double topBidPrice = mgLevelsFilter.bids.begin()->price;
        double topAskSize = mgLevelsFilter.asks.begin()->size;
        double topBidSize = mgLevelsFilter.bids.begin()->size;
        if (!topAskPrice or !topBidPrice or !topAskSize or !topBidSize) return;
        mgFairValue = FN::roundNearest(
          qp.fvModel == mFairValueModel::BBO
            ? (topAskPrice + topBidPrice) / 2
            : (topAskPrice * topAskSize + topBidPrice * topBidSize) / (topAskSize + topBidSize),
          gw->minTick
        );
        if (!mgFairValue or (mgFairValue_ and abs(mgFairValue - mgFairValue_) < gw->minTick)) return;
        gw->evDataWallet(mWallet());
        ((UI*)evUI)->evSend(uiTXT::FairValue, {{"price", mgFairValue}}, true);
      };
    private:
      function<json()> onSnapTrade = []() {
        json k;
        for (unsigned i=0; i<mgTrades.size(); ++i)
          k.push_back(mgTrades[i]);
        return k;
      };
      function<json()> onSnapFair = []() {
        return (json){{{"price", mgFairValue}}};
      };
      function<json()> onSnapEwma = []() {
        return (json){{
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
          {"ewmaSMUDiff", mgEwmaSMUDiff},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"fairValue", mgFairValue}
        }};
      };
      void stdevPUp() {
        if (empty()) return;
        double topBid = mgLevelsFilter.bids.begin()->price;
        double topAsk = mgLevelsFilter.bids.begin()->price;
        if (!topBid or !topAsk) return;
        mgStatFV.push_back(mgFairValue);
        mgStatBid.push_back(topBid);
        mgStatAsk.push_back(topAsk);
        mgStatTop.push_back(topBid);
        mgStatTop.push_back(topAsk);
        calcStdev();
        ((DB*)evDB)->insert(uiTXT::MarketData, {
          {"fv", mgFairValue},
          {"bid", topBid},
          {"ask", topAsk},
          {"time", FN::T()},
        }, false, "NULL", FN::T() - 1e+3 * qp.quotingStdevProtectionPeriods);
      };
      void tradeUp(mTrade k) {
        k.exchange = gw->exchange;
        k.pair = mPair(gw->base, gw->quote);
        k.time = FN::T();
        mgTrades.push_back(k);
        if (mgTrades.size()>69) mgTrades.erase(mgTrades.begin());
        ((UI*)evUI)->evSend(uiTXT::MarketTrade, k, false);
      };
      void levelUp(mLevels k) {
        static unsigned long lastUp = 0;
        filter(k);
        if (lastUp+369 > FN::T()) return;
        ((UI*)evUI)->evSend(uiTXT::MarketData, k, true);
        lastUp = FN::T();
      };
      void ewmaUp() {
        calcEwma(&mgEwmaL, qp.longEwmaPeriods);
        calcEwma(&mgEwmaM, qp.mediumEwmaPeriods);
        calcEwma(&mgEwmaS, qp.shortEwmaPeriods);
        calcTargetPos();
        ((EV*)evEV)->mgTargetPosition();
        ((UI*)evUI)->evSend(uiTXT::EWMAChart, {
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
          {"ewmaSMUDiff", mgEwmaSMUDiff},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"fairValue", mgFairValue}
        }, true);
        ((DB*)evDB)->insert(uiTXT::EWMAChart, {
          {"ewmaLong", mgEwmaL},
          {"ewmaMedium", mgEwmaM},
          {"ewmaShort", mgEwmaS},
          {"time", FN::T()}
        });
      };
      void ewmaPUp() {
        calcEwma(&mgEwmaP, qp.quotingEwmaProtectionPeriods);
        ((EV*)evEV)->mgEwmaQuoteProtection();
      };
      void ewmaSMUUp() {
        calcEwma(&mgEwmaSM, qp.quotingEwmaSMPeriods);
        calcEwma(&mgEwmaSU, qp.quotingEwmaSUPeriods);
        if(mgEwmaSM && mgEwmaSU)
		      mgEwmaSMUDiff = ((mgEwmaSU * 100) / mgEwmaSM) - 100;
        ((EV*)evEV)->mgEwmaSMUProtection();
      };
      void filter(mLevels k) {
        mgLevelsFilter = k;
        if (empty()) return;
        ogMutex.lock();
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          filter(mSide::Bid == it->second.side ? &mgLevelsFilter.bids : &mgLevelsFilter.asks, it->second);
        ogMutex.unlock();
        if (!empty()) {
          calcFairValue();
          ((EV*)evEV)->mgLevels();
        }
      };
      void filter(vector<mLevel>* k, mOrder o) {
        for (vector<mLevel>::iterator it = k->begin(); it != k->end();)
          if (abs(it->price - o.price) < gw->minTick) {
            it->size = it->size - o.quantity;
            if (it->size < gw->minTick) k->erase(it);
            break;
          } else ++it;
      };
      void cleanStdev() {
        size_t periods = (size_t)qp.quotingStdevProtectionPeriods;
        if (mgStatFV.size()>periods) mgStatFV.erase(mgStatFV.begin(), mgStatFV.end()-periods);
        if (mgStatBid.size()>periods) mgStatBid.erase(mgStatBid.begin(), mgStatBid.end()-periods);
        if (mgStatAsk.size()>periods) mgStatAsk.erase(mgStatAsk.begin(), mgStatAsk.end()-periods);
        if (mgStatTop.size()>periods*2) mgStatTop.erase(mgStatTop.begin(), mgStatTop.end()-(periods*2));
      };
      void calcStdev() {
        cleanStdev();
        if (mgStatFV.size() < 2 or mgStatBid.size() < 2 or mgStatAsk.size() < 2 or mgStatTop.size() < 4) return;
        double k = qp.quotingStdevProtectionFactor;
        mgStdevFV = calcStdev(mgStatFV, k, &mgStdevFVMean);
        mgStdevBid = calcStdev(mgStatBid, k, &mgStdevBidMean);
        mgStdevAsk = calcStdev(mgStatAsk, k, &mgStdevAskMean);
        mgStdevTop = calcStdev(mgStatTop, k, &mgStdevTopMean);
      };
      double calcStdev(vector<double> a, double f, double *mean) {
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
      void calcEwma(double *k, int periods) {
        if (*k) {
          double alpha = (double)2 / (periods + 1);
          *k = alpha * mgFairValue + (1 - alpha) * *k;
        } else *k = mgFairValue;
      };
      void calcTargetPos() {
        mgSMA3.push_back(mgFairValue);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
          SMA3 += *it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / mgEwmaL) - 100);
          double newEwmacrossing = ((mgEwmaS * 100 / mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qp.ewmaSensiblityPercentage);
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        mgTargetPos = newTargetPosition;
      };
  };
}

#endif
