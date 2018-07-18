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
          updateOrderState(rawdata);
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
          updateOrderState(mOrder(
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
          ));
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
    private:
      void updateOrderState(mOrder k) {
        mOrder *o = orders.find(k);
        if (!o) return;
        bool saved = k.orderStatus != mStatus::New,
             working = k.orderStatus == mStatus::Working;
        o->orderStatus = k.orderStatus;
        if (!k.exchangeId.empty()) o->exchangeId = k.exchangeId;
        if (k.price) o->price = k.price;
        if (k.quantity) o->quantity = k.quantity;
        if (k.time) o->time = k.time;
        if (k.latency) o->latency = k.latency;
        if (!o->time) o->time = Tstamp;
        if (!o->latency and working) o->latency = Tstamp - o->time;
        if (o->latency) o->time = Tstamp;
        if (k.tradeQuantity)
          orders.tradesHistory.insert(o, k.tradeQuantity);
        k.side = o->side;
        k.price = o->price;
        if (saved and !working)
          orders.erase(o->orderId);
        else DEBOG(" saved " + ((o->side == mSide::Bid ? "BID id " : "ASK id ") + o->orderId) + "::" + o->exchangeId + " [" + to_string((int)o->orderStatus) + "]: " + str8(o->quantity) + " " + o->pair.base + " at price " + str8(o->price) + " " + o->pair.quote);
        DEBOG("memory " + to_string(orders.orders.size()));
        if (saved) {
          wallet->balance.reset(k.side, orders.calcHeldAmount(k.side), market->levels);
          if (k.tradeQuantity) {
            wallet->balance.target.safety.recentTrades.insert(k.side, k.price, k.tradeQuantity);
            wallet->balance.target.safety.calc(market->levels, orders.tradesHistory);
            gw->refreshWallet = true;
          }
          orders.refresh();
          orders.send();
        }
      };
  };
}

#endif
