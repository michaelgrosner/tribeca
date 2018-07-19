#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass,
            public Broker { public: OG() { broker = this; };
    private:
      mButtons btn;
    protected:
      void load() {
        sqlite->backup(&orders.tradesHistory);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          DEBOG("reply  " + rawdata.orderId + "::" + rawdata.exchangeId + " [" + to_string((int)rawdata.orderStatus) + "]: " + str8(rawdata.quantity) + "/" + str8(rawdata.tradeQuantity) + " at price " + str8(rawdata.price));
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
        client->clickme(btn.cleanAllClosedTrades KISS {
          orders.tradesHistory.clearClosed();
        });
        client->clickme(btn.cleanAllTrades KISS {
          orders.tradesHistory.clearAll();
        });
        client->clickme(btn.cleanTrade KISS {
          if (!butterfly.is_string()) return;
          orders.tradesHistory.clearOne(butterfly.get<string>());
        });
        client->clickme(btn.cancelAllOrders KISS {
          for (mOrder &it : orders.working())
            cancelOrder(it.orderId);
        });
        client->clickme(btn.cancelOrder KISS {
          if (!butterfly.is_string()) return;
          cancelOrder(butterfly.get<mRandId>());
        });
        client->clickme(btn.submitNewOrder KISS {
          if (!butterfly.is_object()) return;
          sendOrder(
            {},
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
              vector<mRandId>  toCancel,
        const mSide           &side    ,
        const mPrice          &price   ,
        const mAmount         &qty     ,
        const mOrderType      &type    ,
        const mTimeInForce    &tif     ,
        const bool            &isPong  ,
        const bool            &postOnly
      ) {
        mRandId replaceOrderId;
        if (!toCancel.empty()) {
          replaceOrderId = side == mSide::Bid ? toCancel.back() : toCancel.front();
          toCancel.erase(side == mSide::Bid ? toCancel.end()-1 : toCancel.begin());
          for (mRandId &it : toCancel) cancelOrder(it);
        }
        if (gw->replace and !replaceOrderId.empty()) {
          if (!orders.orders[replaceOrderId].exchangeId.empty()) {
            DEBOG("update " + ((side == mSide::Bid ? "BID" : "ASK") + (" id " + replaceOrderId)) + ":  at price " + str8(price) + " " + gw->quote);
            orders.orders[replaceOrderId].price = price;
            gw->replace(orders.orders[replaceOrderId].exchangeId, str8(price));
          }
        } else {
          if (args.testChamber != 1) cancelOrder(replaceOrderId);
          mRandId newOrderId = gw->randId();
          orders.upsert(mOrder(
            newOrderId,
            mPair(gw->base, gw->quote),
            side,
            qty,
            type,
            isPong,
            price,
            tif,
            mStatus::New,
            postOnly
          ), &wallet->balance, market->levels, &gw->refreshWallet);
          mOrder *o = &orders.orders[newOrderId];
          DEBOG(" send  " + replaceOrderId + "> " + (o->side == mSide::Bid ? "BID" : "ASK") + " id " + o->orderId + ": " + str8(o->quantity) + " " + o->pair.base + " at price " + str8(o->price) + " " + o->pair.quote);
          gw->place(
            o->orderId,
            o->side,
            str8(o->price),
            str8(o->quantity),
            o->type,
            o->timeInForce,
            o->preferPostOnly,
            o->time
          );
          if (args.testChamber == 1) cancelOrder(replaceOrderId);
        }
        engine->monitor.tick_orders();
      };
      void cancelOrder(const mRandId &orderId) {
        mOrder *orderWaitingCancel = orders.cancel(orderId);
        if (orderWaitingCancel)
          gw->cancel(orderWaitingCancel);
      };
  };
}

#endif
