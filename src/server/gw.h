#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  static bool gwState = false;
  static mConnectivity gwConn = mConnectivity::Disconnected;
  static mConnectivity gwMDConn = mConnectivity::Disconnected;
  static mConnectivity gwEOConn = mConnectivity::Disconnected;
  static unsigned int gwCancelAll = 0;
  class GW {
    public:
      static void main() {
        evExit = happyEnding;
        thread([&]() {
          while (true) {
            if (qpRepo["cancelOrdersAuto"].get<bool>() and ++gwCancelAll == 20) {
              gwCancelAll = 0;
              gW->cancelAll();
            }
            gw->pos();
            this_thread::sleep_for(chrono::seconds(15));
          }
        }).detach();
        EV::on(mEv::GatewayMarketConnect, [](json k) {
          mConnectivity conn = (mConnectivity)k["/0"_json_pointer].get<int>();
          _gwCon_(mGatewayType::MarketData, conn);
          if (conn == mConnectivity::Disconnected)
            EV::up(mEv::MarketDataGateway);
        });
        EV::on(mEv::GatewayOrderConnect, [](json k) {
          _gwCon_(mGatewayType::OrderEntry, (mConnectivity)k["/0"_json_pointer].get<int>());
        });
        thread([&]() {
          gw->book();
        }).detach();
        gW = (gw->target == "NULL") ? Gw::E(mExchange::Null) : gw;
        UI::uiSnap(uiTXT::ProductAdvertisement, &onSnapProduct);
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
        hub.run();
      };
      static void gwPosUp(mGWp k) {
        EV::up(mEv::PositionGateway, {
          {"amount", k.amount},
          {"heldAmount", k.held},
          {"currency", k.currency}
        });
      };
      static void gwBookUp(mConnectivity k) {
        EV::up(mEv::GatewayMarketConnect, {(int)k});
      };
      static void gwLevelUp(mGWbls k) {
        json b;
        json a;
        for (vector<mGWbl>::iterator it = k.bids.begin(); it != k.bids.end(); ++it)
          b.push_back({{"price", (*it).price}, {"size", (*it).size}});
        for (vector<mGWbl>::iterator it = k.asks.begin(); it != k.asks.end(); ++it)
          a.push_back({{"price", (*it).price}, {"size", (*it).size}});
        EV::up(mEv::MarketDataGateway, {
          {"bids", b},
          {"asks", a}
        });
      };
      static void gwTradeUp(vector<mGWbt> k) {
        for (vector<mGWbt>::iterator it = k.begin(); it != k.end(); ++it)
          gwTradeUp(*it);
      }
      static void gwTradeUp(mGWbt k) {
        EV::up(mEv::MarketTradeGateway, {
          {"price", k.price},
          {"size", k.size},
          {"make_side", (int)k.make_side}
        });
      };
      static void gwOrderUp(mConnectivity k) {
        EV::up(mEv::GatewayOrderConnect, {(int)k});
      };
      static void gwOrderUp(mGWos k) {
        json o;
        if (k.oI.length()) o["orderId"] = k.oI;
        if (k.oE.length()) o["exchangeId"] = k.oE;
        o["orderStatus"] = (int)k.oS;
        if (k.oS == mORS::Cancelled) o["lastQuantity"] = 0;
        o["time"] = FN::T();
        EV::up(mEv::OrderUpdateGateway, o);
      };
      static void gwOrderUp(mGWol k) {
        EV::up(mEv::OrderUpdateGateway, {
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
        EV::up(mEv::OrderUpdateGateway, o);
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
        EV::up(mEv::OrderUpdateGateway, o);
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
          {"homepage", CF::cfPKString("homepage")}
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
