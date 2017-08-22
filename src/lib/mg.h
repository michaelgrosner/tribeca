#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  static vector<mGWmt> mGWmt_;
  static json mGWmkt;
  static json mGWmktFilter;
  static double mGWfairV = 0;
  static double mGWEwmaL = 0;
  static double mGWEwmaM = 0;
  static double mGWEwmaS = 0;
  static vector<double> mGWSMA3;
  class MG {
    public:
      static void main(Local<Object> exports) {
        json fv = DB::load(uiTXT::EWMAChart);
        if (fv.size()) {
          if (fv["/0/fairValue"_json_pointer].is_number())
            mGWfairV = fv["/0/fairValue"_json_pointer].get<double>();
          if (fv["/0/ewmaLong"_json_pointer].is_number())
            mGWEwmaL = fv["/0/ewmaLong"_json_pointer].get<double>();
          if (fv["/0/ewmaMedium"_json_pointer].is_number())
            mGWEwmaM = fv["/0/ewmaMedium"_json_pointer].get<double>();
          if (fv["/0/ewmaShort"_json_pointer].is_number())
            mGWEwmaS = fv["/0/ewmaShort"_json_pointer].get<double>();
        }
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
      };
    private:
      static void _mgEwmaLong(const FunctionCallbackInfo<Value>& args) {
        mGWEwmaL = calcEwma(args[0]->NumberValue(), mGWEwmaL, qpRepo["longEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWEwmaL));
      };
      static void _mgEwmaMedium(const FunctionCallbackInfo<Value>& args) {
        mGWEwmaM = calcEwma(args[0]->NumberValue(), mGWEwmaM, qpRepo["mediumEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWEwmaM));
      };
      static void _mgEwmaShort(const FunctionCallbackInfo<Value>& args) {
        mGWEwmaS = calcEwma(args[0]->NumberValue(), mGWEwmaL, qpRepo["shortEwmaPeridos"].get<int>());
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWEwmaS));
      };
      static void _mgTBP(const FunctionCallbackInfo<Value>& args) {
        mGWSMA3.push_back(args[0]->NumberValue());
        double newLong = args[1]->NumberValue();
        double newMedium = args[2]->NumberValue();
        double newShort = args[3]->NumberValue();
        if (mGWSMA3.size()>3) mGWSMA3.erase(mGWSMA3.begin(), mGWSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mGWSMA3.begin(); it != mGWSMA3.end(); ++it)
          SMA3 += *it;
        SMA3 /= mGWSMA3.size();
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
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWfairV));
      };
      static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
          k.push_back(tradeUp(mGWmt_[i]));
        return k;
      };
      static json onSnapFair(json z) {
        return {{{"price", mGWfairV}}};
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
        double mGWfairV_ = mGWfairV;
        mGWfairV = SD::roundNearest(
          mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
            ? (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>()) / 2
            : (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() * mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>() * mGWmktFilter["/bids/0/size"_json_pointer].get<double>()) / (mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/size"_json_pointer].get<double>()),
          gw->minTick
        );
        if (!mGWfairV or (mGWfairV_ and abs(mGWfairV - mGWfairV_) < gw->minTick)) return;
        EV::evUp("FairValue");
        UI::uiSend(uiTXT::FairValue, {{"price", mGWfairV}}, true);
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
