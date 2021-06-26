//! \file
//! \brief Data transport/transform helpers.

namespace ₿ {
  class WebSocketFrames {
    protected:
      string frame(string data, const int &opcode, const bool &mask) const {
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
      string unframe(string &data, string &pong, bool &drop) const {
        string msg;
        const size_t max = data.length();
        if (max > 1) {
          const bool flat = (data[0] & 0x40) != 0x40,
                     text = (data[0] & 0x80) == 0x80,
                     mask = (data[1] & 0x80) == 0x80;
          const size_t key = mask ? 4 : 0;
          size_t                            len =    data[1] & 0x7F         , pos = key;
          if      (            len <= 0x7D)                                   pos += 2;
          else if (max > 2 and len == 0x7E) len = (((data[2] & 0xFF) <<  8)
                                                |   (data[3] & 0xFF)       ), pos += 4;
          else if (max > 8 and len == 0x7F) len = (((data[6] & 0xFF) << 24)
                                                |  ((data[7] & 0xFF) << 16)
                                                |  ((data[8] & 0xFF) <<  8)
                                                |   (data[9] & 0xFF)       ), pos += 10;
          if (flat and text and pos > key) {
            if (max >= pos + len) {
              if (mask)
                for (size_t i = 0; i < len; i++)
                  data.at(pos + i) ^= data.at(pos - key + (i % key));
              const unsigned char opcode = data[0] & 0x0F;
              if      (opcode == 0x01)
                msg = data.substr(pos, len);
              else if (opcode == 0x09)
                pong += frame(data.substr(pos, len), 0x0A, !mask);
              else if (opcode != 0x0A)
                drop = true;
              data = data.substr(pos + len);
            }
          } else drop = true;
        }
        return msg;
      };
  };

  class FixFrames {
    protected:
      const string  target;
    private_ref:
      const string &sender;
    public:
      FixFrames(const string &t, const string &s)
        : target(t)
        , sender(s)
      {};
    protected:
      string frame(string data, const string &type, const unsigned long &sequence) const {
        data = "35=" + type                     + "\u0001"
               "34=" + to_string(sequence)      + "\u0001"
               "49=" + sender                   + "\u0001"
               "56=" + target                   + "\u0001"
             + data;
        data = "8=FIX.4.2"                        "\u0001"
               "9="  + to_string(data.length()) + "\u0001"
             + data;
        char ch = 0;
        for (
          size_t i  =  data.length();
                 i --> 0;
          ch += data.at(i)
        );
        stringstream sum;
        sum << setfill('0')
            << setw(3)
            << (ch & 0xFF);
        data += "10=" + sum.str()               + "\u0001";
        return data;
      };
      string unframe(string &data, string &pong, bool &drop) const {
        string msg;
        size_t end = data.find("\u0001" "10=");
        if (end != string::npos and data.length() > end + 7) {
          string raw = data.substr(0, end + 8);
          data = data.substr(raw.length());
          if      (raw.find("\u0001" "35=0" "\u0001") != string::npos
            or     raw.find("\u0001" "35=1" "\u0001") != string::npos
          ) pong = "0";
          else if (raw.find("\u0001" "35=5" "\u0001") != string::npos)
            pong = "5", drop = true;
          else {
            while ((end = raw.find("\u0001")) != string::npos) {
              raw.replace(raw.find("="), 1, "\":\"");
              msg += "\"" + raw.substr(0, end + 2) + "\",";
              raw = raw.substr(end + 3);
            }
            msg.pop_back();
            msg = "{" + msg + "}";
          }
        }
        return msg;
      };
  };

