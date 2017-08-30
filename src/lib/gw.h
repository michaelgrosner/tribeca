#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  static uv_timer_t gwPos_;
  static bool gwState = false;
  static mConnectivity gwConn = mConnectivity::Disconnected;
  static mConnectivity gwMDConn = mConnectivity::Disconnected;
  static mConnectivity gwEOConn = mConnectivity::Disconnected;
  class GW {
    public:
      static void main(Local<Object> exports) {
        evExit = happyEnding;
        thread([&]() {
          if (uv_timer_init(uv_default_loop(), &gwPos_)) { cout << FN::uiT() << "Errrror: GW gwPos_ init timer failed." << endl; exit(1); }
          if (uv_timer_start(&gwPos_, [](uv_timer_t *handle) {
            gw->pos();
          }, 0, 15000)) { cout << FN::uiT() << "Errrror: GW gwPos_ start timer failed." << endl; exit(1); }
        }).detach();
        EV::evOn("GatewayMarketConnect", [](json k) {
          _gwCon_(mGatewayType::MarketData, (mConnectivity)k["/0"_json_pointer].get<int>());
        });
        EV::evOn("GatewayOrderConnect", [](json k) {
          _gwCon_(mGatewayType::OrderEntry, (mConnectivity)k["/0"_json_pointer].get<int>());
        });
        gw->book();
        gW = (gw->target == "NULL") ? Gw::E(mExchange::Null) : gw;
        UI::uiSnap(uiTXT::ProductAdvertisement, &onSnapProduct);
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
      };
      static void gwPosUp(mGWp k) {
        EV::evUp("PositionGateway", {
          {"amount", k.amount},
          {"heldAmount", k.held},
          {"currency", k.currency}
        });
      };
      static void gwBookUp(mConnectivity k) {
        EV::evUp("GatewayMarketConnect", {(int)k});
      };
      static void gwLevelUp(mGWbls k) {
        json b;
        json a;
        for (vector<mGWbl>::iterator it = k.bids.begin(); it != k.bids.end(); ++it)
          b.push_back({{"price", (*it).price}, {"size", (*it).size}});
        for (vector<mGWbl>::iterator it = k.asks.begin(); it != k.asks.end(); ++it)
          a.push_back({{"price", (*it).price}, {"size", (*it).size}});
        EV::evUp("MarketDataGateway", {
          {"bids", b},
          {"asks", a}
        });
      };
      static void gwTradeUp(vector<mGWbt> k) {
        for (vector<mGWbt>::iterator it = k.begin(); it != k.end(); ++it)
          gwTradeUp(*it);
      }
      static void gwTradeUp(mGWbt k) {
        EV::evUp("MarketTradeGateway", {
          {"price", k.price},
          {"size", k.size},
          {"make_side", (int)k.make_side}
        });
      };
      static void gwOrderUp(mConnectivity k) {
        EV::evUp("GatewayOrderConnect", {(int)k});
      };
      static void gwOrderUp(mGWos k) {
        json o;
        if (k.oI.length()) o["orderId"] = k.oI;
        if (k.oE.length()) o["exchangeId"] = k.oE;
        o["orderStatus"] = (int)k.oS;
        if (k.oS == mORS::Cancelled) o["lastQuantity"] = 0;
        o["time"] = FN::T();
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(mGWol k) {
        EV::evUp("OrderUpdateGateway", {
          {"orderId", k.oI},
          {"orderStatus", (int)k.oS},
          {"lastPrice", k.oP},
          {"lastQuantity", k.oQ},
          {"liquidity", (int)k.oL},
          {"time", FN::T()}
        });
      };
      static void gwOrderUp(mGWoS k) {
        json o;
        if (k.oI.length()) o["orderId"] = k.oI;
        if (k.oE.length()) o["exchangeId"] = k.oE;
        o["orderStatus"] = (int)k.os;
        o["lastPrice"] = k.oP;
        o["lastQuantity"] = k.oQ;
        o["side"] = (int)k.oS;
        o["time"] = FN::T();
        EV::evUp("OrderUpdateGateway", o);
      };
      static void gwOrderUp(mGWoa k) {
        json o;
        o["orderId"] = k.oI;
        if (k.oE.length()) o["exchangeId"] = k.oE;
        o["orderStatus"] = (int)k.oS;
        if (k.oP) o["lastPrice"] = k.oP;
        o["lastQuantity"] = k.oLQ;
        o["leavesQuantity"] = k.oQ;
        o["time"] = FN::T();
        EV::evUp("OrderUpdateGateway", o);
      };
    private:
      static json onSnapProduct(json z) {
        return {{
          {"exchange", (double)gw->exchange},
          {"pair", {{"base", (double)gw->base}, {"quote", (double)gw->quote}}},
          {"environment", CF::cfString("BotIdentifier").substr(gwAutoStart?4:0)},
          {"matryoshka", CF::cfString("MatryoshkaUrl")},
          {"homepage", CF::cfPKString("homepage")},
          {"minTick", gw->minTick},
        }};
      };
      static json onSnapStatus(json z) {
        return {{{"status", (int)gwConn}}};
      };
      static json onSnapState(json z) {
        return {{{"state", gwState}}};
      };
      static json onHandState(json k) {
        if (k["state"].get<bool>() != gwAutoStart) {
          gwAutoStart = k["state"].get<bool>();
          gwUpState();
        }
        return {};
      };
      static void _gwCon_(mGatewayType gwT, mConnectivity gwS) {
        if (gwT == mGatewayType::MarketData) {
          if (gwMDConn == gwS) return;
          gwMDConn = gwS;
        } else if (gwT == mGatewayType::OrderEntry) {
          if (gwEOConn == gwS) return;
          gwEOConn = gwS;
        }
        gwConn = gwMDConn == mConnectivity::Connected && gwEOConn == mConnectivity::Connected
          ? mConnectivity::Connected : mConnectivity::Disconnected;
        gwUpState();
        UI::uiSend(uiTXT::ExchangeConnectivity, {{"status", (int)gwConn}});
      };
      static void gwUpState() {
        bool newMode = gwConn != mConnectivity::Connected ? false : gwAutoStart;
        if (newMode != gwState) {
          gwState = newMode;
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Changed quoting state to " << (gwState ? "Enabled" : "Disabled") << "." << endl;
          UI::uiSend(uiTXT::ActiveState, {{"state", gwState}});
        }
        EV::evUp("ExchangeConnect", {
          {"state", gwState},
          {"status", (int)gwConn}
        });
      };
      static void happyEnding(int code) {
        cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Attempting to cancel all open orders, please wait.." << endl;
        gW->cancelAll();
        EV::end(code, 2100);
      };
  };
}

#endif
