#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  class QE: public Klass,
            public Engine { public: QE() { engine = this; };
    protected:
      void load() {
        SQLITE_BACKUP
        broker.calculon.dummyMM.reset("loaded");
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mConnectivity, {
          if (!broker.semaphore.online(rawdata))
            levels.clear();
        });
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          wallet.reset(rawdata, levels);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          levels.reset(rawdata);
          wallet.send_ratelimit(levels);
          calcQuotes();
        });
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          broker.upsert(rawdata, &wallet, levels, &gw->askForFees);
        });
        gw->RAWDATA_ENTRY_POINT(mTrade, {                           PRETTY_DEBUG
          levels.stats.takerTrades.send_push_back(rawdata);
        });
      };
      void waitWebAdmin() {
        CLIENT_WELCOME
        CLIENT_CLICKME
      };
      void waitSysAdmin() {
        SCREEN_PRINTME
        SCREEN_PRESSME
      };
      void run() {                                                  PRETTY_DEBUG
        gw->load_externals();
      };
  };
}

#endif
