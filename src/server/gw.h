#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW: public Klass {
    private:
      mConnectivity adminAgreement     = mConnectivity::Disconnected,
                    greenGatewayOrders = mConnectivity::Disconnected,
                    greenGatewayMarket = mConnectivity::Disconnected;
    protected:
      void load() {
        endingFn.back() = &happyEnding;
        adminAgreement = (mConnectivity)args.autobot;
      };
      void waitData() {
        gw->evConnectOrder = [&](mConnectivity k) {
          gwSemaphore(&greenGatewayOrders, k);
        };
        gw->evConnectMarket = [&](mConnectivity k) {
          gwSemaphore(&greenGatewayMarket, k);
          if (!k) gw->evDataLevels(mLevels());
        };
      };
      void waitUser() {
        client->welcome(mMatter::Connectivity, &hello);
        client->clickme(mMatter::Connectivity, &kiss);
        screen->pressme(mHotkey::ESC, &hotkiss);
      };
      void run() {                                                  _debugEvent_
        handshake();
        if (gw->exchange == mExchange::Coinbase) FN::stunnel(true);
        events->start();
      };
    private:
      function<void()> happyEnding = [&]() {
        events->stop();
        if (gw->exchange == mExchange::Coinbase) FN::stunnel(false);
      };
      function<void(json*)> hello = [&](json *welcome) {
        *welcome = { semaphore() };
      };
      function<void(const json&)> kiss = [&](const json &butterfly) {
        if (!butterfly.is_object() or !butterfly["state"].is_number()) return;
        mConnectivity updated = butterfly["state"].get<mConnectivity>();
        if (adminAgreement != updated) {
          adminAgreement = updated;
          gwAdminSemaphore();
        }
      };
      function<void()> hotkiss = [&]() {
        adminAgreement = (mConnectivity)!adminAgreement;
        gwAdminSemaphore();
      };
      void gwSemaphore(mConnectivity *const current, const mConnectivity &updated) {
        if (*current != updated) {
          *current = updated;
          engine->greenGateway = greenGatewayMarket * greenGatewayOrders;
          gwAdminSemaphore();
        }
      };
      void gwAdminSemaphore() {
        mConnectivity updated = adminAgreement * engine->greenGateway;
        if (engine->greenButton != updated) {
          engine->greenButton = updated;
          screen->log(string("GW ") + gw->name, "Quoting state changed to", string(!engine->greenButton?"DIS":"") + "CONNECTED");
        }
        client->send(mMatter::Connectivity, semaphore());
        screen->refresh();
      };
      json semaphore() {
        return {
          {"state", engine->greenButton},
          {"status", engine->greenGateway}
        };
      };
      inline void handshake() {
        json reply;
        if (gw->exchange == mExchange::Coinbase) {
          gw->randId = FN::uuid36Id;
          gw->symbol = gw->base + "-" + gw->quote;
          reply = FN::wJet(gw->http + "/products/" + gw->symbol);
          gw->minTick = stod(reply.value("quote_increment", "0"));
          gw->minSize = stod(reply.value("base_min_size", "0"));
        }
        else if (gw->exchange == mExchange::HitBtc) {
          gw->randId = FN::uuid32Id;
          gw->symbol = gw->base + gw->quote;
          reply = FN::wJet(gw->http + "/public/symbol/" + gw->symbol);
          gw->minTick = stod(reply.value("tickSize", "0"));
          gw->minSize = stod(reply.value("quantityIncrement", "0"));
          gw->base = reply.value("baseCurrency", gw->base);
          gw->quote = reply.value("quoteCurrency", gw->quote);
        }
        else if (gw->exchange == mExchange::Bitfinex or gw->exchange == mExchange::Ethfinex) {
          gw->randId = FN::int45Id;
          gw->symbol = FN::strL(gw->base + gw->quote);
          reply = FN::wJet(gw->http + "/pubticker/" + gw->symbol);
          if (reply.find("last_price") != reply.end()) {
            stringstream price_;
            price_ << scientific << stod(reply.value("last_price", "0"));
            string _price_ = price_.str();
            for (string::iterator it=_price_.begin(); it!=_price_.end();)
              if (*it == '+' or *it == '-') break; else it = _price_.erase(it);
            stringstream os(string("1e") + to_string(fmax(stod(_price_),-4)-4));
            os >> gw->minTick;
          }
          reply = FN::wJet(gw->http + "/symbols_details");
          if (reply.is_array())
            for (json::iterator it=reply.begin(); it!=reply.end();++it)
              if (it->find("pair") != it->end() and it->value("pair", "") == gw->symbol)
                gw->minSize = stod(it->value("minimum_order_size", "0"));
        }
        else if (gw->exchange == mExchange::OkCoin or gw->exchange == mExchange::OkEx) {
          gw->randId = FN::char16Id;
          gw->symbol = FN::strL(gw->base + "_" + gw->quote);
          gw->minTick = 0.0001;
          gw->minSize = 0.001;
        }
        else if (gw->exchange == mExchange::Kraken) {
          gw->randId = FN::int32Id;
          gw->symbol = gw->base + gw->quote;
          reply = FN::wJet(gw->http + "/0/public/AssetPairs?pair=" + gw->symbol);
          if (reply.find("result") != reply.end())
            for (json::iterator it = reply["result"].begin(); it != reply["result"].end(); ++it)
              if (it.value().find("pair_decimals") != it.value().end()) {
                stringstream os(string("1e-") + to_string(it.value().value("pair_decimals", 0)));
                os >> gw->minTick;
                os = stringstream(string("1e-") + to_string(it.value().value("lot_decimals", 0)));
                os >> gw->minSize;
                gw->symbol = it.key();
                gw->base = it.value().value("base", gw->base);
                gw->quote = it.value().value("quote", gw->quote);
                break;
              }
        }
        else if (gw->exchange == mExchange::Korbit) {
          gw->randId = FN::int45Id;
          gw->symbol = FN::strL(gw->base + "_" + gw->quote);
          reply = FN::wJet(gw->http + "/constants");
          if (reply.find(gw->symbol.substr(0,3).append("TickSize")) != reply.end()) {
            gw->minTick = reply.value(gw->symbol.substr(0,3).append("TickSize"), 0.0);
            gw->minSize = 0.015;
          }
        }
        else if (gw->exchange == mExchange::Poloniex) {
          gw->randId = FN::int45Id;
          gw->symbol = gw->quote + "_" + gw->base;
          reply = FN::wJet(gw->http + "/public?command=returnTicker");
          if (reply.find(gw->symbol) != reply.end()) {
            istringstream os(string("1e-").append(to_string(6-reply[gw->symbol]["last"].get<string>().find("."))));
            os >> gw->minTick;
            gw->minSize = 0.001;
          }
        }
        else if (gw->exchange == mExchange::Null) {
          gw->randId = FN::uuid36Id;
          gw->symbol = gw->base + "_" + gw->quote;
          gw->minTick = 0.01;
          gw->minSize = 0.01;
        }
        if (!gw->randId or gw->symbol.empty())
          exit(screen->error("GW", "Incomplete handshake aborted."));
        if (!gw->minTick or !gw->minSize)
          exit(screen->error("GW", "Unable to fetch data from " + gw->name
            + " for symbol \"" + gw->symbol + "\", possible error message: "
            + reply.dump(),
          true));
        if (gw->exchange != mExchange::Null)
          screen->log(string("GW ") + gw->name, "allows client IP");
        unsigned int precision = gw->minTick < 1e-8 ? 10 : 8;
        screen->log(string("GW ") + gw->name + ":", string("\n")
          + "- autoBot: " + (!adminAgreement ? "no" : "yes") + '\n'
          + "- symbols: " + gw->symbol + '\n'
          + "- minTick: " + FN::strX(gw->minTick, precision) + '\n'
          + "- minSize: " + FN::strX(gw->minSize, precision) + '\n'
          + "- makeFee: " + FN::strX(gw->makeFee, precision) + '\n'
          + "- takeFee: " + FN::strX(gw->takeFee, precision));
      };
  };
}

#endif
