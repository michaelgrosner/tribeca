#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  class MG: public Klass {
    protected:
      void load() {
        sqlite->backup(&engine->levels.stats.ewma.fairValue96h);
        sqlite->backup(&engine->levels.stats.ewma);
        sqlite->backup(&engine->levels.stats.stdev);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mTrade, {                           PRETTY_DEBUG
          engine->levels.stats.takerTrades.send_push_back(rawdata);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          engine->levels.send_reset_filter(rawdata, gw->minTick);
          engine->wallet.send_ratelimit(engine->levels);
          engine->calcQuote();
        });
      };
      void waitSysAdmin() {
        screen->printme(&engine->levels.stats.fairPrice);
        screen->printme(&engine->levels.stats.ewma);
      };
      void waitWebAdmin() {
        client->welcome(engine->levels.diff);
        client->welcome(engine->levels.stats.takerTrades);
        client->welcome(engine->levels.stats.fairPrice);
        client->welcome(engine->levels.stats);
      };
  };
}

#endif
