#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  static vector<mGWmt> mGWmt_;
  static json mGWmkt;
  static json mGWmktF;
  static double mGWfairV = 0;
  class MG {
    public:
      static void main(Local<Object> exports) {
        json fv = DB::load(uiTXT::EWMAChart);
        if (fv.size() and fv["/0/fairValue"_json_pointer].is_number())
          mGWfairV = fv["/0/fairValue"_json_pointer].get<double>();
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
          UI::uiSend(uiTXT::MarketTrade, v8mGWmt(t));
        });
        EV::evOn("MarketDataGateway", [](json o) {
          v8mGWmkt(o);
        });
        EV::evOn("GatewayMarketConnect", [](json k) {
          if ((mConnectivityStatus)k["/0"_json_pointer].get<int>() == mConnectivityStatus::Disconnected)
            v8mGWmkt({});
        });
        EV::evOn("QuotingParameters", [](json k) {
          fairV();
        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        NODE_SET_METHOD(exports, "mgFilter", MG::_mgFilter);
        NODE_SET_METHOD(exports, "mgFairV", MG::_mgFairV);
      };
    private:
      static void _mgFilter(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, mGWmktF.dump())).ToLocalChecked());
      };
      static void _mgFairV(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mGWfairV));
      };
      static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
          k.push_back(v8mGWmt(mGWmt_[i]));
        return k;
      };
      static json onSnapFair(json z) {
        return {{{"price", mGWfairV}}};
      };
      static void v8mGWmkt(json k) {
        mGWmkt = k;
        filter();
        UI::uiSend(uiTXT::MarketData, k, true);
      };
      static json v8mGWmt(mGWmt t) {
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
        mGWmktF = mGWmkt;
        if (mGWmktF.is_null() or mGWmktF["bids"].is_null() or mGWmktF["asks"].is_null()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          filter(mSide::Bid == (mSide)it->second["side"].get<int>() ? "bids" : "asks", it->second);
        if (!mGWmktF["bids"].is_null() and !mGWmktF["asks"].is_null()) {
          fairV();
          EV::evUp("FilteredMarket");
        }
      };
      static void filter(string k, json o) {
        for (json::iterator it = mGWmktF[k].begin(); it != mGWmktF[k].end();)
          if (abs((*it)["price"].get<double>() - o["price"].get<double>()) < gw->minTick) {
            (*it)["size"] = (*it)["size"].get<double>() - o["quantity"].get<double>();
            if ((*it)["size"].get<double>() < gw->minTick) mGWmktF[k].erase(it);
            break;
          } else ++it;
      };
      static void fairV() {
        if (mGWmktF.is_null() or mGWmktF["/bids/0"_json_pointer].is_null() or mGWmktF["/asks/0"_json_pointer].is_null()) return;
        double mGWfairV_ = mGWfairV;
        mGWfairV = SD::roundNearest(
          mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
            ? (mGWmktF["/asks/0/price"_json_pointer].get<double>() + mGWmktF["/bids/0/price"_json_pointer].get<double>()) / 2
            : (mGWmktF["/asks/0/price"_json_pointer].get<double>() * mGWmktF["/asks/0/size"_json_pointer].get<double>() + mGWmktF["/bids/0/price"_json_pointer].get<double>() * mGWmktF["/bids/0/size"_json_pointer].get<double>()) / (mGWmktF["/asks/0/size"_json_pointer].get<double>() + mGWmktF["/bids/0/size"_json_pointer].get<double>()),
          gw->minTick
        );
        if (!mGWfairV or (mGWfairV_ and abs(mGWfairV - mGWfairV_) < gw->minTick)) return;
        EV::evUp("FairValue");
        UI::uiSend(uiTXT::FairValue, {{"price", mGWfairV}}, true);
      };
  };
}

#endif
