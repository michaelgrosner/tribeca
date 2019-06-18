#ifndef K_DATA_H_
#define K_DATA_H_
//! \file
//! \brief Data transfer/transform helpers.

namespace ₿ {
  class WebSocketFrames {
    protected:
      const string frame(string data, const int &opcode, const bool &mask) const {
        const int key = mask ? rand() : 0;
        const int bit = mask ? 0x80   : 0;
        size_t pos = 0,
               len = data.length();
                                data.insert(pos++  , 1,  opcode     | 0x80);
        if      (len <= 0x7D)   data.insert(pos++  , 1,  len        | bit );
        else if (len <= 0xFFFF) data.insert(pos    , 1, (char)(0x7E | bit))
                                    .insert(pos + 1, 1, (len >>  8) & 0xFF)
                                    .insert(pos + 2, 1,  len        & 0xFF), pos += 3;
        else                    data.insert(pos    , 1, (char)(0x7F | bit))
                                    .insert(pos + 1, 4,                  0)
                                    .insert(pos + 5, 1, (len >> 24) & 0xFF)
                                    .insert(pos + 6, 1, (len >> 16) & 0xFF)
                                    .insert(pos + 7, 1, (len >>  8) & 0xFF)
                                    .insert(pos + 8, 1,  len        & 0xFF), pos += 9;
        if (mask) {             data.insert(pos    , 1, (key >> 24) & 0xFF)
                                    .insert(pos + 1, 1, (key >> 16) & 0xFF)
                                    .insert(pos + 2, 1, (key >>  8) & 0xFF)
                                    .insert(pos + 3, 1,  key        & 0xFF), pos += 4;
          for (size_t i = 0; i < len; i++)
            data.at(pos + i) ^= data.at(pos - 4 + (i % 4));
        }
        return data;
      };
      const string unframe(string &data, string &pong, bool &drop) const {
        string msg;
        const size_t max = data.length();
        if (max > 1) {
          const bool flat = (data[0] & 0x40) != 0x40,
                     mask = (data[1] & 0x80) == 0x80;
          const size_t key = mask ? 4 : 0;
          size_t                            len =    data[1] & 0x7F,          pos = key;
          if      (            len <= 0x7D)                                   pos += 2;
          else if (max > 2 and len == 0x7E) len = (((data[2] & 0xFF) <<  8)
                                                |   (data[3] & 0xFF)       ), pos += 4;
          else if (max > 8 and len == 0x7F) len = (((data[6] & 0xFF) << 24)
                                                |  ((data[7] & 0xFF) << 16)
                                                |  ((data[8] & 0xFF) <<  8)
                                                |   (data[9] & 0xFF)       ), pos += 10;
          if (!flat or pos == key)
            drop = true;
          else if (max >= pos + len) {
            if (mask)
              for (size_t i = 0; i < len; i++)
                data.at(pos + i) ^= data.at(pos - key + (i % key));
            const unsigned char opcode = data[0] & 0x0F;
            if (opcode == 0x09)
              pong += frame(data.substr(pos, len), 0x0A, !mask);
            else if (opcode == 0x02
                  or opcode == 0x08
                  or opcode == 0x0A
                  or ((data[0] & 0x80) != 0x80 and (opcode == 0x00
                                                 or opcode == 0x01))
            ) drop = opcode == 0x08;
            else
              msg = data.substr(pos, len);
            data = data.substr(pos + len);
          }
        }
        return msg;
      };
  };

  class FixFrames {
    protected:
      const string frame(string data, const string &type, const unsigned long &sequence, const string &sender, const string &target) const {
        data = "35=" + type                     + "\u0001"
               "34=" + to_string(sequence)      + "\u0001"
               "49=" + sender                   + "\u0001"
               "56=" + target                   + "\u0001"
             + data;
        data = "8=FIX.4.2"                        "\u0001"
               "9="  + to_string(data.length()) + "\u0001"
             + data;
        char ch = 0;
        for (size_t i = data.length(); i --> 0; ch += data.at(i));
        stringstream sum;
        sum << setfill('0')
            << setw(3)
            << (ch & 0xFF);
        data += "10=" + sum.str()               + "\u0001";
        return data;
      };
      const string unframe(string &data, string &pong, bool &drop) const {
        string msg;
        const size_t end = data.find("\u0001" "10=");
        if (end != string::npos and data.length() > end + 7) {
          string raw = data.substr(0, end + 8);
          data = data.substr(raw.length());
          if (raw.find("\u0001" "35=0" "\u0001") != string::npos
            or raw.find("\u0001" "35=1" "\u0001") != string::npos
          ) pong = "0";
          else if (raw.find("\u0001" "35=5" "\u0001") != string::npos) {
            pong = "5";
            drop = true;
          } else {
            size_t tok;
            while ((tok = raw.find("\u0001")) != string::npos) {
              raw.replace(raw.find("="), 1, "\":\"");
              msg += "\"" + raw.substr(0, tok + 2) + "\",";
              raw = raw.substr(tok + 3);
            }
            msg.pop_back();
            msg = "{" + msg + "}";
          }
        }
        return msg;
      };
  };

