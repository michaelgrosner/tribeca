#ifndef K_MG_H_
#define K_MG_H_

namespace K {
  static vector<mGWmt> mGWmt_;
  static json mGWmkt;
  class MG {
    public:
      static void main(Local<Object> exports) {
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
        Isolate* isolate = exports->GetIsolate();
        EV::evOn("MarketDataGateway", [](json o) {
          v8mGWmkt(o);
        });
        EV::evOn("GatewayMarketConnect", [](json k) {
          if ((mConnectivityStatus)k["/0"_json_pointer].get<int>() == mConnectivityStatus::Disconnected)
            v8mGWmkt({});
        });
        UI::uiSnap(uiTXT::MarketData, &onSnapBook);
        UI::uiSnap(uiTXT::MarketTrade, &onSnapTrade);
      };
    private:
      static json onSnapTrade(json z) {
        json k;
        for (unsigned i=0; i<mGWmt_.size(); ++i)
          k.push_back(v8mGWmt(mGWmt_[i]));
        return k;
      };
      static json onSnapBook(json z) {
        return { mGWmkt };
      };
      static void v8mGWmkt(json k) {
        mGWmkt = k;
        EV::evUp("MarketDataBroker", k);
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
  };
}

#endif
