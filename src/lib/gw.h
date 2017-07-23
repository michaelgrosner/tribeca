#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class Gw {
    public:
      static Gw *E(mExchange e);
      mExchange exchange = mExchange::Null;
      double makeFee = 0;
      double takeFee = 0;
      double minTick = 0;
      double minSize = 0;
      string symbol = "";
      string target = "";
      string apikey = "";
      string secret = "";
      string user = "";
      string pass = "";
      string http = "";
      string ws = "";
      string wS = "";
      int quote = 0;
      int base = 0;
      virtual void fetch() = 0;
      virtual void pos() = 0;
  };
  uv_timer_t gwPos_;
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
        gw->base = CF::cfBase();
        gw->quote = CF::cfQuote();
        gw->fetch();
        if (!gw->minTick) { cout << FN::uiT() << "GW Unable to match TradedPair to " << CF::cfString("EXCHANGE") << " symbol \"" << gw->symbol << "\"." << endl; exit(1); }
        else { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " allows client IP." << endl; }
        savedQuotingMode = "auto" == CF::cfString("BotIdentifier").substr(0,4);
        cout << FN::uiT() << "GW " << fixed << CF::cfString("EXCHANGE") << ":" << endl << "- autoBot: " << (savedQuotingMode ? "yes" : "no") << endl << "- pair: " << gw->symbol << endl << "- minTick: " << gw->minTick << endl << "- minSize: " << gw->minSize << endl << "- makeFee: " << gw->makeFee << endl << "- takeFee: " << gw->takeFee << endl;
        if (uv_timer_init(uv_default_loop(), &gwPos_)) { cout << FN::uiT() << "Errrror: GW gwPos_ init timer failed." << endl; exit(1); }
        gwPos_.data = gw;
        if (uv_timer_start(&gwPos_, gwPos__, 0, 15000)) { cout << FN::uiT() << "Errrror: UV gwPos_ start timer failed." << endl; exit(1); }
        EV::evOn("GatewayMarketConnect", [](Local<Object> c) {
          gwCon__(mGatewayType::MarketData, (mConnectivityStatus)c->NumberValue());
        });
        EV::evOn("GatewayOrderConnect", [](Local<Object> c) {
          gwCon__(mGatewayType::OrderEntry, (mConnectivityStatus)c->NumberValue());
        });
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
      };
      static void gwPos__(uv_timer_t *handle) {
        Gw* gw = (Gw*) handle->data;
        gw->pos();
      };
      static void gwPosUp(double amount, double held, int currency) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("amount"), Number::New(isolate, amount));
        o->Set(FN::v8S("heldAmount"), Number::New(isolate, held));
        o->Set(FN::v8S("currency"), Number::New(isolate, currency));
        EV::evUp("PositionGateway", o);
      };
    private:
      static Local<Value> onSnapProduct(Local<Value> z) {
        Isolate* isolate = Isolate::GetCurrent();
        Local<Array> k_ = Array::New(isolate);
        Local<Object> k = Object::New(isolate);
        k->Set(FN::v8S("exchange"), Number::New(isolate, (double)gw->exchange));
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("base"), Number::New(isolate, (double)CF::cfBase()));
        o->Set(FN::v8S("quote"), Number::New(isolate, (double)CF::cfQuote()));
        k->Set(FN::v8S("pair"), o);
        k->Set(FN::v8S("environment"), FN::v8S(isolate, CF::cfString("BotIdentifier").substr(savedQuotingMode?4:0)));
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
        args.GetReturnValue().Set(Number::New(isolate, (double)gw->exchange));
      };
      static void _gwSymbol(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        args.GetReturnValue().Set(FN::v8S(isolate, gw->symbol));
      };
  };
  class GwNull: public Gw {
    public:
      void fetch() {
        symbol = string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfQuote()]);
        minTick = 0.01;
        minSize = 0.01;
      };
      void pos() {
        GW::gwPosUp(500, 50, base);
        GW::gwPosUp(500, 50, quote);
      };
  };
  class GwOkCoin: public Gw {
    public:
      void fetch() {
        exchange = mExchange::OkCoin;
        symbol = FN::S2l(string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfQuote()]));
        target = CF::cfString("OkCoinOrderDestination");
        apikey = CF::cfString("OkCoinApiKey");
        secret = CF::cfString("OkCoinSecretKey");
        http = CF::cfString("OkCoinHttpUrl");
        ws = CF::cfString("OkCoinWsUrl");
        minTick = "btc" == symbol.substr(0,3) ? 0.01 : 0.001;
        minSize = 0.01;
      };
      void pos() {
        string p = string("api_key=").append(apikey);
        string p_ = string(p).append("&secret_key=").append(secret);
        unsigned char digest[MD5_DIGEST_LENGTH];
        MD5((unsigned char*)p_.data(), p_.length(), (unsigned char*)&digest);
        char mdS[33];
        for(int i = 0; i < 16; i++) sprintf(&mdS[i*2], "%02x", (unsigned int)digest[i]);
        json k = FN::wJet(string(http).append("userinfo.do"), p.append("&sign=").append(FN::S2u(mdS)));
        if (k["result"] == true) {
          json free = k["info"]["funds"]["free"];
          json freezed = k["info"]["funds"]["freezed"];
          for (json::iterator it = free.begin(); it != free.end(); ++it)
            if (symbol.find(it.key()) != string::npos)
              GW::gwPosUp(stod(free[it.key()].get<string>()), stod(freezed[it.key()].get<string>()), FN::S2mC(it.key()));
        }
      };
  };
  class GwCoinbase: public Gw {
    public:
      void fetch() {
        exchange = mExchange::Coinbase;
        symbol = string(mCurrency[CF::cfBase()]).append("-").append(mCurrency[CF::cfQuote()]);
        target = CF::cfString("CoinbaseOrderDestination");
        apikey = CF::cfString("CoinbaseApiKey");
        secret = CF::cfString("CoinbaseSecret");
        pass = CF::cfString("CoinbasePassphrase");
        http = CF::cfString("CoinbaseRestUrl");
        ws = CF::cfString("CoinbaseWebsocketUrl");
        json k = FN::wJet(CF::cfString("CoinbaseRestUrl").append("/products/").append(symbol));
        if (k.find("quote_increment") != k.end()) {
          minTick = stod(k["quote_increment"].get<string>());
          minSize = stod(k["base_min_size"].get<string>());
        }
      };
      void pos() { };
  };
  class GwBitfinex: public Gw {
    public:
      void fetch() {
        exchange = mExchange::Bitfinex;
        symbol = FN::S2l(string(mCurrency[CF::cfBase()]).append(mCurrency[CF::cfQuote()]));
        target = CF::cfString("BitfinexOrderDestination");
        apikey = CF::cfString("BitfinexKey");
        secret = CF::cfString("BitfinexSecret");
        http = CF::cfString("BitfinexHttpUrl");
        ws = CF::cfString("BitfinexWebsocketUrl");
        json k = FN::wJet(CF::cfString("BitfinexHttpUrl").append("/pubticker/").append(symbol));
        if (k.find("last_price") != k.end()) {
          istringstream os(string("1e-").append(to_string(5-k["last_price"].get<string>().find("."))));
          os >> minTick;
          minSize = 0.01;
        }
      };
      void pos() { };
  };
  class GwKorbit: public Gw {
    public:
      void fetch() {
        exchange = mExchange::Korbit;
        symbol = FN::S2l(string(mCurrency[CF::cfBase()]).append("_").append(mCurrency[CF::cfQuote()]));
        target = CF::cfString("KorbitOrderDestination");
        apikey = CF::cfString("KorbitApiKey");
        secret = CF::cfString("KorbitSecretKey");
        user = CF::cfString("KorbitUsername");
        pass = CF::cfString("KorbitPassword");
        http = CF::cfString("KorbitHttpUrl");
          json k = FN::wJet(CF::cfString("KorbitHttpUrl").append("/constants"));
          if (k.find(symbol.substr(0,3).append("TickSize")) != k.end()) {
            minTick = k[symbol.substr(0,3).append("TickSize")];
            minSize = 0.015;
          }
      };
      void pos() { };
  };
  class GwHitBtc: public Gw {
    public:
      void fetch() {
        exchange = mExchange::HitBtc;
        symbol = string(mCurrency[CF::cfBase()]).append(mCurrency[CF::cfQuote()]);
        target = CF::cfString("HitBtcOrderDestination");
        apikey = CF::cfString("HitBtcApiKey");
        secret = CF::cfString("HitBtcSecret");
        http = CF::cfString("HitBtcPullUrl");
        ws = CF::cfString("HitBtcOrderEntryUrl");
        wS = CF::cfString("HitBtcMarketDataUrl");
        json k = FN::wJet(CF::cfString("HitBtcPullUrl").append("/api/1/public/symbols"));
        if (k.find("symbols") != k.end())
          for (json::iterator it = k["symbols"].begin(); it != k["symbols"].end(); ++it)
            if ((*it)["symbol"].get<string>() == symbol) {
              minTick = stod((*it)["step"].get<string>());
              minSize = stod((*it)["lot"].get<string>());
              break;
            }
      };
      void pos() { };
  };
  class GwPoloniex: public Gw {
    public:
      void fetch() {
        exchange = mExchange::Poloniex;
        symbol = string(mCurrency[CF::cfQuote()]).append("_").append(mCurrency[CF::cfBase()]);
        string target = CF::cfString("PoloniexOrderDestination");
        apikey = CF::cfString("PoloniexApiKey");
        secret = CF::cfString("PoloniexSecretKey");
        http = CF::cfString("PoloniexHttpUrl");
        ws = CF::cfString("PoloniexWebsocketUrl");
        json k = FN::wJet(CF::cfString("PoloniexHttpUrl").append("/public?command=returnTicker"));
        if (k.find(symbol) != k.end()) {
          istringstream os(string("1e-").append(to_string(6-k[symbol]["last"].get<string>().find("."))));
          os >> minTick;
          minSize = 0.01;
        }
      };
      void pos() { };
  };
  Gw *Gw::E(mExchange e) {
    if (e == mExchange::Null) return new GwNull;
    else if (e == mExchange::OkCoin) return new GwOkCoin;
    else if (e == mExchange::Coinbase) return new GwCoinbase;
    else if (e == mExchange::Bitfinex) return new GwBitfinex;
    else if (e == mExchange::Korbit) return new GwKorbit;
    else if (e == mExchange::HitBtc) return new GwHitBtc;
    else if (e == mExchange::Poloniex) return new GwPoloniex;
    cout << FN::uiT() << "No gateway provided for exchange \"" << CF::cfString("EXCHANGE") << "\"." << endl;
    exit(1);
  };
}

#endif
