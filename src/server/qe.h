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
          if (!broker.semaphore.read_from_gw(rawdata))
            levels.clear();
        });
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          wallet.read_from_gw(rawdata);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {                          PRETTY_DEBUG
          levels.read_from_gw(rawdata);
          wallet.send_ratelimit();
          calcQuotes();
        });
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          broker.read_from_gw(rawdata, &wallet, &gw->askForFees);
        });
        gw->RAWDATA_ENTRY_POINT(mTrade, {                           PRETTY_DEBUG
          levels.stats.takerTrades.read_from_gw(rawdata);
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
