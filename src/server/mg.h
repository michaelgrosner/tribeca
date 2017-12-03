#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass {
    private:
      vector<mTrade> trades;
      double mgEwmaVL = 0;
      double mgEwmaL = 0;
      double mgEwmaM = 0;
      double mgEwmaS = 0;
      double mgEwmaSM = 0;
      vector<double> mgSMA3;
      vector<double> mgStatFV;
      vector<double> mgStatBid;
      vector<double> mgStatAsk;
      vector<double> mgStatTop;
      unsigned int mgT_60s = 0;
      unsigned long mgT_369ms = 0;
    public:
      mLevels levels;
      double fairValue = 0;
      double targetPosition = 0;
      double mgStdevTop = 0;
      double mgStdevTopMean = 0;
      double mgEwmaP = 0;
      double mgEwmaSMUDiff = 0;
      double mgStdevFV = 0;
      double mgStdevFVMean = 0;
      double mgStdevBid = 0;
      double mgStdevBidMean = 0;
      double mgStdevAsk = 0;
      double mgStdevAskMean = 0;
      double mgEwmaSU = 0;
    protected:
      void load() {
        json k = ((DB*)memory)->load(uiTXT::MarketData);
        if (k.size()) {
          for (json::reverse_iterator it = k.rbegin(); it != k.rend(); ++it) {
            if (it->value("time", (unsigned long)0)+qp->quotingStdevProtectionPeriods*1e+3<FN::T()) continue;
            mgStatFV.push_back(it->value("fv", 0.0));
            mgStatBid.push_back(it->value("bid", 0.0));
            mgStatAsk.push_back(it->value("ask", 0.0));
            mgStatTop.push_back(it->value("bid", 0.0));
            mgStatTop.push_back(it->value("ask", 0.0));
          }
          calcStdev();
        }
        FN::log("DB", string("loaded ") + to_string(mgStatFV.size()) + " STDEV Periods");
        if (((CF*)config)->argEwmaVeryLong) mgEwmaVL = ((CF*)config)->argEwmaVeryLong;
        if (((CF*)config)->argEwmaLong) mgEwmaL = ((CF*)config)->argEwmaLong;
        if (((CF*)config)->argEwmaMedium) mgEwmaM = ((CF*)config)->argEwmaMedium;
        if (((CF*)config)->argEwmaShort) mgEwmaS = ((CF*)config)->argEwmaShort;
        k = ((DB*)memory)->load(uiTXT::EWMAChart);
        if (k.size()) {
          k = k.at(0);
          if (!mgEwmaVL and k.value("time", (unsigned long)0) + qp->veryLongEwmaPeriods * 6e+4 > FN::T())
            mgEwmaVL = k.value("ewmaVeryLong", 0.0);
          if (!mgEwmaL and k.value("time", (unsigned long)0) + qp->longEwmaPeriods * 6e+4 > FN::T())
            mgEwmaL = k.value("ewmaLong", 0.0);
          if (!mgEwmaM and k.value("time", (unsigned long)0) + qp->mediumEwmaPeriods * 6e+4 > FN::T())
            mgEwmaM = k.value("ewmaMedium", 0.0);
          if (!mgEwmaS and k.value("time", (unsigned long)0) + qp->shortEwmaPeriods * 6e+4 > FN::T())
            mgEwmaS = k.value("ewmaShort", 0.0);
          if (k.value("time", (unsigned long)0) + qp->quotingEwmaSMPeriods * 6e+4 > FN::T())
            mgEwmaSM = k.value("mgEwmaSM", 0.0);
          if (k.value("time", (unsigned long)0) + qp->quotingEwmaSUPeriods * 6e+4 > FN::T())
            mgEwmaSU = k.value("mgEwmaSU", 0.0);
        }
        if (mgEwmaVL) FN::log(((CF*)config)->argEwmaVeryLong ? "ARG" : "DB", string("loaded EWMA VeryLong = ") + to_string(mgEwmaVL));
        if (mgEwmaL)  FN::log(((CF*)config)->argEwmaLong ? "ARG" : "DB", string("loaded EWMA Long = ") + to_string(mgEwmaL));
        if (mgEwmaM)  FN::log(((CF*)config)->argEwmaMedium ? "ARG" : "DB", string("loaded EWMA Medium = ") + to_string(mgEwmaM));
        if (mgEwmaS)  FN::log(((CF*)config)->argEwmaShort ? "ARG" : "DB", string("loaded EWMA Short = ") + to_string(mgEwmaS));
        if (mgEwmaSM and mgEwmaSU) FN::log("DB", string("loaded EWMA Trend micro/ultra = ") + to_string(mgEwmaSM) + "/" + to_string(mgEwmaSU));
      };
      void waitData() {
        gw->evDataTrade = [&](mTrade k) {
          ((EV*)events)->debug("MG evDataTrade");
          tradeUp(k);
        };
        gw->evDataLevels = [&](mLevels k) {
          ((EV*)events)->debug("MG evDataLevels");
          levelUp(k);
        };
      };
      void waitUser() {
        ((UI*)client)->welcome(uiTXT::MarketTrade, &helloTrade);
        ((UI*)client)->welcome(uiTXT::FairValue, &helloFair);
        ((UI*)client)->welcome(uiTXT::EWMAChart, &helloEwma);
      };
    public:
      bool empty() {
        return !levels.bids.size() or !levels.asks.size();
      };
      void calcStats() {
        if (++mgT_60s == 60) {
          mgT_60s = 0;
          ewmaPUp();
          ewmaSMUUp();
          ewmaUp();
        }
        stdevPUp();
      };
      void calcFairValue() {
        if (empty()) return;
        double fairValue_ = fairValue;
        double topAskPrice = levels.asks.begin()->price;
        double topBidPrice = levels.bids.begin()->price;
        double topAskSize = levels.asks.begin()->size;
        double topBidSize = levels.bids.begin()->size;
        if (!topAskPrice or !topBidPrice or !topAskSize or !topBidSize) return;
        fairValue = FN::roundNearest(
          qp->fvModel == mFairValueModel::BBO
            ? (topAskPrice + topBidPrice) / 2
            : (topAskPrice * topBidSize + topBidPrice * topAskSize) / (topAskSize + topBidSize),
          gw->minTick
        );
        if (!fairValue or (fairValue_ and abs(fairValue - fairValue_) < gw->minTick)) return;
        gw->evDataWallet(mWallet());
        ((UI*)client)->send(uiTXT::FairValue, {{"price", fairValue}}, true);
      };
    private:
      function<json()> helloTrade = [&]() {
        json k;
        for (unsigned i=0; i<trades.size(); ++i)
          k.push_back(trades[i]);
        return k;
      };
      function<json()> helloFair = [&]() {
        return (json){{{"price", fairValue}}};
      };
      function<json()> helloEwma = [&]() {
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
          {"ewmaVeryLong", mgEwmaVL},
          {"fairValue", fairValue}
        }};
      };
      void stdevPUp() {
        if (empty()) return;
        double topBid = levels.bids.begin()->price;
        double topAsk = levels.asks.begin()->price;
        if (!topBid or !topAsk) return;
        mgStatFV.push_back(fairValue);
        mgStatBid.push_back(topBid);
        mgStatAsk.push_back(topAsk);
        mgStatTop.push_back(topBid);
        mgStatTop.push_back(topAsk);
        calcStdev();
        ((DB*)memory)->insert(uiTXT::MarketData, {
          {"fv", fairValue},
          {"bid", topBid},
          {"ask", topAsk},
          {"time", FN::T()},
        }, false, "NULL", FN::T() - 1e+3 * qp->quotingStdevProtectionPeriods);
      };
      void tradeUp(mTrade k) {
        k.exchange = gw->exchange;
        k.pair = mPair(gw->base, gw->quote);
        k.time = FN::T();
        trades.push_back(k);
        if (trades.size()>69) trades.erase(trades.begin());
        ((UI*)client)->send(uiTXT::MarketTrade, k);
      };
      void levelUp(mLevels k) {
        filter(k);
        if (mgT_369ms+369 > FN::T()) return;
        ((UI*)client)->send(uiTXT::MarketData, k, true);
        mgT_369ms = FN::T();
      };
      void ewmaUp() {
        calcEwma(&mgEwmaVL, qp->veryLongEwmaPeriods);
        calcEwma(&mgEwmaL, qp->longEwmaPeriods);
        calcEwma(&mgEwmaM, qp->mediumEwmaPeriods);
        calcEwma(&mgEwmaS, qp->shortEwmaPeriods);
        calcTargetPos();
        ((EV*)events)->mgTargetPosition();
        ((UI*)client)->send(uiTXT::EWMAChart, {
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
          {"ewmaVeryLong", mgEwmaVL},
          {"fairValue", fairValue}
        }, true);
        ((DB*)memory)->insert(uiTXT::EWMAChart, {
          {"ewmaVeryLong", mgEwmaVL},
          {"ewmaLong", mgEwmaL},
          {"ewmaMedium", mgEwmaM},
          {"ewmaShort", mgEwmaS},
          {"mgEwmaSM", mgEwmaSM},
          {"mgEwmaSU", mgEwmaSU},
          {"time", FN::T()}
        });
      };
      void ewmaPUp() {
        calcEwma(&mgEwmaP, qp->quotingEwmaProtectionPeriods);
        ((EV*)events)->mgEwmaQuoteProtection();
      };
      void ewmaSMUUp() {
        calcEwma(&mgEwmaSM, qp->quotingEwmaSMPeriods);
        calcEwma(&mgEwmaSU, qp->quotingEwmaSUPeriods);
        if (mgEwmaSM and mgEwmaSU)
          mgEwmaSMUDiff = ((mgEwmaSU * 100) / mgEwmaSM) - 100;
        ((EV*)events)->mgEwmaSMUProtection();
      };
      void filter(mLevels k) {
        levels = k;
        if (empty()) return;
        map<string, mOrder> ordersSides = ((OG*)broker)->ordersBothSides();
        for (map<string, mOrder>::iterator it = ordersSides.begin(); it != ordersSides.end(); ++it)
          filter(mSide::Bid == it->second.side ? &levels.bids : &levels.asks, it->second);
        if (empty()) return;
        calcFairValue();
        ((EV*)events)->mgLevels();
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
        size_t periods = (size_t)qp->quotingStdevProtectionPeriods;
        if (mgStatFV.size()>periods) mgStatFV.erase(mgStatFV.begin(), mgStatFV.end()-periods);
        if (mgStatBid.size()>periods) mgStatBid.erase(mgStatBid.begin(), mgStatBid.end()-periods);
        if (mgStatAsk.size()>periods) mgStatAsk.erase(mgStatAsk.begin(), mgStatAsk.end()-periods);
        if (mgStatTop.size()>periods*2) mgStatTop.erase(mgStatTop.begin(), mgStatTop.end()-(periods*2));
      };
      void calcStdev() {
        cleanStdev();
        if (mgStatFV.size() < 2 or mgStatBid.size() < 2 or mgStatAsk.size() < 2 or mgStatTop.size() < 4) return;
        double k = qp->quotingStdevProtectionFactor;
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
          *k = alpha * fairValue + (1 - alpha) * *k;
        } else *k = fairValue;
      };
      void calcTargetPos() {
        mgSMA3.push_back(fairValue);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
          SMA3 += *it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if (qp->autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / mgEwmaL) - 100);
          double newEwmacrossing = ((mgEwmaS * 100 / mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp->ewmaSensiblityPercentage);
        } else if (qp->autoPositionMode == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qp->ewmaSensiblityPercentage);
        else if (qp->autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (mgEwmaL < mgEwmaVL) newTargetPosition = -1;
          else newTargetPosition = ((mgEwmaS * 100/ mgEwmaM) - 100) * (1 / qp->ewmaSensiblityPercentage);
        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        targetPosition = newTargetPosition;
      };
  };
}

#endif
