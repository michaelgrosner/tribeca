#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  string A();
  class UI: public Klass {
    protected:
      void load() {
        if (argHeadless) return;
        if (argUser != "NULL" && argPass != "NULL" && argUser.length() > 0 && argPass.length() > 0) {
          B64::Encode(argUser + ':' + argPass, &uiNK64);
          uiNK64 = string("Basic ") + uiNK64;
        }
        ((EV*)events)->uiGroup->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          connections++;
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          FN::logUIsess(connections, addr);
        });
        ((EV*)events)->uiGroup->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          connections--;
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          FN::logUIsess(connections, addr);
        });
        ((EV*)events)->uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          stringstream content;
          string auth = req.getHeader("authorization").toString();
          string addr = res->getHttpSocket()->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          lock_guard<mutex> lock(wsMutex);
          if (argWhitelist != "" and argWhitelist.find(addr) == string::npos) {
            FN::log("UI", "dropping gzip bomb on", addr);
            content << ifstream("etc/K-bomb.gzip").rdbuf();
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            document += "Content-Encoding: gzip\r\nContent-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          } else if (uiNK64 != "" && auth == "") {
            FN::log("UI", "authorization attempt from", addr);
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (uiNK64 != "" && auth != uiNK64) {
            FN::log("UI", "authorization failed from", addr);
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
              FN::log("UI", "authorization success from", addr);
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
        ((EV*)events)->uiGroup->onMessage([&](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          string addr = webSocket->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          if ((argWhitelist != "" and argWhitelist.find(addr) == string::npos) or length < 2)
            return;
          if (uiBIT::SNAP == (uiBIT)message[0] and cbSnap.find(message[1]) != cbSnap.end()) {
            json reply = (*cbSnap[message[1]])();
            lock_guard<mutex> lock(wsMutex);
            if (!reply.is_null()) webSocket->send(string(message, 2).append(reply.dump()).data(), uWS::OpCode::TEXT);
          } else if (uiBIT::MSG == (uiBIT)message[0] and cbMsg.find(message[1]) != cbMsg.end())
            (*cbMsg[message[1]])(json::parse((length > 2 and (message[2] == '[' or message[2] == '{'))
              ? string(message, length).substr(2, length-2) : "{}"
            ));
        });
      };
      void waitTime() {
        if (argHeadless) return;
        ((EV*)events)->tDelay.data = (void*)this;
        uv_timer_start(&((EV*)events)->tDelay, [](uv_timer_t *handle) {
          if (argDebugEvents) FN::log("DEBUG", "EV GW tDelay timer");
          ((UI*)handle->data)->send(((UI*)handle->data)->delayUI > 0);
        }, 0, 0);
      };
      void waitUser() {
        welcome(uiTXT::ApplicationState, &helloServer);
        welcome(uiTXT::Notepad, &helloNotes);
        clickme(uiTXT::Notepad, &kissNotes);
        welcome(uiTXT::ToggleSettings, &helloSettings);
        clickme(uiTXT::ToggleSettings, &kissSettings);
      };
      void run() {
        if (argHeadless) return;
        ((EV*)events)->listen();
      };
    public:
      void welcome(uiTXT k, function<json()> *cb) {
        if (argHeadless) return;
        if (cbSnap.find((char)k) == cbSnap.end())
          cbSnap[(char)k] = cb;
        else FN::logExit("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event", EXIT_SUCCESS);
      };
      void clickme(uiTXT k, function<void(json)> *cb) {
        if (argHeadless) return;
        if (cbMsg.find((char)k) == cbMsg.end())
          cbMsg[(char)k] = cb;
        else FN::logExit("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" event", EXIT_SUCCESS);
      };
      void send(uiTXT k, json o, bool hold = false) {
        if (argHeadless) return;
        if (connections == 0) return;
        if (delayUI and hold) uiHold(k, o);
        else uiUp(k, o);
      };
      void delay(double delayUI_) {
        if (argHeadless) return;
        delayUI = delayUI_;
        wsMutex.lock();
        queue.clear();
        wsMutex.unlock();
        uv_timer_set_repeat(&((EV*)events)->tDelay, delayUI ? (int)(delayUI*1e+3) : 6e+4);
      };
      unsigned int orders60sec = 0;
    private:
      int connections = 0;
      map<uiTXT, vector<json>> queue;
      map<char, function<json()>*> cbSnap;
      map<char, function<void(json)>*> cbMsg;
      string uiNK64 = "";
      string uiNOTE = "";
      double delayUI = 0;
      bool toggleSettings = true;
      function<json()> helloServer = [&]() {
        return (json){ serverState() };
      };
      function<json()> helloNotes = [&]() {
        return (json){ uiNOTE };
      };
      function<void(json)> kissNotes = [&](json k) {
        if (!k.is_null() and k.size())
          uiNOTE = k.at(0);
      };
      function<json()> helloSettings = [&]() {
        return (json){ toggleSettings };
      };
      function<void(json)> kissSettings = [&](json k) {
        if (!k.is_null() and k.size())
          toggleSettings = k.at(0);
      };
      void uiUp(uiTXT k, json o) {
        string m(1, (char)uiBIT::MSG);
        m += string(1, (char)k);
        m += o.is_null() ? "" : o.dump();
        lock_guard<mutex> lock(wsMutex);
        ((EV*)events)->uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
      };
      void uiHold(uiTXT k, json o) {
        lock_guard<mutex> lock(wsMutex);
        if (queue.find(k) != queue.end() and queue[k].size() > 0) {
          if (k != uiTXT::OrderStatusReports) queue[k].clear();
          else for (vector<json>::iterator it = queue[k].begin(); it != queue[k].end();)
            if (it->value("orderId", "") == o.value("orderId", ""))
              it = queue[k].erase(it);
            else ++it;
        }
        queue[k].push_back(o);
      };
      bool send() {
        static unsigned long uiT_1m = 0;
        wsMutex.lock();
        map<uiTXT, vector<json>> msgs;
        for (map<uiTXT, vector<json>>::iterator it_ = queue.begin(); it_ != queue.end();) {
          if (it_->first != uiTXT::OrderStatusReports) {
            msgs[it_->first] = it_->second;
            it_ = queue.erase(it_);
          } else ++it_;
        }
        for (vector<json>::iterator it = queue[uiTXT::OrderStatusReports].begin(); it != queue[uiTXT::OrderStatusReports].end();) {
          msgs[uiTXT::OrderStatusReports].push_back(*it);
          if (mORS::Working != (mORS)it->value("orderStatus", 0))
            it = queue[uiTXT::OrderStatusReports].erase(it);
          else ++it;
        }
        queue.erase(uiTXT::OrderStatusReports);
        wsMutex.unlock();
        for (map<uiTXT, vector<json>>::iterator it_ = msgs.begin(); it_ != msgs.end(); ++it_)
          for (vector<json>::iterator it = it_->second.begin(); it != it_->second.end(); ++it)
            uiUp(it_->first, *it);
        if (uiT_1m+6e+4 > FN::T()) return false;
        uiT_1m = FN::T();
        return true;
      };
      void send(bool delayed) {
        bool sec60 = true;
        if (delayed) sec60 = send();
        if (!sec60) return;
        send(uiTXT::ApplicationState, serverState());
        orders60sec = 0;
      };
      json serverState() {
        time_t rawtime;
        time(&rawtime);
        return {
          {"memory", FN::memory()},
          {"hour", localtime(&rawtime)->tm_hour},
          {"freq", orders60sec},
          {"dbsize", ((DB*)memory)->size()},
          {"a", A()}
        };
      };
  };
}

#endif
