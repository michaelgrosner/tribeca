#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass,
            public Broker { public: OG() { broker = this; };
    protected:
      void load() {
        sqlite->backup(&orders.tradesHistory);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          orders.upsert(rawdata, &wallet->balance, market->levels, &gw->refreshWallet);
        });
      };
      void waitSysAdmin() {
        screen->printme(&orders.tradesHistory);
        screen->printme(&orders);
      };
      void waitWebAdmin() {
        client->welcome(orders.tradesHistory);
        client->welcome(orders);
        client->clickme(orders.btn.cleanTradesClosed KISS {
          orders.tradesHistory.clearClosed();
        });
        client->clickme(orders.btn.cleanTrades KISS {
          orders.tradesHistory.clearAll();
        });
        client->clickme(orders.btn.cleanTrade KISS {
          if (!butterfly.is_string()) return;
          orders.tradesHistory.clearOne(butterfly.get<string>());
        });
        client->clickme(orders.btn.cancelAll KISS {
          cancelOrders(orders.working());
        });
        client->clickme(orders.btn.cancel KISS {
          if (!butterfly.is_string()) return;
          cancelOrder(orders.find(butterfly.get<mRandId>()));
        });
        client->clickme(orders.btn.submit KISS {
          if (!butterfly.is_object()) return;
          sendOrder(
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
    public:
      void sendAllQuotes(const vector<mOrder*> &toCancel, mOrder *const toReplace, const mLevel &quote, const mSide &side, const bool &isPong) {
        cancelOrders(toCancel);
        if (toReplace) {
          if (gw->replace)
            return replaceOrder(toReplace, quote.price);
          if (args.testChamber != 1)
            cancelOrder(toReplace);
        }
        sendOrder(side, quote.price, quote.size, mOrderType::Limit, mTimeInForce::GTC, isPong, true);
        if (args.testChamber == 1 and toReplace)
          cancelOrder(toReplace);
      };
      void stopAllQuotes(const mSide &side) {
        cancelOrders(orders.working(side));
      };
    private:
      void sendOrder(
        const mSide        &side    ,
        const mPrice       &price   ,
        const mAmount      &qty     ,
        const mOrderType   &type    ,
        const mTimeInForce &tif     ,
        const bool         &isPong  ,
        const bool         &postOnly
      ) {
        mOrder *const o = orders.upsert(mOrder(gw->randId(), side, qty, type, isPong, price,tif, mStatus::New, postOnly), true);
        gw->place(o->orderId, o->side, str8(o->price), str8(o->quantity), o->type, o->timeInForce, o->preferPostOnly);
        engine->monitor.tick_orders();
      };
      void replaceOrder(mOrder *const toReplace, const mPrice &price) {
        if (orders.replace(toReplace, price)) {
          gw->replace(toReplace->exchangeId, str8(toReplace->price));
          engine->monitor.tick_orders();
        }
      };
      void cancelOrder(mOrder *const toCancel) {
        if (orders.cancel(toCancel))
          gw->cancel(toCancel->orderId, toCancel->exchangeId);
      };
      void cancelOrders(const vector<mOrder*> &toCancel) {
        for (mOrder *const it : toCancel)
          cancelOrder(it);
      };
  };
}

#endif
