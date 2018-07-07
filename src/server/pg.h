#ifndef K_PG_H_
#define K_PG_H_

namespace K {
  class PG: public Klass,
            public Wallet { public: PG() { wallet = this; };
    protected:
      void load() {
        sqlite->backup(&position.target);
        sqlite->backup(&position.profits);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mWallets, {                         PRETTY_DEBUG
          position.balance.reset(rawdata);
          position.send_ratelimit(market->levels);
        });
      };
      void waitSysAdmin() {
        screen->printme(&position.target);
      };
      void waitWebAdmin() {
        client->welcome(position.target);
        client->welcome(position.safety);
        client->welcome(position);
      };
  };
}

#endif
