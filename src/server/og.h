#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  vector<mTradeHydrated> tradesMemory;
  map<string, void*> toCancel;
  map<string, json> allOrders;
  map<string, string> allOrdersIds;
  class OG {
    public:
      static void main() {
        load();
        ev_gwDataOrder = [](json k) {
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
        map<string, json>::iterator it = allOrders.find(oI);
        if (it != allOrders.end()) allOrders.erase(it);
        if (oE != "") {
          map<string, string>::iterator it_ = allOrdersIds.find(oE);
          if (it_ != allOrdersIds.end()) allOrdersIds.erase(it_);
        } else {
          for (map<string, string>::iterator it_ = allOrdersIds.begin(); it_ != allOrdersIds.end();)
            if (it_->second == oI) it_ = allOrdersIds.erase(it_); else ++it_;
        }
      };
      static void sendOrder(mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oIP, bool oPO) {
        json o = updateOrderState({
          {"orderId", gW->clientId()},
          {"exchange", (int)gw->exchange},
          {"pair", {{"base", gw->base}, {"quote", gw->quote}}},
          {"side", (int)oS},
          {"quantity", oQ},
          {"type", (int)oLM},
          {"isPong", oIP},
          {"price", FN::roundSide(oP, gw->minTick, oS)},
          {"timeInForce", (int)oTIF},
          {"orderStatus", (int)mORS::New},
          {"preferPostOnly", oPO}
        });
        gW->send(
          o.value("orderId", ""),
          (mSide)o.value("side", 0),
          o.value("price", 0.0),
          o.value("quantity", 0.0),
          (mOrderType)o.value("type", 0),
          (mTimeInForce)o.value("timeInForce", 0),
          o.value("preferPostOnly", false),
          o.value("time", (unsigned long)0)
        );
      };
      static void cancelOrder(string k) {
        if (allOrders.find(k) == allOrders.end()) {
          updateOrderState(k, mORS::Cancelled);
          return;
        }
        if (!gW->cancelByClientId and allOrders[k]["exchangeId"].is_null()) {
          toCancel[k] = nullptr;
          return;
        }
        json o = updateOrderState(k, mORS::Working);
        gW->cancel(
          o.value("orderId", ""),
          o.value("exchangeId", ""),
          (mSide)o.value("side", 0),
          o.value("time", (unsigned long)0)
        );
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
      static json onSnapTrades(json z) {
        json k;
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
          it->loadedFromDB = true;
          k.push_back(toJsonTrade(*it));
        }
        return k;
      };
      static json onSnapOrders(json z) {
        json k;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (mORS::Working != (mORS)it->second.value("orderStatus", 0)) continue;
          k.push_back(it->second);
        }
        return k;
      };
      static json onHandCancelAllOrders(json k) {
        cancelOpenOrders();
        return {};
      };
      static json onHandCleanAllClosedOrders(json k) {
        cleanClosedOrders();
        return {};
      };
      static json onHandCleanAllOrders(json k) {
        cleanOrders();
        return {};
      };
      static json onHandCancelOrder(json k) {
        if (!k.is_object() or !k["orderId"].is_string()) {
          cout << FN::uiT() << "JSON" << RRED << " Warrrrning:" << BRED << " Missing orderId at onHandCancelOrder, ignored." << endl;
          return {};
        }
        cancelOrder(k["orderId"].get<string>());
        return {};
      };
      static json onHandCleanTrade(json k) {
        if (!k.is_object() or !k["tradeId"].is_string()) {
          cout << FN::uiT() << "JSON" << RRED << " Warrrrning:" << BRED << " Missing tradeId at onHandCleanTrade, ignored." << endl;
          return {};
        }
        cleanTrade(k["tradeId"].get<string>());
        return {};
      };
      static json onHandSubmitNewOrder(json k) {
        sendOrder(
          k.value("side", "") == "Bid" ? mSide::Bid : mSide::Ask,
          k.value("price", 0.0),
          k.value("quantity", 0.0),
          k.value("orderType", "") == "Limit" ? mOrderType::Limit : mOrderType::Market,
          k.value("timeInForce", "") == "GTC" ? mTimeInForce::GTC : (k.value("timeInForce", "") == "FOK" ? mTimeInForce::FOK : mTimeInForce::IOC),
          false,
          false
        );
        return {};
      };
      static json updateOrderState(string k, mORS os) {
        return updateOrderState({{"orderId", k}, {"orderStatus", (int)os}});
      };
      static json updateOrderState(json k) {
        json o;
        if ((mORS)k.value("orderStatus", 0) == mORS::New) o = k;
        else if (!k["orderId"].is_null() and allOrders.find(k.value("orderId", "")) != allOrders.end())
          o = allOrders[k.value("orderId", "")];
        else if (!k["exchangeId"].is_null() and allOrdersIds.find(k.value("exchangeId", "")) != allOrdersIds.end()) {
          o = allOrders[allOrdersIds[k.value("exchangeId", "")]];
          k["orderId"] = o.value("orderId", "");
        } else return o;
        for (json::iterator it = k.begin(); it != k.end(); ++it)
          if (!it.value().is_null()) o[it.key()] = it.value();
        if (o.value("time", (unsigned long)0)==0) o["time"] = FN::T();
        if (o.value("computationalLatency", (unsigned long)0)==0 and (mORS)o.value("orderStatus", 0) == mORS::Working)
          o["computationalLatency"] = FN::T() - o.value("time", (unsigned long)0);
        if (o.value("computationalLatency", (unsigned long)0)>0) o["time"] = FN::T();
        toMemory(o);
        if (!gW->cancelByClientId and !o["exchangeId"].is_null()) {
          map<string, void*>::iterator it = toCancel.find(o.value("orderId", ""));
          if (it != toCancel.end()) {
            toCancel.erase(it);
            cancelOrder(o.value("orderId", ""));
            if ((mORS)o.value("orderStatus", 0) == mORS::Working) return o;
          }
        }
        EV::up(mEv::OrderUpdateBroker, o);
        UI::uiSend(uiTXT::OrderStatusReports, o, true);
        if (k.value("lastQuantity", 0.0) > 0)
          toHistory(o);
        return o;
      };
      static void cancelOpenOrders() {
        if (gW->supportCancelAll)
          return gW->cancelAll();
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if (mORS::New == (mORS)it->second.value("orderStatus", 0) or mORS::Working == (mORS)it->second.value("orderStatus", 0))
            cancelOrder(it->first);
      };
      static void cleanClosedOrders() {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->Kqty+0.0001 < it->quantity) ++it;
          else {
            it->Kqty = -1;
            UI::uiSend(uiTXT::Trades, toJsonTrade(*it));
            DB::insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesMemory.erase(it);
          }
        }
      };
      static void cleanOrders() {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          it->Kqty = -1;
          UI::uiSend(uiTXT::Trades, toJsonTrade(*it));
          DB::insert(uiTXT::Trades, {}, false, it->tradeId);
          it = tradesMemory.erase(it);
        }
      };
      static void cleanTrade(string k) {
        for (vector<mTradeHydrated>::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->tradeId != k) ++it;
          else {
            it->Kqty = -1;
            UI::uiSend(uiTXT::Trades, toJsonTrade(*it));
            DB::insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesMemory.erase(it);
          }
        }
      };
      static json toJsonTrade(mTradeHydrated k) {
        return {
          {"tradeId", k.tradeId},
          {"time", k.time},
          {"exchange", (int)k.exchange},
          {"pair", {{"base", (int)k.pair.base}, {"quote", (int)k.pair.quote}}},
          {"price", k.price},
          {"quantity", k.quantity},
          {"side", (int)k.side},
          {"value", k.value},
          {"Ktime", k.Ktime},
          {"Kqty", k.Kqty},
          {"Kprice", k.Kprice},
          {"Kvalue", k.Kvalue},
          {"Kdiff", k.Kdiff},
          {"feeCharged", k.feeCharged},
          {"loadedFromDB", k.loadedFromDB},
        };
      };
      static void toHistory(json o) {
        double fee = 0;
        double val = abs(o.value("price", 0.0) * o.value("lastQuantity", 0.0));
        mTradeHydrated trade(
          to_string(FN::T()),
          o.value("time", (unsigned long)0),
          (mExchange)o.value("exchange", 0),
          mPair(o["/pair/base"_json_pointer].get<int>(), o["/pair/quote"_json_pointer].get<int>()),
          o.value("price", 0.0),
          o.value("lastQuantity", 0.0),
          (mSide)o.value("side", 0),
          val, 0, 0, 0, 0, 0, fee, false
        );
        cout << FN::uiT() << "GW " << ((mSide)o.value("side", 0) == mSide::Bid ? RCYAN : RPURPLE) << CF::cfString("EXCHANGE") << " TRADE " << (trade.side == mSide::Bid ? BCYAN : BPURPLE) << (trade.side == mSide::Bid ? "BUY " : "SELL ") << trade.quantity << " " << mCurrency[trade.pair.base] << " at price " << trade.price << " " << mCurrency[trade.pair.quote] << " (value " << trade.value << " " << mCurrency[trade.pair.quote] << ")" << endl;
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
          UI::uiSend(uiTXT::Trades, toJsonTrade(trade));
          DB::insert(uiTXT::Trades, toJsonTrade(trade), false, trade.tradeId);
          tradesMemory.push_back(trade);
        }
        UI::uiSend(uiTXT::TradesChart, {{
          {"price", trade.price},
          {"side", (int)trade.side},
          {"quantity", trade.quantity},
          {"value", trade.value},
          {"pong", o.value("isPong", false)}
        }});
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
            json trade = toJsonTrade(*it);
            UI::uiSend(uiTXT::Trades, trade);
            DB::insert(uiTXT::Trades, trade, false, it->tradeId);
            break;
          }
          if (!eq) {
            UI::uiSend(uiTXT::Trades, toJsonTrade(pong));
            DB::insert(uiTXT::Trades, toJsonTrade(pong), false, pong.tradeId);
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
          json trade = toJsonTrade(*it);
          UI::uiSend(uiTXT::Trades, trade);
          DB::insert(uiTXT::Trades, trade, false, it->tradeId);
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
            UI::uiSend(uiTXT::Trades, toJsonTrade(*it));
            DB::insert(uiTXT::Trades, {}, false, it->tradeId);
            it = tradesMemory.erase(it);
          } else ++it;
        }
      };
      static void toMemory(json k) {
        if ((mORS)k.value("orderStatus", 0) != mORS::Cancelled and (mORS)k.value("orderStatus", 0) != mORS::Complete) {
          if (!k["exchangeId"].is_null())
            allOrdersIds[k.value("exchangeId", "")] = k.value("orderId", "");
          allOrders[k.value("orderId", "")] = k;
        } else {
          allOrdersDelete(k.value("orderId", ""), k["exchangeId"].is_null()?"":k.value("exchangeId", ""));
        }
      };
  };
}

#endif
