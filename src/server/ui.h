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
      mClock uiT_60s = 0;
    public:
      unsigned int orders_60s = 0;
      unsigned int bid_levels = 0;
      unsigned int ask_levels = 0;
    protected:
      void load() {
        if (((CF*)config)->argHeadless
          or ((CF*)config)->argUser.empty()
          or ((CF*)config)->argPass.empty()
        ) return;
        B64auth = string("Basic ") + FN::oB64(((CF*)config)->argUser + ':' + ((CF*)config)->argPass);
      };
      void waitData() {
        if (((CF*)config)->argHeadless) return;
        ((EV*)events)->uiGroup->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          ((SH*)screen)->logUIsess(++connections, webSocket->getAddress().address);
        });
        ((EV*)events)->uiGroup->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          ((SH*)screen)->logUIsess(--connections, webSocket->getAddress().address);
        });
        ((EV*)events)->uiGroup->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          string document;
          stringstream content;
          string auth = req.getHeader("authorization").toString();
          string addr = res->getHttpSocket()->getAddress().address;
          if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
          if (addr.length() > 7 and !((CF*)config)->argWhitelist.empty() and ((CF*)config)->argWhitelist.find(addr) == string::npos) {
            ((SH*)screen)->log("UI", "dropping gzip bomb on", addr);
            content << ifstream("etc/K-bomb.gzip").rdbuf();
            document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
            document += "Content-Encoding: gzip\r\nContent-Length: " + to_string(content.str().length()) + "\r\n\r\n" + content.str();
            res->write(document.data(), document.length());
          } else if (!B64auth.empty() and auth.empty()) {
            ((SH*)screen)->log("UI", "authorization attempt from", addr);
            document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\nContent-Length: 0\r\n\r\n";
            res->write(document.data(), document.length());
          } else if (!B64auth.empty() and auth != B64auth) {
            ((SH*)screen)->log("UI", "authorization failed from", addr);
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
              ((SH*)screen)->log("UI", "authorization success from", addr);
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
            } else if (leaf == "ico") {
              document += "Content-Type: image/x-icon\r\n";
              url = path;
            } else if (leaf == "mp3") {
              document += "Content-Type: audio/mpeg\r\n";
              url = path;
            }
            if (!url.empty())
              content << ifstream(readlink("app/client").substr(48) + url).rdbuf();
            else if (FN::int64() % 21) {
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
            if (addr.length() > 7 and ((CF*)config)->argWhitelist.find(addr) == string::npos) {
              webSocket->send(string((istreambuf_iterator<char>(ifstream("etc/K-bomb.gzip").rdbuf())), istreambuf_iterator<char>()).data(), uWS::OpCode::BINARY);
              return;
            }
          }
          if (mPortal::Hello == (mPortal)message[0] and hello.find(message[1]) != hello.end()) {
            json reply;
            (*hello[message[1]])(&reply);
            if (!reply.is_null()) webSocket->send((string(message, 2) + reply.dump()).data(), uWS::OpCode::TEXT);
          } else if (mPortal::Kiss == (mPortal)message[0] and kisses.find(message[1]) != kisses.end()) {
            json butterfly = json::parse((length > 2 and message[2] == '{') ? string(message, length).substr(2, length-2) : "{}");
            for (json::iterator it = butterfly.begin(); it != butterfly.end();)
              if (it.value().is_null()) it = butterfly.erase(it); else ++it;
            (*kisses[message[1]])(butterfly);
          }
        });
      };
      void waitTime() {
        if (((CF*)config)->argHeadless) return;
        ((EV*)events)->tClient->setData(this);
        ((EV*)events)->tClient->start(timer, 0, 0);
      };
      void waitUser() {
        if (((CF*)config)->argHeadless) {
          welcome = [&](mMatter type, function<void(json*)> *fn) {};
          clickme = [&](mMatter type, function<void(json)> *fn) {};
          delayme = [&](unsigned int delayUI) {};
          send = [&](mMatter type, json msg) {};
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
      function<void(mMatter, function<void(json*)>*)> welcome = [&](mMatter type, function<void(json*)> *fn) {
        if (hello.find((char)type) == hello.end()) hello[(char)type] = fn;
        else exit(_errorEvent_("UI", string("Use only a single unique message handler for \"") + (char)type + "\" welcome event"));
      };
      function<void(mMatter, function<void(json)>*)> clickme = [&](mMatter type, function<void(json)> *fn) {
        if (kisses.find((char)type) == kisses.end()) kisses[(char)type] = fn;
        else exit(_errorEvent_("UI", string("Use only a single unique message handler for \"") + (char)type + "\" clickme event"));
      };
      function<void(unsigned int)> delayme = [&](unsigned int delayUI) {
        realtimeClient = !delayUI;
        ((EV*)events)->tClient->stop();
        ((EV*)events)->tClient->start(timer, 0, realtimeClient ? 6e+4 : delayUI * 1e+3);
      };
      function<void(mMatter, json)> send = [&](mMatter type, json msg) {
        if (connections == 0) return;
        bool delayed = (
          type == mMatter::FairValue
          or type == mMatter::OrderStatusReports
          or type == mMatter::QuoteStatus
          or type == mMatter::Position
          or type == mMatter::TargetBasePosition
          or type == mMatter::EWMAChart
          or type == mMatter::MarketData
        );
        if (realtimeClient or !delayed) broadcast(type, msg.dump());
        else queue[type] = msg.dump();
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
      void broadcast(mMatter type, string msg) {
        msg.insert(msg.begin(), (char)type);
        msg.insert(msg.begin(), (char)mPortal::Kiss);
        ((EV*)events)->deferred([this, msg]() {
          ((EV*)events)->uiGroup->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
        });
      };
      void broadcastQueue() {
        for (map<mMatter, string>::value_type &it : queue)
          broadcast(it.first, it.second);
        queue.clear();
      };
      void (*timer)(Timer*) = [](Timer *tClient) {
        ((UI*)tClient->getData())->timer_60s_or_Xs();
      };
      inline void timer_60s_or_Xs() {                               _debugEvent_
        if (!realtimeClient) {
          broadcastQueue();
          if (uiT_60s + 6e+4 > _Tstamp_) return;
          else uiT_60s = _Tstamp_;
        }
        send(mMatter::ApplicationState, serverState());
        orders_60s = 0;
      };
      unsigned int memorySize() {
        string ps = FN::output(string("ps -p") + to_string(::getpid()) + " -orss | tail -n1");
        ps.erase(remove(ps.begin(), ps.end(), ' '), ps.end());
        if (ps.empty()) ps = "0";
        return stoi(ps) * 1e+3;
      };
      json serverState() {
        return {
          {"memory", memorySize()},
          {"freq", orders_60s},
          {"bids", bid_levels},
          {"asks", ask_levels},
          {"theme", ((CF*)config)->argIgnoreMoon + ((CF*)config)->argIgnoreSun},
          {"dbsize", ((DB*)memory)->size()},
          {"a", gw->A()}
        };
      };
      string readlink(const char* path) {
        string buffer(64, '\0');
#ifndef _WIN32
        ssize_t len;
        while((len = ::readlink(path, &buffer[0], buffer.size())) == (ssize_t)buffer.size())
          buffer.resize(buffer.size() * 2);
        if (len == -1) buffer.clear();
        else buffer.resize(len);
#endif
        return buffer;
      };
  };
}

#endif
