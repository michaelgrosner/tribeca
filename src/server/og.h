#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass,
            public Broker { public: OG() { broker = this; };
    private:
      mButtonSubmitNewOrder       buttonSubmitNewOrder;
      mButtonCancelOrder          buttonCancelOrder;
      mButtonCancelAllOrders      buttonCancelAllOrders;
      mButtonCleanAllClosedTrades buttonCleanAllClosedTrades;
      mButtonCleanAllTrades       buttonCleanAllTrades;
      mButtonCleanTrade           buttonCleanTrade;
    protected:
      void load() {
        sqlite->backup(&tradesHistory);
      };
      void waitData() {
        gw->RAWDATA_ENTRY_POINT(mOrder, {                           PRETTY_DEBUG
          DEBOG("reply  " + rawdata.orderId + "::" + rawdata.exchangeId + " [" + to_string((int)rawdata.orderStatus) + "]: " + str8(rawdata.quantity) + "/" + str8(rawdata.tradeQuantity) + " at price " + str8(rawdata.price));
          updateOrderState(rawdata);
        });
      };
      void waitWebAdmin() {
        client->welcome(tradesHistory);
        client->welcome(orders);
        client->clickme(buttonCancelAllOrders KISS {
          cancelOpenOrders();
        });
        client->clickme(buttonCleanAllClosedTrades KISS {
          cleanClosedTrades();
        });
        client->clickme(buttonCleanAllTrades KISS {
          cleanTrade();
        });
        client->clickme(buttonCleanTrade KISS {
          if (!butterfly.is_string()) return;
          cleanTrade(butterfly.get<string>());
        });
        client->clickme(buttonCancelOrder KISS {
          if (!butterfly.is_string()) return;
          mRandId orderId = butterfly.get<mRandId>();
          if (orderId.empty() or orders.orders.find(orderId) == orders.orders.end()) return;
          cancelOrder(orderId);
        });
        client->clickme(buttonSubmitNewOrder KISS {
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
        if (orderId.empty()) return;
        mOrder *o = &orders.orders[orderId];
        if (o->exchangeId.empty() or o->_waitingCancel + 3e+3 > Tstamp) return;
        o->_waitingCancel = Tstamp;
        DEBOG("cancel " + ((o->side == mSide::Bid ? "BID id " : "ASK id ") + o->orderId) + "::" + o->exchangeId);
        gw->cancel(o->orderId, o->exchangeId);
      };
      void cleanOrder(const mRandId &orderId) {
        DEBOG("remove " + orderId);
        map<mRandId, mOrder>::iterator it = orders.orders.find(orderId);
        if (it != orders.orders.end()) orders.orders.erase(it);
      };
    private:
      void updateOrderState(mOrder k) {
        bool saved = k.orderStatus != mStatus::New,
             working = k.orderStatus == mStatus::Working;
        if (!saved) orders.orders[k.orderId] = k;
        else if (k.orderId.empty() and !k.exchangeId.empty())
          for (map<mRandId, mOrder>::value_type &it : orders.orders)
            if (k.exchangeId == it.second.exchangeId) {
              k.orderId = it.first;
              break;
            }
        if (k.orderId.empty() or orders.orders.find(k.orderId) == orders.orders.end()) return;
        mOrder *o = &orders.orders[k.orderId];
        o->orderStatus = k.orderStatus;
        if (!k.exchangeId.empty()) o->exchangeId = k.exchangeId;
        if (k.price) o->price = k.price;
        if (k.quantity) o->quantity = k.quantity;
        if (k.time) o->time = k.time;
        if (k.latency) o->latency = k.latency;
        if (!o->time) o->time = Tstamp;
        if (!o->latency and working) o->latency = Tstamp - o->time;
        if (o->latency) o->time = Tstamp;
        if (k.tradeQuantity) {
          toHistory(o, k.tradeQuantity);
          gw->refreshWallet = true;
        }
        k.side = o->side;
        if (saved and !working)
          cleanOrder(o->orderId);
        else DEBOG(" saved " + ((o->side == mSide::Bid ? "BID id " : "ASK id ") + o->orderId) + "::" + o->exchangeId + " [" + to_string((int)o->orderStatus) + "]: " + str8(o->quantity) + " " + o->pair.base + " at price " + str8(o->price) + " " + o->pair.quote);
        DEBOG("memory " + to_string(orders.orders.size()));
        if (saved) {
          wallet->position.reset(k.side, orders.calcHeldAmount(k.side));
          wallet->position.send_ratelimit(market->levels);
          toClient(working);
        }
      };
      void cancelOpenOrders() {
        for (map<mRandId, mOrder>::value_type &it : orders.orders)
          if (mStatus::New == it.second.orderStatus or mStatus::Working == it.second.orderStatus)
            cancelOrder(it.first);
      };
      void cleanClosedTrades() {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();)
          if (it->Kqty < it->quantity) ++it;
          else {
            it->Kqty = -1;
            it = tradesHistory.send_push_erase(it);
          }
      };
      void cleanTrade(string k = "") {
        bool all = k.empty();
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();)
          if (!all and it->tradeId != k) ++it;
          else {
            it->Kqty = -1;
            it = tradesHistory.send_push_erase(it);
            if (!all) break;
          }
      };
      void toClient(bool working) {
        screen->log(orders.orders, working);
        orders.send();
      };
      void toHistory(mOrder *o, double tradeQuantity) {
        mAmount fee = 0;
        mTrade trade(
          to_string(Tstamp),
          o->pair,
          o->price,
          tradeQuantity,
          o->side,
          o->time,
          abs(o->price * tradeQuantity),
          0, 0, 0, 0, 0, fee, false
        );
        wallet->safety.recentTrades.insert(o->side, o->price, tradeQuantity);
        wallet->calcSafety();
        screen->log(trade, o->isPong);
        if (qp._matchPings) {
          mPrice widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * trade.price / 100
            : qp.widthPong;
          map<mPrice, string> matches;
          for (mTrade &it : tradesHistory)
            if (it.quantity - it.Kqty > 0
              and it.side != trade.side
              and (qp.pongAt == mPongAt::AveragePingFair
                or qp.pongAt == mPongAt::AveragePingAggressive
                or (trade.side == mSide::Bid
                  ? (it.price > trade.price + widthPong)
                  : (it.price < trade.price - widthPong)
                )
              )
            ) matches[it.price] = it.tradeId;
          matchPong(
            matches,
            trade,
            (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) ? trade.side == mSide::Ask : trade.side == mSide::Bid
          );
        } else {
          tradesHistory.send_push_back(trade);
        }
        if (qp.cleanPongsAuto) cleanAuto(trade.time);
      };
      void matchPong(map<mPrice, string> matches, mTrade pong, bool reverse) {
        if (reverse) for (map<mPrice, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<mPrice, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (mTrades::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it) {
            if (it->price!=pong.price or it->side!=pong.side or it->quantity<=it->Kqty) continue;
            eq = true;
            it->time = pong.time;
            it->quantity = it->quantity + pong.quantity;
            it->value = it->value + pong.value;
            it->loadedFromDB = false;
            it = tradesHistory.send_push_erase(it);
            break;
          }
          if (!eq) {
            tradesHistory.send_push_back(pong);
          }
        }
      };
      bool matchPong(string match, mTrade* pong) {
        for (mTrades::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it) {
          if (it->tradeId != match) continue;
          mAmount Kqty = fmin(pong->quantity, it->quantity - it->Kqty);
          it->Ktime = pong->time;
          it->Kprice = ((Kqty*pong->price) + (it->Kqty*it->Kprice)) / (it->Kqty+Kqty);
          it->Kqty = it->Kqty + Kqty;
          it->Kvalue = abs(it->Kqty*it->Kprice);
          pong->quantity = pong->quantity - Kqty;
          pong->value = abs(pong->price*pong->quantity);
          if (it->quantity<=it->Kqty)
            it->Kdiff = abs(it->quantity * it->price - it->Kqty * it->Kprice);
          it->loadedFromDB = false;
          it = tradesHistory.send_push_erase(it);
          break;
        }
        return pong->quantity > 0;
      };
      void cleanAuto(mClock now) {
        mClock pT_ = now - (abs(qp.cleanPongsAuto) * 86400e3);
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();)
          if ((it->Ktime?:it->time) < pT_ and (qp.cleanPongsAuto < 0 or it->Kqty >= it->quantity)) {
            it->Kqty = -1;
            it = tradesHistory.send_push_erase(it);
          } else ++it;
      };
  };
}

#endif
