#ifndef K_UI_H_
#define K_UI_H_

class UI: public Client { public: UI() { client = this; };
  private:
    uWS::Group<uWS::SERVER> *webui = nullptr;
    int connections = 0;
    unordered_map<mMatter, function<const json()>> hello;
    unordered_map<mMatter, function<void(json&)>> kisses;
    unordered_map<mMatter, string> queue;
  protected:
    void waitWebAdmin() override {
      if (K.num("headless")) return;
      if (!(webui = K.listen(protocol, K.num("port"), !K.num("without-ssl"),
        K.str("ssl-crt"), K.str("ssl-key"), httpServer, wsServer, wsMessage
      ))) error("UI", "Unable to listen to UI port number " + K.str("port")
            + " (may be already in use by another program)");
      K.timer_1s([&](const unsigned int &tick) {
        if (delay and *delay and !(tick % *delay) and !queue.empty()) {
          vector<string> msgs;
          for (const auto &it : queue)
            msgs.push_back((char)mPortal::Kiss + ((char)it.first + it.second));
          queue.clear();
          K.deferred([this, msgs]() {
            for (const auto &it : msgs)
              webui->broadcast(it.data(), it.length(), uWS::OpCode::TEXT);
          });
        }
        return false;
      });
      K.ending([&]() {
        webui->close();
      });
    };
  public:
    void welcome(mToClient &data) override {
      data.send = [&]() {
        if (send) send(data);
      };
      if (!webui) return;
      const mMatter matter = data.about();
      if (hello.find(matter) != hello.end())
        error("UI", string("Too many handlers for \"") + (char)matter + "\" welcome event");
      hello[matter] = [&]() {
        return data.hello();
      };
    };
    void clickme(mFromClient &data, function<void(const json&)> fn) override {
      if (!webui) return;
      const mMatter matter = data.about();
      if (kisses.find(matter) != kisses.end())
        error("UI", string("Too many handlers for \"") + (char)matter + "\" clickme event");
      kisses[matter] = [&data, fn](json &butterfly) {
        data.kiss(&butterfly);
        if (!butterfly.is_null())
          fn(butterfly);
      };
    };
  private:
    function<void(const mToClient&)> send;
    function<const bool(const Connectivity&, const string&)> wsServer = [&](
      const Connectivity &connected,
      const       string &addr
    ) {
      if (!(bool)connected) {
        if (!--connections) send = nullptr;
      } else if (!connections++)
        send = [&](const mToClient &data) {
          if (data.realtime() or !delay or !*delay) {
            const string msg = (char)mPortal::Kiss + ((char)data.about() + data.blob().dump());
            K.deferred([this, msg]() {
              webui->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
            });
          } else queue[data.about()] = data.blob().dump();
        };
      Print::log("UI", to_string(connections) + " client" + string(connections == 1 ? 0 : 1, 's')
        + " connected, last connection was from", addr);
      if (connections > K.num("client-limit")) {
        Print::log("UI", "--client-limit=" + K.str("client-limit") + " reached by", addr);
        return false;
      }
      return true;
    };
    function<const string(const string&, const string&, const string&)> httpServer = [&](
      const string &path,
      const string &auth,
      const string &addr
    ) {
      string document,
             content;
      if (addr != "unknown"
        and !K.str("whitelist").empty()
        and K.str("whitelist").find(addr) == string::npos
      ) {
        Print::log("UI", "dropping gzip bomb on", addr);
        document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\nContent-Encoding: gzip\r\n";
        content = string(&_www_gzip_bomb, _www_gzip_bomb_len);
      } else if (!K.str("B64auth").empty() and auth.empty()) {
        Print::log("UI", "authorization attempt from", addr);
        document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\n";
      } else if (!K.str("B64auth").empty() and auth != K.str("B64auth")) {
        Print::log("UI", "authorization failed from", addr);
        document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\n";
      } else {
        document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
        const string leaf = path.substr(path.find_last_of('.')+1);
        if (leaf == "/") {
          document += "Content-Type: text/html; charset=UTF-8\r\n";
          if (connections < K.num("client-limit")) {
            Print::log("UI", "authorization success from", addr);
            content = string(&_www_html_index, _www_html_index_len);
          } else {
            Print::log("UI", "--client-limit=" + K.str("client-limit") + " reached by", addr);
            content = "Thank you! but our princess is already in this castle!<br/>Refresh the page anytime to retry.";
          }
        } else if (leaf == "js") {
          document += "Content-Type: application/javascript; charset=UTF-8\r\nContent-Encoding: gzip\r\n";
          content = string(&_www_js_client, _www_js_client_len);
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
        if (content.empty()) {
          if (Random::int64() % 21) {
            document = "HTTP/1.1 404 Not Found\r\n";
            content = "Today, is a beautiful day.";
          } else { // Humans! go to any random url to check your luck
            document = "HTTP/1.1 418 I'm a teapot\r\n";
            content = "Today, is your lucky day!";
          }
        }
      }
      return document
        + "Content-Length: " + to_string(content.length())
        + "\r\n\r\n" + content;
    };
    function<const string(string, const string&)> wsMessage = [&](
            string message,
      const string &addr
    ) {
      if (addr != "unknown"
        and !K.str("whitelist").empty()
        and K.str("whitelist").find(addr) == string::npos
      ) return string(&_www_gzip_bomb, _www_gzip_bomb_len);
      const mPortal portal = (mPortal)message.at(0);
      const mMatter matter = (mMatter)message.at(1);
      if (mPortal::Hello == portal and hello.find(matter) != hello.end()) {
        const json reply = hello.at(matter)();
        if (!reply.is_null())
          return (char)portal + ((char)matter + reply.dump());
      } else if (mPortal::Kiss == portal and kisses.find(matter) != kisses.end()) {
        message = message.substr(2);
        json butterfly = json::accept(message)
          ? json::parse(message)
          : json::object();
        for (auto it = butterfly.begin(); it != butterfly.end();)
          if (it.value().is_null()) it = butterfly.erase(it); else ++it;
        kisses.at(matter)(butterfly);
      }
      return string();
    };
} ui;

#endif
