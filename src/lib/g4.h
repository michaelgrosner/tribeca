#ifndef K_G4_H_
#define K_G4_H_

namespace K {
  class GwBitfinex: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      bool stillAlive;
      map<int, string> chan;
      vector<mGWbl> b_;
      vector<mGWbl> a_;
      int seq = 0;
      uWS::WebSocket<uWS::CLIENT> *ows;
      void config() {
        exchange = mExchange::Bitfinex;
        symbol = FN::S2l(string(mCurrency[base]).append(mCurrency[quote]));
        target = CF::cfString("BitfinexOrderDestination");
        apikey = CF::cfString("BitfinexKey");
        secret = CF::cfString("BitfinexSecret");
        http = CF::cfString("BitfinexHttpUrl");
        ws = CF::cfString("BitfinexWebsocketUrl");
        json k = FN::wJet(string(http).append("/pubticker/").append(symbol));
        if (k.find("last_price") != k.end()) {
          string k_ = to_string(stod(k["last_price"].get<string>()) / 10000);
          unsigned int i = stod(k["last_price"].get<string>())<0.00001 ? 1 : 0;
          for (string::iterator it=k_.begin(); it!=k_.end(); ++it)
            if (*it == '0') i++; else if (*it == '.') continue; else break;
          stringstream os(string("1e-").append(to_string(i>8?8:i)));
          os >> minTick;
          minSize = 0.01;
        }
      };
      void pos() {
        string p;
        B64::Encode(string("{\"request\":\"/v1/balances\",\"nonce\":\"").append(to_string(FN::T()*1000)).append("\"}"), &p);
        json k = FN::wJet(string(http).append("/balances"), p, apikey, FN::oHmac384(p, secret), true);
        if (k.find("message") != k.end()) {
          if (k["message"]=="Nonce is too small.") pos();
          else cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Warning " << k["message"] << endl;
        } else for (json::iterator it = k.begin(); it != k.end(); ++it) if ((*it)["type"] == "exchange")
          GW::gwPosUp(mGWp(stod((*it)["available"].get<string>()), stod((*it)["amount"].get<string>()) - stod((*it)["available"].get<string>()), FN::S2mC((*it)["currency"])));
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          ows = w;
          GW::gwBookUp(mConnectivityStatus::Connected);
          GW::gwOrderUp(mConnectivityStatus::Connected);
          string pn = to_string(FN::T()*1000);
          string pp = string("AUTH").append(pn);
          string m = string("{\"event\":\"auth\",\"filter\":[\"trading\"],\"apiKey\":\"").append(apikey).append("\",\"authNonce\":\"").append(pn).append("\",\"authPayload\":\"").append(pp).append("\",\"authSig\":\"").append(FN::oHmac384(pp, secret)).append("\"").append("}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          m = string("{\"event\":\"subscribe\",\"channel\":\"book\",\"symbol\":\"t").append(FN::S2u(symbol)).append("\",\"prec\":\"P0\",\"freq\":\"F0\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          m = string("{\"event\":\"subscribe\",\"channel\":\"trades\",\"symbol\":\"t").append(FN::S2u(symbol)).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
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
          if (k.find("event") != k.end()) {
            if (k["event"] == "pong") stillAlive = true;
            else if (k["event"] == "ping") { string m = string("{\"event\":\"pong\"}"); w->send(m.data(), m.length(), uWS::OpCode::TEXT); }
            else if (k["event"] == "error") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error " << k << endl;
            else if (k["event"] == "info") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Info " << k << endl;
            else if (k["event"] == "subscribed") { chan[k["chanId"]] = k["channel"]; cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming channel " << k["chanId"] << ":" << k["channel"] << endl; }
            else if (k["event"] == "auth") { if (k["status"]!="OK") w->close(); else { chan[k["chanId"]] = "auth"; cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming orders " << k["status"] << (k["msg"].is_string() ? string(" ").append(k["msg"]) : " 0:\"auth\"" ) << endl; } }
          } else if (k.is_array()) {
            if (k["/1"_json_pointer].is_string() and k["/1"_json_pointer] == "hb") stillAlive = true;
            else if (k["/1"_json_pointer].is_string() and k["/1"_json_pointer] == "n") cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Notice " << k << endl;
            else if (k["/0"_json_pointer].is_number() and chan.find(k["/0"_json_pointer]) != chan.end()) {
              if (chan[k["/0"_json_pointer]] == "auth" and k["/1"_json_pointer].is_string() and (k["/1"_json_pointer] == "ou" or k["/1"_json_pointer] == "oc")) { GW::gwOrderUp(mGWoa(to_string(k["/2/2"_json_pointer].get<long>()), to_string(k["/2/0"_json_pointer].get<long>()), getOS(k), k["/2/16"_json_pointer].get<double>(), abs(k["/2/6"_json_pointer].get<double>()), abs(abs(k["/2/7"_json_pointer].get<double>()) - abs(k["/2/6"_json_pointer].get<double>())), abs(abs(k["/2/7"_json_pointer].get<double>()) - abs(k["/2/6"_json_pointer].get<double>())), k["/2/17"_json_pointer].get<double>())); }
              else if (chan[k["/0"_json_pointer]] == "book") {
                if (k["/1/0"_json_pointer].is_number()) k["/1"_json_pointer] = { k["/1"_json_pointer] };
                if (k["/1/0"_json_pointer].is_array())
                  for (json::iterator it = k["/1"_json_pointer].begin(); it != k["/1"_json_pointer].end(); ++it) {
                    string k_ = (*it).dump();
                    string p_ = k_.substr(k_.find_last_of("[")+1);
                    double p = decimal_cast<8>(p_.substr(0, p_.find(","))).getAsDouble();
                    bool c = (*it)["/1"_json_pointer].get<int>() > 0;
                    bool s = (*it)["/2"_json_pointer].get<double>() > 0;
                    p_ = k_.substr(k_.find_last_of(",")+1);
                    p_ = p_.substr(0, p_.find("]"));
                    if (p_.substr(0,1)=="-") p_ = p_.substr(1);
                    double a = decimal_cast<8>(p_).getAsDouble();
                    if (s) {
                      for (vector<mGWbl>::iterator it_ = b_.begin(); it_ != b_.end();)
                        if ((*it_).price == p) it_ = b_.erase(it_); else ++it_;
                    } else for (vector<mGWbl>::iterator it_ = a_.begin(); it_ != a_.end();)
                      if ((*it_).price == p) it_ = a_.erase(it_); else ++it_;
                    if (c) {
                      if (s) b_.push_back(mGWbl(p, a));
                      else a_.push_back(mGWbl(p, a));
                    }
                  }
                sort(b_.begin(), b_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price*-1 < _b_.price*-1; });
                sort(a_.begin(), a_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price < _b_.price; });
                if (b_.size()>69) b_.resize(69, mGWbl(0, 0));
                if (a_.size()>69) a_.resize(69, mGWbl(0, 0));
                vector<mGWbl> _b_;
                vector<mGWbl> _a_;
                for (vector<mGWbl>::iterator it = b_.begin(); it != b_.end(); ++it)
                  if (_b_.size() < 13) _b_.push_back(*it); else break;
                for (vector<mGWbl>::iterator it = a_.begin(); it != a_.end(); ++it)
                  if (_a_.size() < 13) _a_.push_back(*it); else break;
                if (_a_.size() && _b_.size())
                  GW::gwLevelUp(mGWbls(_b_, _a_));
              } else if (chan[k["/0"_json_pointer]] == "trades") {
                if (k["/1"_json_pointer].is_string() && k["/1"_json_pointer].get<string>() == "te")
                  k["/1"_json_pointer] = { k["/2"_json_pointer] };
                if (k["/1"_json_pointer].is_array()) {
                  if (k["/1/0"_json_pointer].is_number())
                    k["/1"_json_pointer] = { k["/1"_json_pointer] };
                  if (k["/1/0"_json_pointer].is_array())
                    for (json::iterator it = k["/1"_json_pointer].begin(); it != k["/1"_json_pointer].end(); ++it)
                      GW::gwTradeUp(mGWbt(
                        (*it)["/3"_json_pointer],
                        abs((*it)["/2"_json_pointer].get<double>()),
                        (*it)["/2"_json_pointer].get<double>() > 0 ? mSide::Bid : mSide::Ask
                      ));
                }
              }
            }
          }
        });
        hub.connect(ws, nullptr);
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {
        string m = string("[0,\"on\",null,{\"gid\":0,\"cid\":").append(oI).append(",\"amount\":\"").append(to_string(oQ * (oS == mSide::Bid ? 1 : -1))).append("\",\"price\":\"").append(to_string(oP)).append("\",\"symbol\":\"t").append(FN::S2u(symbol)).append("\",\"type\":\"").append("EXCHANGE ").append(oLM == mOrderType::Market ? "MARKET" : (oTIF == mTimeInForce::FOK ?"FOK":"LIMIT")).append("\",\"postonly\":").append(oPO ? "1" : "0").append("}]");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {
        string m = string("[0,\"oc\",null,{\"id\":").append(oE).append("}]");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancelAll() {
        string p;
        B64::Encode(string("{\"request\":\"/v1/order/cancel/all\",\"nonce\":\"").append(to_string(FN::T()*1000)).append("\"}"), &p);
        FN::wGet(string(http).append("/order/cancel/all"), p, apikey, FN::oHmac384(p, secret), true);
      }
      string clientId() { string t = to_string(FN::T()); return string(to_string(++seq)).append(t.size()>7?t.substr(t.size()-7):t); }
      mORS getOS(json k) {
        string k_ = k["/2/13"_json_pointer].get<string>();
        if (k_.length()>=6) k_ = string(k_.data(), 6);
        else k_ = "";
        if (k_ == "ACTIVE" or k_ == "PARTIA") return mORS::Working;
        else if (k_ == "EXECUT") return mORS::Complete;
        else return mORS::Cancelled;
      }
  };
}

#endif
