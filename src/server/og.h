#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass {
    private:
      map<string, mOrder> allOrders;
      map<string, string> allOrdersIds;
      mutex ogMutex;
    public:
      vector<mTrade> tradesHistory;
    protected:
      void load() {
        json k = ((DB*)memory)->load(uiTXT::Trades);
        if (k.size())
          for (json::reverse_iterator it = k.rbegin(); it != k.rend(); ++it)
            tradesHistory.push_back(*it);
        FN::log("DB", string("loaded ") + to_string(tradesHistory.size()) + " historical Trades");
      };
      void waitData() {
        gw->evDataOrder = [&](mOrder k) {
          ((EV*)events)->debug("OG evDataOrder");
          debug(string("reply  ") + k.orderId + "::" + k.exchangeId + " [" + to_string((int)k.orderStatus) + "]: " + k.quantity2str() + "/" + k.lastQuantity2str() + " at price " + k.price2str());
          updateOrderState(k);
        };
      };
      void waitUser() {
        ((UI*)client)->welcome(uiTXT::Trades, &helloTrades);
        ((UI*)client)->welcome(uiTXT::OrderStatusReports, &helloOrders);
        ((UI*)client)->clickme(uiTXT::SubmitNewOrder, &kissSubmitNewOrder);
        ((UI*)client)->clickme(uiTXT::CancelOrder, &kissCancelOrder);
        ((UI*)client)->clickme(uiTXT::CancelAllOrders, &kissCancelAllOrders);
        ((UI*)client)->clickme(uiTXT::CleanAllClosedTrades, &kissCleanAllClosedTrades);
        ((UI*)client)->clickme(uiTXT::CleanAllTrades, &kissCleanAllTrades);
        ((UI*)client)->clickme(uiTXT::CleanTrade, &kissCleanTrade);
      };
      void run() {
        if (((CF*)config)->argDebugOrders) return;
        debug = [&](string k) {};
      };
    public:
      void sendOrder(mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oIP, bool oPO) {
        mOrder o = updateOrderState(mOrder(gw->randId(), gw->exchange, mPair(gw->base, gw->quote), oS, oQ, oLM, oIP, FN::roundSide(oP, gw->minTick, oS), oTIF, mORS::New, oPO));
        debug(string(" send  ") + (o.side == mSide::Bid ? "BID id " : "ASK id ") + o.orderId + ": " + o.quantity2str() + " " + o.pair.base + " at price " + o.price2str() + " " + o.pair.quote);
        gw->send(o.orderId, o.side, o.price2str(), o.quantity2str(), o.type, o.timeInForce, o.preferPostOnly, o.time);
        ++((UI*)client)->orders60sec;
      };
      void cancelOrder(string k) {
        map<string, mOrder> orders = ordersBothSides();
        if (orders.find(k) == orders.end() or orders[k].exchangeId == "") return;
        mOrder o = orders[k];
        debug(string("cancel ") + (o.side == mSide::Bid ? "BID id " : "ASK id ") + o.orderId + "::" + o.exchangeId);
        gw->cancel(o.orderId, o.exchangeId, o.side, o.time);
      };
      void cleanOrder(string oI, string oE) {
        ogMutex.lock();
        map<string, mOrder>::iterator it = allOrders.find(oI);
        if (it != allOrders.end()) allOrders.erase(it);
        if (oE != "") {
          map<string, string>::iterator it_ = allOrdersIds.find(oE);
          if (it_ != allOrdersIds.end()) allOrdersIds.erase(it_);
        } else {
          for (map<string, string>::iterator it_ = allOrdersIds.begin(); it_ != allOrdersIds.end();)
            if (it_->second == oI) it_ = allOrdersIds.erase(it_); else ++it_;
        }
        ogMutex.unlock();
        debug(string("remove ") + oI + "::" + oE);
      };
      void countOrders(unsigned int *qNew, unsigned int *qWorking, unsigned int *qDone) {
        map<string, mOrder> orders = ordersBothSides();
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
          if ((mORS)it->second.orderStatus == mORS::New) ++(*qNew);
          else if ((mORS)it->second.orderStatus == mORS::Working) ++(*qWorking);
          else ++(*qDone);
      };
      map<string, mOrder> ordersBothSides() {
        lock_guard<mutex> lock(ogMutex);
        return allOrders;
      };
      multimap<double, mOrder> ordersAtSide(mSide side) {
        multimap<double, mOrder> ordersSide;
        map<string, mOrder> orders = ordersBothSides();
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
          if ((mSide)it->second.side == side)
            ordersSide.insert(pair<double, mOrder>(it->second.price, it->second));
        return ordersSide;
      };
    private:
      function<json()> helloTrades = [&]() {
        json k;
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it) {
          it->loadedFromDB = true;
          k.push_back(*it);
        }
        return k;
      };
      function<json()> helloOrders = [&]() {
        json k;
        map<string, mOrder> orders = ordersBothSides();
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it) {
          if (mORS::Working != it->second.orderStatus) continue;
          k.push_back(it->second);
        }
        return k;
      };
      function<void(json)> kissCancelAllOrders = [&](json k) {
        cancelOpenOrders();
      };
      function<void(json)> kissCleanAllClosedTrades = [&](json k) {
        cleanClosedTrades();
      };
      function<void(json)> kissCleanAllTrades = [&](json k) {
        cleanTrades();
      };
      function<void(json)> kissCancelOrder = [&](json k) {
        if (k.is_object() and k["orderId"].is_string())
          cancelOrder(k["orderId"].get<string>());
        else FN::logWar("JSON", "Missing orderId at kissCancelOrder, ignored");
      };
      function<void(json)> kissCleanTrade = [&](json k) {
        if (k.is_object() and k["tradeId"].is_string())
          cleanTrade(k["tradeId"].get<string>());
        else FN::logWar("JSON", "Missing tradeId at kissCleanTrade, ignored");
      };
      function<void(json)> kissSubmitNewOrder = [&](json k) {
        sendOrder(
          k.value("side", "") == "Bid" ? mSide::Bid : mSide::Ask,
          k.value("price", 0.0),
          k.value("quantity", 0.0),
          k.value("orderType", "") == "Limit" ? mOrderType::Limit : mOrderType::Market,
          k.value("timeInForce", "") == "GTC" ? mTimeInForce::GTC : (k.value("timeInForce", "") == "FOK" ? mTimeInForce::FOK : mTimeInForce::IOC),
          false,
          false
        );
      };
      mOrder updateOrderState(mOrder k) {
        mOrder o;
        map<string, mOrder> orders = ordersBothSides();
        if (k.orderStatus == mORS::New) o = k;
        else if (k.orderId != "" and orders.find(k.orderId) != orders.end())
          o = orders[k.orderId];
        else if (k.exchangeId != "") {
          ogMutex.lock();
          if (allOrdersIds.find(k.exchangeId) != allOrdersIds.end()) {
            o = orders[allOrdersIds[k.exchangeId]];
            k.orderId = o.orderId;
          } else { ogMutex.unlock(); return o; }
          ogMutex.unlock();
        } else return o;
        if (k.orderId!="") o.orderId = k.orderId;
        if (k.exchangeId!="") o.exchangeId = k.exchangeId;
        if ((int)k.exchange!=0) o.exchange = k.exchange;
        if (k.pair.base!="") o.pair.base = k.pair.base;
        if (k.pair.quote!="") o.pair.quote = k.pair.quote;
        if ((int)k.side!=0) o.side = k.side;
        if (k.quantity!=0) o.quantity = k.quantity;
        if ((int)k.type!=0) o.type = k.type;
        if (k.isPong) o.isPong = k.isPong;
        if (k.price!=0) o.price = k.price;
        if ((int)k.timeInForce!=0) o.timeInForce = k.timeInForce;
        if ((int)k.orderStatus!=0) o.orderStatus = k.orderStatus;
        if (k.preferPostOnly) o.preferPostOnly = k.preferPostOnly;
        if (k.lastQuantity!=0) o.lastQuantity = k.lastQuantity;
        if (k.time) o.time = k.time;
        if (k.computationalLatency) o.computationalLatency = k.computationalLatency;
        if (!o.time) o.time = FN::T();
        if (!o.computationalLatency and o.orderStatus == mORS::Working)
          o.computationalLatency = FN::T() - o.time;
        if (o.computationalLatency) o.time = FN::T();
        toMemory(o);
        ((EV*)events)->ogOrder(o);
        if (o.orderStatus != mORS::New) toClient();
        if (k.lastQuantity > 0) toHistory(o);
        return o;
      };
      void cancelOpenOrders() {
        map<string, mOrder> orders = ordersBothSides();
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
          if (mORS::New == (mORS)it->second.orderStatus or mORS::Working == (mORS)it->second.orderStatus)
            cancelOrder(it->first);
      };
      void cleanClosedTrades() {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();) {
          if (it->Kqty+0.0001 < it->quantity) ++it;
          else {
            it->Kqty = -1;
            ((UI*)client)->send(uiTXT::Trades, *it);
            ((DB*)memory)->insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesHistory.erase(it);
          }
        }
      };
      void cleanTrades() {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();) {
          it->Kqty = -1;
          ((UI*)client)->send(uiTXT::Trades, *it);
          ((DB*)memory)->insert(uiTXT::Trades, {}, false, it->tradeId);
          it = tradesHistory.erase(it);
        }
      };
      void cleanTrade(string k) {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();) {
          if (it->tradeId != k) ++it;
          else {
            it->Kqty = -1;
            ((UI*)client)->send(uiTXT::Trades, *it);
            ((DB*)memory)->insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesHistory.erase(it);
            break;
          }
        }
      };
      void toClient() {
        json k;
        map<string, mOrder> orders = ordersBothSides();
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
          if ((mORS)it->second.orderStatus == mORS::Working)
            k.push_back(it->second);
        ((UI*)client)->send(uiTXT::OrderStatusReports, k, true);
      };
      void toHistory(mOrder o) {
        double fee = 0;
        double val = abs(o.price * o.lastQuantity);
        mTrade trade(
          to_string(FN::T()),
          o.exchange,
          o.pair,
          o.price,
          o.lastQuantity,
          o.side,
          o.time,
          val, 0, 0, 0, 0, 0, fee, false
        );
        FN::log(trade, gw->name);
        ((EV*)events)->ogTrade(trade);
        if (qp->_matchPings) {
          double widthPong = qp->widthPercentage
            ? qp->widthPongPercentage * trade.price / 100
            : qp->widthPong;
          map<double, string> matches;
          for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it)
            if (it->quantity - it->Kqty > 0
              and it->side == (trade.side == mSide::Bid ? mSide::Ask : mSide::Bid)
              and (trade.side == mSide::Bid ? (it->price > trade.price + widthPong) : (it->price < trade.price - widthPong))
            ) matches[it->price] = it->tradeId;
          matchPong(matches, (qp->pongAt == mPongAt::LongPingFair or qp->pongAt == mPongAt::LongPingAggressive) ? trade.side == mSide::Ask : trade.side == mSide::Bid, trade);
        } else {
          ((UI*)client)->send(uiTXT::Trades, trade);
          ((DB*)memory)->insert(uiTXT::Trades, trade, false, trade.tradeId);
          tradesHistory.push_back(trade);
        }
        ((UI*)client)->send(uiTXT::TradesChart, {
          {"price", trade.price},
          {"side", (int)trade.side},
          {"quantity", trade.quantity},
          {"value", trade.value},
          {"pong", o.isPong}
        });
        cleanAuto(trade.time, qp->cleanPongsAuto);
      };
      void matchPong(map<double, string> matches, bool reverse, mTrade pong) {
        if (reverse) for (map<double, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<double, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it) {
            if (it->price!=pong.price or it->side!=pong.side or it->quantity<=it->Kqty) continue;
            eq = true;
            it->time = pong.time;
            it->quantity = it->quantity + pong.quantity;
            it->value = it->value + pong.value;
            it->loadedFromDB = false;
            ((UI*)client)->send(uiTXT::Trades, *it);
            ((DB*)memory)->insert(uiTXT::Trades, *it, false, it->tradeId);
            break;
          }
          if (!eq) {
            ((UI*)client)->send(uiTXT::Trades, pong);
            ((DB*)memory)->insert(uiTXT::Trades, pong, false, pong.tradeId);
            tradesHistory.push_back(pong);
          }
        }
      };
      bool matchPong(string match, mTrade* pong) {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it) {
          if (it->tradeId != match) continue;
          double Kqty = fmin(pong->quantity, it->quantity - it->Kqty);
          it->Ktime = pong->time;
          it->Kprice = ((Kqty*pong->price) + (it->Kqty*it->Kprice)) / (it->Kqty+Kqty);
          it->Kqty = it->Kqty + Kqty;
          it->Kvalue = abs(it->Kqty*it->Kprice);
          pong->quantity = pong->quantity - Kqty;
          pong->value = abs(pong->price*pong->quantity);
          if (it->quantity<=it->Kqty)
            it->Kdiff = abs((it->quantity*it->price)-(it->Kqty*it->Kprice));
          it->loadedFromDB = false;
          ((UI*)client)->send(uiTXT::Trades, *it);
          ((DB*)memory)->insert(uiTXT::Trades, *it, false, it->tradeId);
          break;
        }
        return pong->quantity > 0;
      };
      void cleanAuto(unsigned long k, double pT) {
        if (pT == 0) return;
        unsigned long pT_ = k - (abs(pT) * 864e5);
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();) {
          if (it->time < pT_ and (pT < 0 or it->Kqty >= it->quantity)) {
            it->Kqty = -1;
            ((UI*)client)->send(uiTXT::Trades, *it);
            ((DB*)memory)->insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesHistory.erase(it);
          } else ++it;
        }
      };
      void toMemory(mOrder k) {
        if (k.orderStatus != mORS::Cancelled and k.orderStatus != mORS::Complete) {
          ogMutex.lock();
          if (k.exchangeId != "")
            allOrdersIds[k.exchangeId] = k.orderId;
          allOrders[k.orderId] = k;
          ogMutex.unlock();
          debug(string(" save  ") + (k.side == mSide::Bid ? "BID id " : "ASK id ") + k.orderId + "::" + k.exchangeId + " [" + to_string((int)k.orderStatus) + "]: " + k.quantity2str() + " " + k.pair.base + " at price " + k.price2str() + " " + k.pair.quote);
        } else cleanOrder(k.orderId, k.exchangeId);
        debug(string("memory ") + to_string(allOrders.size()) + "/" + to_string(allOrdersIds.size()));
      };
      function<void(string)> debug = [&](string k) {
        FN::log("DEBUG", string("OG ") + k);
      };
  };
}

#endif
