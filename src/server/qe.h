#ifndef K_QE_H_
#define K_QE_H_

namespace K {
  class QE: public Klass,
            public Engine { public: QE() { engine = this; };
    protected:
      void load() {
        SQLITE_BACKUP
        broker.calculon.dummyMM.mode("loaded");
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mConnectivity, {
          broker.semaphore.read_from_gw(rawdata);
          if (broker.semaphore.offline())
            levels.clear();
        });
        gw->RAWDATA_ENTRY_POINT(mWallets, {
          wallet.read_from_gw(rawdata);
        });
        gw->RAWDATA_ENTRY_POINT(mLevels, {
          levels.read_from_gw(rawdata);
          wallet.calcFunds();
          calcQuotes();
        });
        gw->RAWDATA_ENTRY_POINT(mOrder, {
          broker.read_from_gw(rawdata, &gw->askForFees);
        });
        gw->RAWDATA_ENTRY_POINT(mTrade, {
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
      void run() {
        gw->load_externals();
      };
  };
}

#endif
