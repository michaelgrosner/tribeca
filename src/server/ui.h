#ifndef K_UI_H_
#define K_UI_H_

extern const char _www_html_index,     _www_ico_favicon,     _www_css_base,
                  _www_gzip_bomb,      _www_mp3_audio_0,     _www_css_light,
                  _www_js_bundle,      _www_mp3_audio_1,     _www_css_dark;
extern const  int _www_html_index_len, _www_ico_favicon_len, _www_css_base_len,
                  _www_gzip_bomb_len,  _www_mp3_audio_0_len, _www_css_light_len,
                  _www_js_bundle_len,  _www_mp3_audio_1_len, _www_css_dark_len;
namespace K {
  class UI: public Klass {
    private:
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
    protected:
      void load() {
        if (args.headless
          or args.user.empty()
          or args.pass.empty()
        ) return;
        B64auth = string("Basic ") + FN::oB64(args.user + ':' + args.pass);
      };
      void waitData() {
        if (args.headless) return;
        broadcast = ((EV*)events)->listen(
          [&]() { onConnection(); },
          [&]() { onDisconnection(); },
          [&](const string &path, const bool &get, const string &auth, const string &addr) {
            return onHttpRequest(path, get, auth, addr);
          },
          [&](const string &message, const string &addr) {
            return onMessage(message, addr);
          }
        );
      };
      void waitTime() {
        if (args.headless) return;
        ((EV*)events)->tClient->setData(this);
        ((EV*)events)->tClient->start(timer, 0, 0);
      };
      void waitUser() {
        if (args.headless) {
          welcome = [](mMatter type, function<void(json*)> *fn) {};
          clickme = [](mMatter type, function<void(json)> *fn) {};
          delayme = [](unsigned int delayUI) {};
        } else {
          welcome(mMatter::ApplicationState, &helloServer);
          welcome(mMatter::ProductAdvertisement, &helloProduct);
          welcome(mMatter::Notepad, &helloNotes);
          clickme(mMatter::Notepad, &kissNotes);
          welcome(mMatter::ToggleSettings, &helloSettings);
          clickme(mMatter::ToggleSettings, &kissSettings);
        }
      };
    public:
      function<void(mMatter, function<void(json*)>*)> welcome = [&](mMatter type, function<void(json*)> *fn) {
        if (hello.find((char)type) != hello.end())
          exit(screen.error("UI", string("Use only a single unique message handler for \"")
            + (char)type + "\" welcome event"));
        hello[(char)type] = fn;
      };
      function<void(mMatter, function<void(json)>*)> clickme = [&](mMatter type, function<void(json)> *fn) {
        if (kisses.find((char)type) != kisses.end())
          exit(screen.error("UI", string("Use only a single unique message handler for \"")
            + (char)type + "\" clickme event"));
        kisses[(char)type] = fn;
      };
      function<void(unsigned int)> delayme = [&](unsigned int delayUI) {
        realtimeClient = !delayUI;
        ((EV*)events)->tClient->stop();
        ((EV*)events)->tClient->start(timer, 0, realtimeClient ? 6e+4 : delayUI * 1e+3);
      };
      function<void(mMatter, json)> send_nowhere = [](mMatter type, json msg) {};
      function<void(mMatter, json)> send = send_nowhere;
      function<void(mMatter, json)> send_somewhere = [&](mMatter type, json msg) {
        if (realtimeClient or !delayed(type))
          broadcast(type, msg.dump());
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
          {"environment", args.title},
          {"matryoshka", args.matryoshka},
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
      inline void onConnection() {
        send = send_somewhere;
      };
      inline void onDisconnection() {
        send = send_nowhere;
      };
      inline string onHttpRequest(const string &path, const bool &get, const string &auth, const string &addr) {
        string document,
               content,
               address = addr.empty() ? "unknown" : addr;
        if (!addr.empty() and !args.whitelist.empty() and args.whitelist.find(addr) == string::npos) {
          screen.log("UI", "dropping gzip bomb on", address);
          document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\nContent-Encoding: gzip\r\n";
          content = string(&_www_gzip_bomb, _www_gzip_bomb_len);
        } else if (!B64auth.empty() and auth.empty()) {
          screen.log("UI", "authorization attempt from", address);
          document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\n";
        } else if (!B64auth.empty() and auth != B64auth) {
          screen.log("UI", "authorization failed from", address);
          document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\n";
        } else if (get) {
          document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
          const string leaf = path.substr(path.find_last_of('.')+1);
          if (leaf == "/") {
            screen.log("UI", "authorization success from", address);
            document += "Content-Type: text/html; charset=UTF-8\r\n";
            content = string(&_www_html_index, _www_html_index_len);
          } else if (leaf == "js") {
            document += "Content-Type: application/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n";
            content = string(&_www_js_bundle, _www_js_bundle_len);
          } else if (leaf == "css") {
            document += "Content-Type: text/css; charset=UTF-8\r\n";
            if (path.find("css/bootstrap.min.css") != string::npos)
              content = string(&_www_css_base, _www_css_base_len);
            else if (path.find("css/bootstrap-theme-dark.min.css") != string::npos)
              content = string(&_www_css_dark, _www_css_dark_len);
            else if (path.find("css/bootstrap-theme.min.css") != string::npos)
              content = string(&_www_css_light, _www_css_light_len);
          } else if (leaf == "ico") {
            document += "Content-Type: image/x-icon\r\n";
            content = string(&_www_ico_favicon, _www_ico_favicon_len);
          } else if (leaf == "mp3") {
            document += "Content-Type: audio/mpeg\r\n";
            if (path.find("audio/0.mp3") != string::npos)
              content = string(&_www_mp3_audio_0, _www_mp3_audio_0_len);
            else if (path.find("audio/1.mp3") != string::npos)
              content = string(&_www_mp3_audio_1, _www_mp3_audio_1_len);
          }
          if (content.empty())
            if (FN::int64() % 21) {
              document = "HTTP/1.1 404 Not Found\r\n";
              content = "Today, is a beautiful day.";
            } else { // Humans! go to any random url to check your luck
              document = "HTTP/1.1 418 I'm a teapot\r\n";
              content = "Today, is your lucky day!";
            }
        }
        if (!document.empty())
          document += "Content-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
        return document;
      };
      inline string onMessage(const string &message, const string &addr) {
        string response;
        if (!addr.empty() and args.whitelist.find(addr) == string::npos)
          response = string(&_www_gzip_bomb, _www_gzip_bomb_len);
        if (mPortal::Hello == (mPortal)message[0] and hello.find(message[1]) != hello.end()) {
          json reply;
          (*hello[message[1]])(&reply);
          if (!reply.is_null())
            response = message.substr(0, 2) + reply.dump();
        } else if (mPortal::Kiss == (mPortal)message[0] and kisses.find(message[1]) != kisses.end()) {
          json butterfly = json::parse(
            (message.length() > 2 and message[2] == '{')
              ? message.substr(2)
              : "{}"
          );
          for (json::iterator it = butterfly.begin(); it != butterfly.end();)
            if (it.value().is_null()) it = butterfly.erase(it); else it++;
          (*kisses[message[1]])(butterfly);
        }
        return response;
      };
      function<void(const mMatter&, string)> broadcast;
      void broadcastQueue() {
        for (map<mMatter, string>::value_type &it : queue)
          broadcast(it.first, it.second);
        queue.clear();
      };
      inline static bool delayed(const mMatter &type) {
        return type == mMatter::FairValue
          or type == mMatter::OrderStatusReports
          or type == mMatter::QuoteStatus
          or type == mMatter::Position
          or type == mMatter::TargetBasePosition
          or type == mMatter::EWMAChart;
      };
      void (*timer)(uS::Timer*) = [](uS::Timer *tClient) {
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
          {"theme", args.ignoreMoon + args.ignoreSun},
          {"dbsize", ((DB*)memory)->size()},
          {"a", gw->A()}
        };
      };
  };
}

#endif
