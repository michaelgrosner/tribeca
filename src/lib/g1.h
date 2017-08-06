#ifndef K_G1_H_
#define K_G1_H_

namespace K {
  class GwHitBtc: public Gw {
    public:
      bool cancelByClientId = true;
      bool supportCancelAll = false;
      bool snap = false;
      vector<mGWbl> a_;
      vector<mGWbl> b_;
      unsigned int seq = 0;
      uWS::WebSocket<uWS::CLIENT> *ows;
      void config() {
        exchange = mExchange::HitBtc;
        symbol = string(mCurrency[base]).append(mCurrency[quote]);
        target = CF::cfString("HitBtcOrderDestination");
        apikey = CF::cfString("HitBtcApiKey");
        secret = CF::cfString("HitBtcSecret");
        http = CF::cfString("HitBtcPullUrl");
        ws = CF::cfString("HitBtcOrderEntryUrl");
        wS = CF::cfString("HitBtcMarketDataUrl");
        json k = FN::wJet(string(http).append("/api/1/public/symbols"));
        if (k.find("symbols") != k.end())
          for (json::iterator it = k["symbols"].begin(); it != k["symbols"].end(); ++it)
            if ((*it)["symbol"] == symbol) {
              minTick = stod((*it)["step"].get<string>());
              minSize = stod((*it)["lot"].get<string>());
              break;
            }
      };
      void pos() {
        if (http.find("demo") == string::npos) {
          string p = string("apikey=").append(apikey).append("&nonce=").append(to_string(++seq));
          json k = FN::wJet(string(http).append("/api/1/trading/balance?").append(p), p, FN::oHmac512(string("/api/1/trading/balance?").append(p), secret));
          if (k.find("balance") == k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Unable to read wallet data positions." << endl;
          else for (json::iterator it = k["balance"].begin(); it != k["balance"].end(); ++it)
            if ((*it)["currency_code"] == mCurrency[base] || (*it)["currency_code"] == mCurrency[quote])
              GW::gwPosUp(mGWp((*it)["cash"], (*it)["reserved"], FN::S2mC((*it)["currency_code"])));
        } else {
          GW::gwPosUp(mGWp(500, 50, base));
          GW::gwPosUp(mGWp(500, 50, quote));
        }
      };
      void book() {
        hub.onConnection([&](uWS::WebSocket<uWS::CLIENT> *w, uWS::HttpRequest req) {
          long u = (long)w->getUserData();
          if (u == 1) {
            GW::gwBookUp(mConnectivityStatus::Connected);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming book levels." << endl;
          } else if (u == 2) {
            ows = w;
            GW::gwOrderUp(mConnectivityStatus::Connected);
            string p_;
            string p = string("{\"nonce\":").append(to_string(++seq)).append(",\"payload\":{\"Login\":{}}}");
            B64::Encode(FN::oHex(FN::oHmac512(p, secret)), &p_);
            string m = string("{\"apikey\":\"").append(apikey).append("\",\"signature\":\"").append(p_).append("\",\"message\":").append(p).append("}");
            w->send(m.data(), m.length(), uWS::OpCode::TEXT);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Streaming orders." << endl;
          }
        });
        hub.onDisconnection([&](uWS::WebSocket<uWS::CLIENT> *w, int code, char *message, size_t length) {
          long u = (long)w->getUserData();
          if ((long)u == 1) {
            GW::gwBookUp(mConnectivityStatus::Disconnected);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error while reading book levels, reconnecting.." << endl;
            if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
            gwRec_.data = &ws;
            if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
              string* ws = (string*) handle->data;
              hub.connect(*ws, (void*)1);
            }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
          } else if ((long)u == 2) {
            GW::gwOrderUp(mConnectivityStatus::Disconnected);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error while reading orders, reconnecting.." << endl;
            if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
            gwRec_.data = &wS;
            if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
              string* wS = (string*) handle->data;
              hub.connect(*wS, (void*)2);
            }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
          }
        });
        hub.onError([&](void *u) {
          if ((long)u == 1) {
            GW::gwBookUp(mConnectivityStatus::Disconnected);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error while reading book levels, reconnecting.." << endl;
            if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
            gwRec_.data = &ws;
            if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
              string* ws = (string*) handle->data;
              hub.connect(*ws, (void*)1);
            }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
          } else if ((long)u == 2) {
            GW::gwOrderUp(mConnectivityStatus::Disconnected);
            cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " WS Error while reading orders, reconnecting.." << endl;
            if (uv_timer_init(uv_default_loop(), &gwRec_)) { cout << FN::uiT() << "Errrror: GW gwRec_ init timer failed." << endl; exit(1); }
            gwRec_.data = &wS;
            if (uv_timer_start(&gwRec_, [](uv_timer_t *handle) {
              string* wS = (string*) handle->data;
              hub.connect(*wS, (void*)2);
            }, 5000, 0)) { cout << FN::uiT() << "Errrror: GW gwRec_ start timer failed." << endl; exit(1); }
          }
        });
        hub.onMessage([&](uWS::WebSocket<uWS::CLIENT> *w, char *message, size_t length, uWS::OpCode opCode) {
          if (!length || message[0] != '{') { cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Error " << string(message, length) << endl; return; }
          json k = json::parse(string(message, length));
          long u = (long) w->getUserData();
          if (u == 1) {
            if (k["MarketDataIncrementalRefresh"].is_object()) {
              k = k["MarketDataIncrementalRefresh"];
              if (!snap || symbol != k["symbol"]) return;
              for (json::iterator it = k["bid"].begin(); it != k["bid"].end(); ++it) {
                double p = stod((*it)["price"].get<string>());
                double s = (*it)["size"].get<double>();
                if (s) b_.push_back(mGWbl(p, s * minSize));
                else for (vector<mGWbl>::iterator it_ = b_.begin(); it_ != b_.end();)
                  if ((*it_).price == p) it_ = b_.erase(it_); else ++it_;
              }
              for (json::iterator it = k["ask"].begin(); it != k["ask"].end(); ++it) {
                double p = stod((*it)["price"].get<string>());
                double s = (*it)["size"].get<double>();
                if (s) a_.push_back(mGWbl(p, s * minSize));
                else for (vector<mGWbl>::iterator it_ = a_.begin(); it_ != a_.end();)
                  if ((*it_).price == p) it_ = a_.erase(it_); else ++it_;
              }
              sort(b_.begin(), b_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price*-1 < _b_.price*-1; });
              sort(a_.begin(), a_.end(), [](const mGWbl &_a_, const mGWbl &_b_) { return _a_.price < _b_.price; });
            }
            else if (k["MarketDataSnapshotFullRefresh"].is_object()) {
              k = k["MarketDataSnapshotFullRefresh"];
              if (symbol != k["symbol"]) return;
              snap = true;
              b_.clear();
              a_.clear();
              for (json::iterator it = k["bid"].begin(); it != k["bid"].end(); ++it) {
                b_.push_back(mGWbl(
                  stod((*it)["price"].get<string>()),
                  (*it)["size"].get<double>() * minSize
                ));
                if (b_.size() == 13) break;
              }
              for (json::iterator it = k["ask"].begin(); it != k["ask"].end(); ++it) {
                a_.push_back(mGWbl(
                  stod((*it)["price"].get<string>()),
                  (*it)["size"].get<double>() * minSize
                ));
                if (a_.size() == 13) break;
              }
            }
            if (b_.size()>69) b_.resize(69, mGWbl(0, 0));
            if (a_.size()>69) a_.resize(69, mGWbl(0, 0));
            vector<mGWbl> _b_;
            vector<mGWbl> _a_;
            for (vector<mGWbl>::iterator it = b_.begin(); it != b_.end(); ++it)
              if (_b_.size() < 13) _b_.push_back(*it); else break;
            for (vector<mGWbl>::iterator it = a_.begin(); it != a_.end(); ++it)
              if (_a_.size() < 13) _a_.push_back(*it); else break;
            if (_b_.size() && _a_.size())
              GW::gwLevelUp(mGWbls(_b_, _a_));
          } else if (u == 2) {
            if (k.find("error") != k.end()) cout << FN::uiT() << "GW " << CF::cfString("EXCHANGE") << " Error " << k["error"] << endl;
            else orderUp(k);
          }
        });
        hub.connect(wS, (void*)1);
        hub.connect(ws, (void*)2);
      };
      void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) {
        string p_;
        string p = string("{\"nonce\":").append(to_string(++seq)).append(",\"payload\":{\"NewOrder\":{\"clientOrderId\":\"").append(oI).append("\",\"symbol\":\"").append(symbol).append("\",\"side\":\"").append(oS == mSide::Bid ? "buy" : "sell").append("\",\"quantity\":\"").append(to_string((int)(oQ / minSize))).append("\",\"type\":\"").append(mOrderType::Limit == oLM ? "limit" : "market").append("\",\"price\":\"").append(to_string(oP)).append("\",\"timeInForce\":\"").append(oTIF == mTimeInForce::FOK ? "FOK" : (oTIF == mTimeInForce::GTC ? "GTC" : "IOC")).append("\"}}}");
        B64::Encode(FN::oHex(FN::oHmac512(p, secret)), &p_);
        string m = string("{\"apikey\":\"").append(apikey).append("\",\"signature\":\"").append(p_).append("\",\"message\":").append(p).append("}");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancel(string oI, string oE, mSide oS, unsigned long oT) {
        string p_;
        string p = string("{\"nonce\":").append(to_string(++seq)).append(",\"payload\":{\"OrderCancel\":{\"clientOrderId\":\"").append(oI).append("\",\"cancelRequestClientOrderId\":\"").append(oI).append("C\",\"symbol\":\"").append(symbol).append("\",\"side\":\"").append(oS == mSide::Bid ? "buy" : "sell").append("\"}}}");
        B64::Encode(FN::oHex(FN::oHmac512(p, secret)), &p_);
        string m = string("{\"apikey\":\"").append(apikey).append("\",\"signature\":\"").append(p_).append("\",\"message\":").append(p).append("}");
        ows->send(m.data(), m.length(), uWS::OpCode::TEXT);
      }
      void cancelAll() {
        string p = string("apikey=").append(apikey).append("&nonce=").append(to_string(++seq));
        orderUp(FN::wJet(string(http).append("/api/1/trading/cancel_orders?").append(p), string("symbols=").append(symbol), FN::oHmac512(string("/api/1/trading/cancel_orders?").append(p).append("symbols=").append(symbol), secret), true));
      }
      string clientId() {
        srand(time(0));
        char s[15];
        for (int i = 0; i < 15; ++i) s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
        return string(s, 15).append(to_string(++seq));
      }
      void orderUp(json k) {
        if (k.find("ExecutionReport") != k.end()) {
          k = k["ExecutionReport"];
          if (!k.is_array()) k = { k };
          for (json::iterator it = k.begin(); it != k.end(); ++it) {
            if ((*it)["lastQuantity"].get<double>() > 0 && (*it)["execReportType"] == "trade") {
              (*it)["lastQuantity"] = (*it)["lastQuantity"].get<double>() * minSize;
            } else {
              (*it)["lastPrice"] = "0";
              (*it)["lastQuantity"] = 0;
            }
            (*it)["cumQuantity"] = (*it)["cumQuantity"].is_number() ? (*it)["cumQuantity"].get<double>() * minSize : 0;
            (*it)["averagePrice"] = (*it)["averagePrice"].is_string() ? (*it)["averagePrice"].get<string>() : "0";
            GW::gwOrderUp(mGWoa((*it)["clientOrderId"].get<string>(), (*it)["orderId"].get<string>() != "N/A" ? (*it)["orderId"].get<string>() : (*it)["clientOrderId"].get<string>(), getOS(*it), stod((*it)["lastPrice"].get<string>() == "" ? "0" : (*it)["lastPrice"].get<string>()), (*it)["leavesQuantity"].is_number() ? (*it)["leavesQuantity"].get<double>() * minSize : 0, (*it)["lastQuantity"], (*it)["cumQuantity"], stod((*it)["averagePrice"].get<string>())));
           }
        } else if (k.find("CancelReject") != k.end())
          GW::gwOrderUp(mGWos(k["CancelReject"]["clientOrderId"], "", mORS::Cancelled));
      }
      mORS getOS(json k) {
        string k_ = k["execReportType"];
        if (k_ == "new" || k_ == "status") return mORS::Working;
        else if (k_ == "canceled" || k_ == "expired" || k_ == "rejected") return mORS::Cancelled;
        else if (k_ == "trade") return k["orderStatus"] == "filled" ? mORS::Complete : mORS::Working;
        else return mORS::Cancelled;
      }
  };
}

#endif
