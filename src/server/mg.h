#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass,
            public Market { public: MG() { market = this; };
    private:
      vector<mTrade> trades;
      mAmount takersBuySize60s = 0,
              takersSellSize60s = 0;
      mStdevs stdev;
      vector<mPrice> mgSMA3;
      mFairValues fairValue96h;
      mClock mgT_369ms = 0;
      unsigned int mgT_60s = 0,
                   averageCount = 0;
      mPrice averageWidth = 0;
      mLevelsDiff levelsDiff;
    protected:
      void load() {
        sqlite->select(
          FROM mMatter::MarketData
          INTO stdev
          THEN "loaded % STDEV Periods"
        );
        calcStdev();
        sqlite->select(
          FROM mMatter::MarketDataLongTerm
          INTO fairValue96h
          THEN "loaded % historical Fair Values"
        );
        sqlite->select(
          FROM mMatter::EWMAChart
          INTO ewma
          THEN "loaded last % OK"
          WARN "consider to warm up some %"
        );
      };
      void waitData() {
        gw->WRITEME(mTrade,  read_mTrade);
        gw->WRITEME(mLevels, read_mLevels);
      };
      void waitWebAdmin() {
        client->WELCOME(mMatter::MarketData,  hello_Levels);
        client->WELCOME(mMatter::MarketTrade, hello_Trade);
        client->WELCOME(mMatter::FairValue,   hello_Fair);
        client->WELCOME(mMatter::EWMAChart,   hello_Ewma);
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
        mPrice prev  = fairValue,
               topAskPrice = levels.asks.begin()->price,
               topBidPrice = levels.bids.begin()->price;
        mAmount topAskSize = levels.asks.begin()->size,
                topBidSize = levels.bids.begin()->size;
        fairValue = qp.fvModel == mFairValueModel::BBO
          ? (topAskPrice + topBidPrice) / 2
          : (topAskPrice * topBidSize + topBidPrice * topAskSize) / (topAskSize + topBidSize);
        if (!fairValue or (prev and abs(fairValue - prev) < gw->minTick)) return;
        wallet->calcWallet();
        client->send(mMatter::FairValue, {{"price", fairValue}});
        screen->log(fairValue);
        averageWidth = ((averageWidth * averageCount) + topAskPrice - topBidPrice);
        averageWidth /= ++averageCount;
      };
      void calcEwmaHistory() {
        if (FN::trueOnce(&qp._diffVLEP)) calcEwmaHistory(&ewma.mgEwmaVL, qp.veryLongEwmaPeriods, "VeryLong");
        if (FN::trueOnce(&qp._diffLEP)) calcEwmaHistory(&ewma.mgEwmaL, qp.longEwmaPeriods, "Long");
        if (FN::trueOnce(&qp._diffMEP)) calcEwmaHistory(&ewma.mgEwmaM, qp.mediumEwmaPeriods, "Medium");
        if (FN::trueOnce(&qp._diffSEP)) calcEwmaHistory(&ewma.mgEwmaS, qp.shortEwmaPeriods, "Short");
        if (FN::trueOnce(&qp._diffXSEP)) calcEwmaHistory(&ewma.mgEwmaXS, qp.extraShortEwmaPeriods, "ExtraShort");
        if (FN::trueOnce(&qp._diffUEP)) calcEwmaHistory(&ewma.mgEwmaU, qp.ultraShortEwmaPeriods, "UltraShort");
      };
    private:
      void hello_Levels(json *const welcome) {
        *welcome = { levelsDiff.reset(levels) };
      };
      void hello_Trade(json *const welcome) {
        *welcome = trades;
      };
      void hello_Fair(json *const welcome) {
        *welcome = { {
          {"price", fairValue}
        } };
      };
      void hello_Ewma(json *const welcome) {
        *welcome = { chartStats() };
      };
      void read_mTrade(const mTrade &rawdata) {                     PRETTY_DEBUG
        trades.push_back(rawdata);
        client->send(mMatter::MarketTrade, rawdata);
      };
      void read_mLevels(const mLevels &rawdata) {                   PRETTY_DEBUG
        levels = rawdata;
        if (!filterBidOrders.empty()) filter(&levels.bids, filterBidOrders);
        if (!filterAskOrders.empty()) filter(&levels.asks, filterAskOrders);
        calcFairValue();
        engine->calcQuote();
        if (levelsDiff.empty() or levels.empty()
          or mgT_369ms + max(369.0, qp.delayUI * 1e+3) > _Tstamp_
        ) return;
        client->send(mMatter::MarketData, levelsDiff.diff(levels));
        mgT_369ms = _Tstamp_;
      };
      void calcStatsStdevProtection() {
        if (levels.empty()) return;
        stdev.push_back(mStdev(
          fairValue,
          levels.bids.begin()->price,
          levels.asks.begin()->price
        ));
        sqlite->insert(mMatter::MarketData, stdev.back(), false, "NULL", _Tstamp_ - 1e+3 * qp.quotingStdevProtectionPeriods);
        calcStdev();
      };
      void calcStatsTrades() {
        takersSellSize60s = takersBuySize60s = 0;
        if (trades.empty()) return;
        for (mTrade &it : trades)
          (it.side == mSide::Bid
            ? takersSellSize60s
            : takersBuySize60s
          ) += it.quantity;
        trades.clear();
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
        fairValue96h.push_back(mFairValue(fairValue));
        mClock expire = fairValue96h.expire() / 60e+3;
        if (fairValue96h.size() > expire)
          fairValue96h.erase(fairValue96h.begin(), fairValue96h.begin() + fairValue96h.size() - expire);
        calcEwma(&ewma.mgEwmaVL, qp.veryLongEwmaPeriods, fairValue);
        calcEwma(&ewma.mgEwmaL, qp.longEwmaPeriods, fairValue);
        calcEwma(&ewma.mgEwmaM, qp.mediumEwmaPeriods, fairValue);
        calcEwma(&ewma.mgEwmaS, qp.shortEwmaPeriods, fairValue);
        calcEwma(&ewma.mgEwmaXS, qp.extraShortEwmaPeriods, fairValue);
        calcEwma(&ewma.mgEwmaU, qp.ultraShortEwmaPeriods, fairValue);
        if(ewma.mgEwmaXS and ewma.mgEwmaU) mgEwmaTrendDiff = ((ewma.mgEwmaU * 100) / ewma.mgEwmaXS) - 100;
        calcTargetPos();
        wallet->calcTargetBasePos();
        client->send(mMatter::EWMAChart, chartStats());
        sqlite->insert(mMatter::EWMAChart, ewma);
        sqlite->insert(mMatter::MarketDataLongTerm, fairValue96h.back(), false, "NULL", _Tstamp_ - 345600e+3);
      };
      void calcStatsEwmaProtection() {
        calcEwma(&ewma.mgEwmaP, qp.protectionEwmaPeriods, fairValue);
        calcEwma(&ewma.mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
        averageCount = 0;
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
          {"ewmaQuote", ewma.mgEwmaP},
          {"ewmaWidth", ewma.mgEwmaW},
          {"ewmaShort", ewma.mgEwmaS},
          {"ewmaMedium", ewma.mgEwmaM},
          {"ewmaLong", ewma.mgEwmaL},
          {"ewmaVeryLong", ewma.mgEwmaVL},
          {"ewmaTrendDiff", mgEwmaTrendDiff},
          {"tradesBuySize", takersBuySize60s},
          {"tradesSellSize", takersSellSize60s},
          {"fairValue", fairValue}
        };
      };
      void expireStdev() {
        if (stdev.size() > qp.quotingStdevProtectionPeriods)
          stdev.erase(stdev.begin(), stdev.end() - qp.quotingStdevProtectionPeriods);
      };
      void calcStdev() {
        expireStdev();
        if (stdev.size() < 2) return;
        mgStdevFV = calcStdev(&mgStdevFVMean, "fv");
        mgStdevBid = calcStdev(&mgStdevBidMean, "bid");
        mgStdevAsk = calcStdev(&mgStdevAskMean, "ask");
        mgStdevTop = calcStdev(&mgStdevTopMean, "top");
      };
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
      void calcEwmaHistory(mPrice *mean, unsigned int periods, string name) {
        unsigned int n = fairValue96h.size();
        if (!n) return;
        *mean = fairValue96h.front().fv;
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
        mgSMA3.push_back(fairValue);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        mPrice SMA3 = 0;
        for (mPrice &it : mgSMA3) SMA3 += it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / ewma.mgEwmaL) - 100);
          double newEwmacrossing = ((ewma.mgEwmaS * 100 / ewma.mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((ewma.mgEwmaS * 100 / ewma.mgEwmaL) - 100) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (ewma.mgEwmaL < ewma.mgEwmaVL) newTargetPosition = -1;
          else newTargetPosition = ((ewma.mgEwmaS * 100 / ewma.mgEwmaM) - 100) * (1 / qp.ewmaSensiblityPercentage);
        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        targetPosition = newTargetPosition;
      };
  };
}

#endif
