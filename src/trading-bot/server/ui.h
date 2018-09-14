#ifndef K_UI_H_
#define K_UI_H_

extern const char _www_html_index,     _www_ico_favicon,     _www_css_base,
                  _www_gzip_bomb,      _www_mp3_audio_0,     _www_css_light,
                  _www_js_client,      _www_mp3_audio_1,     _www_css_dark;
extern const  int _www_html_index_len, _www_ico_favicon_len, _www_css_base_len,
                  _www_gzip_bomb_len,  _www_mp3_audio_0_len, _www_css_light_len,
                  _www_js_client_len,  _www_mp3_audio_1_len, _www_css_dark_len;
namespace K {
  class UI: public Klass,
            public Client { public: UI() { client = this; };
    private:
      int connections = 0;
      string B64auth = "";
      unordered_map<char, function<json()>> hello;
      unordered_map<char, function<void(json&)>> kisses;
      unordered_map<mMatter, string> queue;
    protected:
      void load() {
        if (!socket
          or args.user.empty()
          or args.pass.empty()
        ) return;
        B64auth = "Basic " + mText::oB64(args.user + ':' + args.pass);
      };
      void waitData() {
        if (!socket) return;
        if (!socket->listen(
          mREST::inet, args.port, uS::TLS::Context(sslContext()), 0,
          &socket->getDefaultGroup<uWS::SERVER>()
        )) {
          const string netstat = mCommand::netstat(args.port);
          exit(screen->error("UI", "Unable to listen to UI port number " + to_string(args.port) + ", "
            + (netstat.empty() ? "try another network interface" : "seems already in use by:\n" + netstat)
          ));
        }
        auto client = &socket->getDefaultGroup<uWS::SERVER>();
        client->onConnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, uWS::HttpRequest req) {
          onConnection();
          const string addr = cleanAddress(webSocket->getAddress().address);
          screen->logUIsess(connections, addr);
          if (connections > args.maxAdmins) {
            screen->log("UI", "--client-limit=" + to_string(args.maxAdmins) + " reached by", addr);
            webSocket->close();
          }
        });
        client->onDisconnection([&](uWS::WebSocket<uWS::SERVER> *webSocket, int code, char *message, size_t length) {
          onDisconnection();
          screen->logUIsess(connections, cleanAddress(webSocket->getAddress().address));
        });
        client->onHttpRequest([&](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
          const string response = onHttpRequest(
            req.getUrl().toString(),
            req.getMethod() == uWS::HttpMethod::METHOD_GET,
            req.getHeader("authorization").toString(),
            cleanAddress(res->getHttpSocket()->getAddress().address)
          );
          if (!response.empty()) res->write(response.data(), response.length());
        });
        client->onMessage([&](uWS::WebSocket<uWS::SERVER> *webSocket, const char *message, size_t length, uWS::OpCode opCode) {
          if (length < 2) return;
          const string response = onMessage(
            string(message, length),
            !args.whitelist.empty()
              ? cleanAddress(webSocket->getAddress().address)
              : "unknown"
          );
          if (!response.empty())
            webSocket->send(
              response.data(),
              response.substr(0, 2) == "PK"
                ? uWS::OpCode::BINARY
                : uWS::OpCode::TEXT
            );
        });
        broadcast = [this, client](const mMatter &type, string msg) {
          msg.insert(msg.begin(), (char)type);
          msg.insert(msg.begin(), (char)mPortal::Kiss);
          events->deferred([client, msg]() {
            client->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
          });
        };
      };
      void run() {
        send = send_nowhere;
      };
    public:
      void welcome(mToClient &data) {
        const char type = (char)data.about();
        if (hello.find(type) != hello.end())
          exit(screen->error("UI", string("Too many handlers for \"") + type + "\" welcome event"));
        hello[type] = [&]() { return data.hello(); };
        sendAsync(data);
      };
      void clickme(mFromClient &data, function<void(const json&)> fn) {
        const char type = (char)data.about();
        if (kisses.find(type) != kisses.end())
          exit(screen->error("UI", string("Too many handlers for \"") + type + "\" clickme event"));
        kisses[type] = [&data, fn](json &butterfly) {
          data.kiss(&butterfly);
          if (!butterfly.is_null())
            fn(butterfly);
        };
      };
      void timer_Xs() {
        for (unordered_map<mMatter, string>::value_type &it : queue)
          broadcast(it.first, it.second);
        queue.clear();
      };
    private:
      void sendAsync(mToClient &data) {
        data.send = [&]() {
          send(data);
        };
      };
      SSL_CTX *sslContext() {
        SSL_CTX *context = nullptr;
        if (!args.withoutSSL and (context = SSL_CTX_new(SSLv23_server_method()))) {
          SSL_CTX_set_options(context, SSL_OP_NO_SSLv3);
          if (args.pathSSLcert.empty() or args.pathSSLkey.empty()) {
            if (!args.pathSSLcert.empty())
              screen->logWar("UI", "Ignored --ssl-crt because --ssl-key is missing");
            if (!args.pathSSLkey.empty())
              screen->logWar("UI", "Ignored --ssl-key because --ssl-crt is missing");
            screen->logWar("UI", "Connected web clients will enjoy unsecure SSL encryption..\n"
              "(because the private key is visible in the source!), consider --ssl-crt and --ssl-key arguments");
            const char *cert = "-----BEGIN CERTIFICATE-----\n"
                               "MIICATCCAWoCCQCiyDyPL5ov3zANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\n"
                               "VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\n"
                               "cyBQdHkgTHRkMB4XDTE2MTIyMjIxMDMyNVoXDTE3MTIyMjIxMDMyNVowRTELMAkG\n"
                               "A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\n"
                               "IFdpZGdpdHMgUHR5IEx0ZDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAunyx\n"
                               "1lNsHkMmCa24Ns9xgJAwV3A6/Jg/S5jPCETmjPRMXqAp89fShZxN2b/2FVtU7q/N\n"
                               "EtNpPyEhfAhPwYrkHCtip/RmZ/b6qY2Cx6otFIsuwO8aUV27CetpoM8TAQSuufcS\n"
                               "jcZD9pCAa9GM/yWeqc45su9qBBmLnAKYuYUeDQUCAwEAATANBgkqhkiG9w0BAQsF\n"
                               "AAOBgQAeZo4zCfnq5/6gFzoNDKg8DayoMnCtbxM6RkJ8b/MIZT5p6P7OcKNJmi1o\n"
                               "XD2evdxNrY0ObQ32dpiLqSS1JWL8bPqloGJBNkSPi3I+eBoJSE7/7HOroLNbp6nS\n"
                               "aaec6n+OlGhhjxn0DzYiYsVBUsokKSEJmHzoLHo3ZestTTqUwg==\n"
                               "-----END CERTIFICATE-----\n";
            const char *pkey = "-----BEGIN RSA PRIVATE KEY-----\n"
                               "MIICXAIBAAKBgQC6fLHWU2weQyYJrbg2z3GAkDBXcDr8mD9LmM8IROaM9ExeoCnz\n"
                               "19KFnE3Zv/YVW1Tur80S02k/ISF8CE/BiuQcK2Kn9GZn9vqpjYLHqi0Uiy7A7xpR\n"
                               "XbsJ62mgzxMBBK659xKNxkP2kIBr0Yz/JZ6pzjmy72oEGYucApi5hR4NBQIDAQAB\n"
                               "AoGBAJi9OrbtOreKjeQNebzCqRcAgeeLz3RFiknzjVYbgK1gBhDWo6XJVe8C9yxq\n"
                               "sjYJyQV5zcAmkaQYEaHR+OjvRiZ4UmXbItukOD+dnq7xs69n3w54FfANjkurdL2M\n"
                               "fPAQm/GJT4TSBDIr7eJQPOrork9uxQStwADTqvklVlKm2YldAkEA80ZYaLrGOBbz\n"
                               "5871ewKxtVJNCCmXdYUwq7nI/lqsLBZnB+wiwnQ+3tgfi4YoUoTnv0hIIwkyLYl9\n"
                               "Z2wqensf6wJBAMQ96gUGnIcYJzknB5CYDNQalcvvTx7tLtgRXDf47bQJ3X/Q5k/t\n"
                               "yDlByUBqvYVShXWs+d4ynNKLze/w18H8Os8CQBYFDAOOxFpXWYRl6zpTKBqtdGOE\n"
                               "wDzW7WzdyB+dvW/QJ0tESHEpbHdnQJO0dPnjJcbemAjz0CLnCv7Nf5rOgjkCQE3Q\n"
                               "izIw+/JptmvoOQyx7ixQ2mNCYmpN/Iw63gln0MHaQ5WCPUEmdYWWu3mqmbn7Deaq\n"
                               "j233Pc4TF7b0FmnaXWsCQAVvyLVU3a9Yactb5MXaN+rEYjUW37GSo+Q1lXfm0OwF\n"
                               "EJB7X66Bavwg4MCfpGykS71OxhTEfDu+y1gylPMCGHY=\n"
                               "-----END RSA PRIVATE KEY-----\n";
            BIO *cbio = BIO_new_mem_buf((void*)cert, -1),
                *kbio = BIO_new_mem_buf((void*)pkey, -1);
            if (SSL_CTX_use_certificate(context, PEM_read_bio_X509(cbio, NULL, 0, NULL)) != 1
              or SSL_CTX_use_RSAPrivateKey(context, PEM_read_bio_RSAPrivateKey(kbio, NULL, 0, NULL)) != 1
            ) context = nullptr;
          } else {
            if (access(args.pathSSLcert.data(), R_OK) == -1)
              screen->logWar("UI", "Unable to read custom .crt file at " + args.pathSSLcert);
            if (access(args.pathSSLkey.data(), R_OK) == -1)
              screen->logWar("UI", "Unable to read custom .key file at " + args.pathSSLkey);
            if (SSL_CTX_use_certificate_chain_file(context, args.pathSSLcert.data()) != 1
              or SSL_CTX_use_RSAPrivateKey_file(context, args.pathSSLkey.data(), SSL_FILETYPE_PEM) != 1
            ) {
              context = nullptr;
              screen->logWar("UI", "Unable to encrypt web clients, will fallback to plain HTTP");
            }
          }
        }
        screen->logUI("HTTP" + string(context ? 1 : 0, 'S'));
        return context;
      };
      function<void(const mMatter &type, string msg)> broadcast = [](const mMatter &type, string msg) {};
      function<void(const mToClient&)> send;
      function<void(const mToClient&)> send_nowhere = [](const mToClient &data) {};
      function<void(const mToClient&)> send_somewhere = [&](const mToClient &data) {
        if (data.realtime())
          broadcast(data.about(), data.blob().dump());
        else queue[data.about()] = data.blob().dump();
      };
      void onConnection() {
        if (!connections++) send = send_somewhere;
      };
      void onDisconnection() {
        if (!--connections) send = send_nowhere;
      };
      string onHttpRequest(const string &path, const bool &get, const string &auth, const string &addr) {
        string document,
               content;
        if (addr != "unknown" and !args.whitelist.empty() and args.whitelist.find(addr) == string::npos) {
          screen->log("UI", "dropping gzip bomb on", addr);
          document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\nContent-Encoding: gzip\r\n";
          content = string(&_www_gzip_bomb, _www_gzip_bomb_len);
        } else if (!B64auth.empty() and auth.empty()) {
          screen->log("UI", "authorization attempt from", addr);
          document = "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"Basic Authorization\"\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\n";
        } else if (!B64auth.empty() and auth != B64auth) {
          screen->log("UI", "authorization failed from", addr);
          document = "HTTP/1.1 403 Forbidden\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nContent-Type:text/plain; charset=UTF-8\r\n";
        } else if (get) {
          document = "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\nVary: Accept-Encoding\r\nCache-Control: public, max-age=0\r\n";
          const string leaf = path.substr(path.find_last_of('.')+1);
          if (leaf == "/") {
            document += "Content-Type: text/html; charset=UTF-8\r\n";
            if (connections < args.maxAdmins) {
              screen->log("UI", "authorization success from", addr);
              content = string(&_www_html_index, _www_html_index_len);
            } else {
              screen->log("UI", "--client-limit=" + to_string(args.maxAdmins) + " reached by", addr);
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
          if (content.empty())
            if (mRandom::int64() % 21) {
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
      string onMessage(const string &message, const string &addr) {
        if (addr != "unknown" and args.whitelist.find(addr) == string::npos)
          return string(&_www_gzip_bomb, _www_gzip_bomb_len);
        if (mPortal::Hello == (mPortal)message.at(0) and hello.find(message.at(1)) != hello.end()) {
          json reply = hello.at(message.at(1))();
          if (!reply.is_null())
            return message.substr(0, 2) + reply.dump();
        } else if (mPortal::Kiss == (mPortal)message.at(0) and kisses.find(message.at(1)) != kisses.end()) {
          json butterfly = json::accept(message.substr(2))
            ? json::parse(message.substr(2))
            : json::object();
          for (json::iterator it = butterfly.begin(); it != butterfly.end();)
            if (it.value().is_null()) it = butterfly.erase(it); else ++it;
          kisses.at(message.at(1))(butterfly);
        }
        return "";
      };
      static string cleanAddress(string addr) {
        if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
        if (addr.length() < 7) addr.clear();
        return addr.empty() ? "unknown" : addr;
      };
  };
}

#endif
