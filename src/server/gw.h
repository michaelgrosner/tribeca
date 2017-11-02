#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  static mConnectivity gwAutoStart = mConnectivity::Disconnected,
                       gwQuotingState = mConnectivity::Disconnected,
                       gwConnectOrder = mConnectivity::Disconnected,
                       gwConnectMarket = mConnectivity::Disconnected,
                       gwConnectExchange = mConnectivity::Disconnected;
  class GW: public Klass {
    protected:
      void load() {
        evExit = happyEnding;
        if (argAutobot) gwAutoStart = mConnectivity::Connected;
        gw->hub = &hub;
        gw->gwGroup = hub.createGroup<uWS::CLIENT>();
        gwLoad(gw->config());
        gW = (argTarget == "NULL") ? Gw::E(mExchange::Null) : gw;
      };
      void waitTime() {
        uv_timer_init(hub.getLoop(), &tWallet);
        uv_timer_start(&tWallet, [](uv_timer_t *handle) {
          if (argDebugEvents) FN::log("DEBUG", "EV GW tWallet timer");
          gw->wallet();
        }, 0, 15e+3);
        uv_timer_init(hub.getLoop(), &tCancel);
        uv_timer_start(&tCancel, [](uv_timer_t *handle) {
          if (argDebugEvents) FN::log("DEBUG", "EV GW tCancel timer");
          if (qp.cancelOrdersAuto)
            gW->cancelAll();
        }, 0, 3e+5);
      };
      void waitData() {
        ev_gwConnectOrder = [](mConnectivity k) {
          gwConnUp(mGatewayType::OrderEntry, k);
        };
        ev_gwConnectMarket = [](mConnectivity k) {
          gwConnUp(mGatewayType::MarketData, k);
          if (k == mConnectivity::Disconnected)
            ev_gwDataLevels(mLevels());
        };
        gw->levels();
      };
      void waitUser() {
        UI::uiSnap(uiTXT::ProductAdvertisement, &onSnapProduct);
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
      };
      void run() {
        hub.run();
        EV::end(eCode);
      };
    public:
      static void gwBookUp(mConnectivity k) {
        ev_gwConnectMarket(k);
      };
      static void gwOrderUp(mConnectivity k) {
        ev_gwConnectOrder(k);
      };
      static void gwPosUp(mWallet k) {
        ev_gwDataWallet(k);
      };
      static void gwOrderUp(mOrder k) {
        ev_gwDataOrder(k);
      };
      static void gwTradeUp(mTrade k) {
        ev_gwDataTrade(k);
      };
      static void gwLevelUp(mLevels k) {
        ev_gwDataLevels(k);
      };
    private:
      static json onSnapProduct() {
        return {{
          {"exchange", (int)gw->exchange},
          {"pair", {{"base", gw->base}, {"quote", gw->quote}}},
          {"minTick", gw->minTick},
          {"environment", argTitle},
          {"matryoshka", argMatryoshka},
          {"homepage", "https://github.com/ctubio/Krypto-trading-bot"}
        }};
      };
      static json onSnapStatus() {
        return {{{"status", (int)gwConnectExchange}}};
      };
      static json onSnapState() {
        return {{{"state",  (int)gwQuotingState}}};
      };
      static void onHandState(json k) {
        if (!k.is_object() or !k["state"].is_number()) {
          FN::logWar("JSON", "Missing state at onHandState, ignored");
          return;
        }
        mConnectivity autoStart = (mConnectivity)k["state"].get<int>();
        if (autoStart != gwAutoStart) {
          gwAutoStart = autoStart;
          gwStateUp();
        }
      };
      static void gwConnUp(mGatewayType gwT, mConnectivity gwS) {
        if (gwT == mGatewayType::MarketData) {
          if (gwConnectMarket == gwS) return;
          gwConnectMarket = gwS;
        } else if (gwT == mGatewayType::OrderEntry) {
          if (gwConnectOrder == gwS) return;
          gwConnectOrder = gwS;
        }
        gwConnectExchange = gwConnectMarket == mConnectivity::Connected and gwConnectOrder == mConnectivity::Connected
          ? mConnectivity::Connected : mConnectivity::Disconnected;
        gwStateUp();
        UI::uiSend(uiTXT::ExchangeConnectivity, {{"status", (int)gwConnectExchange}});
      };
      static void gwStateUp() {
        mConnectivity quotingState = gwConnectExchange;
        if (quotingState == mConnectivity::Connected) quotingState = gwAutoStart;
        if (quotingState != gwQuotingState) {
          gwQuotingState = quotingState;
          FN::log(string("GW ") + argExchange, "Quoting state changed to", gwQuotingState == mConnectivity::Connected ? "CONNECTED" : "DISCONNECTED");
          UI::uiSend(uiTXT::ActiveState, {{"state", (int)gwQuotingState}});
        }
        ev_gwConnectButton(gwQuotingState);
        ev_gwConnectExchange(gwConnectExchange);
      };
      static void gwLoad(mExchange e) {
        if (e == mExchange::Coinbase) {
          system("test -n \"`/bin/pidof stunnel`\" && kill -9 `/bin/pidof stunnel`");
          system("stunnel etc/K-stunnel.conf");
          json k = FN::wJet(string(gw->http).append("/products/").append(gw->symbol));
          gw->minTick = stod(k.value("quote_increment", "0"));
          gw->minSize = stod(k.value("base_min_size", "0"));
        } else if (e == mExchange::HitBtc) {
          json k = FN::wJet(string(gw->http).append("/api/1/public/symbols"));
          if (k.find("symbols") != k.end())
            for (json::iterator it = k["symbols"].begin(); it != k["symbols"].end(); ++it)
              if (it->value("symbol", "") == gw->symbol) {
                gw->minTick = stod(it->value("step", "0"));
                gw->minSize = stod(it->value("lot", "0"));
                break;
              }
        } else if (e == mExchange::Bitfinex) {
          json k = FN::wJet(string(gw->http).append("/pubticker/").append(gw->symbol));
          if (k.find("last_price") != k.end()) {
            stringstream price_;
            price_ << scientific << stod(k.value("last_price", "0"));
            string _price_ = price_.str();
            for (string::iterator it=_price_.begin(); it!=_price_.end();)
              if (*it == '+' or *it == '-') break; else it = _price_.erase(it);
            stringstream os(string("1e").append(to_string(stod(_price_)-4)));
            os >> gw->minTick;
          }
          k = FN::wJet(string(gw->http).append("/symbols_details"));
          for (json::iterator it=k.begin(); it!=k.end();++it)
            if (it->value("pair", "") == gw->symbol)
              gw->minSize = stod(it->value("minimum_order_size", "0"));
        } else if (e == mExchange::OkCoin) {
          gw->minTick = "btc" == gw->symbol.substr(0,3) ? 0.01 : 0.001;
          gw->minSize = 0.01;
        } else if (e == mExchange::Korbit) {
          json k = FN::wJet(string(gw->http).append("/constants"));
          if (k.find(gw->symbol.substr(0,3).append("TickSize")) != k.end()) {
            gw->minTick = k.value(gw->symbol.substr(0,3).append("TickSize"), 0.0);
            gw->minSize = 0.015;
          }
        } else if (e == mExchange::Poloniex) {
          json k = FN::wJet(string(gw->http).append("/public?command=returnTicker"));
          if (k.find(gw->symbol) != k.end()) {
            istringstream os(string("1e-").append(to_string(6-k[gw->symbol]["last"].get<string>().find("."))));
            os >> gw->minTick;
            gw->minSize = 0.01;
          }
        } else if (e == mExchange::Null) {
          gw->minTick = 0.01;
          gw->minSize = 0.01;
        }
        if (gw->minTick and gw->minSize)
          FN::log(string("GW ") + argExchange, "allows client IP");
        else FN::logExit("CF", "Unable to fetch data from " + argExchange + " symbol \"" + gw->symbol + "\"", EXIT_FAILURE);
        stringstream ss;
        ss << setprecision(8) << fixed << '\n'
          << "- autoBot: " << (argAutobot ? "yes" : "no") << '\n'
          << "- pair: " << gw->symbol << '\n'
          << "- minTick: " << gw->minTick << '\n'
          << "- minSize: " << gw->minSize << '\n'
          << "- makeFee: " << gw->makeFee << '\n'
          << "- takeFee: " << gw->takeFee;
        FN::log(string("GW ") + argExchange + ":", ss.str());
      };
      static void happyEnding(int code) {
        eCode = code;
        if (uv_loop_alive(hub.getLoop())) {
          uv_timer_stop(&tCancel);
          uv_timer_stop(&tWallet);
          uv_timer_stop(&tCalcs);
          uv_timer_stop(&tStart);
          uv_timer_stop(&tDelay);
          gw->close();
          gw->gwGroup->close();
          FN::log(string("GW ") + argExchange, "Attempting to cancel all open orders, please wait.");
          gW->cancelAll();
          FN::log(string("GW ") + argExchange, "cancell all open orders OK");
          uiGroup->close();
          FN::close(hub.getLoop());
          hub.getLoop()->destroy();
        }
        EV::end(code);
      };
  };
}

#endif
