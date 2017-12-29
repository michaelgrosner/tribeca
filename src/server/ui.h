#ifndef K_UI_H_
#define K_UI_H_

namespace K {
  class UI: public Klass {
    private:
      int connections = 0;
      string B64auth = "",
             notepad = "";
      bool toggleSettings = true,
           realtimeClient = false;
      map<char, function<void(json*)>*> hello;
      map<char, function<void(json)>*> kisses;
      map<mMatter, string> queue;
      unsigned long uiT_60s = 0;
    public:
      unsigned int orders_60s = 0;
    protected:
      void load() {
        if (((CF*)config)->argHeadless
          or ((CF*)config)->argUser == "NULL"
          or ((CF*)config)->argUser.empty()
          or ((CF*)config)->argPass == "NULL"
          or ((CF*)config)->argPass.empty()
        ) return;
        B64auth = string("Basic ") + FN::oB64(((CF*)config)->argUser + ':' + ((CF*)config)->argPass);
      };
      void waitTime() {
        if (((CF*)config)->argHeadless) return;
        ((EV*)events)->tClient->data = this;
        ((EV*)events)->tClient->start(timer, 0, 0);
      };
      void waitData() {
        if (((CF*)config)->argHeadless) return;
        ((EV*)events)->uiGroup->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          FN::logUIsess(++connections, webSocket->getAddress().address);
        });
        ((EV*)events)->uiGroup->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          FN::logUIsess(--connections, webSocket->getAddress().address);
        });
        ((EV*)events)->uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          stringstream content;
          string auth = req.getHeader("authorization").toString();
          string addr = res->getHttpSocket()->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          if (!((CF*)config)->argWhitelist.empty() and ((CF*)config)->argWhitelist.find(addr) == string::npos) {
            FN::log("UI", "dropping gzip bomb on", addr);
            content << ifstream("etc/K-bomb.gzip").rdbuf();
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            document += "Content-Encoding: gzip\r\nContent-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          } else if (!B64auth.empty() and auth.empty()) {
            FN::log("UI", "authorization attempt from", addr);
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (!B64auth.empty() and auth != B64auth) {
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
            if (!url.empty())
              content << ifstream(FN::readlink("app/client").substr(48) + url).rdbuf();
            else if (stol(FN::int64Id()) % 21) {
              document = "HTTP/1.1 404 Not Found\r\n";
              content << "Today, is a beautiful day.";
            } else { // Humans! go to any random url to check your luck
              document = "HTTP/1.1 418 I'm a teapot\r\n";
              content << "Today, is your lucky day!";
            }
            document += "Content-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          }
        });
        ((EV*)events)->uiGroup->onMessage([&](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          if (length < 2) return;
          if (!((CF*)config)->argWhitelist.empty()) {
            string addr = webSocket->getAddress().address;
            if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
            if (((CF*)config)->argWhitelist.find(addr) == string::npos)
              return;
          }
          if (mPortal::Hello == (mPortal)message[0] and hello.find(message[1]) != hello.end()) {
            json welcome;
            (*hello[message[1]])(&welcome);
            if (!welcome.is_null()) webSocket->send((string(message, 2) + welcome.dump()).data(), uWS::OpCode::TEXT);
          } else if (mPortal::Kiss == (mPortal)message[0] and kisses.find(message[1]) != kisses.end()) {
            json butterfly = json::parse((length > 2 and message[2] == '{') ? string(message, length).substr(2, length-2) : "{}");
            for (json::iterator it = butterfly.begin(); it != butterfly.end();)
              if (it.value().is_null()) it = butterfly.erase(it); else ++it;
            (*kisses[message[1]])(butterfly);
          }
        });
      };
      void waitUser() {
        if (((CF*)config)->argHeadless) {
          welcome = [&](mMatter k, function<void(json*)> *fn) {};
          clickme = [&](mMatter k, function<void(json)> *fn) {};
          delayme = [&](unsigned int delayUI) {};
          send = [&](mMatter k, json o) {};
        } else {
          welcome(mMatter::ApplicationState, &helloServer);
          welcome(mMatter::ProductAdvertisement, &helloProduct);
          welcome(mMatter::Notepad, &helloNotes);
          clickme(mMatter::Notepad, &kissNotes);
          welcome(mMatter::ToggleSettings, &helloSettings);
          clickme(mMatter::ToggleSettings, &kissSettings);
        }
      };
      void run() {
        if (((CF*)config)->argHeadless) return;
        ((EV*)events)->listen();
      };
    public:
      function<void(mMatter, function<void(json*)>*)> welcome = [&](mMatter k, function<void(json*)> *fn) {
        if (hello.find((char)k) == hello.end()) hello[(char)k] = fn;
        else exit(_errorEvent_("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" welcome event"));
      };
      function<void(mMatter, function<void(json)>*)> clickme = [&](mMatter k, function<void(json)> *fn) {
        if (kisses.find((char)k) == kisses.end()) kisses[(char)k] = fn;
        else exit(_errorEvent_("UI", string("Use only a single unique message handler for each \"") + (char)k + "\" clickme event"));
      };
      function<void(unsigned int)> delayme = [&](unsigned int delayUI) {
        realtimeClient = !delayUI;
        ((EV*)events)->tClient->stop();
        ((EV*)events)->tClient->start(timer, 0, realtimeClient ? 6e+4 : delayUI * 1e+3);
      };
      function<void(mMatter, json)> send = [&](mMatter k, json o) {
        if (connections == 0) return;
        bool delayed = (
          k == mMatter::FairValue
          or k == mMatter::OrderStatusReports
          or k == mMatter::QuoteStatus
          or k == mMatter::Position
          or k == mMatter::TargetBasePosition
          or k == mMatter::EWMAChart
          or k == mMatter::MarketData
        );
        if (realtimeClient or !delayed) broadcast(k, o.dump());
        else queue[k] = o.dump();
      };
    private:
      function<void(json*)> helloServer = [&](json *welcome) {
        *welcome = { serverState() };
      };
      function<void(json*)> helloProduct = [&](json *welcome) {
        *welcome = { {
          {"exchange", gw->exchange},
          {"pair", mPair(gw->base, gw->quote)},
          {"minTick", gw->minTick},
          {"environment", ((CF*)config)->argTitle},
          {"matryoshka", ((CF*)config)->argMatryoshka},
          {"homepage", "https://github.com/ctubio/Krypto-trading-bot"}
        } };
      };
      function<void(json*)> helloNotes = [&](json *welcome) {
        *welcome = { notepad };
      };
      function<void(json)> kissNotes = [&](json butterfly) {
        if (butterfly.is_array() and butterfly.size())
          notepad = butterfly.at(0);
      };
      function<void(json*)> helloSettings = [&](json *welcome) {
        *welcome = { toggleSettings };
      };
      function<void(json)> kissSettings = [&](json butterfly) {
        if (butterfly.is_array() and butterfly.size())
          toggleSettings = butterfly.at(0);
      };
      void broadcast(mMatter k, string j) {
        string m(1, (char)mPortal::Kiss);
        m += (char)k + j;
        ((EV*)events)->deferred([this, m]() {
          ((EV*)events)->uiGroup->broadcast(m.data(), m.length(), uWS::OpCode::TEXT);
        });
      };
      void broadcastQueue() {
        for (map<mMatter, string>::value_type &it : queue)
          broadcast(it.first, it.second);
        queue.clear();
      };
      void (*timer)(Timer*) = [](Timer *tClient) {
        ((UI*)tClient->data)->timer_60s_or_Xs();
      };
      void timer_60s_or_Xs() {                                      _debugEvent_
        if (!realtimeClient) {
          broadcastQueue();
          if (uiT_60s + 6e+4 > FN::T()) return;
          else uiT_60s = FN::T();
        }
        send(mMatter::ApplicationState, serverState());
        orders_60s = 0;
      };
      json serverState() {
        return {
          {"memory", FN::memory()},
          {"freq", orders_60s},
          {"dbsize", ((DB*)memory)->size()},
          {"a", gw->A()}
        };
      };
  };
}

#endif
