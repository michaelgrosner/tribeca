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
  Gw* gw;
  class GW {
    public:
      static void main(Local<Object> exports) {
        gw = Gw::E(CF::cfExchange());
        NODE_SET_METHOD(exports, "makeFee", GW::_makeFee);
        NODE_SET_METHOD(exports, "takeFee", GW::_takeFee);
        NODE_SET_METHOD(exports, "minTick", GW::_minTick);
        NODE_SET_METHOD(exports, "minSize", GW::_minSize);
        NODE_SET_METHOD(exports, "exchange", GW::_exchange);
        NODE_SET_METHOD(exports, "setMinTick", GW::_setMinTick);
        NODE_SET_METHOD(exports, "setMinSize", GW::_setMinSize);
      };
    private:
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
  };
}

#endif
