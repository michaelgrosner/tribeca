#ifndef K_G6_H_
#define K_G6_H_

namespace K {
  class GwPoloniex: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      void config() {
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
            GW::gwPosUp(mGWp(stod(k[it.key()]["available"].get<string>()), stod(k[it.key()]["onOrders"].get<string>()), FN::S2mC(it.key())));
      };
      void book() {
        GW::gwBookUp(mConnectivityStatus::Connected);
        GW::gwOrderUp(mConnectivityStatus::Connected);
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &gwBook_)) { cout << FN::uiT() << "Errrror: GW gwBook_ init timer failed." << endl; exit(1); }
          gwBook_.data = this;
          if (uv_timer_start(&gwBook_, [](uv_timer_t *handle) {
            GwPoloniex* gw = (GwPoloniex*) handle->data;
            GW::gwLevelUp(gw->getLevels());
          }, 0, 5222)) { cout << FN::uiT() << "Errrror: GW gwBook_ start timer failed." << endl; exit(1); }
          if (uv_timer_init(uv_default_loop(), &gwBookTrade_)) { cout << FN::uiT() << "Errrror: GW gwBookTrade_ init timer failed." << endl; exit(1); }
          gwBookTrade_.data = this;
          if (uv_timer_start(&gwBookTrade_, [](uv_timer_t *handle) {
            GwPoloniex* gw = (GwPoloniex*) handle->data;
            GW::gwTradeUp(gw->getTrades());
          }, 0, 60000)) { cout << FN::uiT() << "Errrror: GW gwBookTrade_ start timer failed." << endl; exit(1); }
        }).detach();
      };
      mGWbls getLevels() {
        vector<mGWbl> a;
        vector<mGWbl> b;
        json k = FN::wJet(string(http).append("/public?command=returnOrderBook&depth=13&currencyPair=").append(symbol));
        if (k.find("error") != k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["error"] << endl; return mGWbls(b, a); }
        else if (k.find("seq") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read book levels." << endl; return mGWbls(b, a); }
        if (k.find("bids") != k.end())
          for (json::iterator it = k["bids"].begin(); it != k["bids"].end(); ++it) {
            b.push_back(mGWbl(
              stod((*it)["/0"_json_pointer].get<string>()),
              (*it)["/1"_json_pointer].get<double>()
            ));
            if (b.size() == 13) break;
          }
        if (k.find("asks") != k.end())
          for (json::iterator it = k["asks"].begin(); it != k["asks"].end(); ++it) {
            a.push_back(mGWbl(
              stod((*it)["/0"_json_pointer].get<string>()),
              (*it)["/1"_json_pointer].get<double>()
            ));
            if (a.size() == 13) break;
          }
        if (b.size() && a.size())
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
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {
        string p = string("command=").append(oS == mSide::Bid ? "buy" : "sell").append("&nonce=").append(to_string(FN::T())).append("&currencyPair=").append(symbol).append("&rate=").append(to_string(oP)).append("&amount=").append(to_string(oQ)).append("&fillOrKill=").append(to_string(oTIF == mTimeInForce::FOK ? 1 : 0)).append("&immediateOrCancel=").append(to_string(oTIF == mTimeInForce::IOC ? 1 : 0)).append("&postOnly=").append(to_string(oPO ? 1 : 0));
        json k = FN::wJet(string(http).append("/tradingApi"), p, apikey, FN::oHmac512(p, secret));
        if (k.find("error") != k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Error " << k["error"] << endl;
        else if (k.find("orderNumber") == k.end()) GW::gwOrderUp(mGWos(oI, "", mORS::Cancelled));
        else GW::gwOrderUp(mGWoa(oI, k["orderNumber"], mORS::Working, oP, oQ, (double)0, (double)0, (double)0));
      }
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {
        string p = string("command=cancelOrder").append("&nonce=").append(to_string(FN::T())).append("&orderNumber=").append(oE);
        json k = FN::wJet(string(http).append("/tradingApi"), p, apikey, FN::oHmac512(p, secret));
        if (k.find("success") == k.end() or k["success"] != 1) return;
        GW::gwOrderUp(mGWos(oI, oE, mORS::Cancelled));
      }
      void cancelAll() {
        string p = string("command=returnOpenOrders").append("&nonce=").append(to_string(FN::T())).append("&currencyPair=").append(symbol);
        json k = FN::wJet(string(http).append("/tradingApi"), p, apikey, FN::oHmac512(p, secret));
        if (!k.is_array()) return;
        for (json::iterator it = k.begin(); it != k.end(); ++it)
          cancel("", k["orderNumber"].get<string>(), mSide::Unknown, (unsigned long)0);
      }
      string clientId() { string t = to_string(FN::T()); return t.size()>11?t.substr(t.size()-11):t; }
  };
}

#endif
