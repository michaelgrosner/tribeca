#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass {
    private:
      vector<mTrade> trades;
      double takersBuySize60s = 0;
      double takersSellSize60s = 0;
      double mgEwmaVL = 0;
      double mgEwmaL = 0;
      double mgEwmaM = 0;
      double mgEwmaS = 0;
      vector<double> mgSMA3;
      vector<double> mgStatFV;
      vector<double> mgStatBid;
      vector<double> mgStatAsk;
      vector<double> mgStatTop;
      vector<double> fairValue96h;
      unsigned int mgT_60s = 0;
      unsigned long mgT_369ms = 0;
      double averageWidth = 0;
      unsigned int averageCount = 0;
    public:
      mLevels levels;
      double fairValue = 0;
      double targetPosition = 0;
      double mgStdevTop = 0;
      double mgStdevTopMean = 0;
      double mgEwmaP = 0;
      double mgEwmaW = 0;
      double mgStdevFV = 0;
      double mgStdevFVMean = 0;
      double mgStdevBid = 0;
      double mgStdevBidMean = 0;
      double mgStdevAsk = 0;
      double mgStdevAskMean = 0;
    protected:
      void load() {
        for (json &it : ((DB*)memory)->load(mMatter::MarketData)) {
          if (it.value("time", (unsigned long)0) + qp->quotingStdevProtectionPeriods * 1e+3 < _Tstamp_) continue;
          mgStatFV.push_back(it.value("fv", 0.0));
          mgStatBid.push_back(it.value("bid", 0.0));
          mgStatAsk.push_back(it.value("ask", 0.0));
          mgStatTop.push_back(it.value("bid", 0.0));
          mgStatTop.push_back(it.value("ask", 0.0));
        }
        calcStdev();
        FN::log("DB", string("loaded ") + to_string(mgStatFV.size()) + " STDEV Periods");
        if (((CF*)config)->argEwmaVeryLong) mgEwmaVL = ((CF*)config)->argEwmaVeryLong;
        if (((CF*)config)->argEwmaLong) mgEwmaL = ((CF*)config)->argEwmaLong;
        if (((CF*)config)->argEwmaMedium) mgEwmaM = ((CF*)config)->argEwmaMedium;
        if (((CF*)config)->argEwmaShort) mgEwmaS = ((CF*)config)->argEwmaShort;
        json k = ((DB*)memory)->load(mMatter::EWMAChart);
        if (!k.empty()) {
          k = k.at(0);
          if (!mgEwmaVL and k.value("time", (unsigned long)0) + qp->veryLongEwmaPeriods * 6e+4 > _Tstamp_)
            mgEwmaVL = k.value("ewmaVeryLong", 0.0);
          if (!mgEwmaL and k.value("time", (unsigned long)0) + qp->longEwmaPeriods * 6e+4 > _Tstamp_)
            mgEwmaL = k.value("ewmaLong", 0.0);
          if (!mgEwmaM and k.value("time", (unsigned long)0) + qp->mediumEwmaPeriods * 6e+4 > _Tstamp_)
            mgEwmaM = k.value("ewmaMedium", 0.0);
          if (!mgEwmaS and k.value("time", (unsigned long)0) + qp->shortEwmaPeriods * 6e+4 > _Tstamp_)
            mgEwmaS = k.value("ewmaShort", 0.0);
        }
        if (mgEwmaVL) FN::log(((CF*)config)->argEwmaVeryLong ? "ARG" : "DB", string("loaded ") + to_string(mgEwmaVL) + " EWMA VeryLong");
        if (mgEwmaL)  FN::log(((CF*)config)->argEwmaLong ? "ARG" : "DB", string("loaded ") + to_string(mgEwmaL) + " EWMA Long");
        if (mgEwmaM)  FN::log(((CF*)config)->argEwmaMedium ? "ARG" : "DB", string("loaded ") + to_string(mgEwmaM) + " EWMA Medium");
        if (mgEwmaS)  FN::log(((CF*)config)->argEwmaShort ? "ARG" : "DB", string("loaded ") + to_string(mgEwmaS) + " EWMA Short");
        for (json &it : ((DB*)memory)->load(mMatter::MarketDataLongTerm))
          if (it.value("time", (unsigned long)0) + 3456e+5 > _Tstamp_ and it.value("fv", 0.0))
            fairValue96h.push_back(it.value("fv", 0.0));
        FN::log("DB", string("loaded ") + to_string(fairValue96h.size()) + " historical FairValues");
      };
      void waitData() {
        gw->evDataTrade = [&](mTrade k) {                           _debugEvent_
          tradeUp(k);
        };
        gw->evDataLevels = [&](mLevels k) {                         _debugEvent_
          levelUp(k);
        };
      };
      void waitUser() {
        ((UI*)client)->welcome(mMatter::MarketTrade, &helloTrade);
        ((UI*)client)->welcome(mMatter::FairValue, &helloFair);
        ((UI*)client)->welcome(mMatter::EWMAChart, &helloEwma);
      };
    public:
      void calcStats() {
        if (!mgT_60s++) {
          calcStatsTrades();
          calcStatsEwmaProtection();
          calcStatsEwmaPosition();
        } else if (mgT_60s == 60) mgT_60s = 0;
        calcStatsStdevProtection();
      };
      void calcFairValue() {
        if (levels.empty()) return;
        double fairValue_ = fairValue,
               topAskPrice = levels.asks.begin()->price,
               topBidPrice = levels.bids.begin()->price,
               topAskSize = levels.asks.begin()->size,
               topBidSize = levels.bids.begin()->size;
        if (!topAskPrice or !topBidPrice or !topAskSize or !topBidSize) return;
        fairValue = qp->fvModel == mFairValueModel::BBO
          ? (topAskPrice + topBidPrice) / 2
          : (topAskPrice * topBidSize + topBidPrice * topAskSize) / (topAskSize + topBidSize);
        if (!fairValue or (fairValue_ and abs(fairValue - fairValue_) < gw->minTick)) return;
        gw->evDataWallet(mWallet());
        ((UI*)client)->send(mMatter::FairValue, {{"price", fairValue}});
        averageWidth = ((averageWidth * averageCount) + topAskPrice - topBidPrice);
        averageWidth /= ++averageCount;
      };
      void calcEwmaHistory() {
        calcEwmaHistory(&mgEwmaVL, qp->veryLongEwmaPeriods, "VeryLong");
        calcEwmaHistory(&mgEwmaL, qp->longEwmaPeriods, "Long");
        calcEwmaHistory(&mgEwmaM, qp->mediumEwmaPeriods, "Medium");
        calcEwmaHistory(&mgEwmaS, qp->shortEwmaPeriods, "Short");
      };
    private:
      function<void(json*)> helloTrade = [&](json *welcome) {
        for (mTrade &it : trades)
          welcome->push_back(it);
      };
      function<void(json*)> helloFair = [&](json *welcome) {
        *welcome = { {
          {"price", fairValue}
        } };
      };
      function<void(json*)> helloEwma = [&](json *welcome) {
        *welcome = { chartStats() };
      };
      void calcStatsStdevProtection() {
        if (levels.empty()) return;
        double topBid = levels.bids.begin()->price;
        double topAsk = levels.asks.begin()->price;
        if (!topBid or !topAsk) return;
        mgStatFV.push_back(fairValue);
        mgStatBid.push_back(topBid);
        mgStatAsk.push_back(topAsk);
        mgStatTop.push_back(topBid);
        mgStatTop.push_back(topAsk);
        calcStdev();
        ((DB*)memory)->insert(mMatter::MarketData, {
          {"fv", fairValue},
          {"bid", topBid},
          {"ask", topAsk},
          {"time", _Tstamp_},
        }, false, "NULL", _Tstamp_ - 1e+3 * qp->quotingStdevProtectionPeriods);
      };
      void calcStatsTrades() {
        takersSellSize60s = takersBuySize60s = 0;
        if (trades.empty()) return;
        for (mTrade &it : trades)
          if (it.side == mSide::Bid) takersSellSize60s += it.quantity;
          else takersBuySize60s += it.quantity;
        trades.clear();
      };
      void tradeUp(mTrade k) {
        k.pair = mPair(gw->base, gw->quote);
        k.time = _Tstamp_;
        trades.push_back(k);
        ((UI*)client)->send(mMatter::MarketTrade, k);
      };
      void levelUp(mLevels k) {
        filter(k);
        if (mgT_369ms + 369e+0 > _Tstamp_) return;
        ((UI*)client)->bid_levels = k.bids.size();
        ((UI*)client)->ask_levels = k.asks.size();
        ((UI*)client)->send(mMatter::MarketData, k);
        mgT_369ms = _Tstamp_;
      };
      void calcStatsEwmaPosition() {
        fairValue96h.push_back(fairValue);
        if (fairValue96h.size() > 5760)
          fairValue96h.erase(fairValue96h.begin(), fairValue96h.begin()+fairValue96h.size()-5760);
        calcEwma(&mgEwmaVL, qp->veryLongEwmaPeriods, fairValue);
        calcEwma(&mgEwmaL, qp->longEwmaPeriods, fairValue);
        calcEwma(&mgEwmaM, qp->mediumEwmaPeriods, fairValue);
        calcEwma(&mgEwmaS, qp->shortEwmaPeriods, fairValue);
        calcTargetPos();
        ((EV*)events)->mgTargetPosition();
        ((UI*)client)->send(mMatter::EWMAChart, chartStats());
        ((DB*)memory)->insert(mMatter::EWMAChart, {
          {"ewmaVeryLong", mgEwmaVL},
          {"ewmaLong", mgEwmaL},
          {"ewmaMedium", mgEwmaM},
          {"ewmaShort", mgEwmaS},
          {"time", _Tstamp_}
        });
        ((DB*)memory)->insert(mMatter::MarketDataLongTerm, {
          {"fv", fairValue},
          {"time", _Tstamp_},
        }, false, "NULL", _Tstamp_ - 3456e+5);
      };
      void calcStatsEwmaProtection() {
        calcEwma(&mgEwmaP, qp->protectionEwmaPeriods, fairValue);
        calcEwma(&mgEwmaW, qp->protectionEwmaPeriods, averageWidth);
        averageCount = 0;
        ((EV*)events)->mgEwmaQuoteProtection();
      };
      json chartStats() {
        return {
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
          {"ewmaWidth", mgEwmaW},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"ewmaVeryLong", mgEwmaVL},
          {"tradesBuySize", takersBuySize60s},
          {"tradesSellSize", takersSellSize60s},
          {"fairValue", fairValue}
        };
      };
      void filter(mLevels k) {
        levels = k;
        if (levels.empty()) return;
        map<double, double> filterBidOrders,
                            filterAskOrders;
        for (map<string, mOrder>::value_type &it : ((OG*)broker)->orders)
          (mSide::Bid == it.second.side ? filterBidOrders : filterAskOrders)[it.second.price] += it.second.quantity;
        if (!filterBidOrders.empty()) filter(&levels.bids, &filterBidOrders);
        if (!filterAskOrders.empty()) filter(&levels.asks, &filterAskOrders);
        if (levels.empty()) return;
        calcFairValue();
        ((EV*)events)->mgLevels();
      };
      void filter(vector<mLevel> *k, map<double, double> *o) {
        for (vector<mLevel>::iterator it = k->begin(); it != k->end();) {
          bool rm = false;
          for (map<double, double>::iterator it_ = o->begin(); it_ != o->end();)
            if (abs(it->price - it_->first) < gw->minTick) {
              it->size = it->size - it_->second;
              if (it->size < gw->minTick) {
                rm = true;
                it = k->erase(it);
              }
              o->erase(it_);
              break;
            } else ++it_;
          if (o->empty()) break;
          if (!rm) ++it;
        }
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
        mgStdevFV = calcStdev(&mgStdevFVMean, qp->quotingStdevProtectionFactor, mgStatFV);
        mgStdevBid = calcStdev(&mgStdevBidMean, qp->quotingStdevProtectionFactor, mgStatBid);
        mgStdevAsk = calcStdev(&mgStdevAskMean, qp->quotingStdevProtectionFactor, mgStatAsk);
        mgStdevTop = calcStdev(&mgStdevTopMean, qp->quotingStdevProtectionFactor, mgStatTop);
      };
      double calcStdev(double *mean, double factor, vector<double> values) {
        unsigned int n = values.size();
        if (!n) return 0.0;
        double sum = 0;
        for (double &it : values) sum += it;
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (double &it : values) {
          double diff = it - *mean;
          sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * factor;
      };
      void calcEwmaHistory(double *mean, unsigned int periods, string name) {
        unsigned int n = fairValue96h.size();
        if (!n or !periods or n < periods) return;
        n = periods;
        *mean = 0;
        while (n--) calcEwma(mean, periods, *(fairValue96h.rbegin()+n));
        FN::log("MG", string("reloaded ") + to_string(*mean) + " EWMA " + name);
      };
      void calcEwma(double *mean, unsigned int periods, double value) {
        if (*mean) {
          double alpha = 2.0 / (periods + 1);
          *mean = alpha * value + (1 - alpha) * *mean;
        } else *mean = value;
      };
      void calcTargetPos() {
        mgSMA3.push_back(fairValue);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (double &it : mgSMA3) SMA3 += it;
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
