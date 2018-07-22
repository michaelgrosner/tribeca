#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass {
    protected:
      void load() {
        sqlite->backup(&gw->wallet.target);
        sqlite->backup(&gw->wallet.profits);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          gw->wallet.reset(rawdata, gw->levels);
        });
      };
      void waitSysAdmin() {
        screen->printme(&gw->wallet.target);
      };
      void waitWebAdmin() {
        client->welcome(gw->wallet.target.safety);
        client->welcome(gw->wallet.target);
        client->welcome(gw->wallet);
      };
  };
}

#endif
