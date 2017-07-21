#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class Gw {
    public:
      static Gw *E(mExchange e);
      double makeFee = 0;
      double takeFee = 0;
      double minTick = 0;
      double minSize = 0;
      virtual mExchange exchange() = 0;
  };
  class GwNull: public Gw {
    public:
      mExchange exchange() { return mExchange::Null; }
  };
  class GwHitBtc: public Gw {
    public:
      mExchange exchange() { return mExchange::HitBtc; }
  };
  class GwOkCoin: public Gw {
    public:
      mExchange exchange() { return mExchange::OkCoin; }
  };
  class GwCoinbase: public Gw {
    public:
      mExchange exchange() { return mExchange::Coinbase; }
  };
  class GwBitfinex: public Gw {
    public:
      mExchange exchange() { return mExchange::Bitfinex; }
  };
  class GwKorbit: public Gw {
    public:
      mExchange exchange() { return mExchange::Korbit; }
  };
  class GwPoloniex: public Gw {
    public:
      mExchange exchange() { return mExchange::Poloniex; }
  };
  Gw *Gw::E(mExchange e) {
    if (e == mExchange::Null) return new GwNull;
    else if (e == mExchange::HitBtc) return new GwHitBtc;
    else if (e == mExchange::OkCoin) return new GwOkCoin;
    else if (e == mExchange::Coinbase) return new GwCoinbase;
    else if (e == mExchange::Bitfinex) return new GwBitfinex;
    else if (e == mExchange::Korbit) return new GwKorbit;
    else if (e == mExchange::Poloniex) return new GwPoloniex;
    cout << FN::uiT() << "No gateway provided for exchange \"" << CF::cfString("EXCHANGE") << "\"." << endl;
    exit(1);
  };
  bool savedQuotingMode = false;
  bool gwState = false;
  mConnectivityStatus gwConn = mConnectivityStatus::Disconnected;
  mConnectivityStatus gwMDConn = mConnectivityStatus::Disconnected;
  mConnectivityStatus gwEOConn = mConnectivityStatus::Disconnected;
  Gw* gw;
  class GW {
    public:
      static void main(Local<Object> exports) {
        gw = Gw::E(CF::cfExchange());
        EV::evOn("GatewayMarketConnect", [](Local<Object> c) {
          gwCon__(mGatewayType::MarketData, (mConnectivityStatus)c->NumberValue());
        });
        EV::evOn("GatewayOrderConnect", [](Local<Object> c) {
          gwCon__(mGatewayType::OrderEntry, (mConnectivityStatus)c->NumberValue());
        });
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
        NODE_SET_METHOD(exports, "makeFee", GW::_makeFee);
        NODE_SET_METHOD(exports, "takeFee", GW::_takeFee);
        NODE_SET_METHOD(exports, "minTick", GW::_minTick);
        NODE_SET_METHOD(exports, "minSize", GW::_minSize);
        NODE_SET_METHOD(exports, "exchange", GW::_exchange);
        NODE_SET_METHOD(exports, "setMinTick", GW::_setMinTick);
        NODE_SET_METHOD(exports, "setMinSize", GW::_setMinSize);
        NODE_SET_METHOD(exports, "setSavedQuotingMode", GW::_setSavedQuotingMode);
      };
    private:
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
        if (s->BooleanValue() != savedQuotingMode) {
          savedQuotingMode = s->BooleanValue();
          gwUpState();
          Local<Object> o = Object::New(isolate);
          o->Set(FN::v8S("state"), Boolean::New(isolate, gwState));
          o->Set(FN::v8S("status"), Number::New(isolate, (double)gwConn));
          EV::evUp("ExchangeConnect", o);
        }
        return (Local<Value>)Undefined(isolate);
      };
      static void gwCon__(mGatewayType gwT, mConnectivityStatus gwS) {
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
        bool newMode = gwConn != mConnectivityStatus::Connected ? false : savedQuotingMode;
        if (newMode != gwState) {
          gwState = newMode;
          cout << FN::uiT() << "GW changed quoting state to " << (gwState ? "Enabled" : "Disabled") << "." << endl;
          UI::uiSend(isolate, uiTXT::ActiveState, Boolean::New(isolate, gwState)->ToObject());
        }
      }
      static void _makeFee(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->makeFee));
      };
      static void _takeFee(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->takeFee));
      };
      static void _minTick(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->minTick));
      };
      static void _minSize(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, gw->minSize));
      };
      static void _exchange(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(Number::New(isolate, (double)gw->exchange()));
      };
      static void _setMinTick(const FunctionCallbackInfo<Value> &args) {
        gw->minTick = args[0]->NumberValue();
      };
      static void _setMinSize(const FunctionCallbackInfo<Value> &args) {
        gw->minSize = args[0]->NumberValue();
      };
      static void _setSavedQuotingMode(const FunctionCallbackInfo<Value> &args) {
        savedQuotingMode = args[0]->BooleanValue();
      };
  };
}

#endif
