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
      virtual string symbol() = 0;
      virtual void fetch() = 0;
  };
  class GwNull: public Gw {
    public:
      mExchange exchange() { return mExchange::Null; };
      string symbol() { return string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfQuote()]); };
      void fetch() { minTick = 0.01; minSize = 0.01; };
  };
  class GwHitBtc: public Gw {
    public:
      mExchange exchange() { return mExchange::HitBtc; };
      string symbol() { return string(mCurrency[CF::cfBase()]).append(mCurrency[CF::cfQuote()]); };
      void fetch() { minTick = 0.01; minSize = 0.01; };
  };
  class GwOkCoin: public Gw {
    public:
      mExchange exchange() { return mExchange::OkCoin; };
      string symbol() { return FN::S2l(string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfQuote()])); };
      void fetch() { minTick = "btc" == symbol().substr(0,3) ? 0.01 : 0.001; minSize = 0.01; };
  };
  class GwCoinbase: public Gw {
    public:
      mExchange exchange() { return mExchange::Coinbase; };
      string symbol() { return string(mCurrency[CF::cfBase()]).append("-").append(mCurrency[CF::cfQuote()]); };
      void fetch() { minTick = 0.01; minSize = 0.01; };
  };
  class GwBitfinex: public Gw {
    public:
      mExchange exchange() { return mExchange::Bitfinex; };
      string symbol() { return FN::S2l(string(mCurrency[CF::cfBase()]).append(mCurrency[CF::cfQuote()])); };
      void fetch() { minTick = 0.01; minSize = 0.01; };
  };
  class GwKorbit: public Gw {
    public:
      mExchange exchange() { return mExchange::Korbit; };
      string symbol() { return FN::S2l(string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfQuote()])); };
      void fetch() {
        Isolate* isolate = Isolate::GetCurrent();
        JSON Json;
        MaybeLocal<Value> v = Json.Parse(isolate->GetCurrentContext(), FN::v8S(FN::wGet(CF::cfString("KorbitHttpUrl").append("/constants")).data()));
        double _minTick;
        if (!v.IsEmpty()) {
          Local<Object> k = v.ToLocalChecked()->ToObject();
          _minTick = k->Get(FN::v8S(symbol().substr(0,3).append("TickSize")))->NumberValue();
        }
        minTick = _minTick ? _minTick : 500;
        minSize = 0.015;
      };
  };
  class GwPoloniex: public Gw {
    public:
      mExchange exchange() { return mExchange::Poloniex; };
      string symbol() { return FN::S2l(string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfBase()])); };
      void fetch() { minTick = 0.01; minSize = 0.01; };
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
        gw->fetch();
        savedQuotingMode = "auto" == CF::cfString("BotIdentifier").substr(0,4);
        EV::evOn("GatewayMarketConnect", [](Local<Object> c) {
          gwCon__(mGatewayType::MarketData, (mConnectivityStatus)c->NumberValue());
        });
        EV::evOn("GatewayOrderConnect", [](Local<Object> c) {
          gwCon__(mGatewayType::OrderEntry, (mConnectivityStatus)c->NumberValue());
        });
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
        NODE_SET_METHOD(exports, "gwMakeFee", GW::_gwMakeFee);
        NODE_SET_METHOD(exports, "gwTakeFee", GW::_gwTakeFee);
        NODE_SET_METHOD(exports, "gwMinTick", GW::_gwMinTick);
        NODE_SET_METHOD(exports, "gwMinSize", GW::_gwMinSize);
        NODE_SET_METHOD(exports, "gwExchange", GW::_gwExchange);
        NODE_SET_METHOD(exports, "gwSymbol", GW::_gwSymbol);
        NODE_SET_METHOD(exports, "gwSetMinTick", GW::_gwSetMinTick);
        NODE_SET_METHOD(exports, "gwSetMinSize", GW::_gwSetMinSize);
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
        args.GetReturnValue().Set(Number::New(isolate, (double)gw->exchange()));
      };
      static void _gwSymbol(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(FN::v8S(isolate, gw->symbol()));
      };
      static void _gwSetMinTick(const FunctionCallbackInfo<Value> &args) {
        gw->minTick = args[0]->NumberValue();
      };
      static void _gwSetMinSize(const FunctionCallbackInfo<Value> &args) {
        gw->minSize = args[0]->NumberValue();
      };
  };
}

#endif
