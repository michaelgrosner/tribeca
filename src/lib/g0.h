#ifndef K_G0_H_
#define K_G0_H_

namespace K {
  class GwNull: public Gw {
    public:
      bool cancelByClientId = true;
      bool supportCancelAll = false;
      void config() {
        symbol = string(mCurrency[base]).append("_").append(mCurrency[quote]);
        minTick = 0.01;
        minSize = 0.01;
      };
      void pos() {
        GW::gwPosUp(mGWp(5, 0, base));
        GW::gwPosUp(mGWp(5000, 0, quote));
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
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {
        if (oTIF == mTimeInForce::IOC) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " doesn't work with IOC." << endl; exit(1); }
        GW::gwOrderUp(mGWos(oI, oI, mORS::Working));
        srand(time(0));
        if (rand() % 5) GW::gwOrderUp(mGWol(oI, mORS::Complete, oP, oQ, rand() % 4 ? mLiquidity::Make : mLiquidity::Take));
      }
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {
        GW::gwOrderUp(mGWos(oI, oE, mORS::Cancelled));
      }
      void cancelAll() {}
      string clientId() { string t = to_string(FN::T()); return t.size()>9?t.substr(t.size()-9):t; }
  };
}

#endif
