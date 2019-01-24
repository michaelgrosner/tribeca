#ifndef K_UI_H_
#define K_UI_H_

extern const char _www_html_index,     _www_ico_favicon,     _www_css_base,
                  _www_gzip_bomb,      _www_mp3_audio_0,     _www_css_light,
                  _www_js_client,      _www_mp3_audio_1,     _www_css_dark;
extern const  int _www_html_index_len, _www_ico_favicon_len, _www_css_base_len,
                  _www_gzip_bomb_len,  _www_mp3_audio_0_len, _www_css_light_len,
                  _www_js_client_len,  _www_mp3_audio_1_len, _www_css_dark_len;

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
      if (!(webui = K.listen(K.num("port"), sslContext())))
        error("UI", "Unable to listen to UI port number " + K.str("port")
           + " (may be already in use by another program)"
         );
      webui->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
        wakeup(Connectivity::Connected, webSocket);
      });
      webui->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
        wakeup(Connectivity::Disconnected, webSocket);
      });
      webui->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
        if (req.getMethod() != uWS::HttpMethod::METHOD_GET) return;
        const string response = onHttpRequest(
          req.getUrl().toString(),
          req.getHeader("authorization").toString(),
          cleanAddress(res->getHttpSocket()->getAddress().address)
        );
        res->write(response.data(), response.length());
      });
      webui->onMessage([&](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
        if (length < 2) return;
        const string response = onMessage(
          string(message, length),
          cleanAddress(webSocket->getAddress().address)
        );
        if (!response.empty())
          webSocket->send(
            response.data(),
            response.substr(0, 2) == "PK"
              ? uWS::OpCode::BINARY
              : uWS::OpCode::TEXT
          );
      });
      K.timer_1s_always([&](const unsigned int &tick) {
        if (!delay or !*delay or (tick % *delay) or queue.empty())
          return;
        vector<string> msgs;
        for (const auto &it : queue)
          msgs.push_back((char)mPortal::Kiss + ((char)it.first + it.second));
        queue.clear();
        K.deferred([this, msgs]() {
          for (const auto &it : msgs)
            webui->broadcast(it.data(), it.length(), uWS::OpCode::TEXT);
        });
      });
      Print::log("UI", "ready at", Text::strL(protocol) + "://" + K.wtfismyip + ":" + K.str("port"));
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
    void wakeup(const Connectivity &connected, uWS::WebSocket<uWS::SERVER> *const webSocket) {
      if ((bool)connected) {
        if (!connections++)
          send = [&](const mToClient &data) {
            if (data.realtime() or !delay or !*delay) {
              const string msg = (char)mPortal::Kiss + ((char)data.about() + data.blob().dump());
              K.deferred([this, msg]() {
                webui->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
              });
            } else queue[data.about()] = data.blob().dump();
          };
      } else if (!--connections) send = nullptr;
      const string addr = cleanAddress(webSocket->getAddress().address);
      Print::log("UI",
        to_string(connections) + " client" + string(connections == 1 ? 0 : 1, 's')
          + " connected, last connection was from",
        addr
      );
      if (connections > K.num("client-limit")) {
        Print::log("UI", "--client-limit=" + K.str("client-limit") + " reached by", addr);
        webSocket->close();
      }
    };
    SSL_CTX *sslContext() {
      SSL_CTX *ctx = nullptr;
      if (!K.num("without-ssl") and (ctx = SSL_CTX_new(SSLv23_server_method()))) {
        SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
        if (K.str("ssl-crt").empty() or K.str("ssl-key").empty()) {
          if (!K.str("ssl-crt").empty())
            Print::logWar("UI", "Ignored --ssl-crt because --ssl-key is missing");
          if (!K.str("ssl-key").empty())
            Print::logWar("UI", "Ignored --ssl-key because --ssl-crt is missing");
          Print::logWar("UI", "Connected web clients will enjoy unsecure SSL encryption..\n"
            "(because the private key is visible in the source!) consider --ssl-crt and --ssl-key arguments");
          if (!SSL_CTX_use_certificate(ctx,
            PEM_read_bio_X509(BIO_new_mem_buf((void*)
              "-----BEGIN CERTIFICATE-----"                                      "\n"
              "MIICATCCAWoCCQCiyDyPL5ov3zANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB" "\n"
              "VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0" "\n"
              "cyBQdHkgTHRkMB4XDTE2MTIyMjIxMDMyNVoXDTE3MTIyMjIxMDMyNVowRTELMAkG" "\n"
              "A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0" "\n"
              "IFdpZGdpdHMgUHR5IEx0ZDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAunyx" "\n"
              "1lNsHkMmCa24Ns9xgJAwV3A6/Jg/S5jPCETmjPRMXqAp89fShZxN2b/2FVtU7q/N" "\n"
              "EtNpPyEhfAhPwYrkHCtip/RmZ/b6qY2Cx6otFIsuwO8aUV27CetpoM8TAQSuufcS" "\n"
              "jcZD9pCAa9GM/yWeqc45su9qBBmLnAKYuYUeDQUCAwEAATANBgkqhkiG9w0BAQsF" "\n"
              "AAOBgQAeZo4zCfnq5/6gFzoNDKg8DayoMnCtbxM6RkJ8b/MIZT5p6P7OcKNJmi1o" "\n"
              "XD2evdxNrY0ObQ32dpiLqSS1JWL8bPqloGJBNkSPi3I+eBoJSE7/7HOroLNbp6nS" "\n"
              "aaec6n+OlGhhjxn0DzYiYsVBUsokKSEJmHzoLHo3ZestTTqUwg=="             "\n"
              "-----END CERTIFICATE-----"                                        "\n"
            , -1), nullptr, nullptr, nullptr
          )) or !SSL_CTX_use_RSAPrivateKey(ctx,
            PEM_read_bio_RSAPrivateKey(BIO_new_mem_buf((void*)
              "-----BEGIN RSA PRIVATE KEY-----"                                  "\n"
              "MIICXAIBAAKBgQC6fLHWU2weQyYJrbg2z3GAkDBXcDr8mD9LmM8IROaM9ExeoCnz" "\n"
              "19KFnE3Zv/YVW1Tur80S02k/ISF8CE/BiuQcK2Kn9GZn9vqpjYLHqi0Uiy7A7xpR" "\n"
              "XbsJ62mgzxMBBK659xKNxkP2kIBr0Yz/JZ6pzjmy72oEGYucApi5hR4NBQIDAQAB" "\n"
              "AoGBAJi9OrbtOreKjeQNebzCqRcAgeeLz3RFiknzjVYbgK1gBhDWo6XJVe8C9yxq" "\n"
              "sjYJyQV5zcAmkaQYEaHR+OjvRiZ4UmXbItukOD+dnq7xs69n3w54FfANjkurdL2M" "\n"
              "fPAQm/GJT4TSBDIr7eJQPOrork9uxQStwADTqvklVlKm2YldAkEA80ZYaLrGOBbz" "\n"
              "5871ewKxtVJNCCmXdYUwq7nI/lqsLBZnB+wiwnQ+3tgfi4YoUoTnv0hIIwkyLYl9" "\n"
              "Z2wqensf6wJBAMQ96gUGnIcYJzknB5CYDNQalcvvTx7tLtgRXDf47bQJ3X/Q5k/t" "\n"
              "yDlByUBqvYVShXWs+d4ynNKLze/w18H8Os8CQBYFDAOOxFpXWYRl6zpTKBqtdGOE" "\n"
              "wDzW7WzdyB+dvW/QJ0tESHEpbHdnQJO0dPnjJcbemAjz0CLnCv7Nf5rOgjkCQE3Q" "\n"
              "izIw+/JptmvoOQyx7ixQ2mNCYmpN/Iw63gln0MHaQ5WCPUEmdYWWu3mqmbn7Deaq" "\n"
              "j233Pc4TF7b0FmnaXWsCQAVvyLVU3a9Yactb5MXaN+rEYjUW37GSo+Q1lXfm0OwF" "\n"
              "EJB7X66Bavwg4MCfpGykS71OxhTEfDu+y1gylPMCGHY="                     "\n"
              "-----END RSA PRIVATE KEY-----"                                    "\n"
            , -1), nullptr, nullptr, nullptr)
          )) ctx = nullptr;
        } else {
          if (access(K.str("ssl-crt").data(), R_OK) == -1)
            Print::logWar("UI", "Unable to read custom .crt file at " + K.str("ssl-crt"));
          if (access(K.str("ssl-key").data(), R_OK) == -1)
            Print::logWar("UI", "Unable to read custom .key file at " + K.str("ssl-key"));
          if (!SSL_CTX_use_certificate_chain_file(ctx, K.str("ssl-crt").data())
            or !SSL_CTX_use_RSAPrivateKey_file(ctx, K.str("ssl-key").data(), SSL_FILETYPE_PEM)
          ) {
            ctx = nullptr;
            Print::logWar("UI", "Unable to encrypt web clients, will fallback to plain HTTP");
          }
        }
      }
      protocol = "HTTP" + string(ctx ? 1 : 0, 'S');
      return ctx;
    };
    string onHttpRequest(const string &path, const string &auth, const string &addr) {
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
    string onMessage(string message, const string &addr) {
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
      return "";
    };
    static const string cleanAddress(string addr) {
      if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
      if (addr.length() < 7) addr.clear();
      return addr.empty() ? "unknown" : addr;
    };
} ui;

#endif
