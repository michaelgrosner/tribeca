#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass {
    protected:
      void load() {
        sqlite->backup(&gw->broker.tradesHistory);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          gw->broker.upsert(rawdata, &gw->wallet, gw->levels, &gw->askForFees);
        });
      };
      void waitSysAdmin() {
        screen->printme(&gw->broker.tradesHistory);
        screen->printme(&gw->broker);
      };
      void waitWebAdmin() {
        client->welcome(gw->broker.tradesHistory);
        client->welcome(gw->broker);
        client->clickme(gw->broker.btn.cleanTradesClosed KISS {
          gw->broker.tradesHistory.clearClosed();
        });
        client->clickme(gw->broker.btn.cleanTrades KISS {
          gw->broker.tradesHistory.clearAll();
        });
        client->clickme(gw->broker.btn.cleanTrade KISS {
          if (!butterfly.is_string()) return;
          gw->broker.tradesHistory.clearOne(butterfly.get<string>());
        });
        client->clickme(gw->broker.btn.cancelAll KISS {
          for (mOrder *const it : gw->broker.working())
            gw->cancelOrder(it);
        });
        client->clickme(gw->broker.btn.cancel KISS {
          if (!butterfly.is_string()) return;
          gw->cancelOrder(gw->broker.find(butterfly.get<mRandId>()));
        });
        client->clickme(gw->broker.btn.submit KISS {
          if (!butterfly.is_object()) return;
          gw->placeOrder(
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
