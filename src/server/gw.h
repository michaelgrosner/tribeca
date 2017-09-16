#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  static bool gwState = false;
  static bool gwAutoStart = false;
  static mConnectivity gwConn = mConnectivity::Disconnected;
  static mConnectivity gwMDConn = mConnectivity::Disconnected;
  static mConnectivity gwEOConn = mConnectivity::Disconnected;
  class GW {
    public:
      static void main() {
        evExit = happyEnding;
        gwAutoStart = CF::autoStart();
        thread([&]() {
          unsigned int T_5m = 0;
          while (true) {
            if (QP::getBool("cancelOrdersAuto") and ++T_5m == 20) {
              T_5m = 0;
              gW->cancelAll();
            }
            gw->pos();
            this_thread::sleep_for(chrono::seconds(15));
          }
        }).detach();
        ev_gwConnectOrder = [](mConnectivity k) {
          _gwCon_(mGatewayType::OrderEntry, k);
        };
        ev_gwConnectMarket = [](mConnectivity k) {
          _gwCon_(mGatewayType::MarketData, k);
          if (k == mConnectivity::Disconnected)
            EV::up(mEv::MarketDataGateway);
        };
        thread([&]() {
          gw->book();
        }).detach();
        UI::uiSnap(uiTXT::ProductAdvertisement, &onSnapProduct);
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
        hub.run();
      };
      static void gwBookUp(mConnectivity k) {
        ev_gwConnectMarket(k);
      };
      static void gwOrderUp(mConnectivity k) {
        ev_gwConnectOrder(k);
      };
      static void gwPosUp(mWallet k) {
        ev_gwDataWallet(k);
      };
      static void gwTradeUp(mGWbt k) {
        EV::up(mEv::MarketTradeGateway, {
          {"price", k.price},
          {"size", k.size},
          {"make_side", (int)k.make_side}
        });
      };
      static void gwOrderUp(string oI, string oE, mORS oS, double oP = 0, double oQ = 0, double oLQ = 0) {
        json o;
        if (oI.length()) o["orderId"] = oI;
        if (oE.length()) o["exchangeId"] = oE;
        o["orderStatus"] = (int)oS;
        if (oP) o["price"] = oP;
        if (oQ) o["quantity"] = oQ;
        if (oLQ) o["lastQuantity"] = oLQ;
        ev_gwDataOrder(o);
      };
      static void gwTradeUp(vector<mGWbt> k) {
        for (vector<mGWbt>::iterator it = k.begin(); it != k.end(); ++it)
          gwTradeUp(*it);
      };
      static void gwLevelUp(mGWbls k) {
        json b, a;
        for (vector<mGWbl>::iterator it = k.bids.begin(); it != k.bids.end(); ++it)
          b.push_back({{"price", it->price}, {"size", it->size}});
        for (vector<mGWbl>::iterator it = k.asks.begin(); it != k.asks.end(); ++it)
          a.push_back({{"price", it->price}, {"size", it->size}});
        EV::up(mEv::MarketDataGateway, {{"bids", b}, {"asks", a}});
      };
    private:
      static json onSnapProduct(json z) {
        string k = CF::cfString("BotIdentifier");
        return {{
          {"exchange", (double)gw->exchange},
          {"pair", {{"base", (double)gw->base}, {"quote", (double)gw->quote}}},
          {"minTick", gw->minTick},
          {"environment", k.substr(k.length()>4?(k.substr(0,4) == "auto"?4:0):0)},
          {"matryoshka", CF::cfString("MatryoshkaUrl")},
          {"homepage", "https://github.com/ctubio/Krypto-trading-bot"}
        }};
      };
      static json onSnapStatus(json z) {
        return {{{"status", (int)gwConn}}};
      };
      static json onSnapState(json z) {
        return {{{"state", gwState}}};
      };
      static json onHandState(json k) {
        if (k.value("state", false) != gwAutoStart) {
          gwAutoStart = k.value("state", false);
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
        EV::up(mEv::ExchangeConnect, {
          {"state", gwState},
          {"status", (int)gwConn}
        });
      };
      static void happyEnding(int code) {
        cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Attempting to cancel all open orders, please wait.." << endl;
        gW->cancelAll();
        cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " cancell all open orders OK." << endl;
        EV::end(code);
      };
  };
}

#endif