  class Loop {
    public_friend:
      using TimeEvent = function<void(const unsigned int&)>;
      class Timer {
        private:
                  unsigned int tick  = 0;
          mutable unsigned int ticks = 300;
             vector<TimeEvent> jobs;
        public:
          void ticks_factor(const unsigned int &factor) const {
            ticks = 300 * (factor ?: 1);
          };
          void timer_1s() {
            for (const auto &it : jobs) it(tick);
            tick = (tick + 1) % ticks;
          };
          void push_back(const TimeEvent &data) {
            jobs.push_back(data);
          };
      };
      class Async {
        public_friend:
          template<typename T> class Event {
            private_friend:
              class Wakeup {
                private:
                  Async *const event = nullptr;
                public:
                  Wakeup(Async *const e)
                    : event(e)
                  {};
                  ~Wakeup()
                  {
                    event->wakeup();
                  };
              };
            public:
              function<void(const T&)> write = nullptr;
            private:
                              Async *event = nullptr;
              function<vector<T>()>  job   = nullptr;
                  future<vector<T>>  data;
            public:
              void try_write(const T &rawdata) const {
                if (write) write(rawdata);
              };
              void wait_for(Loop *const loop, const function<vector<T>()> j) {
                job = j;
                event = loop->async([&]() {
                  if (data.valid())
                    for (const T &it : data.get()) try_write(it);
                });
              };
              void ask_for() {
                if (!data.valid())
                  data = ::async(launch::async, [&]() {
                    Wakeup again(event);
                    return job();
                  });
              };
          };
        private:
          function<void()> job = nullptr;
        public:
          Async(const function<void()> data)
            : job(data)
          {};
          virtual void wakeup() {};
          void ready() {
            job();
          };
          void link(const function<void()> &data) {
            job = data;
          };
      };
      class Poll: public Async {
        protected:
          curl_socket_t sockfd = 0;
        public:
          Poll(const curl_socket_t &s)
            : Async(nullptr)
            , sockfd(s)
          {};
          virtual void start(const curl_socket_t&, const function<void()>&)  = 0;
          virtual void stop()                                                = 0;
        protected:
          virtual void change(const int&, const function<void()>& = nullptr) = 0;
      };
    public:
      virtual          void  timer_ticks_factor(const unsigned int&) const = 0;
      virtual          void  timer_1s(const TimeEvent&)                    = 0;
      virtual         Async *async(const function<void()>&)                = 0;
      virtual curl_socket_t  poll()                                        = 0;
      virtual          void  walk()                                        = 0;
      virtual          void  end()                                         = 0;
  };
#if defined _WIN32 or defined __APPLE__
  class Events: public Loop {
    public_friend:
      class Timer: public Loop::Timer {
        public:
          uv_timer_t event;
        public:
          Timer()
            : event()
          {
            event.data = this;
            uv_timer_init(uv_default_loop(), &event);
            uv_timer_start(&event, [](uv_timer_t *event) {
              ((Timer*)event->data)->timer_1s();
            }, 0, 1e+3);
          };
      };
      class Async: public Loop::Async {
        public:
          uv_async_t event;
        public:
          Async(const function<void()> &data)
            : Loop::Async(data)
            , event()
          {
            event.data = this;
            uv_async_init(uv_default_loop(), &event, [](uv_async_t *event) {
              ((Async*)event->data)->ready();
            });
          };
          void wakeup() override {
            uv_async_send(&event);
          };
      };
      class Poll: public Loop::Poll {
        protected:
          const int EPOLLIN  = UV_READABLE;
          const int EPOLLOUT = UV_WRITABLE;
        private:
          uv_poll_t event;
        public:
          Poll(const curl_socket_t &s = 0)
            : Loop::Poll(s)
            , event()
          {};
          void start(const curl_socket_t&, const function<void()> &data) override {
            event.data = this;
            uv_poll_init_socket(uv_default_loop(), &event, sockfd);
            change(EPOLLIN, data);
          };
          void stop() override {
            if (event.data) {
              uv_poll_stop(&event);
              uv_close((uv_handle_t*)&event, [](uv_handle_t*) { });
              if (sockfd) {
                BIO_closesocket(sockfd);
                sockfd = 0;
              }
            }
          };
        protected:
          void change(const int &events, const function<void()> &data = nullptr) override {
            if (event.data) {
              if (data) link(data);
              if (!uv_is_closing((uv_handle_t*)&event))
                uv_poll_start(&event, events, [](uv_poll_t *event, int, int) {
                  ((Poll*)event->data)->ready();
                });
            }
          };
      };
    private:
            Timer timer;
      list<Async> events;
    public:
      void timer_ticks_factor(const unsigned int &factor) const override {
        timer.ticks_factor(factor);
      };
      void timer_1s(const TimeEvent &data) override {
        timer.push_back(data);
      };
      Loop::Async *async(const function<void()> &data) override {
        events.emplace_back(data);
        return &events.back();
      };
      curl_socket_t poll() override {
        return 0;
      };
      void walk() override {
        uv_run(uv_default_loop(), UV_RUN_DEFAULT);
      };
      void end() override {
        uv_timer_stop(&timer.event);
        uv_close((uv_handle_t*)&timer.event, [](uv_handle_t*){ });
        for (auto &it : events)
          uv_close((uv_handle_t*)&it.event, [](uv_handle_t*){ });
      };
  };
#else
  class Events: public Loop {
    public_friend:
      class Poll: public Loop::Poll {
        private:
          curl_socket_t loopfd = 0;
        public:
          Poll(const curl_socket_t &s = 0)
            : Loop::Poll(s)
          {};
          void start(const curl_socket_t &l, const function<void()> &data) override {
            loopfd = l;
            link(data);
            ctl(EPOLLIN, EPOLL_CTL_ADD);
          };
          void stop() override {
            if (loopfd and sockfd) {
              ctl(0, EPOLL_CTL_DEL);
              ::close(sockfd);
              sockfd = 0;
            }
          };
        protected:
          void change(const int &events, const function<void()> &data = nullptr) override {
            if (loopfd) {
              if (data) link(data);
              ctl(events, EPOLL_CTL_MOD);
            }
          };
        private:
          void ctl(const int &events, const int &opcode) {
            epoll_event event;
            event.events = events;
            event.data.ptr = this;
            epoll_ctl(loopfd, opcode, sockfd, &event);
          };
      };
      class Timer: public Poll,
                   public Loop::Timer {
        private:
          itimerspec ts = {
            {1, 0}, {0, 1}
          };
        public:
          Timer(const curl_socket_t &loopfd)
            : Poll(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
          {
            if (timerfd_settime(sockfd, 0, &ts, nullptr) != -1)
              Poll::start(loopfd, [&]() {
                uint64_t again = 0;
                if (::read(sockfd, &again, 8) == 8)
                  timer_1s();
              });
          };
      };
      class Async: public Poll {
        private:
          const uint64_t again = 1;
        public:
          Async(const curl_socket_t &loopfd, const function<void()> &data)
            : Poll(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
          {
            Poll::start(loopfd, [this, data]() {
              uint64_t again = 0;
              if (::read(sockfd, &again, 8) == 8)
                data();
            });
          };
          void wakeup() override {
            if (::write(sockfd, &again, 8) == 8) {}
          };
      };
    private:
       curl_socket_t sockfd = 0;
               Timer timer;
         list<Async> events;
         epoll_event ready[32] = {};
    public:
      Events()
        : sockfd(epoll_create1(EPOLL_CLOEXEC))
        , timer(sockfd)
      {};
      void timer_ticks_factor(const unsigned int &factor) const override {
        timer.ticks_factor(factor);
      };
      void timer_1s(const TimeEvent &data) override {
        timer.push_back(data);
      };
      Loop::Async *async(const function<void()> &data) override {
        events.emplace_back(sockfd, data);
        return &events.back();
      };
      curl_socket_t poll() override {
        return sockfd;
      };
      void walk() override {
        while (sockfd)
          for (
            int i  =  epoll_wait(sockfd, ready, 32, -1);
                i --> 0;
            ((Poll*)ready[i].data.ptr)->ready()
          );
      };
      void end() override {
        timer.stop();
        for (auto &it : events)
          it.stop();
        events.clear();
        ::close(sockfd);
        sockfd = 0;
      };
  };
#endif

  static function<void(CURL*)> args_easy_setopt = [](CURL *curl) {
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
  };

  class Curl {
    private_friend:
      class Easy: public Events::Poll {
        protected:
          string in;
        private:
          CURL *curl = nullptr;
          string out;
        protected:
          void cleanup() {
            if (curl) {
              if (!out.empty()) send();
              curl_easy_cleanup(curl);
              curl = nullptr;
              stop();
            }
          };
          bool connected() const {
            return sockfd;
          };
          CURLcode connect(const string &url, const string &header, const string &res1, const string &res2) {
            out = header;
            in.clear();
            CURLcode rc;
            if (CURLE_OK == (rc = init())) {
              args_easy_setopt(curl);
              curl_easy_setopt(curl, CURLOPT_URL, url.data());
              curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
              curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
              if ( CURLE_OK != (rc = curl_easy_perform(curl))
                or CURLE_OK != (rc = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd))
                or CURLE_OK != (rc = send())
                or CURLE_OK != (rc = recv(5))
                or string::npos == in.find(res1)
                or string::npos == in.find(res2)
              ) {
                if (rc == CURLE_OK)
                  rc = string::npos == in.find("invalid API key")
                     ? CURLE_WEIRD_SERVER_REPLY
                     : CURLE_LOGIN_DENIED;
                cleanup();
              }
              in.clear();
            }
            return rc;
          };
          CURLcode send_recv() {
            CURLcode rc = CURLE_COULDNT_CONNECT;
            if (curl
              and sockfd
              and (out.empty() or CURLE_OK == (rc = send()))
              and CURLE_OPERATION_TIMEDOUT == (rc = recv(0))
            ) rc = CURLE_OK;
            if (rc != CURLE_OK)
              cleanup();
            return rc;
          };
          CURLcode emit(const string &data) {
            CURLcode rc = CURLE_OK;
            if (curl and sockfd) {
              out += data;
              change(EPOLLIN | EPOLLOUT);
              send();
            } else {
              rc = CURLE_COULDNT_CONNECT;
              cleanup();
            }
            return rc;
          };
        private:
          CURLcode init() {
            if (!curl) curl = curl_easy_init();
            else curl_easy_reset(curl);
            sockfd = 0;
            return curl
              ? CURLE_OK
              : CURLE_FAILED_INIT;
          };
          CURLcode send() {
            CURLcode rc;
            do {
              do {
                size_t n = 0;
                rc = curl_easy_send(curl, out.data(), out.length(), &n);
                out = out.substr(n);
                if (rc == CURLE_AGAIN and !wait(false, 5))
                  return CURLE_OPERATION_TIMEDOUT;
              } while (rc == CURLE_AGAIN);
              if (rc != CURLE_OK) break;
            } while (!out.empty());
            if (out.empty()) change(EPOLLIN);
            return rc;
          };
          CURLcode recv(const int &timeout) {
            CURLcode rc;
            for(;;) {
              char data[524288];
              size_t n;
              do {
                n = 0;
                rc = curl_easy_recv(curl, data, sizeof(data), &n);
                in.append(data, n);
                if (rc == CURLE_AGAIN and !wait(true, timeout))
                  return CURLE_OPERATION_TIMEDOUT;
              } while (rc == CURLE_AGAIN);
              if ((timeout and in.find("\r\n\r\n") != in.find("\u0001" "10="))
                or rc != CURLE_OK
                or n == 0
              ) break;
            }
            return rc;
          };
          int wait(const bool &io, const int &timeout) const {
            timeval tv = {timeout, 0};
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
          static json xfer(const string &url, const function<void(CURL*)> _easy_setopt = nullptr) {
            static mutex waiting_reply;
            lock_guard<mutex> lock(waiting_reply);
            string reply;
            CURLcode rc = CURLE_FAILED_INIT;
            CURL *curl = curl_easy_init();
            if (curl) {
              args_easy_setopt(curl);
              if (_easy_setopt) _easy_setopt(curl);
              curl_easy_setopt(curl, CURLOPT_TIMEOUT, 21L);
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
          using Easy::in;
        protected:
          CURLcode connect(const string &uri) {
            CURLcode rc = CURLE_URL_MALFORMAT;
            CURLU *url = curl_url();
            char *host,
                 *port,
                 *path,
                 *query;
            string header;
            if (!curl_url_set(url, CURLUPART_URL, ("http" + uri.substr(2)).data(), 0)) {
              if (!curl_url_get(url, CURLUPART_HOST, &host, 0)) {
                header = string(host);
                curl_free(host);
                if (!curl_url_get(url, CURLUPART_PORT, &port, CURLU_DEFAULT_PORT)) {
                  header += ":" + string(port);
                  curl_free(port);
                  if (!curl_url_get(url, CURLUPART_PATH, &path, 0)) {
                    header = "GET " + string(path) + (
                                curl_url_get(url, CURLUPART_QUERY, &query, 0)
                                  ? "" : "?" + string(query)
                             ) + " HTTP/1.1"
                             "\r\n" "Host: " + header +
                             "\r\n" "Upgrade: websocket"
                             "\r\n" "Connection: Upgrade"
                             "\r\n" "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw=="
                             "\r\n" "Sec-WebSocket-Version: 13"
                             "\r\n"
                             "\r\n";
                    curl_free(path);
                    curl_free(query);
                    rc = CURLE_OK;
                  }
                }
              }
            }
            curl_url_cleanup(url);
            return rc == CURLE_OK
              ? Easy::connect(
                  "http" + uri.substr(2),
                  header,
                  "HTTP/1.1 101 Switching Protocols",
                  "HSmrc0sMlYUkAGmm5OPpG2HaGWk="
                )
              : rc;
          };
          CURLcode emit(const string &data, const int &opcode) {
            return Easy::emit(frame(data, opcode, true));
          };
          string unframe() {
            string pong;
            bool drop = false;
            const string msg = WebSocketFrames::unframe(in, pong, drop);
            if (!pong.empty()) Easy::emit(pong);
            if (drop) cleanup();
            return msg;
          };
      };
      class WebSocketTwin: public WebSocket {
        protected:
          virtual string twin(const string&) const = 0;
      };
      class FixSocket: public Easy,
                       public FixFrames {
        private:
          using Easy::in;
          unsigned long sequence = 0;
        public:
          FixSocket(const string &t, const string &s)
            : FixFrames(t, s)
          {};
        protected:
          CURLcode connect(const string &uri, const string &logon) {
            return Easy::connect(
              "https://" + uri,
              frame(logon, "A", sequence = 1),
              "8=FIX.4.2" "\u0001",
              "\u0001" "35=A" "\u0001"
            );
          };
          CURLcode emit(const string &data, const string &type) {
            return Easy::emit(frame(data, type, ++sequence));
          };
          string unframe() {
            string pong;
            bool drop = false;
            const string msg = FixFrames::unframe(in, pong, drop);
            if (!pong.empty()) emit("", pong);
            if (drop) cleanup();
            return msg;
          };
          unsigned long last() const {
            return sequence;
          };
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
      static string CRC32(const string &input) {
        return to_string(crc32(0, (const Bytef*)input.data(), input.length()));
      };
      static string B64(const string &input) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        if (BIO_set_close(bio, BIO_CLOSE)) {}
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, input.data(), input.length());
        if (BIO_flush(bio)) {}
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
        if (BIO_set_close(bio, BIO_CLOSE)) {}
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        int len = BIO_read(bio, output, input.length());
        BIO_free_all(bio);
        return string(output, len);
      };
      static string SHA1  (const string &input, const bool &rawbin = false) {
        return SHA(input, rawbin, ::SHA1,   SHA_DIGEST_LENGTH);
      };
      static string SHA256(const string &input, const bool &rawbin = false) {
        return SHA(input, rawbin, ::SHA256, SHA256_DIGEST_LENGTH);
      };
      static string SHA512(const string &input, const bool &rawbin = false) {
        return SHA(input, rawbin, ::SHA512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC1  (const string &key, const string &input, const bool &rawbin = false) {
        return HMAC(key, input, rawbin, EVP_sha1,   SHA_DIGEST_LENGTH);
      };
      static string HMAC256(const string &key, const string &input, const bool &rawbin = false) {
        return HMAC(key, input, rawbin, EVP_sha256, SHA256_DIGEST_LENGTH);
      };
      static string HMAC512(const string &key, const string &input, const bool &rawbin = false) {
        return HMAC(key, input, rawbin, EVP_sha512, SHA512_DIGEST_LENGTH);
      };
      static string HMAC384(const string &key, const string &input, const bool &rawbin = false) {
        return HMAC(key, input, rawbin, EVP_sha384, SHA384_DIGEST_LENGTH);
      };
    private:
      static string SHA(
        const string  &input,
        const bool    &rawbin,
        unsigned char *(*md)(const unsigned char*, size_t, unsigned char*),
        const int     &digest_len
      ) {
        unsigned char digest[digest_len];
        md((unsigned char*)input.data(), input.length(), digest);
        return rawbin
             ? string((char*)digest, digest_len)
             :    HEX((char*)digest, digest_len);
      };
      static string HMAC(
        const string &key,
        const string &input,
        const bool   &rawbin,
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
        return rawbin
             ? string((char*)digest, digest_len)
             :    HEX((char*)digest, digest_len);
      };
      static string HEX(const char *digest, const int &digest_len) {
        stringstream output;
        output << hex << setfill('0');
        for (int i = 0; i < digest_len; i++)
          output << setw(2) << (digest[i] & 0xFF);
        return output.str();
      };
  };

  class WebServer {
    public_friend:
      using Response = function<string(string, const string&, const string&)>;
      using Upgrade  = function<int(const int&, const string&)>;
      using Message  = function<string(string, const string&)>;
    private_friend:
      struct Session {
          string auth;
        Response response = nullptr;
         Upgrade upgrade  = nullptr;
         Message message  = nullptr;
      };
      class Socket: public Events::Poll {
        public:
          Socket(const curl_socket_t &s = 0)
            : Poll(s)
          {};
          void shutdown() {
            stop();
          };
          void cork(const int &enable) const {
#ifndef _WIN32
            setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &enable, sizeof(enable));
#endif
#ifdef __APPLE__
            if (!enable) ::send(sockfd, "", 0, MSG_NOSIGNAL);
#endif
          };
      };
      class Frontend: public Socket,
                      public WebSocketFrames {
        private:
                    SSL *ssl     = nullptr;
          const Session *session = nullptr;
                  Clock  time    = 0;
                 string  addr,
                         out,
                         in;
        public:
          Frontend(const curl_socket_t &s, const curl_socket_t &loopfd, SSL *S, const Session *e)
            : Socket(s)
            , ssl(S)
            , session(e)
            , time(Tstamp)
          {
            Socket::start(loopfd, ioHttp);
          };
          void shutdown() {
            if (ssl) {
              SSL_shutdown(ssl);
              SSL_free(ssl);
            }
            Socket::shutdown();
            if (!time) session->upgrade(-1, addr);
          };
          bool upgraded() const {
            return !time;
          };
          void send(const string &data) {
            out += data;
            change(EPOLLIN | EPOLLOUT);
          };
          bool stale() {
            if (time and sockfd and Tstamp > time + 21e+3)
              shutdown();
            return !sockfd;
          };
        protected:
          string unframe() {
            bool drop = false;
            const string msg = WebSocketFrames::unframe(in, out, drop);
            if (drop) shutdown();
            return msg;
          };
        private:
          function<void()> ioWs = [&]() {
            io();
            if (!sockfd or in.empty()) return;
            const string msg = unframe();
            if (msg.empty()) return;
            const string reply = session->message(msg, addr);
            if (reply.empty()) return;
            out += frame(reply, reply.substr(0, 2) == "PK" ? 0x02 : 0x01, false);
            change(EPOLLIN | EPOLLOUT);
          };
          function<void()> ioHttp = [&]() {
            io();
            if (sockfd
              and out.empty()
              and in.length() > 5
              and in.substr(0, 5) == "GET /"
              and in.find("\r\n\r\n") != string::npos
            ) {
              if (addr.empty())
                addr = address();
              const string path   = in.substr(4, in.find(" HTTP/1.1") - 4);
              const size_t papers = in.find("Authorization: Basic ");
              string auth;
              if (papers != string::npos) {
                auth = in.substr(papers + 21);
                auth = auth.substr(0, auth.find("\r\n"));
              }
              const size_t key = in.find("Sec-WebSocket-Key: ");
              int allowed = 1;
              if (key == string::npos) {
                out = session->response(path, auth, addr);
                if (out.empty())
                  shutdown();
                else change(EPOLLIN | EPOLLOUT);
              } else if ((session->auth.empty() or auth == session->auth)
                and in.find("\r\n" "Upgrade: websocket" "\r\n") != string::npos
                and in.find("\r\n" "Connection: ")              != string::npos
                and in.find(" Upgrade")                         != string::npos
                and in.find("Sec-WebSocket-Version: 13" "\r\n") != string::npos
                and (allowed = session->upgrade(allowed, addr))
              ) {
                time = 0;
                out = "HTTP/1.1 101 Switching Protocols"
                      "\r\n" "Connection: Upgrade"
                      "\r\n" "Upgrade: websocket"
                      "\r\n" "Sec-WebSocket-Version: 13"
                      "\r\n" "Sec-WebSocket-Accept: "
                               + Text::B64(Text::SHA1(
                                 in.substr(key + 19, in.substr(key + 19).find("\r\n"))
                                   + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
                                 true
                               )) +
                      "\r\n"
                      "\r\n";
                in.clear();
                change(EPOLLIN | EPOLLOUT, ioWs);
              } else {
                if (!allowed) session->upgrade(allowed, addr);
                shutdown();
              }
            }
          };
          string address() const {
            string addr;
            union {
              sockaddr         sa;
              sockaddr_in      s4;
              sockaddr_in6     s6;
              sockaddr_storage ss;
            } u;
            socklen_t len = sizeof(u);
            if (getpeername(sockfd, &u.sa, &len) != -1) {
              char buf[INET6_ADDRSTRLEN];
              if (u.ss.ss_family == AF_INET)
                inet_ntop(AF_INET,  &u.s4.sin_addr,  buf, sizeof(buf));
              else
                inet_ntop(AF_INET6, &u.s6.sin6_addr, buf, sizeof(buf));
              addr = string(buf);
              if (addr.length() > 7 and addr.substr(0, 7) == "::ffff:") addr = addr.substr(7);
              if (addr.length() < 7) addr.clear();
            }
            return addr.empty() ? "unknown" : addr;
          };
          void io() {
            if (ssl) {
              if (!out.empty()) {
                cork(1);
                int n = SSL_write(ssl, out.data(), out.length());
                switch (SSL_get_error(ssl, n)) {
                  case SSL_ERROR_WANT_READ:
                  case SSL_ERROR_WANT_WRITE:  break;
                  case SSL_ERROR_NONE:        out.clear();
                                              change(EPOLLIN);  [[fallthrough]];
                  case SSL_ERROR_ZERO_RETURN: if (!time) break; [[fallthrough]];
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
                if      (n > 0) out = out.substr(n);
                else if (n < 0) {
                  shutdown();
                  return;
                }
                cork(0);
                if (out.empty()) {
                  if (time) {
                    shutdown();
                    return;
                  }
                  change(EPOLLIN);
                }
              }
              char data[1024];
              ssize_t n = ::recv(sockfd, data, sizeof(data), 0);
              if (n > 0) in.append(data, n);
            }
          };
      };
    public_friend:
      class Backend: public Socket,
                     public WebSocketFrames {
        private:
                 SSL_CTX *ctx = nullptr;
                 Session  session;
          list<Frontend>  requests;
        public:
          bool idle() const {
            return !any_of(requests.begin(), requests.end(), [](auto &it) {
              return it.upgraded();
            });
          };
          int clients() const {
            return count_if(requests.begin(), requests.end(), [](auto &it) {
              return it.upgraded();
            });
          };
          string protocol() const {
            return "HTTP" + string(!!ctx, 'S');
          };
          void broadcast(const char &portal, const unordered_map<char, string> &queue) {
            string msgs;
            for (const auto &it : queue)
              msgs += frame(portal + (it.first + it.second), 0x01, false);
            for (auto &it : requests)
              if (it.upgraded())
                it.send(msgs);
          };
          void purge() {
            while (!requests.empty()) {
              requests.back().shutdown();
              requests.pop_back();
            }
            shutdown();
          };
          void timeouts() {
            for (auto it = requests.begin(); it != requests.end();)
              if (it->stale())
                it = requests.erase(it);
              else ++it;
          };
          bool listen(const curl_socket_t &loopfd, const string &inet, const int &port, const bool &ipv6, const Session &data) {
            if (sockfd) return false;
            addrinfo  hints = {AI_PASSIVE, AF_UNSPEC, SOCK_STREAM, 0, 0, nullptr, nullptr, nullptr},
                     *result,
                     *rp;
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
                const
#ifdef _WIN32
                char
#else
                int
#endif
                enabled = 1;
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
                if (::bind(sockfd, rp->ai_addr, rp->ai_addrlen) or ::listen(sockfd, 512))
                  shutdown();
                else {
                  session = data;
                  start(loopfd, [this, loopfd]() {
                    accept_request(loopfd);
                  });
                }
              }
              freeaddrinfo(result);
            }
            return sockfd;
          };
          vector<string> ssl_context(const string &crt, const string &key) {
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
          string document(const string &content, const unsigned int &code, const string &type) const {
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
        private:
          void accept_request(const curl_socket_t &loopfd) {
            curl_socket_t clientfd = accept4(sockfd, nullptr, nullptr, SOCK_CLOEXEC | SOCK_NONBLOCK);
            if (clientfd == -1) return;
#ifdef __APPLE__
            const int noSigpipe = 1;
            setsockopt(clientfd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe));
#endif
            SSL *ssl = nullptr;
            if (ctx) {
              ssl = SSL_new(ctx);
              SSL_set_accept_state(ssl);
              SSL_set_fd(ssl, clientfd);
              SSL_set_mode(ssl, SSL_MODE_RELEASE_BUFFERS);
            }
            requests.emplace_back(clientfd, loopfd, ssl, &session);
          };
          void socket(const int &domain, const int &type, const int &protocol) {
            sockfd = ::socket(domain, type | SOCK_CLOEXEC | SOCK_NONBLOCK, protocol);
            if (sockfd == -1) sockfd = 0;
#ifdef __APPLE__
            else {
              const int noSigpipe = 1;
              setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe));
            }
#endif
          };
      };
  };

  class Random {
    public:
      static unsigned long long int64() {
        static random_device rd;
        static mt19937_64 gen(rd());
        return uniform_int_distribution<unsigned long long>()(gen);
      };
      static string int45Id() {
        return to_string(int64()).substr(0, 10);
      };
      static string int32Id() {
        return to_string(int64()).substr(0,  8);
      };
      static string char16Id() {
        string id = string(16, ' ');
        for (auto &it : id) {
         const int offset = int64() % (26 + 26 + 10);
         if      (offset < 26)      it = 'a' + offset;
         else if (offset < 26 + 26) it = 'A' + offset - 26;
         else                       it = '0' + offset - 26 - 26;
        }
        return id;
      };
      static string uuid36Id() {
        string uuid = string(36, ' ');
        uuid[8]  =
        uuid[13] =
        uuid[18] =
        uuid[23] = '-';
        uuid[14] = '4';
        unsigned long long rnd = int64();
        for (auto &it : uuid)
          if (it == ' ') {
            if (rnd <= 0x02) rnd = 0x2000000 + (int64() * 0x1000000);
            rnd >>= 4;
            const int offset = (uuid[17] != ' ' and uuid[19] == ' ')
              ? ((rnd & 0xF) & 0x3) | 0x8
              : rnd & 0xf;
            if (offset < 10) it = '0' + offset;
            else             it = 'a' + offset - 10;
          }
        return uuid;
      };
      static string uuid32Id() {
        string uuid = uuid36Id();
        uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());
        return uuid;
      }
  };

  class Decimal {
    public:
      stringstream stream;
      double step = 0;
    private:
      double tick = 0;
    public:
      Decimal()
      {
        stream << fixed;
      };
      void precision(const double &t) {
        stream.precision(ceil(abs(log10(tick = t))));
        step = pow(10, -1 * stream.precision());
      };
      double round(const double &input) const {
        return ::round((::round(
          input / step) * step)
                / tick) * tick;
      };
      double floor(const double &input) const {
        return ::floor((::floor(
          input / step) * step)
                / tick) * tick;
      };
      string str(const double &input) {
        stream.str("");
        stream << round(input);
        return stream.str();
      };
  };
}
