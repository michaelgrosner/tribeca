#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass,
            public Market { public: MG() { market = this; };
    private:
      mStdevs stdev;
      vector<mPrice> mgSMA3;
      mFairValues fairValue96h;
      unsigned int mgT_60s = 0,
                   averageCount = 0;
      mPrice averageWidth = 0;
      mLevelsDiff levelsDiff;
    protected:
      void load() {
        sqlite->backup(
          INTO stdev
          THEN "loaded % STDEV Periods"
        );
        calcStdev();
        sqlite->backup(
          INTO fairValue96h
          THEN "loaded % historical Fair Values"
        );
        sqlite->backup(
          INTO stats.ewma
          THEN "loaded last % OK"
          WARN "consider to warm up some %"
        );
      };
      void waitData() {
        gw->WRITEME(mTrade,  read_mTrade);
        gw->WRITEME(mLevels, read_mLevels);
      };
      void waitWebAdmin() {
        client->WELCOME(levelsDiff,        hello_Levels);
        client->WELCOME(stats.takerTrades, hello_Trade);
        client->WELCOME(stats.fairValue,   hello_Fair);
        client->WELCOME(stats,             hello_Chart);
      };
    public:
      void calcStats() {
        if (!mgT_60s++) {
          calcStatsTakers();
          calcStatsEwmaProtection();
          calcStatsEwmaPosition();
          prepareEwmaHistory();
        } else if (mgT_60s == 60) mgT_60s = 0;
        calcStatsStdevProtection();
      };
      void calcFairValue() {
        if (levels.empty()) return;
        mPrice prev  = stats.fairValue.fv,
               topAskPrice = levels.asks.begin()->price,
               topBidPrice = levels.bids.begin()->price;
        mAmount topAskSize = levels.asks.begin()->size,
                topBidSize = levels.bids.begin()->size;
        stats.fairValue.fv = qp.fvModel == mFairValueModel::BBO
          ? (topAskPrice + topBidPrice) / 2
          : (topAskPrice * topBidSize + topBidPrice * topAskSize) / (topAskSize + topBidSize);
        if (!stats.fairValue.fv or (prev and abs(stats.fairValue.fv - prev) < gw->minTick)) return;
        wallet->calcWallet();
        stats.fairValue.send();
        screen->log(stats.fairValue.fv);
        averageWidth = ((averageWidth * averageCount) + topAskPrice - topBidPrice);
        averageWidth /= ++averageCount;
      };
      void calcEwmaHistory() {
        if (FN::trueOnce(&qp._diffVLEP)) calcEwmaHistory(&stats.ewma.mgEwmaVL, qp.veryLongEwmaPeriods, "VeryLong");
        if (FN::trueOnce(&qp._diffLEP)) calcEwmaHistory(&stats.ewma.mgEwmaL, qp.longEwmaPeriods, "Long");
        if (FN::trueOnce(&qp._diffMEP)) calcEwmaHistory(&stats.ewma.mgEwmaM, qp.mediumEwmaPeriods, "Medium");
        if (FN::trueOnce(&qp._diffSEP)) calcEwmaHistory(&stats.ewma.mgEwmaS, qp.shortEwmaPeriods, "Short");
        if (FN::trueOnce(&qp._diffXSEP)) calcEwmaHistory(&stats.ewma.mgEwmaXS, qp.extraShortEwmaPeriods, "ExtraShort");
        if (FN::trueOnce(&qp._diffUEP)) calcEwmaHistory(&stats.ewma.mgEwmaU, qp.ultraShortEwmaPeriods, "UltraShort");
      };
    private:
      void hello_Levels(json *const welcome) {
        *welcome = { levelsDiff.reset(levels) };
      };
      void hello_Trade(json *const welcome) {
        *welcome = stats.takerTrades;
      };
      void hello_Fair(json *const welcome) {
        *welcome = { stats.fairValue };
      };
      void hello_Chart(json *const welcome) {
        *welcome = { stats };
      };
      void read_mTrade(const mTrade &rawdata) {                     PRETTY_DEBUG
        stats.takerTrades.send_push_back(rawdata);
      };
      void read_mLevels(const mLevels &rawdata) {                   PRETTY_DEBUG
        levels = rawdata;
        if (!filterBidOrders.empty()) filter(&levels.bids, filterBidOrders);
        if (!filterAskOrders.empty()) filter(&levels.asks, filterAskOrders);
        calcFairValue();
        engine->calcQuote();
        levelsDiff.send_diff(levels);
      };
      void calcStatsStdevProtection() {
        if (levels.empty()) return;
        stdev.push_back(mStdev(
          stats.fairValue.fv,
          levels.bids.begin()->price,
          levels.asks.begin()->price
        ));
        calcStdev();
      };
      void calcStatsTakers() {
        stats.takerTrades.calcSize60s();
      };
      void filter(vector<mLevel> *k, map<mPrice, mAmount> o) {
        for (vector<mLevel>::iterator it = k->begin(); it != k->end();) {
          for (map<mPrice, mAmount>::iterator it_ = o.begin(); it_ != o.end();)
            if (abs(it->price - it_->first) < gw->minTick) {
              it->size = it->size - it_->second;
              o.erase(it_);
              break;
            } else ++it_;
          if (it->size < gw->minTick) it = k->erase(it);
          else ++it;
          if (o.empty()) break;
        }
      };
      void calcStatsEwmaPosition() {
        calcEwma(&stats.ewma.mgEwmaVL, qp.veryLongEwmaPeriods, stats.fairValue.fv);
        calcEwma(&stats.ewma.mgEwmaL, qp.longEwmaPeriods, stats.fairValue.fv);
        calcEwma(&stats.ewma.mgEwmaM, qp.mediumEwmaPeriods, stats.fairValue.fv);
        calcEwma(&stats.ewma.mgEwmaS, qp.shortEwmaPeriods, stats.fairValue.fv);
        calcEwma(&stats.ewma.mgEwmaXS, qp.extraShortEwmaPeriods, stats.fairValue.fv);
        calcEwma(&stats.ewma.mgEwmaU, qp.ultraShortEwmaPeriods, stats.fairValue.fv);
        if(stats.ewma.mgEwmaXS and stats.ewma.mgEwmaU) stats.ewma.mgEwmaTrendDiff = ((stats.ewma.mgEwmaU * 100) / stats.ewma.mgEwmaXS) - 100;
        calcTargetPos();
        wallet->calcTargetBasePos();
        stats.send_push();
      };
      void calcStatsEwmaProtection() {
        calcEwma(&stats.ewma.mgEwmaP, qp.protectionEwmaPeriods, stats.fairValue.fv);
        calcEwma(&stats.ewma.mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
        averageCount = 0;
      };
      void calcStdev() {
        if (stdev.size() < 2) return;
        stats.mgStdevFV = calcStdev(&stats.mgStdevFVMean, "fv");
        stats.mgStdevBid = calcStdev(&stats.mgStdevBidMean, "bid");
        stats.mgStdevAsk = calcStdev(&stats.mgStdevAskMean, "ask");
        stats.mgStdevTop = calcStdev(&stats.mgStdevTopMean, "top");
      }; // stdev.calc plz
      double calcStdev(mPrice *mean, string type) {
        unsigned int n = stdev.size() * (type == "top" ? 2 : 1);
        if (!n) return 0.0;
        double sum = 0;
        for (mStdev &it : stdev)
          if (type == "fv")
            sum += it.fv;
          else if (type == "bid")
            sum += it.topBid;
          else if (type == "ask")
            sum += it.topAsk;
          else if (type == "top") {
            sum += it.topBid + it.topAsk;
          }
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (mStdev &it : stdev) {
          mPrice diff = 0;
          if (type == "fv")
            diff = it.fv;
          else if (type == "bid")
            diff = it.topBid;
          else if (type == "ask")
            diff = it.topAsk;
          else if (type == "top") {
            diff = it.topBid;
          }
          diff -= *mean;
          sq_diff_sum += diff * diff;
          if (type == "top") {
            diff = it.topAsk - *mean;
            sq_diff_sum += diff * diff;
          }
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * qp.quotingStdevProtectionFactor;
      };
      void prepareEwmaHistory() {
        fairValue96h.push_back(mFairValue(stats.fairValue.fv));
      };
      void calcEwmaHistory(mPrice *mean, unsigned int periods, string name) {
        unsigned int n = fairValue96h.size();
        if (!n) return;
        *mean = fairValue96h.begin()->fv;
        while (n--) calcEwma(mean, periods, (fairValue96h.rbegin()+n)->fv);
        screen->log("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
      };
      void calcEwma(mPrice *mean, unsigned int periods, mPrice value) {
        if (*mean) {
          double alpha = 2.0 / (periods + 1);
          *mean = alpha * value + (1 - alpha) * *mean;
        } else *mean = value;
      };
      void calcTargetPos() {
        mgSMA3.push_back(stats.fairValue.fv);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        mPrice SMA3 = 0;
        for (mPrice &it : mgSMA3) SMA3 += it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / stats.ewma.mgEwmaL) - 100);
          double newEwmacrossing = ((stats.ewma.mgEwmaS * 100 / stats.ewma.mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((stats.ewma.mgEwmaS * 100 / stats.ewma.mgEwmaL) - 100) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (stats.ewma.mgEwmaL < stats.ewma.mgEwmaVL) newTargetPosition = -1;
          else newTargetPosition = ((stats.ewma.mgEwmaS * 100 / stats.ewma.mgEwmaM) - 100) * (1 / qp.ewmaSensiblityPercentage);
        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        targetPosition = newTargetPosition;
      };
  };
}

#endif
