#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  vector<mTradeHydrated> tradesMemory;
  map<string, void*> toCancel;
  map<string, mOrder> allOrders;
  map<string, string> allOrdersIds;
  class OG {
    public:
      static void main() {
        load();
        ev_gwDataOrder = [](mOrder k) {
          if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW reply  " << k.orderId << "::" << k.exchangeId << " [" << (int)k.orderStatus << "]: " << k.quantity << "/" << k.lastQuantity << " at price " << k.price << "." << endl;
          updateOrderState(k);
        };
        UI::uiSnap(uiTXT::Trades, &onSnapTrades);
        UI::uiSnap(uiTXT::OrderStatusReports, &onSnapOrders);
        UI::uiHand(uiTXT::SubmitNewOrder, &onHandSubmitNewOrder);
        UI::uiHand(uiTXT::CancelOrder, &onHandCancelOrder);
        UI::uiHand(uiTXT::CancelAllOrders, &onHandCancelAllOrders);
        UI::uiHand(uiTXT::CleanAllClosedOrders, &onHandCleanAllClosedOrders);
        UI::uiHand(uiTXT::CleanAllOrders, &onHandCleanAllOrders);
        UI::uiHand(uiTXT::CleanTrade, &onHandCleanTrade);
      };
      static void allOrdersDelete(string oI, string oE) {
        map<string, mOrder>::iterator it = allOrders.find(oI);
        if (it != allOrders.end()) allOrders.erase(it);
        if (oE != "") {
          map<string, string>::iterator it_ = allOrdersIds.find(oE);
          if (it_ != allOrdersIds.end()) allOrdersIds.erase(it_);
        } else {
          for (map<string, string>::iterator it_ = allOrdersIds.begin(); it_ != allOrdersIds.end();)
            if (it_->second == oI) it_ = allOrdersIds.erase(it_); else ++it_;
        }
        if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW remove " << oI << "::" << oE << "." << endl;
      };
      static void sendOrder(mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oIP, bool oPO) {
        mOrder o = updateOrderState(mOrder(gW->clientId(), gw->exchange, mPair(gw->base, gw->quote), oS, oQ, oLM, oIP, FN::roundSide(oP, gw->minTick, oS), oTIF, mORS::New, oPO));
        if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW  send  " << (o.side == mSide::Bid ? "BID id " : "ASK id ") << o.orderId << ": " << o.quantity << " " << mCurrency[o.pair.base] << " at price " << o.price << " " << mCurrency[o.pair.quote] << "." << endl;
        gW->send(o.orderId, o.side, o.price, o.quantity, o.type, o.timeInForce, o.preferPostOnly, o.time);
      };
      static void cancelOrder(string k) {
        if (allOrders.find(k) == allOrders.end()) {
          updateOrderState(mOrder(k, mORS::Cancelled));
          if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW cancel unknown id " << k << "." << endl;
          return;
        }
        if (!gW->cancelByClientId and allOrders[k].exchangeId == "") {
          toCancel[k] = nullptr;
          if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW cancel pending id " << k << "." << endl;
          return;
        }
        mOrder o = updateOrderState(mOrder(k, mORS::Working));
        if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW cancel " << (o.side == mSide::Bid ? "BID id " : "ASK id ") << o.orderId << "::" << o.exchangeId << "." << endl;
        gW->cancel(o.orderId, o.exchangeId, o.side, o.time);
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::Trades);
        if (k.size())
          for (json::iterator it = k.begin(); it != k.end(); ++it)
            tradesMemory.push_back(mTradeHydrated(
              (*it)["tradeId"].get<string>(),
              (*it)["time"].get<unsigned long>(),
              (mExchange)(*it)["exchange"].get<int>(),
              mPair((*it)["/pair/base"_json_pointer].get<int>(), (*it)["/pair/quote"_json_pointer].get<int>()),
              (*it)["price"].get<double>(),
              (*it)["quantity"].get<double>(),
              (mSide)(*it)["side"].get<int>(),
              (*it)["value"].get<double>(),
              (*it)["Ktime"].get<unsigned long>(),
              (*it)["Kqty"].get<double>(),
              (*it)["Kprice"].get<double>(),
              (*it)["Kvalue"].get<double>(),
              (*it)["Kdiff"].get<double>(),
              (*it)["feeCharged"].get<double>(),
              (*it)["loadedFromDB"].get<bool>()
            ));
        cout << FN::uiT() << "DB" << RWHITE << " loaded " << tradesMemory.size() << " historical Trades." << endl;
      };
      static json onSnapTrades() {
        json k;
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
          it->loadedFromDB = true;
          k.push_back(*it);
        }
        return k;
      };
      static json onSnapOrders() {
        json k;
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (mORS::Working != it->second.orderStatus) continue;
          k.push_back(it->second);
        }
        return k;
      };
      static void onHandCancelAllOrders(json k) {
        cancelOpenOrders();
      };
      static void onHandCleanAllClosedOrders(json k) {
        cleanClosedOrders();
      };
      static void onHandCleanAllOrders(json k) {
        cleanOrders();
      };
      static void onHandCancelOrder(json k) {
        if (k.is_object() and k["orderId"].is_string())
          cancelOrder(k["orderId"].get<string>());
        else cout << FN::uiT() << "JSON" << RRED << " Warrrrning:" << BRED << " Missing orderId at onHandCancelOrder, ignored." << endl;
      };
      static void onHandCleanTrade(json k) {
        if (k.is_object() and k["tradeId"].is_string())
          cleanTrade(k["tradeId"].get<string>());
        else cout << FN::uiT() << "JSON" << RRED << " Warrrrning:" << BRED << " Missing tradeId at onHandCleanTrade, ignored." << endl;
      };
      static void onHandSubmitNewOrder(json k) {
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
      static mOrder updateOrderState(mOrder k) {
        mOrder o;
        if (k.orderStatus == mORS::New) o = k;
        else if (k.orderId != "" and allOrders.find(k.orderId) != allOrders.end())
          o = allOrders[k.orderId];
        else if (k.exchangeId != "" and allOrdersIds.find(k.exchangeId) != allOrdersIds.end()) {
          o = allOrders[allOrdersIds[k.exchangeId]];
          k.orderId = o.orderId;
        } else return o;
        if (k.orderId!="") o.orderId = k.orderId;
        if (k.exchangeId!="") o.exchangeId = k.exchangeId;
        if ((int)k.exchange!=0) o.exchange = k.exchange;
        if ((int)k.pair.base!=0) o.pair.base = k.pair.base;
        if ((int)k.pair.quote!=0) o.pair.quote = k.pair.quote;
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
        if (!gW->cancelByClientId and o.exchangeId != "") {
          map<string, void*>::iterator it = toCancel.find(o.orderId);
          if (it != toCancel.end()) {
            toCancel.erase(it);
            cancelOrder(o.orderId);
            if (o.orderStatus == mORS::Working) return o;
          }
        }
        ev_ogOrder(o);
        UI::uiSend(uiTXT::OrderStatusReports, o, true);
        if (k.lastQuantity > 0)
          toHistory(o);
        return o;
      };
      static void cancelOpenOrders() {
        if (gW->supportCancelAll) return gW->cancelAll();
        for (map<string, mOrder>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if (mORS::New == (mORS)it->second.orderStatus or mORS::Working == (mORS)it->second.orderStatus)
            cancelOrder(it->first);
      };
      static void cleanClosedOrders() {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->Kqty+0.0001 < it->quantity) ++it;
          else {
            it->Kqty = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesMemory.erase(it);
          }
        }
      };
      static void cleanOrders() {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          it->Kqty = -1;
          UI::uiSend(uiTXT::Trades, *it);
          DB::insert(uiTXT::Trades, {}, false, it->tradeId);
          it = tradesMemory.erase(it);
        }
      };
      static void cleanTrade(string k) {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->tradeId != k) ++it;
          else {
            it->Kqty = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesMemory.erase(it);
          }
        }
      };
      static void toHistory(mOrder o) {
        double fee = 0;
        double val = abs(o.price * o.lastQuantity);
        mTradeHydrated trade(
          to_string(FN::T()),
          o.time,
          o.exchange,
          o.pair,
          o.price,
          o.lastQuantity,
          o.side,
          val, 0, 0, 0, 0, 0, fee, false
        );
        cout << FN::uiT() << "GW " << (o.side == mSide::Bid ? RCYAN : RPURPLE) << argExchange << " TRADE " << (trade.side == mSide::Bid ? BCYAN : BPURPLE) << (trade.side == mSide::Bid ? "BUY " : "SELL ") << trade.quantity << " " << mCurrency[trade.pair.base] << " at price " << trade.price << " " << mCurrency[trade.pair.quote] << " (value " << trade.value << " " << mCurrency[trade.pair.quote] << ")" << endl;
        ev_ogTrade(trade);
        if (QP::matchPings()) {
          double widthPong = QP::getBool("widthPercentage")
            ? QP::getDouble("widthPongPercentage") * trade.price / 100
            : QP::getDouble("widthPong");
          map<double, string> matches;
          for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
            if (it->quantity - it->Kqty > 0
              and it->side == (trade.side == mSide::Bid ? mSide::Ask : mSide::Bid)
              and (trade.side == mSide::Bid ? (it->price > trade.price + widthPong) : (it->price < trade.price - widthPong))
            ) matches[it->price] = it->tradeId;
          matchPong(matches, ((mPongAt)QP::getInt("pongAt") == mPongAt::LongPingFair or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive) ? trade.side == mSide::Ask : trade.side == mSide::Bid, trade);
        } else {
          UI::uiSend(uiTXT::Trades, trade);
          DB::insert(uiTXT::Trades, trade, false, trade.tradeId);
          tradesMemory.push_back(trade);
        }
        UI::uiSend(uiTXT::TradesChart, {
          {"price", trade.price},
          {"side", (int)trade.side},
          {"quantity", trade.quantity},
          {"value", trade.value},
          {"pong", o.isPong}
        });
        cleanAuto(trade.time, QP::getDouble("cleanPongsAuto"));
      };
      static void matchPong(map<double, string> matches, bool reverse, mTradeHydrated pong) {
        if (reverse) for (map<double, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<double, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
            if (it->price!=pong.price or it->side!=pong.side or it->quantity<=it->Kqty) continue;
            eq = true;
            it->time = pong.time;
            it->quantity = it->quantity + pong.quantity;
            it->value = it->value + pong.value;
            it->loadedFromDB = false;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, *it, false, it->tradeId);
            break;
          }
          if (!eq) {
            UI::uiSend(uiTXT::Trades, pong);
            DB::insert(uiTXT::Trades, pong, false, pong.tradeId);
            tradesMemory.push_back(pong);
          }
        }
      };
      static bool matchPong(string match, mTradeHydrated* pong) {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
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
          UI::uiSend(uiTXT::Trades, *it);
          DB::insert(uiTXT::Trades, *it, false, it->tradeId);
          break;
        }
        return pong->quantity > 0;
      };
      static void cleanAuto(unsigned long k, double pT) {
        if (pT == 0) return;
        unsigned long pT_ = k - (abs(pT) * 864e5);
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->time < pT_ and (pT < 0 or it->Kqty >= it->quantity)) {
            it->Kqty = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesMemory.erase(it);
          } else ++it;
        }
      };
      static void toMemory(mOrder k) {
        if (k.orderStatus != mORS::Cancelled and k.orderStatus != mORS::Complete) {
          if (k.exchangeId != "")
            allOrdersIds[k.exchangeId] = k.orderId;
          allOrders[k.orderId] = k;
          if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW  save  " << (k.side == mSide::Bid ? "BID id " : "ASK id ") << k.orderId << "::" << k.exchangeId << " [" << (int)k.orderStatus << "]: " << k.quantity << " " << mCurrency[k.pair.base] << " at price " << k.price << " " << mCurrency[k.pair.quote] << "." << endl;
        } else allOrdersDelete(k.orderId, k.exchangeId);
        if (argDebugOrders) cout << FN::uiT() << "DEBUG " << RWHITE << "GW memory " << allOrders.size() << "/" << allOrdersIds.size() << "." << endl;
      };
  };
}

#endif
