#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass,
            public Market { public: MG() { market = this; };
    protected:
      void load() {
        sqlite->backup(&levels.stats.ewma.fairValue96h);
        sqlite->backup(&levels.stats.ewma);
        sqlite->backup(&levels.stats.stdev);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mTrade, {                           PRETTY_DEBUG
          levels.stats.takerTrades.send_push_back(rawdata);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          levels.send_reset_filter(rawdata, gw->minTick);
          wallet->position.send_ratelimit(levels);
          engine->calcQuote();
        });
      };
      void waitSysAdmin() {
        screen->printme(&levels.stats.fairPrice);
        screen->printme(&levels.stats.ewma);
      };
      void waitWebAdmin() {
        client->welcome(levels.diff);
        client->welcome(levels.stats.takerTrades);
        client->welcome(levels.stats.fairPrice);
        client->welcome(levels.stats);
      };
  };
}

#endif
