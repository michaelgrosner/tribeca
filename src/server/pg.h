#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass,
            public Wallet { public: PG() { wallet = this; };
    protected:
      void load() {
        sqlite->backup(&balance.target);
        sqlite->backup(&balance.profits);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          balance.reset(rawdata, market->levels);
        });
      };
      void waitSysAdmin() {
        screen->printme(&balance.target);
      };
      void waitWebAdmin() {
        client->welcome(balance.target.safety);
        client->welcome(balance.target);
        client->welcome(balance);
      };
  };
}

#endif
