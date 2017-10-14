#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  static mConnectivity gwAutoStart = mConnectivity::Disconnected,
                       gwQuotingState = mConnectivity::Disconnected,
                       gwConnectOrder = mConnectivity::Disconnected,
                       gwConnectMarket = mConnectivity::Disconnected,
                       gwConnectExchange = mConnectivity::Disconnected;
  class GW {
    public:
      static void main() {
        evExit = happyEnding;
        gwAutoStart = argAutobot ? mConnectivity::Connected : mConnectivity::Disconnected;
        thread([&]() {
          unsigned int T_5m = 0;
          while (true) {
            if (argDebugEvents) FN::log("DEBUG", "EV GW cancel thread.");
            if (QP::getBool("cancelOrdersAuto") and ++T_5m == 20) {
              T_5m = 0;
              gW->cancelAll();
            }
            gw->wallet();
            this_thread::sleep_for(chrono::seconds(15));
          }
        }).detach();
        ev_gwConnectOrder = [](mConnectivity k) {
          _gwCon_(mGatewayType::OrderEntry, k);
        };
        ev_gwConnectMarket = [](mConnectivity k) {
          _gwCon_(mGatewayType::MarketData, k);
          if (k == mConnectivity::Disconnected)
            ev_gwDataLevels(mLevels());
        };
        UI::uiSnap(uiTXT::ProductAdvertisement, &onSnapProduct);
        UI::uiSnap(uiTXT::ExchangeConnectivity, &onSnapStatus);
        UI::uiSnap(uiTXT::ActiveState, &onSnapState);
        UI::uiHand(uiTXT::ActiveState, &onHandState);
        if (argHeadless)
          thread([&]() { gw->levels(); }).join();
        else {
          thread([&]() { gw->levels(); }).detach();
          hub.run();
        }
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
          gwUpState();
        }
      };
      static void _gwCon_(mGatewayType gwT, mConnectivity gwS) {
        if (gwT == mGatewayType::MarketData) {
          if (gwConnectMarket == gwS) return;
          gwConnectMarket = gwS;
        } else if (gwT == mGatewayType::OrderEntry) {
          if (gwConnectOrder == gwS) return;
          gwConnectOrder = gwS;
        }
        gwConnectExchange = gwConnectMarket == mConnectivity::Connected && gwConnectOrder == mConnectivity::Connected
          ? mConnectivity::Connected : mConnectivity::Disconnected;
        gwUpState();
        UI::uiSend(uiTXT::ExchangeConnectivity, {{"status", (int)gwConnectExchange}});
      };
      static void gwUpState() {
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
      static void happyEnding(int code) {
        gW->freeSockets();
        if (gW->exchange != gw->exchange) gw->freeSockets();
        FN::log(string("GW ") + argExchange, "Attempting to cancel all open orders, please wait.");
        gW->cancelAll();
        FN::log(string("GW ") + argExchange, "cancell all open orders OK");
        EV::end(code);
      };
  };
}

#endif
