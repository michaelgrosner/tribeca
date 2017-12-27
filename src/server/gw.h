#ifndef K_GW_H_
#define K_GW_H_

namespace K {
  class GW: public Klass {
    private:
      mConnectivity gwAdminEnabled  = mConnectivity::Disconnected,
                    gwConnectOrders = mConnectivity::Disconnected,
                    gwConnectMarket = mConnectivity::Disconnected;
      unsigned int gwT_5m = 0;
    protected:
      void load() {
        gwEndings.back() = &happyEnding;
        gwAdminEnabled = (mConnectivity)((CF*)config)->argAutobot;
        handshake(gw->exchange);
      };
      void waitTime() {
        ((EV*)events)->tServer->data = this;
        ((EV*)events)->tServer->start([](Timer *handle) {
          GW *k = (GW*)handle->data;
          ((EV*)k->events)->debug("GW tServer timer");
          k->gw->wallet();
          if (k->qp->cancelOrdersAuto)
            if (!k->gwT_5m++) k->gw->cancelAll();
            else if (k->gwT_5m == 20) k->gwT_5m = 0;
        }, 0, 15e+3);
      };
      void waitData() {
        gw->evConnectOrder = [&](mConnectivity k) {
          serverSemaphore(&gwConnectOrders, k);
        };
        gw->evConnectMarket = [&](mConnectivity k) {
          if (!serverSemaphore(&gwConnectMarket, k))
            gw->evDataLevels(mLevels());
        };
        gw->levels();
      };
      void waitUser() {
        ((UI*)client)->welcome(mMatter::Connectivity, &hello);
        ((UI*)client)->clickme(mMatter::Connectivity, &kiss);
      };
      void run() {
        ((EV*)events)->start();
      };
    private:
      function<void()> happyEnding = [&]() {
        ((EV*)events)->stop([&](){
          FN::log(string("GW ") + gw->name, "Attempting to cancel all open orders, please wait.");
          gw->cancelAll();
          FN::log(string("GW ") + gw->name, "cancell all open orders OK");
        });
      };
      function<void(json*)> hello = [&](json *welcome) {
        *welcome = { serverState() };
      };
      function<void(json)> kiss = [&](json butterfly) {
        if (!butterfly.is_object() or !butterfly["state"].is_number()) return;
        mConnectivity updated = butterfly["state"].get<mConnectivity>();
        if (gwAdminEnabled != updated) {
          gwAdminEnabled = updated;
          clientSemaphore();
        }
      };
      mConnectivity serverSemaphore(mConnectivity *current, mConnectivity updated) {
        if (*current != updated) {
          *current = updated;
          ((QE*)engine)->gwConnectExchange = gwConnectMarket * gwConnectOrders;
          clientSemaphore();
        }
        return updated;
      };
      void clientSemaphore() {
        mConnectivity updated = gwAdminEnabled * ((QE*)engine)->gwConnectExchange;
        if (((QE*)engine)->gwConnectButton != updated) {
          ((QE*)engine)->gwConnectButton = updated;
          FN::log(string("GW ") + gw->name, "Quoting state changed to", string(!((QE*)engine)->gwConnectButton?"DIS":"") + "CONNECTED");
        }
        ((UI*)client)->send(mMatter::Connectivity, serverState());
      };
      json serverState() {
        return {
          {"state", ((QE*)engine)->gwConnectButton},
          {"status", ((QE*)engine)->gwConnectExchange}
        };
      };
      void handshake(mExchange k) {
        json reply;
        if (k == mExchange::Coinbase) {
          FN::stunnel();
          gw->randId = FN::uuidId;
          gw->symbol = FN::S2u(string(gw->base) + "-" + gw->quote);
          reply = FN::wJet(string(gw->http) + "/products/" + gw->symbol);
          gw->minTick = stod(reply.value("quote_increment", "0"));
          gw->minSize = stod(reply.value("base_min_size", "0"));
        }
        else if (k == mExchange::HitBtc) {
          gw->randId = FN::charId;
          gw->symbol = FN::S2u(string(gw->base) + gw->quote);
          reply = FN::wJet(string(gw->http) + "/public/symbol/" + gw->symbol);
          gw->minTick = stod(reply.value("tickSize", "0"));
          gw->minSize = stod(reply.value("quantityIncrement", "0"));
        }
        else if (k == mExchange::Bitfinex or k == mExchange::BitfinexMargin) {
          gw->randId = FN::int64Id;
          gw->symbol = FN::S2l(string(gw->base) + gw->quote);
          reply = FN::wJet(string(gw->http) + "/pubticker/" + gw->symbol);
          if (reply.find("last_price") != reply.end()) {
            stringstream price_;
            price_ << scientific << stod(reply.value("last_price", "0"));
            string _price_ = price_.str();
            for (string::iterator it=_price_.begin(); it!=_price_.end();)
              if (*it == '+' or *it == '-') break; else it = _price_.erase(it);
            stringstream os(string("1e") + to_string(fmax(stod(_price_),-4)-4));
            os >> gw->minTick;
          }
          reply = FN::wJet(string(gw->http) + "/symbols_details");
          if (reply.is_array())
            for (json::iterator it=reply.begin(); it!=reply.end();++it)
              if (it->find("pair") != it->end() and it->value("pair", "") == gw->symbol)
                gw->minSize = stod(it->value("minimum_order_size", "0"));
        }
        else if (k == mExchange::OkCoin or k == mExchange::OkEx) {
          gw->randId = FN::charId;
          gw->symbol = FN::S2l(string(gw->base) + "_" + gw->quote);
          gw->minTick = 0.0001;
          gw->minSize = 0.001;
        }
        else if (k == mExchange::Kraken) {
          gw->randId = FN::int64Id;
          gw->symbol = FN::S2u(string(gw->base) + gw->quote);
          reply = FN::wJet(string(gw->http) + "/0/public/AssetPairs?pair=" + gw->symbol);
          if (reply.find("result") != reply.end())
            for (json::iterator it = reply["result"].begin(); it != reply["result"].end(); ++it) {
              if (it.value().find("pair_decimals") != it.value().end()) {
                stringstream os(string("1e-") + to_string(it.value().value("pair_decimals", 0)));
                os >> gw->minTick;
                gw->symbol = it.key();
                gw->base = it.value().value("base", gw->base);
                gw->quote = it.value().value("quote", gw->quote);
              }
              break;
            }
          gw->minSize = 0.01;
        }
        else if (k == mExchange::Korbit) {
          gw->randId = FN::int64Id;
          gw->symbol = FN::S2l(string(gw->base) + "_" + gw->quote);
          reply = FN::wJet(string(gw->http) + "/constants");
          if (reply.find(gw->symbol.substr(0,3).append("TickSize")) != reply.end()) {
            gw->minTick = reply.value(gw->symbol.substr(0,3).append("TickSize"), 0.0);
            gw->minSize = 0.015;
          }
        }
        else if (k == mExchange::Poloniex) {
          gw->randId = FN::int64Id;
          gw->symbol = FN::FN::S2u(string(gw->base) + "_" + gw->quote);
          reply = FN::wJet(string(gw->http) + "/public?command=returnTicker");
          if (reply.find(gw->symbol) != reply.end()) {
            istringstream os(string("1e-").append(to_string(6-reply[gw->symbol]["last"].get<string>().find("."))));
            os >> gw->minTick;
            gw->minSize = 0.01;
          }
        }
        else if (k == mExchange::Null) {
          gw->randId = FN::int64Id;
          gw->symbol = FN::FN::S2u(string(gw->base) + "_" + gw->quote);
          gw->minTick = 0.01;
          gw->minSize = 0.01;
        }
        if (!gw->minTick or !gw->minSize)
          exit(((EV*)events)->error("CF", "Unable to fetch data from " + gw->name + " for symbol \"" + gw->symbol + "\", possible error message: " + reply.dump(), true));
        FN::log(string("GW ") + gw->name, "allows client IP");
        stringstream ss;
        ss << setprecision(8) << fixed << '\n'
          << "- autoBot: " << (!gwAdminEnabled ? "no" : "yes") << '\n'
          << "- symbols: " << gw->symbol << '\n'
          << "- minTick: " << gw->minTick << '\n'
          << "- minSize: " << gw->minSize << '\n'
          << "- makeFee: " << gw->makeFee << '\n'
          << "- takeFee: " << gw->takeFee;
        FN::log(string("GW ") + gw->name + ":", ss.str());
      };
  };
}

#endif
