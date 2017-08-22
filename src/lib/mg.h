#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  static uv_timer_t mgEwmaP_;
  static vector<mGWmt> mGWmt_;
  static json mGWmkt;
  static json mGWmktFilter;
  static double mgfairV = 0;
  static double mgEwmaL = 0;
  static double mgEwmaM = 0;
  static double mgEwmaS = 0;
  static double mgEwmaP = 0;
  static vector<double> mgSMA3;
  class MG {
    public:
      static void main(Local<Object> exports) {
        load();
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &mgEwmaP_)) { cout << FN::uiT() << "Errrror: GW mgEwmaP_ init timer failed." << endl; exit(1); }
          mgEwmaP_.data = gw;
          if (uv_timer_start(&mgEwmaP_, [](uv_timer_t *handle) {
            if (mgfairV) {
              mgEwmaP = calcEwma(mgfairV, mgEwmaP, qpRepo["quotingEwmaProtectionPeridos"].get<int>());
              EV::evUp("EWMAProtectionCalculator");
            } else cout << FN::uiT() << "EWMA notice: missing fair value." << endl;
          }, 0, 60000)) { cout << FN::uiT() << "Errrror: GW mgEwmaP_ start timer failed." << endl; exit(1); }
        }).detach();
        EV::evOn("MarketTradeGateway", [](json k) {
          mGWmt t(
            gw->exchange,
            gw->base,
            gw->quote,
            k["price"].get<double>(),
            k["size"].get<double>(),
            FN::T(),
            (mSide)k["make_side"].get<int>()
          );
          mGWmt_.push_back(t);
          if (mGWmt_.size()>69) mGWmt_.erase(mGWmt_.begin());
          EV::evUp("MarketTrade");
          UI::uiSend(uiTXT::MarketTrade, tradeUp(t));
        });
        EV::evOn("MarketDataGateway", [](json o) {
          mktUp(o);
        });
        EV::evOn("GatewayMarketConnect", [](json k) {
          if ((mConnectivityStatus)k["/0"_json_pointer].get<int>() == mConnectivityStatus::Disconnected)
            mktUp({});
        });
        EV::evOn("QuotingParameters", [](json k) {
          fairV();
        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        NODE_SET_METHOD(exports, "mgFilter", MG::_mgFilter);
        NODE_SET_METHOD(exports, "mgFairV", MG::_mgFairV);
        NODE_SET_METHOD(exports, "mgEwmaLong", MG::_mgEwmaLong);
        NODE_SET_METHOD(exports, "mgEwmaMedium", MG::_mgEwmaMedium);
        NODE_SET_METHOD(exports, "mgEwmaShort", MG::_mgEwmaShort);
        NODE_SET_METHOD(exports, "mgTBP", MG::_mgTBP);
        NODE_SET_METHOD(exports, "mgEwmaProtection", MG::_mgEwmaProtection);
      };
    private:
      static void load() {
        json k = DB::load(uiTXT::EWMAChart);
        if (k.size()) {
          if (k["/0/fairValue"_json_pointer].is_number())
            mgfairV = k["/0/fairValue"_json_pointer].get<double>();
          if (k["/0/ewmaLong"_json_pointer].is_number())
            mgEwmaL = k["/0/ewmaLong"_json_pointer].get<double>();
          if (k["/0/ewmaMedium"_json_pointer].is_number())
            mgEwmaM = k["/0/ewmaMedium"_json_pointer].get<double>();
          if (k["/0/ewmaShort"_json_pointer].is_number())
            mgEwmaS = k["/0/ewmaShort"_json_pointer].get<double>();
        }
      };
      static void _mgEwmaProtection(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgEwmaP));
      };
      static void _mgEwmaLong(const FunctionCallbackInfo<Value>& args) {
        mgEwmaL = calcEwma(args[0]->NumberValue(), mgEwmaL, qpRepo["longEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgEwmaL));
      };
      static void _mgEwmaMedium(const FunctionCallbackInfo<Value>& args) {
        mgEwmaM = calcEwma(args[0]->NumberValue(), mgEwmaM, qpRepo["mediumEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgEwmaM));
      };
      static void _mgEwmaShort(const FunctionCallbackInfo<Value>& args) {
        mgEwmaS = calcEwma(args[0]->NumberValue(), mgEwmaL, qpRepo["shortEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgEwmaS));
      };
      static void _mgTBP(const FunctionCallbackInfo<Value>& args) {
        mgSMA3.push_back(args[0]->NumberValue());
        double newLong = args[1]->NumberValue();
        double newMedium = args[2]->NumberValue();
        double newShort = args[3]->NumberValue();
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
          SMA3 += *it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / newLong) - 100);
          double newEwmacrossing = ((newShort * 100 / newMedium) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        } else if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((newShort * 100/ newLong) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), newTargetPosition));
      };
      static void _mgFilter(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, mGWmktFilter.dump())).ToLocalChecked());
      };
      static void _mgFairV(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgfairV));
      };
      static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
          k.push_back(tradeUp(mGWmt_[i]));
        return k;
      };
      static json onSnapFair(json z) {
        return {{{"price", mgfairV}}};
      };
      static void mktUp(json k) {
        mGWmkt = k;
        filter();
        UI::uiSend(uiTXT::MarketData, k, true);
      };
      static json tradeUp(mGWmt t) {
        json o = {
          {"exchange", (int)t.exchange},
          {"pair", {{"base", t.base}, {"quote", t.quote}}},
          {"price", t.price},
          {"size", t.size},
          {"time", t.time},
          {"make_size", (int)t.make_side}
        };
        return o;
      };
      static void filter() {
        mGWmktFilter = mGWmkt;
        if (mGWmktFilter.is_null() or mGWmktFilter["bids"].is_null() or mGWmktFilter["asks"].is_null()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          filter(mSide::Bid == (mSide)it->second["side"].get<int>() ? "bids" : "asks", it->second);
        if (!mGWmktFilter["bids"].is_null() and !mGWmktFilter["asks"].is_null()) {
          fairV();
          EV::evUp("FilteredMarket");
        }
      };
      static void filter(string k, json o) {
        for (json::iterator it = mGWmktFilter[k].begin(); it != mGWmktFilter[k].end();)
          if (abs((*it)["price"].get<double>() - o["price"].get<double>()) < gw->minTick) {
            (*it)["size"] = (*it)["size"].get<double>() - o["quantity"].get<double>();
            if ((*it)["size"].get<double>() < gw->minTick) mGWmktFilter[k].erase(it);
            break;
          } else ++it;
      };
      static void fairV() {
        if (mGWmktFilter.is_null() or mGWmktFilter["/bids/0"_json_pointer].is_null() or mGWmktFilter["/asks/0"_json_pointer].is_null()) return;
        double mgfairV_ = mgfairV;
        mgfairV = SD::roundNearest(
          mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
            ? (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>()) / 2
            : (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() * mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>() * mGWmktFilter["/bids/0/size"_json_pointer].get<double>()) / (mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/size"_json_pointer].get<double>()),
          gw->minTick
        );
        if (!mgfairV or (mgfairV_ and abs(mgfairV - mgfairV_) < gw->minTick)) return;
        EV::evUp("FairValue");
        UI::uiSend(uiTXT::FairValue, {{"price", mgfairV}}, true);
      };
      static double calcEwma(double newValue, double previous, int periods) {
        if (previous) {
          double alpha = 2 / (periods + 1);
          return alpha * newValue + (1 - alpha) * previous;
        }
        return newValue;
      };
  };
}

#endif
