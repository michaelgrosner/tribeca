#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW {
    public:
      static void main(Local<Object> exports) {
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &gwPos_)) { cout << FN::uiT() << "Errrror: GW gwPos_ init timer failed." << endl; exit(1); }
          gwPos_.data = gw;
          if (uv_timer_start(&gwPos_, [](uv_timer_t *handle) {
            Gw* gw = (Gw*) handle->data;
            gw->pos();
          }, 0, 15000)) { cout << FN::uiT() << "Errrror: GW gwPos_ start timer failed." << endl; exit(1); }
        }).detach();
        EV::evOn("GatewayMarketConnect", [](Local<Object> c) {
          _gwCon_(mGatewayType::MarketData, (mConnectivityStatus)c->NumberValue());
        });
        EV::evOn("GatewayOrderConnect", [](Local<Object> c) {
          _gwCon_(mGatewayType::OrderEntry, (mConnectivityStatus)c->NumberValue());
        });
        gw->book();
        gw_ = (gw->target == "NULL") ? Gw::E(mExchange::Null) : gw;
        UI::uiSnap(uiTXT::ProductAdvertisement, &onSnapProduct);
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
        NODE_SET_METHOD(exports, "gwMakeFee", GW::_gwMakeFee);
        NODE_SET_METHOD(exports, "gwTakeFee", GW::_gwTakeFee);
        NODE_SET_METHOD(exports, "gwMinTick", GW::_gwMinTick);
        NODE_SET_METHOD(exports, "gwMinSize", GW::_gwMinSize);
        NODE_SET_METHOD(exports, "gwExchange", GW::_gwExchange);
        NODE_SET_METHOD(exports, "gwSymbol", GW::_gwSymbol);
        NODE_SET_METHOD(exports, "gwCancelByClientId", GW::_gwCancelByClientId);
        NODE_SET_METHOD(exports, "gwSupportCancelAll", GW::_gwSupportCancelAll);
        NODE_SET_METHOD(exports, "gwClientId", GW::_gwClientId);
        NODE_SET_METHOD(exports, "gwCancelAll", GW::_gwCancelAll);
        NODE_SET_METHOD(exports, "gwCancel", GW::_gwCancel);
        NODE_SET_METHOD(exports, "gwSend", GW::_gwSend);
      };
      static void gwPosUp(mGWp k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "amount"), Number::New(isolate, k.amount));
        o->Set(FN::v8S(isolate, "heldAmount"), Number::New(isolate, k.held));
        o->Set(FN::v8S(isolate, "currency"), Number::New(isolate, k.currency));
        EV::evUp("PositionGateway", o);
      };
      static void gwBookUp(mConnectivityStatus k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        EV::evUp("GatewayMarketConnect", Number::New(isolate, (int)k));
      };
      static void gwLevelUp(mGWbls k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        Local<Object> b = Array::New(isolate);
        Local<Object> a = Array::New(isolate);
        int i = 0;
        for (vector<mGWbl>::iterator it = k.bids.begin(); it != k.bids.end(); ++it) {
          Local<Object> b_ = Object::New(isolate);
          b_->Set(FN::v8S(isolate, "price"), Number::New(isolate, (*it).price));
          b_->Set(FN::v8S(isolate, "size"), Number::New(isolate, (*it).size));
          b->Set(i++, b_);
        }
        i = 0;
        for (vector<mGWbl>::iterator it = k.asks.begin(); it != k.asks.end(); ++it) {
          Local<Object> a_ = Object::New(isolate);
          a_->Set(FN::v8S(isolate, "price"), Number::New(isolate, (*it).price));
          a_->Set(FN::v8S(isolate, "size"), Number::New(isolate, (*it).size));
          a->Set(i++, a_);
        }
        o->Set(FN::v8S(isolate, "bids"), b);
        o->Set(FN::v8S(isolate, "asks"), a);
        EV::evUp("MarketDataGateway", o);
      };
      static void gwTradeUp(vector<mGWbt> k) {
        for (vector<mGWbt>::iterator it = k.begin(); it != k.end(); ++it)
          gwTradeUp(*it);
      }
      static void gwTradeUp(mGWbt k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "price"), Number::New(isolate, k.price));
        o->Set(FN::v8S(isolate, "size"), Number::New(isolate, k.size));
        o->Set(FN::v8S(isolate, "make_side"), Number::New(isolate, (double)k.make_side));
        EV::evUp("MarketTradeGateway", o);
      };
      static void gwOrderUp(mConnectivityStatus k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        EV::evUp("GatewayOrderConnect", Number::New(isolate, (int)k));
      };
      static void gwOrderUp(mGWos k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        if (k.oI.length()) o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, k.oI));
        if (k.oE.length()) o->Set(FN::v8S(isolate, "exchangeId"), FN::v8S(isolate, k.oE));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)k.oS));
        if (k.oS == mORS::Cancelled) o->Set(FN::v8S(isolate, "lastQuantity"), Number::New(isolate, 0));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(mGWol k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, k.oI));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)k.oS));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        o->Set(FN::v8S(isolate, "lastPrice"), Number::New(isolate, k.oP));
        o->Set(FN::v8S(isolate, "lastQuantity"), Number::New(isolate, k.oQ));
        o->Set(FN::v8S(isolate, "liquidity"), Number::New(isolate, (double)k.oL));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(mGWoS k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        if (k.oI.length()) o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, k.oI));
        if (k.oE.length()) o->Set(FN::v8S(isolate, "exchangeId"), FN::v8S(isolate, k.oE));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)k.os));
        o->Set(FN::v8S(isolate, "side"), Number::New(isolate, (int)k.oS));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        o->Set(FN::v8S(isolate, "lastPrice"), Number::New(isolate, k.oP));
        o->Set(FN::v8S(isolate, "lastQuantity"), Number::New(isolate, k.oQ));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(mGWoa k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, k.oI));
        if (k.oE.length()) o->Set(FN::v8S(isolate, "exchangeId"), FN::v8S(isolate, k.oE));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)k.os));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        if (k.oP) o->Set(FN::v8S(isolate, "lastPrice"), Number::New(isolate, k.oP));
        o->Set(FN::v8S(isolate, "leavesQuantity"), Number::New(isolate, k.oQ));
        o->Set(FN::v8S(isolate, "lastQuantity"), Number::New(isolate, k.oLQ));
        if (k.oC) o->Set(FN::v8S(isolate, "cumQuantity"), Number::New(isolate, k.oC));
        if (k.oA) o->Set(FN::v8S(isolate, "averagePrice"), Number::New(isolate, k.oA));
        EV::evUp("OrderUpdateGateway", o);
      };
    private:
      static Local<Value> onSnapProduct(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k_ = Array::New(isolate);
        Local<Object> k = Object::New(isolate);
        k->Set(FN::v8S("exchange"), Number::New(isolate, (double)gw->exchange));
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("base"), Number::New(isolate, (double)gw->base));
        o->Set(FN::v8S("quote"), Number::New(isolate, (double)gw->quote));
        k->Set(FN::v8S("pair"), o);
        k->Set(FN::v8S("environment"), FN::v8S(isolate, CF::cfString("BotIdentifier").substr(gwAutoStart?4:0)));
        k->Set(FN::v8S("matryoshka"), FN::v8S(isolate, CF::cfString("MatryoshkaUrl")));
        k->Set(FN::v8S("homepage"), FN::v8S(isolate, CF::cfPKString("homepage")));
        k->Set(FN::v8S("minTick"), Number::New(isolate, gw->minTick));
        k_->Set(0, k);
        return k_;
      };
      static Local<Value> onSnapStatus(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, Number::New(isolate, (double)gwConn));
        return k;
      };
      static Local<Value> onSnapState(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k = Array::New(isolate);
        k->Set(0, Boolean::New(isolate, gwState));
        return k;
      };
      static Local<Value> onHandState(Local<Value> s) {
        Isolate* isolate = Isolate::GetCurrent();
        if (s->BooleanValue() != gwAutoStart) {
          gwAutoStart = s->BooleanValue();
          gwUpState();
          Local<Object> o = Object::New(isolate);
          o->Set(FN::v8S("state"), Boolean::New(isolate, gwState));
          o->Set(FN::v8S("status"), Number::New(isolate, (double)gwConn));
          EV::evUp("ExchangeConnect", o);
        }
        return (Local<Value>)Undefined(isolate);
      };
      static void _gwCon_(mGatewayType gwT, mConnectivityStatus gwS) {
        if (gwT == mGatewayType::MarketData) {
          if (gwMDConn == gwS) return;
          gwMDConn = gwS;
        } else if (gwT == mGatewayType::OrderEntry) {
          if (gwEOConn == gwS) return;
          gwEOConn = gwS;
        }
        gwConn = gwMDConn == mConnectivityStatus::Connected && gwEOConn == mConnectivityStatus::Connected
          ? mConnectivityStatus::Connected : mConnectivityStatus::Disconnected;
        gwUpState();
        Isolate* isolate = Isolate::GetCurrent();
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("state"), Boolean::New(isolate, gwState));
        o->Set(FN::v8S("status"), Number::New(isolate, (double)gwConn));
        EV::evUp("ExchangeConnect", o);
        UI::uiSend(isolate, uiTXT::ExchangeConnectivity, Number::New(isolate, (double)gwConn)->ToObject());
      };
      static void gwUpState() {
        Isolate* isolate = Isolate::GetCurrent();
        bool newMode = gwConn != mConnectivityStatus::Connected ? false : gwAutoStart;
        if (newMode != gwState) {
          gwState = newMode;
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Changed quoting state to " << (gwState ? "Enabled" : "Disabled") << "." << endl;
          UI::uiSend(isolate, uiTXT::ActiveState, Boolean::New(isolate, gwState)->ToObject());
        }
      }
      static void _gwMakeFee(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->makeFee));
      };
      static void _gwTakeFee(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->takeFee));
      };
      static void _gwMinTick(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->minTick));
      };
      static void _gwMinSize(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->minSize));
      };
      static void _gwExchange(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, (double)gw->exchange));
      };
      static void _gwSymbol(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(FN::v8S(isolate, gw->symbol));
      };
      static void _gwCancelByClientId(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Boolean::New(isolate, gw_->cancelByClientId));
      };
      static void _gwSupportCancelAll(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Boolean::New(isolate, gw_->supportCancelAll));
      };
      static void _gwSend(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = args[0]->ToObject();
        string oI = FN::S8v(o->Get(FN::v8S(isolate, "orderId"))->ToString());
        unsigned long oT = o->Get(FN::v8S(isolate, "time"))->NumberValue();
        double oP = decimal_cast<8>(o->Get(FN::v8S(isolate, "price"))->NumberValue()).getAsDouble();
        double oQ = decimal_cast<8>(o->Get(FN::v8S(isolate, "quantity"))->NumberValue()).getAsDouble();
        mSide oS = (mSide)o->Get(FN::v8S(isolate, "side"))->NumberValue();
        mOrderType oLM = (mOrderType)o->Get(FN::v8S(isolate, "type"))->NumberValue();
        mTimeInForce oTIF = (mTimeInForce)o->Get(FN::v8S(isolate, "timeInForce"))->NumberValue();
        bool oPO = o->Get(FN::v8S(isolate, "preferPostOnly"))->BooleanValue();
        gw_->send(oI, oS, oP, oQ, oLM, oTIF, oPO, oT);
      };
      static void _gwCancel(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = args[0]->ToObject();
        string oI = FN::S8v(o->Get(FN::v8S(isolate, "orderId"))->ToString());
        string oE = o->Get(FN::v8S(isolate, "exchangeId"))->IsUndefined() ? "" : FN::S8v(o->Get(FN::v8S(isolate, "exchangeId"))->ToString());
        mSide oS = (mSide)o->Get(FN::v8S(isolate, "side"))->NumberValue();
        unsigned long oT = o->Get(FN::v8S(isolate, "time"))->NumberValue();
        gw_->cancel(oI, oE, oS, oT);
      };
      static void _gwCancelAll(const FunctionCallbackInfo<Value> &args) {
        gw_->cancelAll();
      };
      static void _gwClientId(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(FN::v8S(isolate, gw_->clientId()));
      };
  };
}

#endif
