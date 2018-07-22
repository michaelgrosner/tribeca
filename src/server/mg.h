#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass {
    protected:
      void load() {
        sqlite->backup(&gw->levels.stats.ewma.fairValue96h);
        sqlite->backup(&gw->levels.stats.ewma);
        sqlite->backup(&gw->levels.stats.stdev);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mTrade, {                           PRETTY_DEBUG
          gw->levels.stats.takerTrades.send_push_back(rawdata);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          gw->levels.send_reset_filter(rawdata, gw->minTick);
          gw->wallet.send_ratelimit(gw->levels);
          engine->calcQuote();
        });
      };
      void waitSysAdmin() {
        screen->printme(&gw->levels.stats.fairPrice);
        screen->printme(&gw->levels.stats.ewma);
      };
      void waitWebAdmin() {
        client->welcome(gw->levels.diff);
        client->welcome(gw->levels.stats.takerTrades);
        client->welcome(gw->levels.stats.fairPrice);
        client->welcome(gw->levels.stats);
      };
  };
}

#endif
