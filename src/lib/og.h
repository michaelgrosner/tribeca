#ifndef K_OG_H_
#define K_OG_H_

namespace K {
  static uv_timer_t gwCancelAll_;
  static json tradesMemory;
  static map<string, void*> toCancel;
  static map<string, json> allOrders;
  static map<string, string> allOrdersIds;
  class OG {
    public:
      static void main(Local<Object> exports) {
        load();
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &gwCancelAll_)) { cout << FN::uiT() << "Errrror: GW gwCancelAll_ init timer failed." << endl; exit(1); }
          gwCancelAll_.data = NULL;
          if (uv_timer_start(&gwCancelAll_, [](uv_timer_t *handle) {
            if (qpRepo["cancelOrdersAuto"].get<bool>())
              gW->cancelAll();
          }, 0, 300000)) { cout << FN::uiT() << "Errrror: GW gwCancelAll_ start timer failed." << endl; exit(1); }
        }).detach();
        UI::uiSnap(uiTXT::Trades, &onSnapTrades);
        UI::uiSnap(uiTXT::OrderStatusReports, &onSnapOrders);
        UI::uiHand(uiTXT::SubmitNewOrder, &onHandSubmitNewOrder);
        UI::uiHand(uiTXT::CancelOrder, &onHandCancelOrder);
        UI::uiHand(uiTXT::CancelAllOrders, &onHandCancelAllOrders);
        UI::uiHand(uiTXT::CleanAllClosedOrders, &onHandCleanAllClosedOrders);
        UI::uiHand(uiTXT::CleanAllOrders, &onHandCleanAllOrders);
        UI::uiHand(uiTXT::CleanTrade, &onHandCleanTrade);
        EV::evOn("OrderUpdateGateway", [](json k) {
          updateOrderState(k);
        });
        NODE_SET_METHOD(exports, "allOrders", OG::_allOrders);
        NODE_SET_METHOD(exports, "allOrdersDelete", OG::_allOrdersDelete);
        NODE_SET_METHOD(exports, "sendOrder", OG::_sendOrder);
        NODE_SET_METHOD(exports, "cancelOrder", OG::_cancelOrder);
        NODE_SET_METHOD(exports, "cancelOpenOrders", OG::_cancelOpenOrders);
      };
      static json updateOrderState(json k) {
        json o;
        if ((mORS)k["orderStatus"].get<int>() == mORS::New) o = k;
        else {
          if (!k["orderId"].is_null() and allOrders.find(k["orderId"].get<string>()) != allOrders.end())
            o = allOrders[k["orderId"].get<string>()];
          else if (!k["exchangeId"].is_null() and allOrdersIds.find(k["exchangeId"].get<string>()) != allOrdersIds.end()) {
            o = allOrders[allOrdersIds[k["exchangeId"].get<string>()]];
            k["orderId"] = o["orderId"].get<string>();
          } else return o;
        }
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
        EV::evUp("OrderUpdateBroker", o);
        UI::uiSend(uiTXT::OrderStatusReports, o, true);
        if (!k["lastQuantity"].is_null() and k["lastQuantity"].get<double>() > 0)
          toHistory(o);
        return o;
      };
    private:
      static void load() {
        tradesMemory = DB::load(uiTXT::Trades);
      };
      static json onSnapTrades(json z) {
        for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
          (*it)["loadedFromDB"] = true;
        return tradesMemory;
      };
      static json onSnapOrders(json z) {
        return ogO(false);
      };
      static void _allOrders(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(v8ogO(args.GetIsolate(), true));
      };
      static void _cancelOpenOrders(const FunctionCallbackInfo<Value>& args) {
        cancelOpenOrders();
      };
      static void cancelOpenOrders() {
        if (gW->supportCancelAll)
          return gW->cancelAll();
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          if (mORS::New == (mORS)it->second["orderStatus"].get<int>() or mORS::Working == (mORS)it->second["orderStatus"].get<int>())
            cancelOrder(it->first);
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
          k["side"].get<string>() == "Ask" ? mSide::Ask : mSide::Bid,
          k["price"].get<double>(),
          k["quantity"].get<double>(),
          (mOrderType)k["orderType"].get<int>(),
          (mTimeInForce)k["timeInForce"].get<int>(),
          k["isPong"].get<bool>(),
          k["preferPostOnly"].get<bool>()
        );
        return {};
      };
      static void _sendOrder(const FunctionCallbackInfo<Value>& args) {
        sendOrder(
          (mSide)args[0]->NumberValue(),
          args[1]->NumberValue(),
          args[2]->NumberValue(),
          (mOrderType)args[3]->NumberValue(),
          (mTimeInForce)args[4]->NumberValue(),
          args[5]->BooleanValue(),
          args[6]->BooleanValue()
        );
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
      static void _cancelOrder(const FunctionCallbackInfo<Value>& args) {
        cancelOrder(FN::S8v(args[0]->ToString()));
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
      static void _allOrdersDelete(const FunctionCallbackInfo<Value>& args) {
        allOrdersDelete(FN::S8v(args[0]->ToString()), "");
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
        EV::evUp("OrderTradeBroker", trade);
        if ((mQuotingMode)qpRepo["mode"].get<int>() == mQuotingMode::Boomerang or (mQuotingMode)qpRepo["mode"].get<int>() == mQuotingMode::HamelinRat or (mQuotingMode)qpRepo["mode"].get<int>() == mQuotingMode::AK47) {
          double widthPong = qpRepo["widthPercentage"].get<bool>()
            ? qpRepo["widthPongPercentage"].get<double>() * trade["price"].get<double>() / 100
            : qpRepo["widthPong"].get<double>();
          map<double, string> matches;
          for (json::iterator it = tradesMemory.begin(); it != tradesMemory.end(); ++it)
            if ((*it)["quantity"].get<double>() - (*it)["Kqty"].get<double>() > 0
              and (mSide)(*it)["side"].get<int>() == ((mSide)trade["side"].get<int>() == mSide::Bid ? mSide::Ask : mSide::Bid)
              and ((mSide)trade["side"].get<int>() == mSide::Bid ? ((*it)["price"].get<double>() > (trade["price"].get<double>() + widthPong)) : ((*it)["price"].get<double>() < (trade["price"].get<double>() - widthPong)))
            ) matches[(*it)["price"].get<double>()] = (*it)["tradeId"].get<string>();
          matchPong(matches, ((mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingFair or (mPongAt)qpRepo["pongAt"].get<int>() == mPongAt::LongPingAggressive) ? (mSide)trade["side"].get<int>() == mSide::Ask : (mSide)trade["side"].get<int>() == mSide::Bid, trade);
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
        cleanAuto(o["time"].get<unsigned long>(), qpRepo["cleanPongsAuto"].get<double>());
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
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope hs(isolate);
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
      static json ogO(bool all) {
        json k;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (!all and mORS::Working != (mORS)it->second["orderStatus"].get<int>()) continue;
          k.push_back(it->second);
        }
        return k;
      };
      static Local<Array> v8ogO(Isolate* isolate, bool all) {
        Local<Array> k = Array::New(isolate);
        int i;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it) {
          if (!all and mORS::Working != (mORS)it->second["orderStatus"].get<int>()) continue;
          k->Set(i++, v8ogO_(it->second));
        }
        return k;
      };
      static Local<Object> v8ogO_(json j) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Object> o = Object::New(isolate);
        if (!j["orderId"].is_null()) o->Set(FN::v8S("orderId"), FN::v8S(j["orderId"].get<string>()));
        if (!j["exchangeId"].is_null()) o->Set(FN::v8S("exchangeId"), FN::v8S(j["exchangeId"].get<string>()));
        if (!j["time"].is_null()) o->Set(FN::v8S("time"), Number::New(isolate, j["time"].get<unsigned long>()));
        if (!j["exchange"].is_null()) o->Set(FN::v8S("exchange"), Number::New(isolate, j["exchange"].get<int>()));
        if (!j["pair"].is_null()) {
          Local<Object> o_ = Object::New(isolate);
          o_->Set(FN::v8S("base"), Number::New(isolate, j["/pair/base"_json_pointer].get<int>()));
          o_->Set(FN::v8S("quote"), Number::New(isolate, j["/pair/quote"_json_pointer].get<int>()));
          o->Set(FN::v8S("pair"), o_);
        }
        if (!j["side"].is_null()) o->Set(FN::v8S("side"), Number::New(isolate, j["side"].get<double>()));
        if (!j["price"].is_null()) o->Set(FN::v8S("price"), Number::New(isolate, j["price"].get<double>()));
        if (!j["quantity"].is_null()) o->Set(FN::v8S("quantity"), Number::New(isolate, j["quantity"].get<double>()));
        if (!j["type"].is_null()) o->Set(FN::v8S("type"), Number::New(isolate, j["type"].get<int>()));
        if (!j["timeInForce"].is_null()) o->Set(FN::v8S("timeInForce"), Number::New(isolate, j["timeInForce"].get<int>()));
        if (!j["orderStatus"].is_null()) o->Set(FN::v8S("orderStatus"), Number::New(isolate, j["orderStatus"].get<int>()));
        if (!j["lastQuantity"].is_null()) o->Set(FN::v8S("lastQuantity"), Number::New(isolate, j["lastQuantity"].get<double>()));
        if (!j["lastPrice"].is_null()) o->Set(FN::v8S("lastPrice"), Number::New(isolate, j["lastPrice"].get<double>()));
        if (!j["leavesQuantity"].is_null()) o->Set(FN::v8S("leavesQuantity"), Number::New(isolate, j["leavesQuantity"].get<double>()));
        if (!j["computationalLatency"].is_null()) o->Set(FN::v8S("computationalLatency"), Number::New(isolate, j["computationalLatency"].get<unsigned long>()));
        if (!j["liquidity"].is_null()) o->Set(FN::v8S("liquidity"), Number::New(isolate, j["liquidity"].get<int>()));
        if (!j["isPong"].is_null()) o->Set(FN::v8S("isPong"), Boolean::New(isolate, j["isPong"].get<bool>()));
        if (!j["preferPostOnly"].is_null()) o->Set(FN::v8S("preferPostOnly"), Boolean::New(isolate, j["preferPostOnly"].get<bool>()));
        return o;
      };
  };
}

#endif