  class Curl {
    public:
      static function<void(CURL*)> global_setopt;
    private:
      class Easy {
        private:
                   CURL *curl   = nullptr;
          curl_socket_t  sockfd = 0;
        private_ref:
          string &buffer;
        public:
          Easy(string &b)
            : buffer(b)
          {};
        protected:
          void cleanup() {
            if (curl) curl_easy_cleanup(curl);
            curl   = nullptr;
            sockfd = 0;
          };
          const bool connected() const {
            return sockfd;
          };
          const CURLcode connect(const string &url, const string &header, const string &res1, const string &res2) {
            buffer.clear();
            CURLcode rc;
            if (CURLE_OK == (rc = init())) {
              global_setopt(curl);
              curl_easy_setopt(curl, CURLOPT_URL, url.data());
              curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
              if ( CURLE_OK != (rc = curl_easy_perform(curl))
                or CURLE_OK != (rc = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd))
                or CURLE_OK != (rc = send(header))
                or CURLE_OK != (rc = recv(5))
                or string::npos == buffer.find(res1)
                or string::npos == buffer.find(res2)
              ) {
                if (rc == CURLE_OK)
                  rc = CURLE_WEIRD_SERVER_REPLY;
                cleanup();
              }
              buffer.clear();
            }
            return rc;
          };
          const CURLcode receive() {
            CURLcode rc = CURLE_COULDNT_CONNECT;
            if (curl and sockfd and CURLE_OPERATION_TIMEDOUT == (rc = recv(0)))
              rc = CURLE_OK;
            if (rc != CURLE_OK)
              cleanup();
            return rc;
          };
          const CURLcode emit(const string &data) {
            CURLcode rc = CURLE_COULDNT_CONNECT;
            if (!curl or !sockfd or CURLE_OK != (rc = send(data)))
              cleanup();
            return rc;
          };
        private:
          const CURLcode init() {
            if (!curl) curl = curl_easy_init();
            else curl_easy_reset(curl);
            sockfd = 0;
            return curl
              ? CURLE_OK
              : CURLE_FAILED_INIT;
          };
          const CURLcode send(const string &data) {
            CURLcode rc;
            size_t len  = data.length(),
                   sent = 0;
            do {
              do {
                size_t n = 0;
                rc = curl_easy_send(curl, data.substr(sent).data(), len - sent, &n);
                sent += n;
                if (rc == CURLE_AGAIN and !wait(false, 5))
                  return CURLE_OPERATION_TIMEDOUT;
              } while (rc == CURLE_AGAIN);
              if (rc != CURLE_OK) break;
            } while (sent < len);
            return rc;
          };
          const CURLcode recv(const int &timeout) {
            CURLcode rc;
            for(;;) {
              char data[524288];
              size_t n;
              do {
                n = 0;
                rc = curl_easy_recv(curl, data, sizeof(data), &n);
                buffer.append(data, n);
                if (rc == CURLE_AGAIN and !wait(true, timeout))
                  return CURLE_OPERATION_TIMEDOUT;
              } while (rc == CURLE_AGAIN);
              if ((timeout and buffer.find("\r\n\r\n") != buffer.find("\u0001" "10="))
                or rc != CURLE_OK
                or n == 0
              ) break;
            }
            return rc;
          };
          const int wait(const bool &io, const int &timeout) const {
            struct timeval tv = {timeout, 10000};
            fd_set infd,
                   outfd;
            FD_ZERO(&infd);
            FD_ZERO(&outfd);
            FD_SET(sockfd, io ? &infd : &outfd);
            return select(sockfd + 1, &infd, &outfd, nullptr, &tv);
          };
      };
    public_friend:
      class Web {
        public:
          static const json xfer(const string &url, const long &timeout = 13) {
            return request(url, [&](CURL *curl) {
              curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
            });
          };
          static const json xfer(const string &url, const string &post) {
            return request(url, [&](CURL *curl) {
              struct curl_slist *h_ = nullptr;
              h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
              curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
              curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
            });
          };
          static const json request(const string &url, const function<void(CURL*)> custom_setopt) {
            static mutex waiting_reply;
            lock_guard<mutex> lock(waiting_reply);
            string reply;
            CURLcode rc = CURLE_FAILED_INIT;
            CURL *curl = curl_easy_init();
            if (curl) {
              custom_setopt(curl);
              global_setopt(curl);
              curl_easy_setopt(curl, CURLOPT_URL, url.data());
              curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write);
              curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
              rc = curl_easy_perform(curl);
              curl_easy_cleanup(curl);
            }
            return rc == CURLE_OK
              ? (json::accept(reply)
                  ? json::parse(reply)
                  : json::object()
                )
              : (json){ {"error", string("CURL Request Error: ") + curl_easy_strerror(rc)} };
          };
        private:
          static size_t write(void *buf, size_t size, size_t nmemb, void *reply) {
            ((string*)reply)->append((char*)buf, size *= nmemb);
            return size;
          };
      };
      class WebSocket: public Easy,
                       public WebSocketFrames {
        private:
          string buffer;
        public:
          WebSocket()
            : Easy(buffer)
          {};
        protected:
          const CURLcode connect(const string &uri) {
            CURLcode rc = CURLE_URL_MALFORMAT;
            CURLU *url = curl_url();
            char *host,
                 *port,
                 *path;
            if (  !curl_url_set(url, CURLUPART_URL, ("http" + uri.substr(2)).data(), 0)
              and !curl_url_get(url, CURLUPART_HOST, &host, 0)
              and !curl_url_get(url, CURLUPART_PORT, &port, CURLU_DEFAULT_PORT)
              and !curl_url_get(url, CURLUPART_PATH, &path, 0)
            ) rc = CURLE_OK;
            curl_url_cleanup(url);
            return rc ?: Easy::connect(
              "http" + uri.substr(2),
              "GET " + string(path) + " HTTP/1.1"
                "\r\n" "Host: " + string(host) + ":" + string(port) +
                "\r\n" "Upgrade: websocket"
                "\r\n" "Connection: Upgrade"
                "\r\n" "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw=="
                "\r\n" "Sec-WebSocket-Version: 13"
                "\r\n"
                "\r\n",
              "HTTP/1.1 101 Switching Protocols",
              "HSmrc0sMlYUkAGmm5OPpG2HaGWk="
            );
          };
          const CURLcode emit(const string &data, const int &opcode) {
            return Easy::emit(frame(data, opcode, true));
          };
          const string unframe() {
            string pong;
            bool drop = false;
            const string msg = WebSocketFrames::unframe(buffer, pong, drop);
            if (!pong.empty()) Easy::emit(pong);
            if (drop) cleanup();
            return msg;
          };
      };
      class FixSocket: public Easy,
                       public FixFrames {
        private:
          string buffer;
          unsigned long sequence = 0;
        private_ref:
          const string &sender,
                       &target;
        public:
          FixSocket(const string &s, const string &t)
            : Easy(buffer)
            , sender(s)
            , target(t)
          {};
        protected:
          const CURLcode connect(const string &uri, const string &logon) {
            return Easy::connect(
              "https://" + uri,
              frame(logon, "A", sequence = 1, sender, target),
              "8=FIX.4.2" "\u0001",
              "\u0001" "35=A" "\u0001"
            );
          };
          const CURLcode emit(const string &data, const string &type) {
            return Easy::emit(frame(data, type, ++sequence, sender, target));
          };
          const string unframe() {
            string pong;
            bool drop = false;
            const string msg = FixFrames::unframe(buffer, pong, drop);
            if (!pong.empty()) emit("", pong);
            if (drop) cleanup();
            return msg;
          };
          const unsigned long last() const {
            return sequence;
          };
      };
  };

  class WebServer: public WebSocketFrames {
    private:
      class Socket {
        public:
          curl_socket_t sockfd = 0;
        public:
          Socket(const curl_socket_t &s)
            : sockfd(s)
          {};
          void shutdown() {
#ifdef _WIN32
            closesocket(sockfd);
#else
            close(sockfd);
#endif
            sockfd = 0;
          };
          void cork(const int &enable) const {
#if defined(TCP_CORK)
            setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &enable, sizeof(int));
#elif defined(TCP_NOPUSH)
            setsockopt(sockfd, IPPROTO_TCP, TCP_NOPUSH, &enable, sizeof(int));
            if (!enable) ::send(sockfd, "", 0, MSG_NOSIGNAL);
#endif
          };
      };
      class Frontend: public Socket {
        public:
             SSL *ssl  = nullptr;
           Clock  time = 0;
          string  addr,
                  out,
                  in;
        public:
          Frontend(const curl_socket_t &s, SSL *l)
            : Socket(s)
            , ssl(l)
            , time(Tstamp)
          {};
          void shutdown() {
            if (ssl) {
              SSL_shutdown(ssl);
              SSL_free(ssl);
            }
            Socket::shutdown();
          };
          const string address() const {
            string addr;
#ifndef _WIN32
            sockaddr_storage ss;
            socklen_t len = sizeof(ss);
            if (getpeername(sockfd, (sockaddr*)&ss, &len) != -1) {
              char buf[INET6_ADDRSTRLEN];
              if (ss.ss_family == AF_INET) {
                auto *ipv4 = (sockaddr_in*)&ss;
                inet_ntop(AF_INET, &ipv4->sin_addr, buf, sizeof(buf));
              } else {
                auto *ipv6 = (sockaddr_in6*)&ss;                                //-V641
                inet_ntop(AF_INET6, &ipv6->sin6_addr, buf, sizeof(buf));
              }
              addr = string(buf);
              if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
              if (addr.length() < 7) addr.clear();
            }
#endif
            return addr.empty() ? "unknown" : addr;
          };
          void io() {
            if (ssl) {
              if (!out.empty()) {
                cork(1);
                int n = SSL_write(ssl, out.data(), (int)out.length());
                switch (SSL_get_error(ssl, n)) {
                  case SSL_ERROR_WANT_READ:
                  case SSL_ERROR_WANT_WRITE:  break;
                  case SSL_ERROR_NONE:        out = out.substr(n);
                  case SSL_ERROR_ZERO_RETURN: if (!time) break;
                  default:                    shutdown();
                                              return;
                }
                cork(0);
              }
              do {
                char data[1024];
                int n = SSL_read(ssl, data, sizeof(data));
                switch (SSL_get_error(ssl, n)) {
                  case SSL_ERROR_NONE:        in.append(data, n);
                  case SSL_ERROR_WANT_READ:
                  case SSL_ERROR_WANT_WRITE:
                  case SSL_ERROR_ZERO_RETURN: break;
                  default:                    if (!time) break;
                                              shutdown();
                                              return;
                }
              } while (SSL_pending(ssl));
            } else {
              if (!out.empty()) {
                cork(1);
                ssize_t n = ::send(sockfd, out.data(), out.length(), MSG_NOSIGNAL);
                if (n > 0) out = out.substr(n);
                if ((!time and n < 0)
                  or (time and out.empty())
                ) {
                  shutdown();
                  return;
                }
                cork(0);
              }
              char data[1024];
              ssize_t n = ::recv(sockfd, data, sizeof(data), 0);
              if (n > 0) in.append(data, n);
            }
          };
      };
    public_friend:
      class Backend: public Socket {
        public:
                   SSL_CTX *ctx = nullptr;
          vector<Frontend>  requests,
                            sockets;
        public:
          Backend()
            : Socket(0)
          {};
          const bool listen(const string &inet, const int &port, const bool &ipv6) {
            if (sockfd) return false;
            struct addrinfo  hints,
                            *result,
                            *rp;
            memset(&hints, 0, sizeof(addrinfo));
            hints.ai_flags    = AI_PASSIVE;
            hints.ai_family   = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            if (!getaddrinfo(inet.empty() ? nullptr : inet.data(), to_string(port).data(), &hints, &result)) {
              if (ipv6)
                for (rp = result; rp and !sockfd; rp = sockfd ? rp : rp->ai_next)
                  if (rp->ai_family == AF_INET6)
                    socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
              if (!sockfd)
                for (rp = result; rp and !sockfd; rp = sockfd ? rp : rp->ai_next)
                  if (rp->ai_family == AF_INET)
                    socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
              if (sockfd) {
                int enabled = 1;
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int));
                if (::bind(sockfd, rp->ai_addr, rp->ai_addrlen) || ::listen(sockfd, 512))
                  shutdown();
              }
              freeaddrinfo(result);
            }
            return sockfd;
          };
          const vector<string> ssl_context(const string &crt, const string &key) {
            vector<string> warn;
            ctx = SSL_CTX_new(SSLv23_server_method());
            if (ctx) {
              SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv3);
              if (crt.empty() or key.empty()) {
                if (!crt.empty())
                  warn.emplace_back("Ignored .crt file because .key file is missing");
                if (!key.empty())
                  warn.emplace_back("Ignored .key file because .crt file is missing");
                warn.emplace_back("Connected web clients will enjoy unsecure SSL encryption..\n"
                  "(because the private key is visible in the source!). See --help argument to setup your own SSL");
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
                if (access(crt.data(), R_OK) == -1)
                  warn.emplace_back("Unable to read SSL .crt file at " + crt);
                if (access(key.data(), R_OK) == -1)
                  warn.emplace_back("Unable to read SSL .key file at " + key);
                if (!SSL_CTX_use_certificate_chain_file(ctx, crt.data())
                  or !SSL_CTX_use_RSAPrivateKey_file(ctx, key.data(), SSL_FILETYPE_PEM)
                ) {
                  ctx = nullptr;
                  warn.emplace_back("Unable to encrypt web clients, will fallback to plain text");
                }
              }
            }
            return warn;
          };
          const bool accept_request() {
            curl_socket_t clientfd;
#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
            clientfd = accept4(sockfd, nullptr, nullptr, SOCK_CLOEXEC | SOCK_NONBLOCK);
#else
            clientfd = accept(sockfd, nullptr, nullptr);
#endif
#ifdef __APPLE__
            if (clientfd != -1) {
                int noSigpipe = 1;
                setsockopt(clientfd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(int));
            }
#endif
            if (clientfd != -1) {
              SSL *ssl = nullptr;
              if (ctx) {
                ssl = SSL_new(ctx);
                SSL_set_accept_state(ssl);
                SSL_set_fd(ssl, clientfd);
                SSL_set_mode(ssl, SSL_MODE_RELEASE_BUFFERS);
              }
              requests.push_back(Frontend(clientfd, ssl));
            }
            return clientfd > 0;
          };
        private:
          void socket(const int &domain, const int &type, const int &protocol) {
            const int flags =
    #if defined(SOCK_CLOEXEC) and defined(SOCK_NONBLOCK)
            SOCK_CLOEXEC | SOCK_NONBLOCK;
    #else
            0
    #endif
            ;
            sockfd = ::socket(domain, type | flags, protocol);
    #ifdef __APPLE__
            if (sockfd != -1) {
              int noSigpipe = 1;
              setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(int));
            }
    #endif
            if (sockfd == -1) sockfd = 0;
          };
      };
    protected:
      const string unframe(Frontend &client) {
        bool drop = false;
        const string msg = WebSocketFrames::unframe(client.in, client.out, drop);
        if (drop) client.shutdown();
        return msg;
      };
      const string document(const string &content, const unsigned int &code, const string &type) const {
        string headers;
        if      (code == 200) headers = "HTTP/1.1 200 OK"
                                        "\r\n" "Connection: keep-alive"
                                        "\r\n" "Accept-Ranges: bytes"
                                        "\r\n" "Vary: Accept-Encoding"
                                        "\r\n" "Cache-Control: public, max-age=0";
        else if (code == 401) headers = "HTTP/1.1 401 Unauthorized"
                                        "\r\n" "Connection: keep-alive"
                                        "\r\n" "Accept-Ranges: bytes"
                                        "\r\n" "Vary: Accept-Encoding"
                                        "\r\n" "WWW-Authenticate: Basic realm=\"Basic Authorization\"";
        else if (code == 403) headers = "HTTP/1.1 403 Forbidden"
                                        "\r\n" "Connection: keep-alive"
                                        "\r\n" "Accept-Ranges: bytes"
                                        "\r\n" "Vary: Accept-Encoding";
        else if (code == 418) headers = "HTTP/1.1 418 I'm a teapot";
        else                  headers = "HTTP/1.1 404 Not Found";
        return headers
             + string((content.length() > 2 and (content.substr(0, 2) == "PK" or (
                 content.at(0) == '\x1F' and content.at(1) == '\x8B'
             ))) ? "\r\n" "Content-Encoding: gzip" : "")
             + "\r\n" "Content-Type: "   + type
             + "\r\n" "Content-Length: " + to_string(content.length())
             + "\r\n"
               "\r\n"
             + content;
      };
  };

  class Text {
    public:
      static string strL(string input) {
        transform(input.begin(), input.end(), input.begin(), ::tolower);
        return input;
      };
      static string strU(string input) {
        transform(input.begin(), input.end(), input.begin(), ::toupper);
        return input;
      };
      static string B64(const string &input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        BIO_set_close(bio, BIO_CLOSE);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, input.data(), input.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        const string output(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        return output;
      };
      static string B64_decode(const string &input) {
        BIO *bio, *b64;
        char output[input.length()];
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new_mem_buf(input.data(), input.length());
        bio = BIO_push(b64, bio);
        BIO_set_close(bio, BIO_CLOSE);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        int len = BIO_read(bio, output, input.length());
        BIO_free_all(bio);
        return string(output, len);
      };
      static string SHA1(const string &input, const bool &hex = false) {
        return SHA(input, hex, ::SHA1, SHA_DIGEST_LENGTH);
      };
      static string SHA256(const string &input, const bool &hex = false) {
        return SHA(input, hex, ::SHA256, SHA256_DIGEST_LENGTH);
      };
      static string SHA512(const string &input) {
        return SHA(input, false, ::SHA512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC1(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha1, SHA_DIGEST_LENGTH);
      };
      static string HMAC256(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha256, SHA256_DIGEST_LENGTH);
      };
      static string HMAC512(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC384(const string &key, const string &input, const bool &hex = false) {
        return HMAC(key, input, hex, EVP_sha384, SHA384_DIGEST_LENGTH);
      };
    private:
      static string SHA(
        const string  &input,
        const bool    &hex,
        unsigned char *(*md)(const unsigned char*, size_t, unsigned char*),
        const int     &digest_len
      ) {
        unsigned char digest[digest_len];
        md((unsigned char*)input.data(), input.length(), (unsigned char*)&digest);
        char output[digest_len * 2 + 1];
        for (unsigned int i = 0; i < digest_len; i++)
          sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
        return hex ? HEX(output) : output;
      };
      static string HMAC(
        const string &key,
        const string &input,
        const bool   &hex,
        const EVP_MD *(evp_md)(),
        const int    &digest_len
      ) {
        unsigned char* digest;
        digest = ::HMAC(
          evp_md(),
          input.data(), input.length(),
          (unsigned char*)key.data(), key.length(),
          nullptr, nullptr
        );
        char output[digest_len * 2 + 1];
        for (unsigned int i = 0; i < digest_len; i++)
          sprintf(&output[i * 2], "%02x", (unsigned int)digest[i]);
        return hex ? HEX(output) : output;
      };
      static string HEX(const string &input) {
        const unsigned int len = input.length();
        string output;
        for (unsigned int i = 0; i < len; i += 2)
          output.push_back(
            (char)(int)strtol(input.substr(i, 2).data(), nullptr, 16)
          );
        return output;
      };
  };

  class Random {
    public:
      static const unsigned long long int64() {
        static random_device rd;
        static mt19937_64 gen(rd());
        return uniform_int_distribution<unsigned long long>()(gen);
      };
      static const RandId int45Id() {
        return to_string(int64()).substr(0, 10);
      };
      static const RandId int32Id() {
        return to_string(int64()).substr(0,  8);
      };
      static const RandId char16Id() {
        string id = string(16, ' ');
        for (auto &it : id) {
         const int offset = int64() % (26 + 26 + 10);
         if      (offset < 26)      it = 'a' + offset;
         else if (offset < 26 + 26) it = 'A' + offset - 26;
         else                       it = '0' + offset - 26 - 26;
        }
        return id;
      };
      static const RandId uuid36Id() {
        string uuid = string(36, ' ');
        uuid[8]  =
        uuid[13] =
        uuid[18] =
        uuid[23] = '-';
        uuid[14] = '4';
        unsigned long long rnd = int64();
        for (auto &it : uuid)
          if (it == ' ') {
            if (rnd <= 0x02) rnd = 0x2000000 + (int64() * 0x1000000) | 0;
            rnd >>= 4;
            const int offset = (uuid[17] != ' ' and uuid[19] == ' ')
              ? ((rnd & 0xf) & 0x3) | 0x8
              : rnd & 0xf;
            if (offset < 10) it = '0' + offset;
            else             it = 'a' + offset - 10;
          }
        return uuid;
      };
      static const RandId uuid32Id() {
        RandId uuid = uuid36Id();
        uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());
        return uuid;
      }
  };
}

#endif
