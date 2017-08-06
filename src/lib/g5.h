#ifndef K_G5_H_
#define K_G5_H_

namespace K {
  class GwKorbit: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      unsigned long token_time_ = 0;
      string token_ = "";
      string token_refresh_ = "";
      void config() {
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
          GW::gwPosUp(mGWp(amount, held, FN::S2mC(it->first)));
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
        refresh();
        json k = FN::wJet(string(http).append("/user/transactions?category=fills&currency_pair=").append(symbol), token_, true);
        if (k.is_array()) {
          for (json::iterator it = k.begin(); it != k.end(); ++it)
            if ((*it)["type"] == "buy" or (*it)["type"] == "sell")
              GW::gwOrderUp(mGWoS("", (*it)["fillsDetail"]["orderId"], mORS::Complete, stod((*it)["fillsDetail"]["price"]["value"].get<string>()), stod((*it)["fillsDetail"]["amount"]["value"].get<string>()), (*it)["type"] == "buy" ? mSide::Bid : mSide::Ask));
        }
        vector<mGWbt> v;
        k = FN::wJet(string(http).append("/transactions?time=minute&currency_pair=").append(symbol));
        if (k.is_array()) for (json::iterator it = k.begin(); it != k.end(); ++it)
          v.push_back(mGWbt(
            stod((*it)["price"].get<string>()),
            stod((*it)["amount"].get<string>()),
            mSide::Ask
          ));
        return v;
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {
        refresh();
        json k = FN::wJet(string(http).append("/user/orders/").append(oS == mSide::Bid ? "buy" : "sell"), string("currency_pair=").append(symbol).append("&type=").append(oLM == mOrderType::Market ? "market" : "limit").append("&price=").append(to_string(oP)).append("&coin_amount=").append(to_string(oQ)).append("&nonce=").append(to_string(FN::T())), token_, "", false, true);
        if (k.find("orderId") == k.end() or k.find("status") == k.end() or k["status"] != "success") GW::gwOrderUp(mGWos(oI, "", mORS::Cancelled));
        else GW::gwOrderUp(mGWoa(oI, k["orderId"].get<string>(), mORS::Working, oP, oQ, (double)0, (double)0, (double)0));
      }
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {
        refresh();
        json k = FN::wJet(string(http).append("/user/orders/cancel"), string("currency_pair=").append(symbol).append("&nonce=").append(to_string(FN::T())).append("&id=").append(oE), token_, "", false, true);
        if (!k.is_array()) return;
        GW::gwOrderUp(mGWos(oI, "", mORS::Cancelled));
      }
      void cancelAll() {
        refresh();
        json k = FN::wJet(string(http).append("/user/orders/open"), string("currency_pair=").append(symbol), token_, "", false, true);
        if (!k.is_array()) return;
        for (json::iterator it = k.begin(); it != k.end(); ++it)
          cancel("", k["id"].get<string>(), mSide::Unknown, (unsigned long)0);
      }
      string clientId() { string t = to_string(FN::T()); return t.size()>9?t.substr(t.size()-9):t; }
  };
}

#endif
