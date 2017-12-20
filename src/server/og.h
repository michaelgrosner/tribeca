#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  class OG: public Klass {
    public:
      map<string, mOrder> orders;
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
          debug(string("reply  ") + k.orderId + "::" + k.exchangeId + " [" + to_string((int)k.orderStatus) + "]: " + k.quantity2str() + "/" + k.tradeQuantity2str() + " at price " + k.price2str());
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
        mOrder o = updateOrderState(mOrder(gw->randId(), mPair(gw->base, gw->quote), oS, oQ, oLM, oIP, FN::roundSide(oP, gw->minTick, oS), oTIF, mStatus::New, oPO));
        debug(string(" send  ") + (o.side == mSide::Bid ? "BID id " : "ASK id ") + o.orderId + ": " + o.quantity2str() + " " + o.pair.base + " at price " + o.price2str() + " " + o.pair.quote);
        gw->send(o.orderId, o.side, o.price2str(), o.quantity2str(), o.type, o.timeInForce, o.preferPostOnly, o.time);
        ((UI*)client)->orders60sec++;
      };
      void cancelOrder(string k) {
        if (orders.find(k) == orders.end() or orders[k].exchangeId == "") return;
        mOrder o = orders[k];
        debug(string("cancel ") + (o.side == mSide::Bid ? "BID id " : "ASK id ") + o.orderId + "::" + o.exchangeId);
        gw->cancel(o.orderId, o.exchangeId, o.side, o.time);
      };
      void cleanOrder(string oI) {
        map<string, mOrder>::iterator it = orders.find(oI);
        if (it != orders.end()) orders.erase(it);
        debug(string("remove ") + oI);
      };
    private:
      function<void(json*)> helloTrades = [&](json *welcome) {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end(); ++it) {
          it->loadedFromDB = true;
          welcome->push_back(*it);
        }
      };
      function<void(json*)> helloOrders = [&](json *welcome) {
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it) {
          if (mStatus::Working != it->second.orderStatus) continue;
          welcome->push_back(it->second);
        }
      };
      function<void(json)> kissCancelAllOrders = [&](json butterfly) {
        cancelOpenOrders();
      };
      function<void(json)> kissCleanAllClosedTrades = [&](json butterfly) {
        cleanClosedTrades();
      };
      function<void(json)> kissCleanAllTrades = [&](json butterfly) {
        cleanTrade("", true);
      };
      function<void(json)> kissCancelOrder = [&](json butterfly) {
        if (butterfly.is_object() and butterfly["orderId"].is_string())
          cancelOrder(butterfly["orderId"].get<string>());
      };
      function<void(json)> kissCleanTrade = [&](json butterfly) {
        if (butterfly.is_object() and butterfly["tradeId"].is_string())
          cleanTrade(butterfly["tradeId"].get<string>());
      };
      function<void(json)> kissSubmitNewOrder = [&](json butterfly) {
        sendOrder(
          butterfly.value("side", "") == "Bid" ? mSide::Bid : mSide::Ask,
          butterfly.value("price", 0.0),
          butterfly.value("quantity", 0.0),
          butterfly.value("orderType", "") == "Limit" ? mOrderType::Limit : mOrderType::Market,
          butterfly.value("timeInForce", "") == "GTC" ? mTimeInForce::GTC : (butterfly.value("timeInForce", "") == "FOK" ? mTimeInForce::FOK : mTimeInForce::IOC),
          false,
          false
        );
      };
      mOrder updateOrderState(mOrder k) {
        mOrder o;
        if (k.orderStatus == mStatus::New) o = k;
        else if (k.orderId != "" and orders.find(k.orderId) != orders.end())
          o = orders[k.orderId];
        else if (k.exchangeId != "")
          for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
            if (it->second.exchangeId == k.exchangeId) {
              o = it->second;
              break;
            }
        if (!o.orderId.length()) return o;
        if (k.exchangeId.length()) o.exchangeId = k.exchangeId;
        if (k.orderStatus != mStatus::New) o.orderStatus = k.orderStatus;
        if (k.price) o.price = k.price;
        if (k.quantity) o.quantity = k.quantity;
        if (k.time) o.time = k.time;
        if (k.computationalLatency) o.computationalLatency = k.computationalLatency;
        if (!o.time) o.time = FN::T();
        if (!o.computationalLatency and o.orderStatus == mStatus::Working)
          o.computationalLatency = FN::T() - o.time;
        if (o.computationalLatency) o.time = FN::T();
        toMemory(o);
        ((EV*)events)->ogOrder(o);
        if (o.orderStatus != mStatus::New)
          toClient();
        toHistory(o, k.tradeQuantity);
        return o;
      };
      void cancelOpenOrders() {
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
          if (mStatus::New == it->second.orderStatus or mStatus::Working == it->second.orderStatus)
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
      void cleanTrade(string k, bool all = false) {
        for (vector<mTrade>::iterator it = tradesHistory.begin(); it != tradesHistory.end();) {
          if (!all and it->tradeId != k) ++it;
          else {
            it->Kqty = -1;
            ((UI*)client)->send(uiTXT::Trades, *it);
            ((DB*)memory)->insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesHistory.erase(it);
            if (!all) break;
          }
        }
      };
      void toClient() {
        json k = json::array();
        for (map<string, mOrder>::iterator it = orders.begin(); it != orders.end(); ++it)
          if (it->second.orderStatus == mStatus::Working)
            k.push_back(it->second);
        ((UI*)client)->send(uiTXT::OrderStatusReports, k, true);
      };
      void toHistory(mOrder o, double qty) {
        if (!qty) return;
        double fee = 0;
        mTrade trade(
          to_string(FN::T()),
          o.pair,
          o.price,
          qty,
          o.side,
          o.time,
          abs(o.price * qty),
          0, 0, 0, 0, 0, fee, false
        );
        ((EV*)events)->ogTrade(trade);
        FN::log(trade, gw->name);
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
            it->Kdiff = abs(it->quantity * it->price - it->Kqty * it->Kprice);
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
        if (k.orderStatus == mStatus::New or k.orderStatus == mStatus::Working) {
          orders[k.orderId] = k;
          debug(string(" save  ") + (k.side == mSide::Bid ? "BID id " : "ASK id ") + k.orderId + "::" + k.exchangeId + " [" + to_string((int)k.orderStatus) + "]: " + k.quantity2str() + " " + k.pair.base + " at price " + k.price2str() + " " + k.pair.quote);
        } else cleanOrder(k.orderId);
        debug(string("memory ") + to_string(orders.size()));
      };
      function<void(string)> debug = [&](string k) {
        FN::log("DEBUG", string("OG ") + k);
      };
  };
}

#endif
