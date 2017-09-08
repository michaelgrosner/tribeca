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
        gW->send(o["orderId"].get<string>(), (mSide)o["side"].get<int>(), o["price"].get<double>(), o["quantity"].get<double>(), (mOrderType)o["type"].get<int>(), (mTimeInForce)o["timeInForce"].get<int>(), o["preferPostOnly"].get<bool>(), o["time"].get<unsigned long>());
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
        gW->cancel(o["orderId"].get<string>(), o["exchangeId"].get<string>(), (mSide)o["side"].get<int>(), o["time"].get<unsigned long>());
      };
    private:
      static void load() {
        tradesMemory = DB::load(uiTXT::Trades);
        cout << FN::uiT() << "DB loaded " << tradesMemory.size() << " historical Trades." << endl;
      };
      static json onSnapTrades(json z) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          (*it)["loadedFromDB"] = true;
        return tradesMemory;
      };
      static json onSnapOrders(json z) {
        json k;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (mORS::Working != (mORS)it->second["orderStatus"].get<int>()) continue;
          k.push_back(it->second);
        }
        return k;
      };
      static json onHandCancelOrder(json k) {
        cancelOrder(k["orderId"].get<string>());
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
        cleanTrade(k["tradeId"].get<string>());
        return {};
      };
      static json onHandSubmitNewOrder(json k) {
        sendOrder(
          k["side"].get<string>() == "Bid" ? mSide::Bid : mSide::Ask,
          k["price"].get<double>(),
          k["quantity"].get<double>(),
          k["orderType"].get<string>() == "Limit" ? mOrderType::Limit : mOrderType::Market,
          k["timeInForce"].get<string>() == "GTC" ? mTimeInForce::GTC : (k["timeInForce"].get<string>() == "FOK" ? mTimeInForce::FOK : mTimeInForce::IOC),
          false,
          false
        );
        return {};
      };
      static json updateOrderState(json k) {
        json o;
        if ((mORS)k["orderStatus"].get<int>() == mORS::New) o = k;
        else if (!k["orderId"].is_null() and allOrders.find(k["orderId"].get<string>()) != allOrders.end())
          o = allOrders[k["orderId"].get<string>()];
        else if (!k["exchangeId"].is_null() and allOrdersIds.find(k["exchangeId"].get<string>()) != allOrdersIds.end()) {
          o = allOrders[allOrdersIds[k["exchangeId"].get<string>()]];
          k["orderId"] = o["orderId"].get<string>();
        } else return o;
        for (json::iterator it = k.begin(); it != k.end(); ++it)
          if (!it.value().is_null()) o[it.key()] = it.value();
        if (o["time"].is_null()) o["time"] = FN::T();
        if (o["computationalLatency"].is_null() and (mORS)o["orderStatus"].get<int>() == mORS::Working)
          o["computationalLatency"] = FN::T() - o["time"].get<unsigned long>();
        if (!o["computationalLatency"].is_null()) o["time"] = FN::T();
        toMemory(o);
        if (!gW->cancelByClientId and !o["exchangeId"].is_null()) {
          map<string, void*>::iterator it = toCancel.find(o["orderId"].get<string>());
          if (it != toCancel.end()) {
            toCancel.erase(it);
            cancelOrder(o["orderId"].get<string>());
            if ((mORS)o["orderStatus"].get<int>() == mORS::Working) return o;
          }
        }
        EV::up(mEv::OrderUpdateBroker, o);
        UI::uiSend(uiTXT::OrderStatusReports, o, true);
        if (!k["lastQuantity"].is_null() and k["lastQuantity"].get<double>() > 0)
          toHistory(o);
        return o;
      };
      static void cancelOpenOrders() {
        if (gW->supportCancelAll)
          return gW->cancelAll();
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if (mORS::New == (mORS)it->second["orderStatus"].get<int>() or mORS::Working == (mORS)it->second["orderStatus"].get<int>())
            cancelOrder(it->first);
      };
      static void cleanClosedOrders() {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if ((*it)["Kqty"].get<double>()+0.0001 < (*it)["quantity"].get<double>()) ++it;
          else {
            (*it)["Kqty"] = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, (*it)["tradeId"].get<string>());
            it = tradesMemory.erase(it);
          }
        }
      };
      static void cleanOrders() {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          (*it)["Kqty"] = -1;
          UI::uiSend(uiTXT::Trades, *it);
          DB::insert(uiTXT::Trades, {}, false, (*it)["tradeId"].get<string>());
          it = tradesMemory.erase(it);
        }
      };
      static void cleanTrade(string k) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if ((*it)["tradeId"].get<string>() != k) ++it;
          else {
            (*it)["Kqty"] = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, (*it)["tradeId"].get<string>());
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
        double val = abs(o["lastPrice"].get<double>() * o["lastQuantity"].get<double>());
        if (!o["liquidity"].is_null()) {
          fee = (mLiquidity)o["liquidity"].get<int>() == mLiquidity::Make ? gw->makeFee : gw->takeFee;
          if (fee) val -= ((mSide)o["side"].get<int>() == mSide::Bid ? 1 : -1) * fee;
        }
        json trade = {
          {"tradeId", to_string(FN::T())},
          {"time", o["time"].get<unsigned long>()},
          {"exchange", o["exchange"].get<int>()},
          {"pair", o["pair"]},
          {"price", o["lastPrice"].get<double>()},
          {"quantity", o["lastQuantity"].get<double>()},
          {"side", o["side"].get<int>()},
          {"value", val},
          {"liquidity", o["liquidity"].is_null() ? 0 : o["liquidity"].get<int>()},
          {"Ktime", 0},
          {"Kqty", 0},
          {"Kprice", 0},
          {"Kvalue", 0},
          {"Kdiff", 0},
          {"feeCharged", fee},
          {"loadedFromDB", false},
        };
        cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " TRADE " << ((mSide)o["side"].get<int>() == mSide::Bid ? "BUY " : "SELL ") << o["lastQuantity"].get<double>() << " " << mCurrency[gw->base] << " at price " << o["lastPrice"].get<double>() << " " << mCurrency[gw->quote] << endl;
        EV::up(mEv::OrderTradeBroker, trade);
        if (QP::matchPings()) {
          double widthPong = QP::getBool("widthPercentage")
            ? QP::getDouble("widthPongPercentage") * trade["price"].get<double>() / 100
            : QP::getDouble("widthPong");
          map<double, string> matches;
          for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
            if ((*it)["quantity"].get<double>() - (*it)["Kqty"].get<double>() > 0
              and (mSide)(*it)["side"].get<int>() == ((mSide)trade["side"].get<int>() == mSide::Bid ? mSide::Ask : mSide::Bid)
              and ((mSide)trade["side"].get<int>() == mSide::Bid ? ((*it)["price"].get<double>() > (trade["price"].get<double>() + widthPong)) : ((*it)["price"].get<double>() < (trade["price"].get<double>() - widthPong)))
            ) matches[(*it)["price"].get<double>()] = (*it)["tradeId"].get<string>();
          matchPong(matches, ((mPongAt)QP::getInt("pongAt") == mPongAt::LongPingFair or (mPongAt)QP::getInt("pongAt") == mPongAt::LongPingAggressive) ? (mSide)trade["side"].get<int>() == mSide::Ask : (mSide)trade["side"].get<int>() == mSide::Bid, trade);
        } else {
          UI::uiSend(uiTXT::Trades, trade);
          DB::insert(uiTXT::Trades, trade, false, trade["tradeId"].get<string>());
          tradesMemory.push_back(trade);
        }
        json t = {
          {"price", o["lastPrice"].get<double>()},
          {"side", o["side"].get<int>()},
          {"quantity", o["lastQuantity"].get<double>()},
          {"value", val},
          {"pong", o["isPong"].get<bool>()}
        };
        UI::uiSend(uiTXT::TradesChart, t);
        cleanAuto(o["time"].get<unsigned long>(), QP::getDouble("cleanPongsAuto"));
      };
      static void matchPong(map<double, string> matches, bool reverse, json pong) {
        if (reverse) for (map<double, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<double, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong["quantity"].get<double>() > 0) {
          bool eq = false;
          for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
            if ((*it)["price"].get<double>()!=pong["price"].get<double>() or (*it)["side"].get<int>()!=pong["side"].get<int>() or (*it)["quantity"].get<double>()<=(*it)["Kqty"].get<double>()) continue;
            eq = true;
            (*it)["time"] = pong["time"].get<unsigned long>();
            (*it)["quantity"] = (*it)["quantity"].get<double>() + pong["quantity"].get<double>();
            (*it)["value"] = (*it)["value"].get<double>() + pong["value"].get<double>();
            (*it)["loadedFromDB"] = false;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, *it, false, (*it)["tradeId"].get<string>());
            break;
          }
          if (!eq) {
            UI::uiSend(uiTXT::Trades, pong);
            DB::insert(uiTXT::Trades, pong, false, pong["tradeId"].get<string>());
            tradesMemory.push_back(pong);
          }
        }
      };
      static bool matchPong(string match, json* pong) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it) {
          if ((*it)["tradeId"].get<string>() != match) continue;
          double Kqty = fmin((*pong)["quantity"].get<double>(), (*it)["quantity"].get<double>() - (*it)["Kqty"].get<double>());
          (*it)["Ktime"] = (*pong)["time"].get<unsigned long>();
          (*it)["Kprice"] = ((Kqty*(*pong)["price"].get<double>()) + ((*it)["Kqty"].get<double>()*(*it)["Kprice"].get<double>())) / ((*it)["Kqty"].get<double>()+Kqty);
          (*it)["Kqty"] = (*it)["Kqty"].get<double>() + Kqty;
          (*it)["Kvalue"] = abs((*it)["Kqty"].get<double>()*(*it)["Kprice"].get<double>());
          (*pong)["quantity"] = (*pong)["quantity"].get<double>() - Kqty;
          (*pong)["value"] = abs((*pong)["price"].get<double>()*(*pong)["quantity"].get<double>());
          if ((*it)["quantity"].get<double>()<=(*it)["Kqty"].get<double>())
            (*it)["Kdiff"] = abs(((*it)["quantity"].get<double>()*(*it)["price"].get<double>())-((*it)["Kqty"].get<double>()*(*it)["Kprice"].get<double>()));
          (*it)["loadedFromDB"] = false;
          UI::uiSend(uiTXT::Trades, *it);
          DB::insert(uiTXT::Trades, *it, false, (*it)["tradeId"].get<string>());
          break;
        }
        return (*pong)["quantity"].get<double>() > 0;
      };
      static void cleanAuto(unsigned long k, double pT) {
        if (pT == 0) return;
        unsigned long pT_ = k - (abs(pT) * 864e5);
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end();) {
          if ((*it)["time"].get<unsigned long>() < pT_ and (pT < 0 or (*it)["Kqty"].get<double>() >= (*it)["quantity"].get<double>())) {
            (*it)["Kqty"] = -1;
            UI::uiSend(uiTXT::Trades, *it);
            DB::insert(uiTXT::Trades, {}, false, (*it)["tradeId"].get<string>());
            it = tradesMemory.erase(it);
          } else ++it;
        }
      };
      static void toMemory(json k) {
        if ((mORS)k["orderStatus"].get<int>() != mORS::Cancelled and (mORS)k["orderStatus"].get<int>() != mORS::Complete) {
          if (!k["exchangeId"].is_null()) allOrdersIds[k["exchangeId"].get<string>()] = k["orderId"].get<string>();
          allOrders[k["orderId"].get<string>()] = k;
        } else allOrdersDelete(k["orderId"].get<string>(), k["exchangeId"].is_null() ? "" : k["exchangeId"].get<string>());
      };
  };
}

#endif
