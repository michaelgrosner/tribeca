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
          for (mOrder &it : orders.working())
            cancelOrder(it.orderId);
        });
        client->clickme(orders.btn.cancel KISS {
          if (!butterfly.is_string()) return;
          cancelOrder(butterfly.get<mRandId>());
        });
        client->clickme(orders.btn.submit KISS {
          if (!butterfly.is_object()) return;
          sendOrder(
            "",
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
      void sendOrder(
        const mRandId      &replaceOrderId,
        const mSide        &side          ,
        const mPrice       &price         ,
        const mAmount      &qty           ,
        const mOrderType   &type          ,
        const mTimeInForce &tif           ,
        const bool         &isPong        ,
        const bool         &postOnly
      ) {
        if (gw->replace and !replaceOrderId.empty()) {
          mRandId replaceExchangeId = orders.replace(replaceOrderId, price);
          if (!replaceExchangeId.empty())
            gw->replace(replaceExchangeId, str8(price));
        } else {
          if (args.testChamber != 1) cancelOrder(replaceOrderId);
          mOrder *o = orders.upsert(mOrder(
            gw->randId(),
            side,
            qty,
            type,
            isPong,
            price,
            tif,
            mStatus::New,
            postOnly
          ), true);
          gw->place(
            o->orderId,
            o->side,
            str8(o->price),
            str8(o->quantity),
            o->type,
            o->timeInForce,
            o->preferPostOnly
          );
          if (args.testChamber == 1) cancelOrder(replaceOrderId);
        }
        engine->monitor.tick_orders();
      };
      void cancelOrder(const mRandId &orderId) {
        mOrder *orderWaitingCancel = orders.cancel(orderId);
        if (orderWaitingCancel)
          gw->cancel(
            orderWaitingCancel->orderId,
            orderWaitingCancel->exchangeId
          );
      };
      void cancelOrders(const vector<mRandId> &toCancel) {
        for_each(
          toCancel.begin(), toCancel.end(),
          [&](const mRandId &orderId) {
            cancelOrder(orderId);
          }
        );
      };
  };
}

#endif
