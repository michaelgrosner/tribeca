#ifndef K_G3_H_
#define K_G3_H_

namespace K {
  class GwCoinbase: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      double seq = 0;
      map<double, map<string, double>> a_;
      map<double, map<string, double>> b_;
      vector<json> q_;
      void config() {
        exchange = mExchange::Coinbase;
        symbol = string(mCurrency[base]).append("-").append(mCurrency[quote]);
        target = CF::cfString("CoinbaseOrderDestination");
        apikey = CF::cfString("CoinbaseApiKey");
        secret = CF::cfString("CoinbaseSecret");
        pass = CF::cfString("CoinbasePassphrase");
        http = CF::cfString("CoinbaseRestUrl");
        ws = CF::cfString("CoinbaseWebsocketUrl");
        json k = FN::wJet(string(http).append("/products/").append(symbol));
        if (k.find("quote_increment") != k.end()) {
          minTick = stod(k["quote_increment"].get<string>());
          minSize = stod(k["base_min_size"].get<string>());
        }
      };
      void pos() {
        unsigned long t = FN::T() / 1000;
        string p;
        B64::Decode(secret, &p);
        B64::Encode(FN::oHmac256(string(to_string(t)).append("GET/accounts"), p), &p);
        json k = FN::wJet(string(http).append("/accounts"), to_string(t), apikey, p, pass);
        if (k.find("message") != k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["message"] << endl;
        else for (json::iterator it = k.begin(); it != k.end(); ++it)
          if ((*it)["currency"] == mCurrency[base] || (*it)["currency"] == mCurrency[quote])
            GW::gwPosUp(mGWp(stod((*it)["available"].get<string>()), stod((*it)["hold"].get<string>()), FN::S2mC((*it)["currency"])));
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          GW::gwBookUp(mConnectivityStatus::Connected);
          unsigned long t = FN::T() / 1000;
          string p;
          B64::Decode(secret, &p);
          B64::Encode(FN::oHmac256(string(to_string(t)).append("GET/users/self"), p), &p);
          string m = string("{\"type\":\"subscribe\",\"product_ids\":[\"").append(symbol).append("\"],\"signature\":\"").append(p).append("\",\"key\":\"").append(apikey).append("\",\"passphrase\":\"").append(pass).append("\",\"timestamp\":\"").append(to_string(t)).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming book levels." << endl;
        });
        hub.onDisconnection([&](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) {
          GW::gwBookUp(mConnectivityStatus::Disconnected);
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Disconnected, reconnecting.." << endl;
          if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
          gwRec_.data = &ws;
          if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
            string* ws = (string*) handle->data;
            hub.connect(*ws, nullptr);
          }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
        });
        hub.onError([&](void *u) {
          GW::gwBookUp(mConnectivityStatus::Disconnected);
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error, reconnecting.." << endl;
          if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
          gwRec_.data = &ws;
          if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
            string* ws = (string*) handle->data;
            hub.connect(*ws, nullptr);
          }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
        });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          json k = json::parse(string(message, length));
          if (k.find("type") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Message Error " << k << endl; return; }
          else if (k.find("message") != k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << k["type"] << k["message"] << endl; return; }
          else if (k.find("sequence") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Message Error " << k << endl; return; }
          else getTrades(k);
        });
        hub.connect(ws, nullptr);
        getLevels();
      };
      void getLevels() {
        seq = 0;
        b_.clear();
        a_.clear();
        unsigned long t = FN::T() / 1000;
        string p;
        B64::Decode(secret, &p);
        B64::Encode(FN::oHmac256(string(to_string(t)).append("GET/accounts"), p), &p);
        json k = FN::wJet(string(http).append("/products/").append(symbol).append("/book?level=3"), to_string(t), apikey, p, pass);
        if (k.find("sequence") == k.end()) { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read book levels." << endl; return; }
        seq = k["sequence"];
        for (json::iterator it = k["bids"].begin(); it != k["bids"].end(); ++it)
          b_[decimal_cast<8>((*it)["/0"_json_pointer].get<string>()).getAsDouble()][(*it)["/2"_json_pointer].get<string>()] = decimal_cast<8>((*it)["/1"_json_pointer].get<string>()).getAsDouble();
        for (json::iterator it = k["asks"].begin(); it != k["asks"].end(); ++it)
          a_[decimal_cast<8>((*it)["/0"_json_pointer].get<string>()).getAsDouble()][(*it)["/2"_json_pointer].get<string>()] = decimal_cast<8>((*it)["/1"_json_pointer].get<string>()).getAsDouble();
        getBook();
        for (vector<json>::iterator it = q_.begin(); it != q_.end(); ++it)
          getTrades(*it);
        q_.clear();
      };
      void getBook() {
        if (b_.size() && a_.size()) {
          vector<mGWbl> _b_;
          vector<mGWbl> _a_;
          for (map<double, map<string, double>>::reverse_iterator it = b_.rbegin(); it != b_.rend(); ++it) {
            double bsize = decimal_cast<8>(0).getAsDouble();
            for (map<string, double>::iterator it_ = it->second.begin(); it_ != it->second.end(); ++it_)
              bsize += it_->second;
            _b_.push_back(mGWbl(it->first, bsize));
            if (_b_.size() == 13) break;
          }
          for (map<double, map<string, double>>::iterator it = a_.begin(); it != a_.end(); ++it) {
            double asize = decimal_cast<8>(0).getAsDouble();
            for (map<string, double>::iterator it_ = it->second.begin(); it_ != it->second.end(); ++it_)
              asize += it_->second;
            _a_.push_back(mGWbl(it->first, asize));
            if (_a_.size() == 13) break;
          }
          GW::gwLevelUp(mGWbls(_b_, _a_));
        }
      };
      void getTrades(json k) {
        if (!seq) { q_.push_back(k); return; }
        else if (k["sequence"].get<double>() <= seq) return;
        else if (k["sequence"].get<double>() != seq + 1) return getLevels();
        seq = k["sequence"];
        if (k["type"]=="received") return;
        bool s = k["side"] == "buy";
        if (k["type"]=="done") {
          if (s) for (map<double, map<string, double>>::iterator it = b_.begin(); it != b_.end();) {
            if (b_[it->first].find(k["order_id"].get<string>()) != b_[it->first].end()) {
              b_[it->first].erase(k["order_id"].get<string>());
              if (!b_[it->first].size()) it = b_.erase(it);
              else ++it;
            } else ++it;
          } else for (map<double, map<string, double>>::iterator it = a_.begin(); it != a_.end();) {
            if (a_[it->first].find(k["order_id"].get<string>()) != a_[it->first].end()) {
              a_[it->first].erase(k["order_id"].get<string>());
              if (!a_[it->first].size()) it = a_.erase(it);
              else ++it;
            } else ++it;
          }
        } else if (k["type"]=="open") {
          double p = decimal_cast<8>(k["price"].get<string>()).getAsDouble();
          double a = decimal_cast<8>(k["remaining_size"].get<string>()).getAsDouble();
          if (s) b_[p][k["order_id"].get<string>()] = a;
          else a_[p][k["order_id"].get<string>()] = a;
        } else if (k["type"]=="match") {
          double p = decimal_cast<8>(k["price"].get<string>()).getAsDouble();
          double a = decimal_cast<8>(k["size"].is_string() ? k["size"].get<string>() : k["remaining_size"].get<string>()).getAsDouble();
          string oi = k["maker_order_id"].get<string>();
          GW::gwTradeUp(mGWbt(p, a, s ? mSide::Bid : mSide::Ask));
          if (s && b_.find(p) != b_.end() && b_[p].find(oi) != b_[p].end()) {
            if (b_[p][oi] == a) {
              b_[p].erase(oi);
              if (!b_[p].size()) b_.erase(p);
            } else b_[p][oi] -= a;
          } else if (!s && a_.find(p) != a_.end() && a_[p].find(oi) != a_[p].end()) {
            if (a_[p][oi] == a) {
              a_[p].erase(oi);
              if (!a_[p].size()) a_.erase(p);
            } else a_[p][oi] -= a;
          }
        } else if (k["type"]=="change") {
          double p = decimal_cast<8>(k["price"].get<string>()).getAsDouble();
          double a = decimal_cast<8>(k["new_size"].get<string>()).getAsDouble();
          string oi = k["order_id"].get<string>();
          if (s && b_.find(p) != b_.end() && b_[p].find(oi) != b_[p].end()) b_[p][oi] = a;
          else if (!s && a_.find(p) != a_.end() && a_[p].find(oi) != a_[p].end()) a_[p][oi] = a;
        }
        getBook();
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {}
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {}
      void cancelAll() {}
      string clientId() { return ""; }
  };
}

#endif
