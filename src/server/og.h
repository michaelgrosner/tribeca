#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  json tradesMemory;
  map<string, void*> toCancel;
  map<string, json> allOrders;
  map<string, string> allOrdersIds;
  class OG {
    public:
      static void main() {
        load();
        EV::on(mEv::OrderUpdateGateway, [](json k) {
          updateOrderState(k);
        });
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
        } else for (map<string, string>::iterator it_ = allOrdersIds.begin(); it_ != allOrdersIds.end();)
          if (it_->second == oI) it_ = allOrdersIds.erase(it_); else ++it_;
      };
      static void sendOrder(mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oIP, bool oPO) {
        json o = updateOrderState({
          {"orderId", gW->clientId()},
          {"exchange", (int)gw->exchange},
          {"pair", {{"base", gw->base}, {"quote", gw->quote}}},
          {"side", (int)oS},
          {"quantity", oQ},
          {"leavesQuantity", oQ},
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
        tradesMemory = DB::load(uiTXT::Trades);
        cout << FN::uiT() << "DB" << RWHITE << " loaded " << tradesMemory.size() << " historical Trades." << endl;
      };
      static json onSnapTrades(json z) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          (*it)["loadedFromDB"] = true;
        return tradesMemory;
      };
      static json onSnapOrders(json z) {
        json k;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (mORS::Working != (mORS)it->second.value("orderStatus", 0)) continue;
          k.push_back(it->second);
        }
        return k;
      };
      static json onHandCancelOrder(json k) {
        string orderId = k.value("orderId", "");
        if (orderId!="") cancelOrder(orderId);
        return {};
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
      static json onHandCleanTrade(json k) {
        string tradeId = k.value("tradeId", "");
        if (tradeId!="") cleanTrade(tradeId);
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
        if (o["time"].is_null()) o["time"] = FN::T();
        if (o["computationalLatency"].is_null() and (mORS)o.value("orderStatus", 0) == mORS::Working)
          o["computationalLatency"] = FN::T() - o.value("time", (unsigned long)0);
        if (!o["computationalLatency"].is_null()) o["time"] = FN::T();
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
        if (!k["lastQuantity"].is_null() and k.value("lastQuantity", 0.0) > 0)
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
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->value("Kqty", 0.0)+0.0001 < it->value("quantity", 0.0)) ++it;
          else {
            (*it)["Kqty"] = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, it->value("tradeId", ""));
            it = tradesMemory.erase(it);
          }
        }
      };
      static void cleanOrders() {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          (*it)["Kqty"] = -1;
          UI::uiSend(uiTXT::Trades, *it);
          DB::insert(uiTXT::Trades, {}, false, it->value("tradeId", ""));
          it = tradesMemory.erase(it);
        }
      };
      static void cleanTrade(string k) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->value("tradeId", "") != k) ++it;
          else {
            (*it)["Kqty"] = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, it->value("tradeId", ""));
            it = tradesMemory.erase(it);
          }
        }
      };
      static json updateOrderState(string k, mORS os) {
        if (os == mORS::Cancelled) return updateOrderState({{"orderId", k}, {"orderStatus", (int)os}, {"leavesQuantity", 0}});
        else return updateOrderState({{"orderId", k}, {"orderStatus", (int)os}});
      };
      static void toHistory(json o) {
        double fee = 0;
        double val = abs(o.value("lastPrice", 0.0) * o.value("lastQuantity", 0.0));
        json trade = {
          {"tradeId", to_string(FN::T())},
          {"time", o.value("time", (unsigned long)0)},
          {"exchange", o.value("exchange", 0)},
          {"pair", o["pair"]},
          {"price", o.value("lastPrice", 0.0)},
          {"quantity", o.value("lastQuantity", 0.0)},
          {"side", o.value("side", 0)},
          {"value", val},
          {"Ktime", 0},
          {"Kqty", 0},
          {"Kprice", 0},
          {"Kvalue", 0},
          {"Kdiff", 0},
          {"feeCharged", fee},
          {"loadedFromDB", false},
        };
        cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << ((mSide)o.value("side", 0) == mSide::Bid ? BBLUE : BPURPLE) << " TRADE " << ((mSide)o.value("side", 0) == mSide::Bid ? "BUY " : "SELL ") << o.value("lastQuantity", 0.0) << " " << mCurrency[gw->base] << " at price " << o.value("lastPrice", 0.0) << " " << mCurrency[gw->quote] << " (value " << val << " " << mCurrency[gw->quote] << ")" << endl;
        EV::up(mEv::OrderTradeBroker, trade);
        if (QP::matchPings()) {
          double widthPong = QP::getBool("widthPercentage")
            ? QP::getDouble("widthPongPercentage") * trade.value("price", 0.0) / 100
            : QP::getDouble("widthPong");
          map<double, string> matches;
          for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
            if (it->value("quantity", 0.0) - it->value("Kqty", 0.0) > 0
              and (mSide)it->value("side", 0) == ((mSide)trade["side"].get<int>() == mSide::Bid ? mSide::Ask : mSide::Bid)
              and ((mSide)trade.value("side", 0) == mSide::Bid ? (it->value("price", 0.0) > (trade.value("price", 0.0) + widthPong)) : (it->value("price", 0.0) < (trade.value("price", 0.0) - widthPong)))
            ) matches[it->value("price", 0.0)] = it->value("tradeId", "");
          matchPong(matches, ((mPongAt)QP::getInt("pongAt") == mPongAt::LongPingFair or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive) ? (mSide)trade.value("side", 0) == mSide::Ask : (mSide)trade.value("side", 0) == mSide::Bid, trade);
        } else {
          UI::uiSend(uiTXT::Trades, trade);
          DB::insert(uiTXT::Trades, trade, false, trade.value("tradeId", ""));
          tradesMemory.push_back(trade);
        }
        json t = {
          {"price", o.value("lastPrice", 0.0)},
          {"side", o.value("side", 0)},
          {"quantity", o.value("lastQuantity", 0.0)},
          {"value", val},
          {"pong", o.value("isPong", false)}
        };
        UI::uiSend(uiTXT::TradesChart, t);
        cleanAuto(o.value("time", (unsigned long)0), QP::getDouble("cleanPongsAuto"));
      };
      static void matchPong(map<double, string> matches, bool reverse, json pong) {
        if (reverse) for (map<double, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<double, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong.value("quantity", 0.0) > 0) {
          bool eq = false;
          for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
            if (it->value("price", 0.0)!=pong.value("price", 0.0) or it->value("side", 0)!=pong.value("side", 0) or it->value("quantity", 0)<=it->value("Kqty", 0.0)) continue;
            eq = true;
            (*it)["time"] = pong.value("time", (unsigned long)0);
            (*it)["quantity"] = it->value("quantity", 0.0) + pong.value("quantity", 0.0);
            (*it)["value"] = it->value("value", 0.0) + pong.value("value", 0.0);
            (*it)["loadedFromDB"] = false;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, *it, false, it->value("tradeId", ""));
            break;
          }
          if (!eq) {
            UI::uiSend(uiTXT::Trades, pong);
            DB::insert(uiTXT::Trades, pong, false, pong.value("tradeId", ""));
            tradesMemory.push_back(pong);
          }
        }
      };
      static bool matchPong(string match, json* pong) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
          if (it->value("tradeId", "") != match) continue;
          double Kqty = fmin(pong->value("quantity", 0.0), it->value("quantity", 0.0) - it->value("Kqty", 0.0));
          (*it)["Ktime"] = pong->value("time", (unsigned long)0);
          (*it)["Kprice"] = ((Kqty*pong->value("price", 0.0)) + (it->value("Kqty", 0.0)*it->value("Kprice", 0.0))) / (it->value("Kqty", 0.0)+Kqty);
          (*it)["Kqty"] = it->value("Kqty", 0.0) + Kqty;
          (*it)["Kvalue"] = abs(it->value("Kqty", 0.0)*it->value("Kprice", 0.0));
          (*pong)["quantity"] = pong->value("quantity", 0.0) - Kqty;
          (*pong)["value"] = abs(pong->value("price", 0.0)*pong->value("quantity", 0.0));
          if (it->value("quantity", 0.0)<=it->value("Kqty", 0.0))
            (*it)["Kdiff"] = abs((it->value("quantity", 0.0)*it->value("price", 0.0))-(it->value("Kqty", 0.0)*it->value("Kprice", 0.0)));
          (*it)["loadedFromDB"] = false;
          UI::uiSend(uiTXT::Trades, *it);
          DB::insert(uiTXT::Trades, *it, false, it->value("tradeId", ""));
          break;
        }
        return pong->value("quantity", 0.0) > 0;
      };
      static void cleanAuto(unsigned long k, double pT) {
        if (pT == 0) return;
        unsigned long pT_ = k - (abs(pT) * 864e5);
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if (it->value("time", (unsigned long)0) < pT_ and (pT < 0 or it->value("Kqty", 0) >= it->value("quantity", 0.0))) {
            (*it)["Kqty"] = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, it->value("tradeId", ""));
            it = tradesMemory.erase(it);
          } else ++it;
        }
      };
      static void toMemory(json k) {
        if ((mORS)k.value("orderStatus", 0) != mORS::Cancelled and (mORS)k.value("orderStatus", 0) != mORS::Complete) {
          if (!k["exchangeId"].is_null()) allOrdersIds[k.value("exchangeId", "")] = k.value("orderId", "");
          allOrders[k.value("orderId", "")] = k;
        } else allOrdersDelete(k.value("orderId", ""), k.value("exchangeId", ""));
      };
  };
}

#endif
