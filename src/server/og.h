#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass {
    protected:
      void load() {
        sqlite->backup(&engine->broker.tradesHistory);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          engine->broker.upsert(rawdata, &engine->wallet, engine->levels, &gw->askForFees);
        });
      };
      void waitSysAdmin() {
        screen->printme(&engine->broker.tradesHistory);
        screen->printme(&engine->broker);
      };
      void waitWebAdmin() {
        client->welcome(engine->broker.tradesHistory);
        client->welcome(engine->broker);
        client->clickme(engine->broker.btn.cleanTradesClosed KISS {
          engine->broker.tradesHistory.clearClosed();
        });
        client->clickme(engine->broker.btn.cleanTrades KISS {
          engine->broker.tradesHistory.clearAll();
        });
        client->clickme(engine->broker.btn.cleanTrade KISS {
          if (!butterfly.is_string()) return;
          engine->broker.tradesHistory.clearOne(butterfly.get<string>());
        });
        client->clickme(engine->broker.btn.cancelAll KISS {
          for (mOrder *const it : engine->broker.working())
            engine->cancelOrder(it);
        });
        client->clickme(engine->broker.btn.cancel KISS {
          if (!butterfly.is_string()) return;
          engine->cancelOrder(engine->broker.find(butterfly.get<mRandId>()));
        });
        client->clickme(engine->broker.btn.submit KISS {
          if (!butterfly.is_object()) return;
          engine->placeOrder(
            butterfly.value("side", "") == "Bid" ? mSide::Bid : mSide::Ask,
            butterfly.value("price", 0.0),
            butterfly.value("quantity", 0.0),
            butterfly.value("orderType", "") == "Limit"
              ? mOrderType::Limit
              : mOrderType::Market,
            butterfly.value("timeInForce", "") == "GTC"
              ? mTimeInForce::GTC
              : (butterfly.value("timeInForce", "") == "FOK"
                ? mTimeInForce::FOK
                : mTimeInForce::IOC
              ),
            false,
            false
          );
        });
      };
  };
}

#endif
