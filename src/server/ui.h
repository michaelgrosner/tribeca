#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  string A();
  static uWS::Hub hub(0, false);
  typedef void (*uiMsg_)(json);
  typedef json (*uiSnap_)();
  struct uiSess { map<char, uiSnap_> cbSnap; map<char, uiMsg_> cbMsg; map<uiTXT, vector<json>> D; int u = 0; };
  static uWS::Group<uWS::SERVER> *uiGroup = hub.createGroup<uWS::SERVER>(uWS::PERMESSAGE_DEFLATE);
  static bool uiVisibleOpt = true;
  static unsigned int uiOSR_1m = 0;
  static double ui_delayUI = 0;
  static string uiNOTE = "";
  static string uiNK64 = "";
  class UI {
    public:
      static void main() {
        if (!argHeadless) {
          uiGroup->setUserData(new uiSess);
          uiSess *sess = (uiSess *) uiGroup->getUserData();
          if (argUser != "NULL" && argPass != "NULL" && argUser.length() > 0 && argPass.length() > 0) {
            B64::Encode(argUser + ':' + argPass, &uiNK64);
            uiNK64 = string("Basic ") + uiNK64;
          }
          uiGroup->onConnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
            sess->u++;
            typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
            FN::logUIsess(sess->u, address.address);
          });
          uiGroup->onDisconnection([sess](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
            sess->u--;
            typename uWS::WebSocket<uWS::SERVER>::Address address = webSocket->getAddress();
            FN::logUIsess(sess->u, address.address);
          });
          uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
            string document;
            string auth = req.getHeader("authorization").toString();
            typename uWS::WebSocket<uWS::SERVER>::Address address = res->getHttpSocket()->getAddress();
            if (uiNK64 != "" && auth == "") {
              FN::log("UI", "authorization attempt from", address.address);
              document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
              res->write(document.data(), document.length());
            } else if (uiNK64 != "" && auth != uiNK64) {
              FN::log("UI", "authorization failed from", address.address);
              document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
              res->write(document.data(), document.length());
            } else if (req.getMethod() == uWS::HttpMethod::METHOD_GET) {
              string url = "";
              document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
              string path = req.getUrl().toString();
              string::size_type n = 0;
              while ((n = path.find("..", n)) != string::npos) path.replace(n, 2, "");
              const string leaf = path.substr(path.find_last_of('.')+1);
              if (leaf == "/") {
                FN::log("UI", "authorization success from", address.address);
                document += "Content-Type: text/html; charset=UTF-8\r\n";
                url = "/index.html";
              } else if (leaf == "js") {
                document += "Content-Type: application/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n";
                url = path;
              } else if (leaf == "css") {
                document += "Content-Type: text/css; charset=UTF-8\r\n";
                url = path;
              } else if (leaf == "png") {
                document += "Content-Type: image/png\r\n";
                url = path;
              } else if (leaf == "mp3") {
                document += "Content-Type: audio/mpeg\r\n";
                url = path;
              }
              stringstream content;
              if (url.length() > 0) content << ifstream(FN::readlink("app/client").substr(3) + url).rdbuf();
              else {
                struct timespec txxs;
                clock_gettime(CLOCK_MONOTONIC, &txxs);
                srand((time_t)txxs.tv_nsec);
                if (rand() % 21) {
                  document = "HTTP/1.1 404 Not Found\r\n";
                  content << "Today, is a beautiful day.";
                } else { // Humans! go to any random url to check your luck
                  document = "HTTP/1.1 418 I'm a teapot\r\n";
                  content << "Today, is your lucky day!";
                }
              }
              document += "Content-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
              res->write(document.data(), document.length());
            }
          });
          uiGroup->onMessage([sess](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
            if (length > 1) {
              json v;
              if (length > 2 and (message[2] == '[' or message[2] == '{'))
                v = json::parse(string(message, length).substr(2, length-2).data());
              if (uiBIT::SNAP == (uiBIT)message[0] and sess->cbSnap.find(message[1]) != sess->cbSnap.end()) {
                json reply = (*sess->cbSnap[message[1]])();
                if (!reply.is_null()) webSocket->send(string(message, 2).append(reply.dump()).data(), uWS::OpCode::TEXT);
              } else if (uiBIT::MSG == (uiBIT)message[0] and sess->cbMsg.find(message[1]) != sess->cbMsg.end())
                (*sess->cbMsg[message[1]])(v);
            }
          });
          uS::TLS::Context c = uS::TLS::createContext("etc/sslcert/server.crt", "etc/sslcert/server.key", "");
          if ((access("etc/sslcert/server.crt", F_OK) != -1) && (access("etc/sslcert/server.key", F_OK) != -1) && hub.listen(argPort, c, 0, uiGroup))
            uiPrtcl = "HTTPS";
          else if (hub.listen(argPort, nullptr, 0, uiGroup))
            uiPrtcl = "HTTP";
          else { FN::logErr("IU", string("Use another UI port number, ") + to_string(argPort) + " seems already in use by:\n" + FN::output(string("netstat -anp 2>/dev/null | grep ") + to_string(argPort)) + "\n"); exit(EXIT_SUCCESS); }
          FN::logUI(uiPrtcl, argPort);
        }
        UI::uiSnap(uiTXT::ApplicationState, &onSnapApp);
        UI::uiSnap(uiTXT::Notepad, &onSnapNote);
        UI::uiHand(uiTXT::Notepad, &onHandNote);
        UI::uiSnap(uiTXT::ToggleConfigs, &onSnapOpt);
        UI::uiHand(uiTXT::ToggleConfigs, &onHandOpt);
        CF::api();
      };
      static void uiSnap(uiTXT k, uiSnap_ cb) {
        if (argHeadless) return;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->cbSnap.find((char)k) != sess->cbSnap.end()) { FN::logWar("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event"); exit(EXIT_SUCCESS); }
        else sess->cbSnap[(char)k] = cb;
      };
      static void uiHand(uiTXT k, uiMsg_ cb) {
        if (argHeadless) return;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->cbMsg.find((char)k) != sess->cbMsg.end()) { FN::logWar("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event"); exit(EXIT_SUCCESS); }
        else sess->cbMsg[(char)k] = cb;
      };
      static void uiSend(uiTXT k, json o, bool h = false) {
        static unsigned long uiT_MKT = 0;
        if (argHeadless) return;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->u == 0) return;
        if (k == uiTXT::MarketData) {
          if (uiT_MKT+369 > FN::T()) return;
          uiT_MKT = FN::T();
        }
        if (h) uiHold(k, o);
        else uiUp(k, o);
      };
      static void delay(double delayUI) {
        static unsigned int uiThread = 0;
        if (argHeadless) return;
        ui_delayUI = delayUI;
        wsMutex.lock();
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        sess->D.clear();
        wsMutex.unlock();
        thread([&]() {
          unsigned int uiThread_ = ++uiThread;
          double k = ui_delayUI;
          int timeout = k ? (int)(k*1e+3) : 6e+4;
          while (uiThread_ == uiThread) {
            if (argDebugEvents) FN::log("DEBUG", "EV UI thread");
            if (k) appPush();
            else appState();
            this_thread::sleep_for(chrono::milliseconds(timeout));
          }
        }).detach();
      };
    private:
      static json onSnapApp() {
        return { serverState() };
      };
      static json onSnapNote() {
        return { uiNOTE };
      };
      static void onHandNote(json k) {
        if (!k.is_null() and k.size())
          uiNOTE = k.at(0);
      };
      static json onSnapOpt() {
        return { uiVisibleOpt };
      };
      static void onHandOpt(json k) {
        if (!k.is_null() and k.size())
          uiVisibleOpt = k.at(0);
      };
      static void uiUp(uiTXT k, json o) {
        string m(1, (char)uiBIT::MSG);
        m += string(1, (char)k);
        m += o.is_null() ? "" : o.dump();
        lock_guard<mutex> lock(wsMutex);
        uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      };
      static void uiHold(uiTXT k, json o) {
        if (o.is_null()) {
          FN::logWar("UI", string(" uiHold sending ") + (char)k + " but is null, ignored");
          return;
        }
        bool isOSR = k == uiTXT::OrderStatusReports;
        if (isOSR && mORS::New == (mORS)o.value("orderStatus", 0)) return (void)++uiOSR_1m;
        if (!ui_delayUI) return uiUp(k, o);
        lock_guard<mutex> lock(wsMutex);
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        if (sess->D.find(k) != sess->D.end() && sess->D[k].size() > 0) {
          if (!isOSR) sess->D[k].clear();
          else for (vector<json>::iterator it = sess->D[k].begin(); it != sess->D[k].end();)
            if (it->value("orderId", "") == o.value("orderId", ""))
              it = sess->D[k].erase(it);
            else ++it;
        }
        sess->D[k].push_back(o);
      };
      static json serverState() {
        time_t rawtime;
        time(&rawtime);
        return {
          {"memory", FN::memory()},
          {"hour", localtime(&rawtime)->tm_hour},
          {"freq", uiOSR_1m / 2},
          {"dbsize", DB::size()},
          {"a", A()}
        };
      };
      static void appState() {
        uiSend(uiTXT::ApplicationState, serverState());
        uiOSR_1m = 0;
      };
      static void appPush() {
        static unsigned long uiT_1m = 0;
        wsMutex.lock();
        map<uiTXT, vector<json>> msgs;
        uiSess *sess = (uiSess *) uiGroup->getUserData();
        for (map<uiTXT, vector<json>>::iterator it_=sess->D.begin(); it_!=sess->D.end();) {
          if (it_->first != uiTXT::OrderStatusReports) {
            for (vector<json>::iterator it = it_->second.begin(); it != it_->second.end(); ++it)
              msgs[it_->first].push_back(*it);
            it_ = sess->D.erase(it_);
          } else ++it_;
        }
        if (sess->D.find(uiTXT::OrderStatusReports) != sess->D.end() && sess->D[uiTXT::OrderStatusReports].size() > 0) {
          int ki = 0;
          json k;
          for (vector<json>::iterator it = sess->D[uiTXT::OrderStatusReports].begin(); it != sess->D[uiTXT::OrderStatusReports].end();) {
            k.push_back(*it);
            if (mORS::Working != (mORS)it->value("orderStatus", 0))
              it = sess->D[uiTXT::OrderStatusReports].erase(it);
            else ++it;
          }
          if (!k.is_null())
            msgs[uiTXT::OrderStatusReports].push_back(k);
          sess->D.erase(uiTXT::OrderStatusReports);
        }
        wsMutex.unlock();
        for (map<uiTXT, vector<json>>::iterator it_=msgs.begin(); it_!=msgs.end(); ++it_)
          for (vector<json>::iterator it = it_->second.begin(); it != it_->second.end(); ++it)
            uiUp(it_->first, *it);
        if (uiT_1m+60000 > FN::T()) return;
        uiT_1m = FN::T();
        appState();
      };
  };
}

#endif
