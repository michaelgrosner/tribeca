#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass {
    protected:
      void load() {
        sqlite->backup(&engine->wallet.target);
        sqlite->backup(&engine->wallet.profits);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          engine->wallet.reset(rawdata, engine->levels);
        });
      };
      void waitSysAdmin() {
        screen->printme(&engine->wallet.target);
      };
      void waitWebAdmin() {
        client->welcome(engine->wallet.target.safety);
        client->welcome(engine->wallet.target);
        client->welcome(engine->wallet);
      };
  };
}

#endif
