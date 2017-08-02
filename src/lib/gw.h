#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW {
    public:
      static void main(Local<Object> exports) {
        gw = Gw::E(CF::cfExchange());
        gw->base = CF::cfBase();
        gw->quote = CF::cfQuote();
        gw->fetch();
        if (!gw->minTick) { cout << FN::uiT() << "Errrror: Unable to match TradedPair to " << CF::cfString("EXCHANGE") << " symbol \"" << gw->symbol << "\"." << endl; exit(1); }
        else { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " allows client IP." << endl; }
        gwAutoStart = "auto" == CF::cfString("BotIdentifier").substr(0,4);
        cout << FN::uiT() << "GW " << setprecision(8) << fixed << CF::cfString("EXCHANGE") << ":" << endl << "- autoBot: " << (gwAutoStart ? "yes" : "no") << endl << "- pair: " << gw->symbol << endl << "- minTick: " << gw->minTick << endl << "- minSize: " << gw->minSize << endl << "- makeFee: " << gw->makeFee << endl << "- takeFee: " << gw->takeFee << endl;
        if (uv_timer_init(uv_default_loop(), &gwPos_)) { cout << FN::uiT() << "Errrror: GW gwPos_ init timer failed." << endl; exit(1); }
        gwPos_.data = gw;
        if (uv_timer_start(&gwPos_, [](uv_timer_t *handle) {
          Gw* gw = (Gw*) handle->data;
          gw->pos();
        }, 0, 15000)) { cout << FN::uiT() << "Errrror: GW gwPos_ start timer failed." << endl; exit(1); }
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
      static void gwPosUp(double amount, double held, int currency) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "amount"), Number::New(isolate, amount));
        o->Set(FN::v8S(isolate, "heldAmount"), Number::New(isolate, held));
        o->Set(FN::v8S(isolate, "currency"), Number::New(isolate, currency));
        EV::evUp("PositionGateway", o);
      };
      static void gwBookUp(mConnectivityStatus k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        EV::evUp("GatewayMarketConnect", Number::New(isolate, (int)k));
      };
      static void gwLevelUp(mGWbls ls) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        Local<Object> b = Array::New(isolate);
        Local<Object> a = Array::New(isolate);
        int i = 0;
        for (vector<mGWbl>::iterator it = ls.bids.begin(); it != ls.bids.end(); ++it) {
          Local<Object> b_ = Object::New(isolate);
          b_->Set(FN::v8S(isolate, "price"), Number::New(isolate, (*it).price));
          b_->Set(FN::v8S(isolate, "size"), Number::New(isolate, (*it).size));
          b->Set(i++, b_);
        }
        i = 0;
        for (vector<mGWbl>::iterator it = ls.asks.begin(); it != ls.asks.end(); ++it) {
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
      static void gwTradeUp(mGWbt t) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "price"), Number::New(isolate, t.price));
        o->Set(FN::v8S(isolate, "size"), Number::New(isolate, t.size));
        o->Set(FN::v8S(isolate, "make_side"), Number::New(isolate, (double)t.make_side));
        EV::evUp("MarketTradeGateway", o);
      };
      static void gwOrderUp(mConnectivityStatus k) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        EV::evUp("GatewayOrderConnect", Number::New(isolate, (int)k));
      };
      static void gwOrderUp(string oI, mORS oS) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, oI));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)oS));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(string oI, unsigned long oT) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, oI));
        o->Set(FN::v8S(isolate, "computationalLatency"), Number::New(isolate, FN::T() - oT));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(string oI, string oE, mORS oS, unsigned long oT) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, oI));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)oS));
        o->Set(FN::v8S(isolate, "exchangeId"), FN::v8S(isolate, oE));
        o->Set(FN::v8S(isolate, "computationalLatency"), Number::New(isolate, FN::T() - oT));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(string oI, mORS oS, double oP, double oQ, mLiquidity oL) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, oI));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)oS));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        o->Set(FN::v8S(isolate, "lastPrice"), Number::New(isolate, oP));
        o->Set(FN::v8S(isolate, "lastQuantity"), Number::New(isolate, oQ));
        o->Set(FN::v8S(isolate, "liquidity"), Number::New(isolate, (double)oL));
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(string oI, string oE, mORS oS, double oP, double oQ, double oC, double oA) {
        Isolate* isolate = Isolate::GetCurrent();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S(isolate, "orderId"), FN::v8S(isolate, oI));
        if (oE.length()) o->Set(FN::v8S(isolate, "exchangeId"), FN::v8S(isolate, oE));
        o->Set(FN::v8S(isolate, "orderStatus"), Number::New(isolate, (int)oS));
        o->Set(FN::v8S(isolate, "time"), Number::New(isolate, FN::T()));
        o->Set(FN::v8S(isolate, "lastPrice"), Number::New(isolate, oP));
        o->Set(FN::v8S(isolate, "lastQuantity"), Number::New(isolate, oQ));
        if (oC) o->Set(FN::v8S(isolate, "cumQuantity"), Number::New(isolate, oC));
        if (oA) o->Set(FN::v8S(isolate, "averagePrice"), Number::New(isolate, oA));
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
        gw_->send(oI, oS, oP, oQ, oLM, oTIF, oT);
      };
      static void _gwCancel(const FunctionCallbackInfo<Value> &args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = args[0]->ToObject();
        string oI = FN::S8v(o->Get(FN::v8S(isolate, "orderId"))->ToString());
        mSide oS = (mSide)o->Get(FN::v8S(isolate, "side"))->NumberValue();
        unsigned long oT = o->Get(FN::v8S(isolate, "time"))->NumberValue();
        gw_->cancel(oI, oS, oT);
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
  class GwNull: public Gw {
    public:
      bool cancelByClientId = true;
      bool supportCancelAll = false;
      void fetch() {
        symbol = string(mCurrency[base]).append("_").append(mCurrency[quote]);
        minTick = 0.01;
        minSize = 0.01;
      };
      void pos() {
        GW::gwPosUp(5, 0, base);
        GW::gwPosUp(5000, 0, quote);
      };
      void book() {
        GW::gwBookUp(mConnectivityStatus::Connected);
        GW::gwOrderUp(mConnectivityStatus::Connected);
        if (uv_timer_init(uv_default_loop(), &gwBook_)) { cout << FN::uiT() << "Errrror: GW gwBook_ init timer failed." << endl; exit(1); }
        gwBook_.data = this;
        if (uv_timer_start(&gwBook_, [](uv_timer_t *handle) {
          GwNull* gw = (GwNull*) handle->data;
          srand(time(0));
          GW::gwLevelUp(gw->genLevels());
          if (rand() % 2) GW::gwTradeUp(gw->genTrades());
        }, 0, 2000)) { cout << FN::uiT() << "Errrror: GW gwBook_ start timer failed." << endl; exit(1); }
      };
      mGWbls genLevels() {
        vector<mGWbl> a;
        vector<mGWbl> b;
        for (unsigned i=0; i<13; ++i) {
          b.push_back(mGWbl(
            SD::roundNearest(1000 + -1 * 100 * ((rand() % (160000-120000)) / 10000.0), 0.01),
            (rand() % (160000-120000)) / 10000.0
          ));
          a.push_back(mGWbl(
            SD::roundNearest(1000 + 1 * 100 * ((rand() % (160000-120000)) / 10000.0), 0.01),
            (rand() % (160000-120000)) / 10000.0
          ));
        }
        sort(b.begin(), b.end(), [](const mGWbl &a_, const mGWbl &b_) { return a_.price*-1 < b_.price*-1; });
        sort(a.begin(), a.end(), [](const mGWbl &a_, const mGWbl &b_) { return a_.price < b_.price; });
        return mGWbls(b, a);
      };
      mGWbt genTrades() {
        mSide side = rand() % 2 ? mSide::Bid : mSide::Ask;
        int sign = side == mSide::Ask ? 1 : -1;
        mGWbt t(
          SD::roundNearest(1000 + sign * 100 * ((rand() % (160000-120000)) / 10000.0), 0.01),
          (rand() % (160000-120000)) / 10000.0,
          side
        );
        return t;
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {
        if (oTIF == mTimeInForce::IOC) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " doesn't work with IOC." << endl; exit(1); }
        GW::gwOrderUp(oI, oI, mORS::Working, oT);
        srand(time(0));
        if (rand() % 2) GW::gwOrderUp(oI, mORS::Working, oP, oQ, rand() % 2 ? mLiquidity::Make : mLiquidity::Take);
      }
      void cancel(string oI, mSide oS, unsigned long oT) {
        GW::gwOrderUp(oI, oI, mORS::Complete, oT);
      }
      void cancelAll() {}
      string clientId() { string t = to_string(FN::T()); return t.size()>9?t.substr(t.size()-9):t; }
  };
  class GwOkCoin: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      string symbolr = "";
      string chanM = "";
      string chanT = "";
      void fetch() {
        exchange = mExchange::OkCoin;
        symbol = FN::S2l(string(mCurrency[base]).append("_").append(mCurrency[quote]));
        symbolr = FN::S2l(string(mCurrency[quote]).append("_").append(mCurrency[base]));
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
        json k = FN::wJet(string(http).append("userinfo.do"), p.append("&sign=").append(FN::oMd5(string(p).append("&secret_key=").append(secret))));
        if (k["result"] != true) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl;
        else for (json::iterator it = k["info"]["funds"]["free"].begin(); it != k["info"]["funds"]["free"].end(); ++it)
          if (symbol.find(it.key()) != string::npos)
            GW::gwPosUp(stod(k["info"]["funds"]["free"][it.key()].get<string>()), stod(k["info"]["funds"]["freezed"][it.key()].get<string>()), FN::S2mC(it.key()));
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          GW::gwBookUp(mConnectivityStatus::Connected);
          chanM = string("ok_sub_spot").append(symbolr).append("_depth_20");
          chanT = string("ok_sub_spot").append(symbolr).append("_trades");
          string m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanM).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanT).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
        });
        hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) { GW::gwBookUp(mConnectivityStatus::Disconnected); });
        hub.onError([&](void *user) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error, reconnecting.." << endl; hub.connect(ws, nullptr); });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          json k = json::parse(string(message, length));
          if (k.is_array()) {
            k = k["/0"_json_pointer];
            if (k.find("channel") != k.end()) {
              if (k["channel"] == "addChannel") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming channel " << (k["data"]["result"]?"true":"false") << " " << k["data"]["channel"] << endl;
              else if (k["channel"] == chanT)
                for (json::iterator it = k["data"].begin(); it != k["data"].end(); ++it)
                  GW::gwTradeUp(mGWbt(
                    stod((*it)["/1"_json_pointer].get<string>()),
                    stod((*it)["/2"_json_pointer].get<string>()),
                    (*it)["/4"_json_pointer].get<string>() > "bid" ? mSide::Bid : mSide::Ask
                  ));
              else if (k["channel"] == chanM) {
                vector<mGWbl> a;
                vector<mGWbl> b;
                for (json::iterator it = k["data"]["bids"].begin(); it != k["data"]["bids"].end(); ++it) {
                  b.push_back(mGWbl(
                    stod((*it)["/0"_json_pointer].get<string>()),
                    stod((*it)["/1"_json_pointer].get<string>())
                  ));
                  if (b.size() == 13) break;
                }
                for (json::reverse_iterator it = k["data"]["asks"].rbegin(); it != k["data"]["asks"].rend(); ++it) {
                  a.push_back(mGWbl(
                    stod((*it)["/0"_json_pointer].get<string>()),
                    stod((*it)["/1"_json_pointer].get<string>())
                  ));
                  if (a.size() == 13) break;
                }
                if (a.size() && b.size())
                  GW::gwLevelUp(mGWbls(b, a));
              }
            }
          }
        });
        hub.connect(ws, nullptr);
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {}
      void cancel(string oI, mSide oS, unsigned long oT) {}
      void cancelAll() {}
      string clientId() { return ""; }
  };
  class GwCoinbase: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      double seq = 0;
      map<double, map<string, double>> a_;
      map<double, map<string, double>> b_;
      vector<json> q_;
      void fetch() {
        exchange = mExchange::Coinbase;
        symbol = string(mCurrency[base]).append("-").append(mCurrency[quote]);
        target = CF::cfString("CoinbaseOrderDestination");
        apikey = CF::cfString("CoinbaseApiKey");
        secret = CF::cfString("CoinbaseSecret");
        pass = CF::cfString("CoinbasePassphrase");
        http = CF::cfString("CoinbaseRestUrl");
        ws = CF::cfString("CoinbaseWebsocketUrl");
        json k = FN::wJet(string(http).append("/products/").append(symbol));
        if (k.find("quote_increment") != k.end()) {
          minTick = stod(k["quote_increment"].get<string>());
          minSize = stod(k["base_min_size"].get<string>());
        }
      };
      void pos() {
        unsigned long t = FN::T() / 1000;
        string p;
        B64::Decode(secret, &p);
        B64::Encode(FN::oHmac256(string(to_string(t)).append("GET/accounts"), p), &p);
        json k = FN::wJet(string(http).append("/accounts"), to_string(t), apikey, p, pass);
        if (k.find("message") != k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["message"] << endl;
        else for (json::iterator it = k.begin(); it != k.end(); ++it)
          if ((*it)["currency"] == mCurrency[base] || (*it)["currency"] == mCurrency[quote])
            GW::gwPosUp(stod((*it)["available"].get<string>()), stod((*it)["hold"].get<string>()), FN::S2mC((*it)["currency"]));
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          GW::gwBookUp(mConnectivityStatus::Connected);
          unsigned long t = FN::T() / 1000;
          string p;
          B64::Decode(secret, &p);
          B64::Encode(FN::oHmac256(string(to_string(t)).append("GET/users/self"), p), &p);
          string m = string("{\"type\":\"subscribe\",\"product_ids\":[\"").append(symbol).append("\"],\"signature\":\"").append(p).append("\",\"key\":\"").append(apikey).append("\",\"passphrase\":\"").append(pass).append("\",\"timestamp\":\"").append(to_string(t)).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming book levels." << endl;
        });
        hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) { GW::gwBookUp(mConnectivityStatus::Disconnected); });
        hub.onError([&](void *user) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error, reconnecting.." << endl; hub.connect(ws, nullptr); });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          json k = json::parse(string(message, length));
          if (k.find("type") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Message Error " << k << endl; return; }
          else if (k.find("message") != k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << k["type"] << k["message"] << endl; return; }
          else if (k.find("sequence") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Message Error " << k << endl; return; }
          else getTrades(k);
        });
        hub.connect(ws, nullptr);
        getLevels();
      };
      void getLevels() {
        seq = 0;
        b_.clear();
        a_.clear();
        unsigned long t = FN::T() / 1000;
        string p;
        B64::Decode(secret, &p);
        B64::Encode(FN::oHmac256(string(to_string(t)).append("GET/accounts"), p), &p);
        json k = FN::wJet(string(http).append("/products/").append(symbol).append("/book?level=3"), to_string(t), apikey, p, pass);
        if (k.find("sequence") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read book levels." << endl; return; }
        seq = k["sequence"];
        for (json::iterator it = k["bids"].begin(); it != k["bids"].end(); ++it)
          b_[decimal_cast<8>((*it)["/0"_json_pointer].get<string>()).getAsDouble()][(*it)["/2"_json_pointer].get<string>()] = decimal_cast<8>((*it)["/1"_json_pointer].get<string>()).getAsDouble();
        for (json::iterator it = k["asks"].begin(); it != k["asks"].end(); ++it)
          a_[decimal_cast<8>((*it)["/0"_json_pointer].get<string>()).getAsDouble()][(*it)["/2"_json_pointer].get<string>()] = decimal_cast<8>((*it)["/1"_json_pointer].get<string>()).getAsDouble();
        getBook();
        for (vector<json>::iterator it = q_.begin(); it != q_.end(); ++it)
          getTrades(*it);
        q_.clear();
      };
      void getBook() {
        if (b_.size() && a_.size()) {
          vector<mGWbl> _b_;
          vector<mGWbl> _a_;
          for (map<double, map<string, double>>::reverse_iterator it = b_.rbegin(); it != b_.rend(); ++it) {
            double bsize = decimal_cast<8>(0).getAsDouble();
            for (map<string, double>::iterator it_ = it->second.begin(); it_ != it->second.end(); ++it_)
              bsize += it_->second;
            _b_.push_back(mGWbl(it->first, bsize));
            if (_b_.size() == 13) break;
          }
          for (map<double, map<string, double>>::iterator it = a_.begin(); it != a_.end(); ++it) {
            double asize = decimal_cast<8>(0).getAsDouble();
            for (map<string, double>::iterator it_ = it->second.begin(); it_ != it->second.end(); ++it_)
              asize += it_->second;
            _a_.push_back(mGWbl(it->first, asize));
            if (_a_.size() == 13) break;
          }
          GW::gwLevelUp(mGWbls(_b_, _a_));
        }
      };
      void getTrades(json k) {
        if (!seq) { q_.push_back(k); return; }
        else if (k["sequence"].get<double>() <= seq) return;
        else if (k["sequence"].get<double>() != seq + 1) return getLevels();
        seq = k["sequence"];
        if (k["type"]=="received") return;
        bool s = k["side"] == "buy";
        if (k["type"]=="done") {
          if (s) for (map<double, map<string, double>>::iterator it = b_.begin(); it != b_.end();) {
            if (b_[it->first].find(k["order_id"].get<string>()) != b_[it->first].end()) {
              b_[it->first].erase(k["order_id"].get<string>());
              if (!b_[it->first].size()) it = b_.erase(it);
              else ++it;
            } else ++it;
          } else for (map<double, map<string, double>>::iterator it = a_.begin(); it != a_.end();) {
            if (a_[it->first].find(k["order_id"].get<string>()) != a_[it->first].end()) {
              a_[it->first].erase(k["order_id"].get<string>());
              if (!a_[it->first].size()) it = a_.erase(it);
              else ++it;
            } else ++it;
          }
        } else if (k["type"]=="open") {
          double p = decimal_cast<8>(k["price"].get<string>()).getAsDouble();
          double a = decimal_cast<8>(k["remaining_size"].get<string>()).getAsDouble();
          if (s) b_[p][k["order_id"].get<string>()] = a;
          else a_[p][k["order_id"].get<string>()] = a;
        } else if (k["type"]=="match") {
          double p = decimal_cast<8>(k["price"].get<string>()).getAsDouble();
          double a = decimal_cast<8>(k["size"].is_string() ? k["size"].get<string>() : k["remaining_size"].get<string>()).getAsDouble();
          string oi = k["maker_order_id"].get<string>();
          GW::gwTradeUp(mGWbt(p, a, s ? mSide::Bid : mSide::Ask));
          if (s && b_.find(p) != b_.end() && b_[p].find(oi) != b_[p].end()) {
            if (b_[p][oi] == a) {
              b_[p].erase(oi);
              if (!b_[p].size()) b_.erase(p);
            } else b_[p][oi] -= a;
          } else if (!s && a_.find(p) != a_.end() && a_[p].find(oi) != a_[p].end()) {
            if (a_[p][oi] == a) {
              a_[p].erase(oi);
              if (!a_[p].size()) a_.erase(p);
            } else a_[p][oi] -= a;
          }
        } else if (k["type"]=="change") {
          double p = decimal_cast<8>(k["price"].get<string>()).getAsDouble();
          double a = decimal_cast<8>(k["new_size"].get<string>()).getAsDouble();
          string oi = k["order_id"].get<string>();
          if (s && b_.find(p) != b_.end() && b_[p].find(oi) != b_[p].end()) b_[p][oi] = a;
          else if (!s && a_.find(p) != a_.end() && a_[p].find(oi) != a_[p].end()) a_[p][oi] = a;
        }
        getBook();
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {}
      void cancel(string oI, mSide oS, unsigned long oT) {}
      void cancelAll() {}
      string clientId() { return ""; }
  };
  class GwBitfinex: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      bool stillAlive;
      map<int, string> chan;
      vector<mGWbl> b_;
      vector<mGWbl> a_;
      void fetch() {
        exchange = mExchange::Bitfinex;
        symbol = FN::S2l(string(mCurrency[base]).append(mCurrency[quote]));
        target = CF::cfString("BitfinexOrderDestination");
        apikey = CF::cfString("BitfinexKey");
        secret = CF::cfString("BitfinexSecret");
        http = CF::cfString("BitfinexHttpUrl");
        ws = CF::cfString("BitfinexWebsocketUrl");
        json k = FN::wJet(string(http).append("/pubticker/").append(symbol));
        if (k.find("last_price") != k.end()) {
          string k_ = to_string(stod(k["last_price"].get<string>()) / 10000);
          unsigned int i = stod(k["last_price"].get<string>())<0.00001 ? 1 : 0;
          for (string::iterator it=k_.begin(); it!=k_.end(); ++it)
            if (*it == '0') i++; else if (*it == '.') continue; else break;
          stringstream os(string("1e-").append(to_string(i>8?8:i)));
          os >> minTick;
          minSize = 0.01;
        }
      };
      void pos() {
        string p;
        B64::Encode(string("{\"request\":\"/v1/balances\",\"nonce\":\"").append(to_string(FN::T()*1000)).append("\"}"), &p);
        json k = FN::wJet(string(http).append("/balances"), p, apikey, FN::oHmac384(p, secret), true);
        if (k.find("message") != k.end()) {
          if (k["message"]=="Nonce is too small.") pos();
          else cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["message"] << endl;
        } else for (json::iterator it = k.begin(); it != k.end(); ++it) if ((*it)["type"] == "exchange")
          GW::gwPosUp(stod((*it)["available"].get<string>()), stod((*it)["amount"].get<string>()) - stod((*it)["available"].get<string>()), FN::S2mC((*it)["currency"]));
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          GW::gwBookUp(mConnectivityStatus::Connected);
          string m = string("{\"event\":\"subscribe\",\"channel\":\"book\",\"symbol\":\"t").append(FN::S2u(symbol)).append("\",\"prec\":\"P0\",\"freq\":\"F0\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          m = string("{\"event\":\"subscribe\",\"channel\":\"trades\",\"symbol\":\"t").append(FN::S2u(symbol)).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
        });
        hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) { GW::gwBookUp(mConnectivityStatus::Disconnected); });
        hub.onError([&](void *user) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error, reconnecting.." << endl; hub.connect(ws, nullptr); });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          json k = json::parse(string(message, length));
          if (k.find("event") != k.end()) {
            if (k["event"] == "pong") stillAlive = true;
            else if (k["event"] == "ping") { string m = string("{\"event\":\"pong\"}"); w->send(m.data(), m.length(), uWS::OpCode::TEXT); }
            else if (k["event"] == "error") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error " << k << endl;
            else if (k["event"] == "info") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Info " << k << endl;
            else if (k["event"] == "subscribed") { chan[k["chanId"]] = k["channel"]; cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming channel " << k["chanId"] << ":" << k["channel"] << endl; }
          } else if (k.is_array()) {
            if (k["/1"_json_pointer].is_string() && k["/1"_json_pointer] == "hb") stillAlive = true;
            else if (k["/1"_json_pointer].is_string() && k["/1"_json_pointer] == "n") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Notice " << k << endl;
            else if (k["/0"_json_pointer].is_number() && chan.find(k["/0"_json_pointer]) != chan.end()) {
              if (chan[k["/0"_json_pointer]] == "book") {
                if (k["/1/0"_json_pointer].is_number()) k["/1"_json_pointer] = { k["/1"_json_pointer] };
                if (k["/1/0"_json_pointer].is_array())
                  for (json::iterator it = k["/1"_json_pointer].begin(); it != k["/1"_json_pointer].end(); ++it) {
                    double p = (*it)["/0"_json_pointer].get<double>();
                    bool c = (*it)["/1"_json_pointer].get<double>() > 0;
                    bool s = (*it)["/2"_json_pointer].get<double>() > 0;
                    double a = abs((*it)["/2"_json_pointer].get<double>());
                    if (c) {
                      if (s) b_.push_back(mGWbl(p, a));
                      else a_.push_back(mGWbl(p, a));
                    } else {
                      if (s) {
                        for (vector<mGWbl>::iterator it_ = b_.begin(); it_ != b_.end();)
                          if ((*it_).price == p) it_ = b_.erase(it_); else ++it_;
                      } else for (vector<mGWbl>::iterator it_ = a_.begin(); it_ != a_.end();)
                        if ((*it_).price == p) it_ = a_.erase(it_); else ++it_;
                    }
                  }
                sort(b_.begin(), b_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price*-1 < _b_.price*-1; });
                sort(a_.begin(), a_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price < _b_.price; });
                if (b_.size()>21) b_.resize(21, mGWbl(0, 0));
                if (a_.size()>21) a_.resize(21, mGWbl(0, 0));
                vector<mGWbl> _b_;
                vector<mGWbl> _a_;
                for (vector<mGWbl>::iterator it = b_.begin(); it != b_.end(); ++it)
                  if (_b_.size() < 13) _b_.push_back(*it); else break;
                for (vector<mGWbl>::iterator it = a_.begin(); it != a_.end(); ++it)
                  if (_a_.size() < 13) _a_.push_back(*it); else break;
                if (_a_.size() && _b_.size())
                  GW::gwLevelUp(mGWbls(_b_, _a_));
              } else if (chan[k["/0"_json_pointer]] == "trades") {
                if (k["/1"_json_pointer].is_string() && k["/1"_json_pointer].get<string>() == "te")
                  k["/1"_json_pointer] = { k["/2"_json_pointer] };
                if (k["/1"_json_pointer].is_array()) {
                  if (k["/1/0"_json_pointer].is_number())
                    k["/1"_json_pointer] = { k["/1"_json_pointer] };
                  if (k["/1/0"_json_pointer].is_array())
                    for (json::iterator it = k["/1"_json_pointer].begin(); it != k["/1"_json_pointer].end(); ++it)
                      GW::gwTradeUp(mGWbt(
                        (*it)["/3"_json_pointer],
                        abs((*it)["/2"_json_pointer].get<double>()),
                        (*it)["/2"_json_pointer].get<double>() > 0 ? mSide::Bid : mSide::Ask
                      ));
                }
              }
            }
          }
        });
        hub.connect(ws, nullptr);
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {}
      void cancel(string oI, mSide oS, unsigned long oT) {}
      void cancelAll() {}
      string clientId() { return ""; }
  };
  class GwKorbit: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      unsigned long token_time_ = 0;
      string token_ = "";
      string token_refresh_ = "";
      void fetch() {
        exchange = mExchange::Korbit;
        symbol = FN::S2l(string(mCurrency[base]).append("_").append(mCurrency[quote]));
        target = CF::cfString("KorbitOrderDestination");
        apikey = CF::cfString("KorbitApiKey");
        secret = CF::cfString("KorbitSecretKey");
        user = CF::cfString("KorbitUsername");
        pass = CF::cfString("KorbitPassword");
        http = CF::cfString("KorbitHttpUrl");
        json k = FN::wJet(string(http).append("/constants"));
        if (k.find(symbol.substr(0,3).append("TickSize")) != k.end()) {
          minTick = k[symbol.substr(0,3).append("TickSize")];
          minSize = 0.015;
        }
      };
      void pos() {
        refresh();
        json k = FN::wJet(string(http).append("/user/wallet?currency_pair=").append(symbol), token_, true);
        map<string, vector<double>> wallet;
        if (k.find("available") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl; return; }
        for (json::iterator it = k["available"].begin(); it != k["available"].end(); ++it) wallet[(*it)["currency"].get<string>()].push_back(stod((*it)["value"].get<string>()));
        for (json::iterator it = k["pendingOrders"].begin(); it != k["pendingOrders"].end(); ++it) wallet[(*it)["currency"].get<string>()].push_back(stod((*it)["value"].get<string>()));
        for (map<string, vector<double>>::iterator it=wallet.begin(); it!=wallet.end(); ++it) {
          double amount; double held;
          for (unsigned i=0; i<it->second.size(); ++i) if (i==0) amount = it->second[i]; else if (i==1) held = it->second[i];
          GW::gwPosUp(amount, held, FN::S2mC(it->first));
        }
      };
      void book() {
        GW::gwBookUp(mConnectivityStatus::Connected);
        if (uv_timer_init(uv_default_loop(), &gwBook_)) { cout << FN::uiT() << "Errrror: GW gwBook_ init timer failed." << endl; exit(1); }
        gwBook_.data = this;
        if (uv_timer_start(&gwBook_, [](uv_timer_t *handle) {
          GwKorbit* gw = (GwKorbit*) handle->data;
          GW::gwLevelUp(gw->getLevels());
        }, 0, 2222)) { cout << FN::uiT() << "Errrror: GW gwBook_ start timer failed." << endl; exit(1); }
        if (uv_timer_init(uv_default_loop(), &gwBookTrade_)) { cout << FN::uiT() << "Errrror: GW gwBookTrade_ init timer failed." << endl; exit(1); }
        gwBookTrade_.data = this;
        if (uv_timer_start(&gwBookTrade_, [](uv_timer_t *handle) {
          GwKorbit* gw = (GwKorbit*) handle->data;
          GW::gwTradeUp(gw->getTrades());
        }, 0, 60000)) { cout << FN::uiT() << "Errrror: GW gwBookTrade_ start timer failed." << endl; exit(1); }
      };
      void refresh() {
        if (!token_time_) {
          json k = FN::wJet(string(http).append("/oauth2/access_token"), string("client_id=").append(apikey).append("&client_secret=").append(secret).append("&username=").append(user).append("&password=").append(pass).append("&grant_type=password"), token_, "", false, true);
          if (k.find("expires_in") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Authentication failed, got no response." << endl; return; }
          token_time_ = (k["expires_in"].get<int>() * 1000) + FN::T();
          token_ = k["access_token"];
          token_refresh_ = k["refresh_token"];
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Authentication successful, new token expires at " << to_string(token_time_) << "." << endl;
        } else if (FN::T()+60 > token_time_) {
          json k = FN::wJet(string(http).append("/oauth2/access_token"), string("client_id=").append(apikey).append("&client_secret=").append(secret).append("&refresh_token=").append(token_refresh_).append("&grant_type=refresh_token"), token_, "", false, true);
          if (k.find("expires_in") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Authentication failed, got no response." << endl; return; }
          token_time_ = (k["expires_in"].get<int>() * 1000) + FN::T();
          token_ = k["access_token"];
          token_refresh_ = k["refresh_token"];
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Authentication refresh successful, new token expires at " << to_string(token_time_) << "." << endl;
        }
      };
      mGWbls getLevels() {
        vector<mGWbl> a;
        vector<mGWbl> b;
        json k = FN::wJet(string(http).append("/orderbook?category=all&currency_pair=").append(symbol));
        if (k.find("timestamp") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read book levels." << endl; return mGWbls(b, a); }
        for (json::iterator it = k["bids"].begin(); it != k["bids"].end(); ++it) {
          b.push_back(mGWbl(
            stod((*it)["/0"_json_pointer].get<string>()),
            stod((*it)["/1"_json_pointer].get<string>())
          ));
          if (b.size() == 13) break;
        }
        for (json::iterator it = k["asks"].begin(); it != k["asks"].end(); ++it) {
          a.push_back(mGWbl(
            stod((*it)["/0"_json_pointer].get<string>()),
            stod((*it)["/1"_json_pointer].get<string>())
          ));
          if (a.size() == 13) break;
        }
        return mGWbls(b, a);
      };
      vector<mGWbt> getTrades() {
        vector<mGWbt> v;
        json k = FN::wJet(string(http).append("/transactions?time=minute&currency_pair=").append(symbol));
        for (json::iterator it = k.begin(); it != k.end(); ++it)
          v.push_back(mGWbt(
            stod((*it)["price"].get<string>()),
            stod((*it)["amount"].get<string>()),
            mSide::Ask
          ));
        return v;
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {}
      void cancel(string oI, mSide oS, unsigned long oT) {}
      void cancelAll() {}
      string clientId() { return ""; }
  };
  class GwHitBtc: public Gw {
    public:
      bool cancelByClientId = true;
      bool supportCancelAll = false;
      bool snap = false;
      vector<mGWbl> a_;
      vector<mGWbl> b_;
      unsigned int seq = 0;
      uWS::WebSocket<uWS::CLIENT> *ows;
      void fetch() {
        exchange = mExchange::HitBtc;
        symbol = string(mCurrency[base]).append(mCurrency[quote]);
        target = CF::cfString("HitBtcOrderDestination");
        apikey = CF::cfString("HitBtcApiKey");
        secret = CF::cfString("HitBtcSecret");
        http = CF::cfString("HitBtcPullUrl");
        ws = CF::cfString("HitBtcOrderEntryUrl");
        wS = CF::cfString("HitBtcMarketDataUrl");
        json k = FN::wJet(string(http).append("/api/1/public/symbols"));
        if (k.find("symbols") != k.end())
          for (json::iterator it = k["symbols"].begin(); it != k["symbols"].end(); ++it)
            if ((*it)["symbol"] == symbol) {
              minTick = stod((*it)["step"].get<string>());
              minSize = stod((*it)["lot"].get<string>());
              break;
            }
      };
      void pos() {
        if (http.find("demo") == string::npos) {
          string p = string("apikey=").append(apikey).append("&nonce=").append(to_string(++seq));
          json k = FN::wJet(string(http).append("/api/1/trading/balance?").append(p), p, FN::oHmac512(string("/api/1/trading/balance?").append(p), secret));
          if (k.find("balance") == k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl;
          else for (json::iterator it = k["balance"].begin(); it != k["balance"].end(); ++it)
            if ((*it)["currency_code"] == mCurrency[base] || (*it)["currency_code"] == mCurrency[quote])
              GW::gwPosUp((*it)["cash"], (*it)["reserved"], FN::S2mC((*it)["currency_code"]));
        } else {
          GW::gwPosUp(500, 50, base);
          GW::gwPosUp(500, 50, quote);
        }
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          ows = w;
          long u = (long)w->getUserData();
          if (u == 1) {
            GW::gwBookUp(mConnectivityStatus::Connected);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming book levels." << endl;
          } else if (u == 2) {
            GW::gwOrderUp(mConnectivityStatus::Connected);
            string p_;
            string p = string("{\"nonce\":").append(to_string(++seq)).append(",\"payload\":{\"Login\":{}}}");
            B64::Encode(FN::oHex(FN::oHmac512(p, secret)), &p_);
            string m = string("{\"apikey\":\"").append(apikey).append("\",\"signature\":\"").append(p_).append("\",\"message\":").append(p).append("}");
            w->send(m.data(), m.length(), uWS::OpCode::TEXT);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming orders." << endl;
          }
        });
        hub.onDisconnection([](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) {
          long u = (long)w->getUserData();
          if (u == 1) GW::gwBookUp(mConnectivityStatus::Disconnected);
          else if (u == 2) GW::gwOrderUp(mConnectivityStatus::Disconnected);
        });
        hub.onError([&](void *u) {
          if ((long)u == 1) {
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error while reading book levels, reconnecting.." << endl;
            hub.connect(wS, (void*)1);
          } else if ((long)u == 2) {
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error while reading orders, reconnecting.." << endl;
            hub.connect(wS, (void*)2);
          }
        });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          if (!length || message[0] != '{') { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Error " << string(message, length) << endl; return; }
          long u = (long) w->getUserData();
          json k = json::parse(string(message, length));
          if (u == 1) {
            if (k["MarketDataIncrementalRefresh"].is_object()) {
              k = k["MarketDataIncrementalRefresh"];
              if (!snap || symbol != k["symbol"]) return;
              for (json::iterator it = k["bid"].begin(); it != k["bid"].end(); ++it) {
                double p = stod((*it)["price"].get<string>());
                double s = (*it)["size"].get<double>();
                if (s) b_.push_back(mGWbl(p, s * minSize));
                else for (vector<mGWbl>::iterator it_ = b_.begin(); it_ != b_.end();)
                  if ((*it_).price == p) it_ = b_.erase(it_); else ++it_;
              }
              for (json::iterator it = k["ask"].begin(); it != k["ask"].end(); ++it) {
                double p = stod((*it)["price"].get<string>());
                double s = (*it)["size"].get<double>();
                if (s) a_.push_back(mGWbl(p, s * minSize));
                else for (vector<mGWbl>::iterator it_ = a_.begin(); it_ != a_.end();)
                  if ((*it_).price == p) it_ = a_.erase(it_); else ++it_;
              }
              sort(b_.begin(), b_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price*-1 < _b_.price*-1; });
              sort(a_.begin(), a_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price < _b_.price; });
            }
            else if (k["MarketDataSnapshotFullRefresh"].is_object()) {
              k = k["MarketDataSnapshotFullRefresh"];
              if (symbol != k["symbol"]) return;
              snap = true;
              b_.clear();
              a_.clear();
              for (json::iterator it = k["bid"].begin(); it != k["bid"].end(); ++it) {
                b_.push_back(mGWbl(
                  stod((*it)["price"].get<string>()),
                  (*it)["size"].get<double>() * minSize
                ));
                if (b_.size() == 13) break;
              }
              for (json::iterator it = k["ask"].begin(); it != k["ask"].end(); ++it) {
                a_.push_back(mGWbl(
                  stod((*it)["price"].get<string>()),
                  (*it)["size"].get<double>() * minSize
                ));
                if (a_.size() == 13) break;
              }
            }
            if (b_.size()>21) b_.resize(21, mGWbl(0, 0));
            if (a_.size()>21) a_.resize(21, mGWbl(0, 0));
            vector<mGWbl> _b_;
            vector<mGWbl> _a_;
            for (vector<mGWbl>::iterator it = b_.begin(); it != b_.end(); ++it)
              if (_b_.size() < 13) _b_.push_back(*it); else break;
            for (vector<mGWbl>::iterator it = a_.begin(); it != a_.end(); ++it)
              if (_a_.size() < 13) _a_.push_back(*it); else break;
            if (_b_.size() && _a_.size())
              GW::gwLevelUp(mGWbls(_b_, _a_));
          } else if (u == 2) {
            if (k.find("error") != k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Error " << k["error"] << endl;
            else orderUp(k);
          }
        });
        hub.connect(wS, (void*)1);
        hub.connect(ws, (void*)2);
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {
        string p_;
        string p = string("{\"nonce\":").append(to_string(++seq)).append(",\"payload\":{\"NewOrder\":{\"clientOrderId\":\"").append(oI).append("\",\"symbol\":\"").append(symbol).append("\",\"side\":\"").append(oS == mSide::Bid ? "buy" : "sell").append("\",\"quantity\":\"").append(to_string(oQ / minSize)).append("\",\"type\":\"").append(mOrderType::Limit == oLM ? "limit" : "market").append("\",\"price\":\"").append(to_string(oP)).append("\",\"timeInForce\":\"").append(oTIF == mTimeInForce::FOK ? "FOK" : (oTIF == mTimeInForce::GTC ? "GTC" : "IOC")).append("\"}}}");
        B64::Encode(FN::oHex(FN::oHmac512(p, secret)), &p_);
        string m = string("{\"apikey\":\"").append(apikey).append("\",\"signature\":\"").append(p_).append("\",\"message\":").append(p).append("}");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancel(string oI, mSide oS, unsigned long oT) {
        string p_;
        string p = string("{\"nonce\":").append(to_string(++seq)).append(",\"payload\":{\"OrderCancel\":{\"clientOrderId\":\"").append(oI).append("\",\"cancelRequestClientOrderId\":\"").append(oI).append("C\",\"symbol\":\"").append(symbol).append("\",\"side\":\"").append(oS == mSide::Bid ? "buy" : "sell").append("\"}}}");
        B64::Encode(FN::oHex(FN::oHmac512(p, secret)), &p_);
        string m = string("{\"apikey\":\"").append(apikey).append("\",\"signature\":\"").append(p_).append("\",\"message\":").append(p).append("}");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancelAll() {
        string p = string("apikey=").append(apikey).append("&nonce=").append(to_string(++seq));
        orderUp(FN::wJet(string(http).append("/api/1/trading/cancel_orders?").append(p), string("symbols=").append(symbol), FN::oHmac512(string("/api/1/trading/cancel_orders?").append(p).append("symbols=").append(symbol), secret), true));
      }
      string clientId() {
        srand(time(0));
        char s[15];
        for (int i = 0; i < 15; ++i) s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        return string(s, 15).append(to_string(++seq));
      }
      void orderUp(json k) {
        if (k.find("ExecutionReport") != k.end()) {
          k = k["ExecutionReport"];
          if (!k.is_array()) k = { k };
          for (json::iterator it = k.begin(); it != k.end(); ++it) {
            if ((*it)["lastQuantity"].get<double>() > 0 && (*it)["execReportType"] == "trade") {
              (*it)["lastQuantity"] = (*it)["lastQuantity"].get<double>() * minSize;
            } else { (*it)["lastPrice"] = "0"; (*it)["lastQuantity"] = 0; }
            (*it)["cumQuantity"] = (*it)["cumQuantity"].is_number() ? (*it)["cumQuantity"].get<double>() * minSize : 0;
            (*it)["averagePrice"] = (*it)["averagePrice"].is_string() ? (*it)["averagePrice"].get<string>() : "0";
            GW::gwOrderUp((*it)["clientOrderId"].get<string>(), (*it)["orderId"].get<string>() != "N/A" ? (*it)["orderId"].get<string>() : (*it)["clientOrderId"].get<string>(), getOS(*it), stod((*it)["lastPrice"].get<string>() == "" ? "0" : (*it)["lastPrice"].get<string>()), (*it)["lastQuantity"], (*it)["cumQuantity"], stod((*it)["averagePrice"].get<string>()));
           }
        } else if (k.find("CancelReject") != k.end())
          GW::gwOrderUp(k["CancelReject"]["clientOrderId"], mORS::Cancelled);
      }
      mORS getOS(json k) {
        string k_ = k["execReportType"];
        if (k_ == "new" || k_ == "status") return mORS::Working;
        else if (k_ == "canceled" || k_ == "expired" || k_ == "rejected") return mORS::Cancelled;
        else if (k_ == "trade") return k["orderStatus"] == "filled" ? mORS::Complete : mORS::Working;
        else return mORS::Cancelled;
      }
  };
  class GwPoloniex: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      void fetch() {
        exchange = mExchange::Poloniex;
        symbol = string(mCurrency[quote]).append("_").append(mCurrency[base]);
        string target = CF::cfString("PoloniexOrderDestination");
        apikey = CF::cfString("PoloniexApiKey");
        secret = CF::cfString("PoloniexSecretKey");
        http = CF::cfString("PoloniexHttpUrl");
        ws = CF::cfString("PoloniexWebsocketUrl");
        json k = FN::wJet(string(http).append("/public?command=returnTicker"));
        if (k.find(symbol) != k.end()) {
          istringstream os(string("1e-").append(to_string(6-k[symbol]["last"].get<string>().find("."))));
          os >> minTick;
          minSize = 0.01;
        }
      };
      void pos() {
        string p = string("command=returnCompleteBalances&nonce=").append(to_string(FN::T()));
        json k = FN::wJet(string(http).append("/tradingApi"), p, apikey, FN::oHmac512(p, secret));
        if (k.find(mCurrency[base]) == k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl;
        else for (json::iterator it = k.begin(); it != k.end(); ++it)
          if (it.key() == mCurrency[base] || it.key() == mCurrency[quote])
            GW::gwPosUp(stod(k[it.key()]["available"].get<string>()), stod(k[it.key()]["onOrders"].get<string>()), FN::S2mC(it.key()));
      };
      void book() {
        GW::gwBookUp(mConnectivityStatus::Connected);
        if (uv_timer_init(uv_default_loop(), &gwBook_)) { cout << FN::uiT() << "Errrror: GW gwBook_ init timer failed." << endl; exit(1); }
        gwBook_.data = this;
        if (uv_timer_start(&gwBook_, [](uv_timer_t *handle) {
          GwPoloniex* gw = (GwPoloniex*) handle->data;
          GW::gwLevelUp(gw->getLevels());
        }, 0, 2222)) { cout << FN::uiT() << "Errrror: GW gwBook_ start timer failed." << endl; exit(1); }
        if (uv_timer_init(uv_default_loop(), &gwBookTrade_)) { cout << FN::uiT() << "Errrror: GW gwBookTrade_ init timer failed." << endl; exit(1); }
        gwBookTrade_.data = this;
        if (uv_timer_start(&gwBookTrade_, [](uv_timer_t *handle) {
          GwPoloniex* gw = (GwPoloniex*) handle->data;
          GW::gwTradeUp(gw->getTrades());
        }, 0, 60000)) { cout << FN::uiT() << "Errrror: GW gwBookTrade_ start timer failed." << endl; exit(1); }
      };
      mGWbls getLevels() {
        vector<mGWbl> a;
        vector<mGWbl> b;
        json k = FN::wJet(string(http).append("/public?command=returnOrderBook&depth=13&currencyPair=").append(symbol));
        if (k.find("error") != k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["error"] << endl; return mGWbls(b, a); }
        if (k.find("seq") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read book levels." << endl; return mGWbls(b, a); }
        for (json::iterator it = k["bids"].begin(); it != k["bids"].end(); ++it) {
          b.push_back(mGWbl(
            stod((*it)["/0"_json_pointer].get<string>()),
            (*it)["/1"_json_pointer].get<double>()
          ));
          if (b.size() == 13) break;
        }
        for (json::iterator it = k["asks"].begin(); it != k["asks"].end(); ++it) {
          a.push_back(mGWbl(
            stod((*it)["/0"_json_pointer].get<string>()),
            (*it)["/1"_json_pointer].get<double>()
          ));
          if (a.size() == 13) break;
        }
        return mGWbls(b, a);
      };
      vector<mGWbt> getTrades() {
        vector<mGWbt> v;
        unsigned long t = FN::T();
        json k = FN::wJet(string(http).append("/public?command=returnTradeHistory&currencyPair=").append(symbol).append("&start=").append(to_string((int)((t-60000)/1000))).append("&end=").append(to_string(t)));
        if (k.find("error") != k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["error"] << endl; return v; }
        for (json::iterator it = k.begin(); it != k.end(); ++it)
          v.push_back(mGWbt(
            stod((*it)["rate"].get<string>()),
            stod((*it)["amount"].get<string>()),
            (*it)["type"].get<string>() == "buy" ? mSide::Bid : mSide::Ask
          ));
        return v;
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, unsigned long oT) {}
      void cancel(string oI, mSide oS, unsigned long oT) {}
      void cancelAll() {}
      string clientId() { return ""; }
  };
  Gw *Gw::E(mExchange e) {
    if (e == mExchange::Null) return new GwNull;
    else if (e == mExchange::OkCoin) return new GwOkCoin;
    else if (e == mExchange::Coinbase) return new GwCoinbase;
    else if (e == mExchange::Bitfinex) return new GwBitfinex;
    else if (e == mExchange::Korbit) return new GwKorbit;
    else if (e == mExchange::HitBtc) return new GwHitBtc;
    else if (e == mExchange::Poloniex) return new GwPoloniex;
    cout << FN::uiT() << "Errrror: No gateway provided for exchange \"" << CF::cfString("EXCHANGE") << "\"." << endl;
    exit(1);
  };
}

#endif
