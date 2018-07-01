#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass,
            public Market { public: MG() { market = this; };
    private:
      mStdevs stdev;
      vector<mPrice> mgSMA3;
    protected:
      void load() {
        sqlite->backup(&levels.stats.ewma.fairValue96h);
        sqlite->backup(&levels.stats.ewma);
        sqlite->backup(&stdev);
        calcStdev();
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mTrade,  {                          PRETTY_DEBUG
          levels.stats.takerTrades.send_push_back(rawdata);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          levels.send_reset_filter(rawdata);
          wallet->calcWallet();
          engine->calcQuote();
        });
      };
      void waitSysAdmin() {
        screen->printme(levels.stats.fairPrice);
        screen->printme(levels.stats.ewma);
      };
      void waitWebAdmin() {
        client->welcome(levels.diff);
        client->welcome(levels.stats.takerTrades);
        client->welcome(levels.stats.fairPrice);
        client->welcome(levels.stats);
      };
    public:
      void timer_1s() {
        calcStatsStdevProtection();
      };
      void timer_60s() {
        levels.stats.takerTrades.calcSize60s();
        calcStatsEwmaProtection();
        calcStatsEwmaPosition();
        levels.stats.prepareEwmaHistory();
      };
    private:
      void calcStatsStdevProtection() {
        if (levels.empty()) return;
        stdev.push_back(mStdev(
          levels.fairValue,
          levels.bids.begin()->price,
          levels.asks.begin()->price
        ));
        calcStdev();
      };
      void calcStdev() {
        if (stdev.size() < 2) return;
        levels.stats.mgStdevFV = stdev.calc(&levels.stats.mgStdevFVMean, "fv");
        levels.stats.mgStdevBid = stdev.calc(&levels.stats.mgStdevBidMean, "bid");
        levels.stats.mgStdevAsk = stdev.calc(&levels.stats.mgStdevAskMean, "ask");
        levels.stats.mgStdevTop = stdev.calc(&levels.stats.mgStdevTopMean, "top");
      };
      void calcStatsEwmaProtection() {
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaP, qp.protectionEwmaPeriods, levels.fairValue);
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaW, qp.protectionEwmaPeriods, levels.resetAverageWidth());
      };
      void calcStatsEwmaPosition() {
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaVL, qp.veryLongEwmaPeriods, levels.fairValue);
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaL, qp.longEwmaPeriods, levels.fairValue);
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaM, qp.mediumEwmaPeriods, levels.fairValue);
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaS, qp.shortEwmaPeriods, levels.fairValue);
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaXS, qp.extraShortEwmaPeriods, levels.fairValue);
        levels.stats.ewma.calc(&levels.stats.ewma.mgEwmaU, qp.ultraShortEwmaPeriods, levels.fairValue);
        if(levels.stats.ewma.mgEwmaXS and levels.stats.ewma.mgEwmaU) levels.stats.ewma.mgEwmaTrendDiff = ((levels.stats.ewma.mgEwmaU * 100) / levels.stats.ewma.mgEwmaXS) - 100;
        calcTargetPos();
        wallet->calcTargetBasePos();
        levels.stats.send_push();
      };
      void calcTargetPos() {
        mgSMA3.push_back(levels.fairValue);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        mPrice SMA3 = 0;
        for (mPrice &it : mgSMA3) SMA3 += it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / levels.stats.ewma.mgEwmaL) - 100);
          double newEwmacrossing = ((levels.stats.ewma.mgEwmaS * 100 / levels.stats.ewma.mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((levels.stats.ewma.mgEwmaS * 100 / levels.stats.ewma.mgEwmaL) - 100) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (levels.stats.ewma.mgEwmaL < levels.stats.ewma.mgEwmaVL) newTargetPosition = -1;
          else newTargetPosition = ((levels.stats.ewma.mgEwmaS * 100 / levels.stats.ewma.mgEwmaM) - 100) * (1 / qp.ewmaSensiblityPercentage);
        }
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        targetPosition = newTargetPosition;
      };
  };
}

#endif
