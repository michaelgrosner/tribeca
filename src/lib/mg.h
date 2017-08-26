#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  int mgT = 0;
  static vector<mGWmt> mGWmt_;
  static json mGWmkt;
  static json mGWmktFilter;
  static double mgfairV = 0;
  static double mgEwmaL = 0;
  static double mgEwmaM = 0;
  static double mgEwmaS = 0;
  static double mgEwmaP = 0;
  static vector<double> mgSMA3;
  static vector<double> mgStatFV;
  static vector<double> mgStatBid;
  static vector<double> mgStatAsk;
  static vector<double> mgStatTop;
  static double mgStdevFV = 0;
  static double mgStdevFVMean = 0;
  static double mgStdevBid = 0;
  static double mgStdevBidMean = 0;
  static double mgStdevAsk = 0;
  static double mgStdevAskMean = 0;
  static double mgStdevTop = 0;
  static double mgStdevTopMean = 0;
  static double mgTargetPos = 0;
  class MG {
    public:
      static void main(Local<Object> exports) {
        load();
        EV::evOn("MarketTradeGateway", [](json k) {
          tradeUp(k);
        });
        EV::evOn("MarketDataGateway", [](json o) {
          levelUp(o);
        });
        EV::evOn("GatewayMarketConnect", [](json k) {
          if ((mConnectivityStatus)k["/0"_json_pointer].get<int>() == mConnectivityStatus::Disconnected)
            levelUp({});
        });
        EV::evOn("QuotingParameters", [](json k) {
          fairV();
        });
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
        UI::uiSnap(uiTXT::FairValue, &onSnapFair);
        UI::uiSnap(uiTXT::EWMAChart, &onSnapEwma);
        NODE_SET_METHOD(exports, "mgFilter", MG::_mgFilter);
        NODE_SET_METHOD(exports, "mgFairV", MG::_mgFairV);
        NODE_SET_METHOD(exports, "mgEwmaProtection", MG::_mgEwmaProtection);
        NODE_SET_METHOD(exports, "mgStdevProtection", MG::_mgStdevProtection);
      };
      static void calc() {
        if (++mgT == 60) {
          mgT = 0;
          ewmaPUp();
          ewmaUp();
        }
        stdevPUp();
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
        k = DB::load(uiTXT::MarketData);
        if (k.size()) {
          for (json::iterator it = k.begin(); it != k.end(); ++it) {
            mgStatFV.push_back((*it)["fv"].get<double>());
            mgStatBid.push_back((*it)["bid"].get<double>());
            mgStatAsk.push_back((*it)["ask"].get<double>());
            mgStatTop.push_back((*it)["bid"].get<double>());
            mgStatTop.push_back((*it)["ask"].get<double>());
          }
          calcStdev();
        }
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
      static json onSnapEwma(json z) {
        return {{
          {"stdevWidth", {
            {"fv", mgStdevFV},
            {"fvMean", mgStdevFVMean},
            {"tops", mgStdevTop},
            {"topsMean", mgStdevTopMean},
            {"bid", mgStdevBid},
            {"bidMean", mgStdevBidMean},
            {"ask", mgStdevAsk},
            {"askMean", mgStdevAskMean}
          }},
          {"ewmaQuote", mgEwmaP},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"fairValue", mgfairV}
        }};
      };
      static void stdevPUp() {
        if (empty()) return;
        mgStatFV.push_back(mgfairV);
        mgStatBid.push_back(mGWmktFilter["/bids/0/price"_json_pointer].get<double>());
        mgStatAsk.push_back(mGWmktFilter["/asks/0/price"_json_pointer].get<double>());
        mgStatTop.push_back(mGWmktFilter["/bids/0/price"_json_pointer].get<double>());
        mgStatTop.push_back(mGWmktFilter["/asks/0/price"_json_pointer].get<double>());
        calcStdev();
        DB::insert(uiTXT::MarketData, {
          {"fv", mgfairV},
          {"bid", mGWmktFilter["/bids/0/price"_json_pointer].get<double>()},
          {"ask", mGWmktFilter["/bids/0/price"_json_pointer].get<double>()},
          {"time", FN::T()},
        }, false, "NULL", FN::T() - 1000 * qpRepo["quotingStdevProtectionPeriods"].get<int>());
      };
      static void tradeUp(json k) {
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
      };
      static void levelUp(json k) {
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
      static void ewmaUp() {
        calcEwma(&mgEwmaL, qpRepo["longEwmaPeridos"].get<int>());
        calcEwma(&mgEwmaM, qpRepo["mediumEwmaPeridos"].get<int>());
        calcEwma(&mgEwmaS, qpRepo["shortEwmaPeridos"].get<int>());
        calcTargetPos();
        EV::evUp("PositionBroker");
        UI::uiSend(uiTXT::EWMAChart, {
          {"stdevWidth", {
            {"fv", mgStdevFV},
            {"fvMean", mgStdevFVMean},
            {"tops", mgStdevTop},
            {"topsMean", mgStdevTopMean},
            {"bid", mgStdevBid},
            {"bidMean", mgStdevBidMean},
            {"ask", mgStdevAsk},
            {"askMean", mgStdevAskMean}
          }},
          {"ewmaQuote", mgEwmaP},
          {"ewmaShort", mgEwmaS},
          {"ewmaMedium", mgEwmaM},
          {"ewmaLong", mgEwmaL},
          {"fairValue", mgfairV}
        }, true);
        DB::insert(uiTXT::EWMAChart, {
          {"fairValue", mgfairV},
          {"ewmaLong", mgEwmaL},
          {"ewmaMedium", mgEwmaM},
          {"ewmaShort", mgEwmaS}
        });
      };
      static void ewmaPUp() {
        calcEwma(&mgEwmaP, qpRepo["quotingEwmaProtectionPeridos"].get<int>());
        EV::evUp("EWMAProtectionCalculator");
      };
      static bool empty() {
        return (mGWmktFilter.is_null()
          or mGWmktFilter["bids"].is_null()
          or mGWmktFilter["asks"].is_null()
        );
      };
      static void filter() {
        mGWmktFilter = mGWmkt;
        if (empty()) return;
        for (map<string, json>::iterator it = allOrders.begin(); it != allOrders.end(); ++it)
          filter(mSide::Bid == (mSide)it->second["side"].get<int>() ? "bids" : "asks", it->second);
        if (!empty()) {
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
        // if (mGWmktFilter.is_null() or mGWmktFilter["/bids/0"_json_pointer].is_null() or mGWmktFilter["/asks/0"_json_pointer].is_null()) return;
        if (empty()) return;
        double mgfairV_ = mgfairV;
        mgfairV = FN::roundNearest(
          mFairValueModel::BBO == (mFairValueModel)qpRepo["fvModel"].get<int>()
            ? (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>()) / 2
            : (mGWmktFilter["/asks/0/price"_json_pointer].get<double>() * mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/price"_json_pointer].get<double>() * mGWmktFilter["/bids/0/size"_json_pointer].get<double>()) / (mGWmktFilter["/asks/0/size"_json_pointer].get<double>() + mGWmktFilter["/bids/0/size"_json_pointer].get<double>()),
          gw->minTick
        );
        if (!mgfairV or (mgfairV_ and abs(mgfairV - mgfairV_) < gw->minTick)) return;
        EV::evUp("FairValue");
        UI::uiSend(uiTXT::FairValue, {{"price", mgfairV}}, true);
      };
      static void cleanStdev() {
        int periods = qpRepo["quotingStdevProtectionPeriods"].get<int>();
        if (mgStatFV.size()>periods) mgStatFV.erase(mgStatFV.begin(), mgStatFV.end()-periods);
        if (mgStatBid.size()>periods) mgStatBid.erase(mgStatBid.begin(), mgStatBid.end()-periods);
        if (mgStatAsk.size()>periods) mgStatAsk.erase(mgStatAsk.begin(), mgStatAsk.end()-periods);
        if (mgStatTop.size()>periods*2) mgStatTop.erase(mgStatTop.begin(), mgStatTop.end()-(periods*2));
      };
      static void calcStdev() {
        cleanStdev();
        if (mgStatFV.size() < 2 or mgStatBid.size() < 2 or mgStatAsk.size() < 2 or mgStatTop.size() < 4) return;
        double k = qpRepo["quotingStdevProtectionFactor"].get<double>();
        mgStdevFV = calcStdev(mgStatFV, k, &mgStdevFVMean);
        mgStdevBid = calcStdev(mgStatBid, k, &mgStdevBidMean);
        mgStdevAsk = calcStdev(mgStatAsk, k, &mgStdevAskMean);
        mgStdevTop = calcStdev(mgStatTop, k, &mgStdevTopMean);
      };
      static double calcStdev(vector<double> a, double f, double *mean) {
        int n = a.size();
        if (n == 0) return 0.0;
        double sum = 0;
        for (int i = 0; i < n; ++i) sum += a[i];
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (int i = 0; i < n; ++i) {
          double diff = a[i] - *mean;
          sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * f;
      };
      static void calcEwma(double *k, int periods) {
        if (*k) {
          double alpha = (double)2 / (periods + 1);
          *k = alpha * mgfairV + (1 - alpha) * *k;
        } else *k = mgfairV;
      };
      static void calcTargetPos() {
        mgSMA3.push_back(mgfairV);
        if (mgSMA3.size()>3) mgSMA3.erase(mgSMA3.begin(), mgSMA3.end()-3);
        double SMA3 = 0;
        for (vector<double>::iterator it = mgSMA3.begin(); it != mgSMA3.end(); ++it)
          SMA3 += *it;
        SMA3 /= mgSMA3.size();
        double newTargetPosition = 0;
        if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 100 / mgEwmaL) - 100);
          double newEwmacrossing = ((mgEwmaS * 100 / mgEwmaM) - 100);
          newTargetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        } else if ((mAutoPositionMode)qpRepo["autoPositionMode"].get<int>() == mAutoPositionMode::EWMA_LS)
          newTargetPosition = ((mgEwmaS * 100/ mgEwmaL) - 100) * (1 / qpRepo["ewmaSensiblityPercentage"].get<double>());
        if (newTargetPosition > 1) newTargetPosition = 1;
        else if (newTargetPosition < -1) newTargetPosition = -1;
        mgTargetPos = newTargetPosition;
      };
      static void _mgEwmaProtection(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgEwmaP));
      };
      static void _mgStdevProtection(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        HandleScope scope(isolate);
        Local<Object> o = Object::New(isolate);
        o->Set(FN::v8S("fv"), Number::New(isolate, mgStdevFV));
        o->Set(FN::v8S("fvMean"), Number::New(isolate, mgStdevFVMean));
        o->Set(FN::v8S("tops"), Number::New(isolate, mgStdevTop));
        o->Set(FN::v8S("topsMean"), Number::New(isolate, mgStdevTopMean));
        o->Set(FN::v8S("bid"), Number::New(isolate, mgStdevBid));
        o->Set(FN::v8S("bidMean"), Number::New(isolate, mgStdevBidMean));
        o->Set(FN::v8S("ask"), Number::New(isolate, mgStdevAsk));
        o->Set(FN::v8S("askMean"), Number::New(isolate, mgStdevAskMean));
        args.GetReturnValue().Set(o);
      };
      static void _mgFilter(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        JSON Json;
        args.GetReturnValue().Set(Json.Parse(isolate->GetCurrentContext(), FN::v8S(isolate, mGWmktFilter.dump())).ToLocalChecked());
      };
      static void _mgFairV(const FunctionCallbackInfo<Value>& args) {
        args.GetReturnValue().Set(Number::New(args.GetIsolate(), mgfairV));
      };

  };
}

#endif
