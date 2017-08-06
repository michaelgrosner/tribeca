#ifndef K_G2_H_
#define K_G2_H_

namespace K {
  class GwOkCoin: public Gw {
    public:
      bool cancelByClientId = false;
      bool supportCancelAll = false;
      string symbolr = "";
      string symbolq = "";
      string chanM = "";
      string chanTm = "";
      string chanTq = "";
      string chanS = "";
      string chanC = "";
      vector<map<string, double>> oIQ;
      uWS::WebSocket<uWS::CLIENT> *ows;
      void config() {
        exchange = mExchange::OkCoin;
        symbol = FN::S2l(string(mCurrency[base]).append("_").append(mCurrency[quote]));
        symbolr = FN::S2l(string(mCurrency[quote]).append("_").append(mCurrency[base]));
        symbolq = FN::S2l(string(mCurrency[quote]));
        target = CF::cfString("OkCoinOrderDestination");
        apikey = CF::cfString("OkCoinApiKey");
        secret = CF::cfString("OkCoinSecretKey");
        http = CF::cfString("OkCoinHttpUrl");
        ws = CF::cfString("OkCoinWsUrl");
        minTick = "btc" == symbol.substr(0,3) ? 0.01 : 0.001;
        minSize = 0.01;
      };
      void pos() {
        string p = string("api_key=").append(apikey);
        json k = FN::wJet(string(http).append("userinfo.do"), p.append("&sign=").append(FN::oMd5(string(p).append("&secret_key=").append(secret))));
        if (k["result"] != true) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl;
        else for (json::iterator it = k["info"]["funds"]["free"].begin(); it != k["info"]["funds"]["free"].end(); ++it)
          if (symbol.find(it.key()) != string::npos)
            GW::gwPosUp(mGWp(stod(k["info"]["funds"]["free"][it.key()].get<string>()), stod(k["info"]["funds"]["freezed"][it.key()].get<string>()), FN::S2mC(it.key())));
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          ows = w;
          GW::gwBookUp(mConnectivityStatus::Connected);
          chanM = string("ok_sub_spot").append(symbolr).append("_depth_20");
          chanTm = string("ok_sub_spot").append(symbolr).append("_trades");
          string m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanM).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanTm).append("\"}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
          GW::gwOrderUp(mConnectivityStatus::Connected);
          chanTq = string("ok_sub_spot").append(symbolq).append("_trades");
          chanS = string("ok_spot").append(symbolq).append("_trade");
          chanC = string("ok_spot").append(symbolq).append("_cancel_order");
          string p_ = string("api_key=").append(apikey);
          string p = string("\"api_key\":\"").append(apikey).append("\"");
          p.append(",\"sign\":\"").append(FN::oMd5(string(p_).append("&secret_key=").append(secret))).append("\"");
          m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanTq).append("\",\"parameters\":{").append(p).append("}}");
          w->send(m.data(), m.length(), uWS::OpCode::TEXT);
        });
        hub.onDisconnection([&](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) {
          GW::gwBookUp(mConnectivityStatus::Disconnected);
          GW::gwOrderUp(mConnectivityStatus::Disconnected);
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Disconnected, reconnecting in 5s.." << endl;
          if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
          gwRec_.data = &ws;
          if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
            string* ws = (string*) handle->data;
            hub.connect(*ws, nullptr);
          }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
        });
        hub.onError([&](void *u) {
          GW::gwBookUp(mConnectivityStatus::Disconnected);
          GW::gwOrderUp(mConnectivityStatus::Disconnected);
          cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error, reconnecting in 5s.." << endl;
          if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
          gwRec_.data = &ws;
          if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
            string* ws = (string*) handle->data;
            hub.connect(*ws, nullptr);
          }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
        });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          json k = json::parse(string(message, length));
          if (k.is_array()) {
            for (json::iterator it = k.begin(); it != k.end(); ++it)
              if ((*it).find("channel") != (*it).end())
                if ((!(*it)["result"].is_null() and !(*it)["result"]) or !(*it)["error_code"].is_null()) {
                  if (!(*it)["error_code"].is_null() && ((*it)["error_code"] == 10010 or (*it)["error_code"] == 10010))
                    for (vector<map<string, double>>::iterator it_ = oIQ.begin(); it_ != oIQ.end();) {
                      for (map<string, double>::iterator _it_ = (*it_).begin(); _it_ != (*it_).end();) {
                        GW::gwOrderUp(mGWos(_it_->first, "", mORS::Cancelled));
                        break;
                      }
                      oIQ.erase(it_);
                      break;
                    }
                  cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error " << (*it) << endl;
                }
                else if ((*it)["channel"] == "addChannel") { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming channel " << (*it)["data"]["channel"].get<string>() << " " << ((*it)["data"]["result"] ? "true" : "false") << endl; }
                else if ((*it)["channel"] == chanTm) {
                  for (json::iterator it_ = (*it)["data"].begin(); it_ != (*it)["data"].end(); ++it_)
                    GW::gwTradeUp(mGWbt(
                      stod((*it_)["/1"_json_pointer].get<string>()),
                      stod((*it_)["/2"_json_pointer].get<string>()),
                      (*it_)["/4"_json_pointer].get<string>() > "bid" ? mSide::Bid : mSide::Ask
                    ));
                }
                else if ((*it)["channel"] == chanM) {
                  vector<mGWbl> a;
                  vector<mGWbl> b;
                  for (json::iterator it_ = (*it)["data"]["bids"].begin(); it_ != (*it)["data"]["bids"].end(); ++it_) {
                    b.push_back(mGWbl(
                      stod((*it_)["/0"_json_pointer].get<string>()),
                      stod((*it_)["/1"_json_pointer].get<string>())
                    ));
                    if (b.size() == 13) break;
                  }
                  for (json::reverse_iterator it_ = (*it)["data"]["asks"].rbegin(); it_ != (*it)["data"]["asks"].rend(); ++it_) {
                    a.push_back(mGWbl(
                      stod((*it_)["/0"_json_pointer].get<string>()),
                      stod((*it_)["/1"_json_pointer].get<string>())
                    ));
                    if (a.size() == 13) break;
                  }
                  if (a.size() && b.size())
                    GW::gwLevelUp(mGWbls(b, a));
                }
                else if ((*it)["channel"] == chanTq) {
                    GW::gwOrderUp(mGWoa("", to_string((*it)["data"]["orderId"].get<int>()), getOS(*it), stod((*it)["data"]["sigTradePrice"].is_string() ? (*it)["data"]["sigTradePrice"].get<string>() : "0"), stod((*it)["data"]["sigTradeAmount"].is_string() ? (*it)["data"]["sigTradeAmount"].get<string>() : "0"), 0, 0, stod((*it)["data"]["averagePrice"].get<string>())));
                }
                else if ((*it)["channel"] == chanS) {
                  if (!(*it)["data"].is_object()) return;
                  for (vector<map<string, double>>::iterator it_ = oIQ.begin(); it_ != oIQ.end();) {
                    for (map<string, double>::iterator _it_ = (*it_).begin(); _it_ != (*it_).end();) {
                      if ((*it)["data"]["result"]) GW::gwOrderUp(mGWoa(_it_->first, to_string((*it)["data"]["order_id"].get<int>()), mORS::Working, 0, _it_->second, 0, 0, 0));
                      else GW::gwOrderUp(mGWos(_it_->first, "", mORS::Cancelled));
                      break;
                    }
                    oIQ.erase(it_);
                    break;
                  }
                }
                else if ((*it)["channel"] == chanC) {
                  if (!(*it)["data"].is_object() or !(*it)["data"]["order_id"].is_number()) return;
                  GW::gwOrderUp(mGWos("", to_string((*it)["data"]["order_id"].get<int>()), mORS::Cancelled));
                }
          } else cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error " << k << endl;
        });
        hub.connect(ws, nullptr);
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {
        map<string, double> oIQ_;
        oIQ_[oI] = oQ;
        oIQ.push_back(oIQ_);
        string p_ = string("amount=").append(to_string(oQ)).append("&api_key=").append(apikey).append("&price=").append(to_string(oP)).append("&symbol=").append(symbol).append("&type=").append(string(oS == mSide::Bid ? "buy" : "sell").append(mOrderType::Limit == oLM ? "" : "_market"));
        string p = string("\"amount\":\"").append(to_string(oQ)).append("\",").append("\"api_key\":\"").append(apikey).append("\",").append("\"price\":\"").append(to_string(oP)).append("\",").append("\"symbol\":\"").append(symbol).append("\",").append("\"type\":\"").append(string(oS == mSide::Bid ? "buy" : "sell").append(mOrderType::Limit == oLM ? "" : "_market")).append("\"");
        p.append(",\"sign\":\"").append(FN::oMd5(string(p_).append("&secret_key=").append(secret))).append("\"");
        string m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanS).append("\",\"parameters\":{").append(p).append("}}");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {
        string p_ = string("api_key=").append(apikey).append("&order_id=").append(oE).append("&symbol=").append(symbol);
        string p = string("\"api_key\":\"").append(apikey).append("\",").append("\"order_id\":\"").append(oE).append("\",").append("\"symbol\":\"").append(symbol).append("\"");
        p.append(",\"sign\":\"").append(FN::oMd5(string(p_).append("&secret_key=").append(secret))).append("\"");
        string m = string("{\"event\":\"addChannel\",\"channel\":\"").append(chanC).append("\",\"parameters\":{").append(p).append("}}");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancelAll() {
        string p = string("api_key=").append(apikey).append("&order_id=").append("-1").append("&symbol=").append(symbol);
        json k = FN::wJet(string(http).append("order_info.do"), p.append("&sign=").append(FN::oMd5(string(p).append("&secret_key=").append(secret))));
        if (k.find("orders") == k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl;
        else for (json::iterator it = k["orders"].begin(); it != k["orders"].end(); ++it) {
          p = string("api_key=").append(apikey).append("&order_id=").append(to_string((*it)["order_id"].get<int>())).append("&symbol=").append(symbol);
          json k_ = FN::wJet(string(http).append("cancel_order.do"), p.append("&sign=").append(FN::oMd5(string(p).append("&secret_key=").append(secret))));
          if (k_.find("result") != k_.end() and k_["result"])
            GW::gwOrderUp(mGWos("", to_string(k_["order_id"].get<int>()), mORS::Cancelled));
        }
      }
      string clientId() {
        srand(time(0));
        char s[8];
        for (int i = 0; i < 8; ++i) s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        return string(s, 8);
      }
      mORS getOS(json k) {
        int k_ = k["data"]["status"].get<int>();
        if (k_ == -1) return mORS::Cancelled;
        if (k_ == 0) return mORS::Working;
        if (k_ == 1) return mORS::Working;
        if (k_ == 2) return mORS::Complete;
        if (k_ == 4) return mORS::Working;
        else return mORS::Cancelled;
      }
  };
}

#endif
