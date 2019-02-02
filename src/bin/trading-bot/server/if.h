#ifndef K_IF_H_
#define K_IF_H_

class WorldWideWeb {
  public:
    string protocol = "HTTP";
  private:
    uWS::Group<uWS::SERVER> *webui = nullptr;
    int connections = 0;
    unordered_map<mMatter, function<const json()>> hello;
    unordered_map<mMatter, function<void(json&)>> kisses;
    unordered_map<mMatter, string> queue;
  private_ref:
    const unsigned int &delay;
  public:
    WorldWideWeb(const unsigned int d)
      : delay(d)
    {};
    void listen() {
      if (K.num("headless")) return;
      if (!(webui = K.listen(protocol, K.num("port"), !K.num("without-ssl"),
        K.str("ssl-crt"), K.str("ssl-key"), httpServer, wsServer, wsMessage
      ))) error("UI", "Unable to listen to UI port number " + K.str("port")
            + " (may be already in use by another program)");
      K.timer_1s([&](const unsigned int &tick) {
        if (delay and !(tick % delay) and !queue.empty()) {
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
    void welcome(mToClient &data) {
      data.send = [&]() {
        if (connections) send(data);
      };
      if (!webui) return;
      const mMatter matter = data.about();
      if (hello.find(matter) != hello.end())
        error("UI", string("Too many handlers for \"") + (char)matter + "\" welcome event");
      hello[matter] = [&]() {
        return data.hello();
      };
    };
    void clickme(mFromClient &data, function<void(const json&)> fn) {
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
    void send(const mToClient &data) {
      if (data.realtime() or !delay) {
        const string msg = (char)mPortal::Kiss + ((char)data.about() + data.blob().dump());
        K.deferred([this, msg]() {
          webui->broadcast(msg.data(), msg.length(), uWS::OpCode::TEXT);
        });
      } else queue[data.about()] = data.blob().dump();
    };
    function<const bool(const bool&, const string&)> wsServer = [&](
      const   bool &connection,
      const string &addr
    ) {
      connections += connection ?: -1;
      Print::log("UI", to_string(connections) + " client" + string(connections == 1 ? 0 : 1, 's')
        + " connected, last connection was from", addr);
      if (connections > K.num("client-limit")) {
        Print::log("UI", "--client-limit=" + K.str("client-limit") + " reached by", addr);
        return false;
      }
      return true;
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
};

class Engine: public Klass {
#define SQLITE_BACKUP      \
        SQLITE_BACKUP_LIST \
      ( SQLITE_BACKUP_CODE )
#define SQLITE_BACKUP_CODE(data)       K.backup(&data);
#define SQLITE_BACKUP_LIST(code)       \
code( qp                             ) \
code( wallet.target                  ) \
code( wallet.safety.trades           ) \
code( wallet.profits                 ) \
code( levels.stats.ewma.fairValue96h ) \
code( levels.stats.ewma              ) \
code( levels.stats.stdev             )

#define HOTKEYS      \
        HOTKEYS_LIST \
      ( HOTKEYS_CODE )
#define HOTKEYS_CODE(key, fn)          K.hotkey(key, [&]() { fn(); });
#define HOTKEYS_LIST(code)             \
code( 'Q'  , exit                    ) \
code( 'q'  , exit                    ) \
code( '\e' , broker.semaphore.toggle )

#define CLIENT_WELCOME      \
        CLIENT_WELCOME_LIST \
      ( CLIENT_WELCOME_CODE )
#define CLIENT_WELCOME_CODE(data) client.welcome(data);
#define CLIENT_WELCOME_LIST(code) \
code( qp                       )  \
code( monitor                  )  \
code( product                  )  \
code( orders                   )  \
code( wallet.target            )  \
code( wallet.safety            )  \
code( wallet.safety.trades     )  \
code( wallet                   )  \
code( levels.diff              )  \
code( levels.stats.takerTrades )  \
code( levels.stats.fairPrice   )  \
code( levels.stats             )  \
code( broker.semaphore         )  \
code( broker.calculon          )  \
code( btn.notepad              )

#define CLIENT_CLICKME      \
        CLIENT_CLICKME_LIST \
      ( CLIENT_CLICKME_CODE )
#define CLIENT_CLICKME_CODE(btn, fn, val) \
                client.clickme(btn, [&](const json &butterfly) { fn(val); });
#define CLIENT_CLICKME_LIST(code)                                            \
code( qp                    , savedQuotingParameters           ,           ) \
code( broker.semaphore      , void                             ,           ) \
code( btn.notepad           , void                             ,           ) \
code( btn.submit            , manualSendOrder                  , butterfly ) \
code( btn.cancel            , manualCancelOrder                , butterfly ) \
code( btn.cancelAll         , cancelOrders                     ,           ) \
code( btn.cleanTrade        , wallet.safety.trades.clearOne    , butterfly ) \
code( btn.cleanTrades       , wallet.safety.trades.clearAll    ,           ) \
code( btn.cleanTradesClosed , wallet.safety.trades.clearClosed ,           )
  public:
     mQuotingParams qp;
       WorldWideWeb client;
           mButtons btn;
           mMonitor monitor;
           mProduct product;
            mOrders orders;
      mMarketLevels levels;
    mWalletPosition wallet;
            mBroker broker;
    Engine()
      : client(qp.delayUI)
      , monitor(K)
      , product(K)
      , orders(K)
      , levels(K, orders, qp)
      , wallet(K, orders, qp, levels.stats.ewma.targetPositionAutoPercentage, levels.fairValue)
      , broker(K, orders, qp, levels, wallet)
    {};
    void savedQuotingParameters() {
      K.timer_ticks_factor(qp.delayUI);
      broker.calculon.dummyMM.mode("saved");
      levels.stats.ewma.calcFromHistory(qp._diffEwma);
    };
    void cancelOrders() {
      for (mOrder *const it : orders.working())
        cancelOrder(it);
    };
    void manualSendOrder(mOrder raw) {
      raw.orderId = K.gateway->randId();
      placeOrder(raw);
    };
    void manualCancelOrder(const RandId &orderId) {
      cancelOrder(orders.find(orderId));
    };
  protected:
    void load() override {
      {
        SQLITE_BACKUP
      } {
        K.timer_ticks_factor(qp.delayUI);
        broker.calculon.dummyMM.mode("loaded");
      } {
        broker.semaphore.agree(K.num("autobot"));
        K.timer_1s([&](const unsigned int &tick) {
          if (!K.gateway->countdown and !levels.warn_empty()) {
            levels.timer_1s();
            if (!(tick % 60)) {
              levels.timer_60s();
              monitor.timer_60s();
            }
            wallet.safety.timer_1s();
            calcQuotes();
          }
          return false;
        });
        client.listen();
        K.gateway->askForCancelAll = &qp.cancelOrdersAuto;
      }
    };
    void waitData() override {
      K.gateway->write_Connectivity = [&](const Connectivity &rawdata) {
        broker.semaphore.read_from_gw(rawdata);
        if (broker.semaphore.offline())
          levels.clear();
      };
      K.gateway->write_mWallets = [&](const mWallets &rawdata) {
        wallet.read_from_gw(rawdata);
      };
      K.gateway->write_mLevels = [&](const mLevels &rawdata) {
        levels.read_from_gw(rawdata);
        wallet.calcFunds();
        calcQuotes();
      };
      K.gateway->write_mOrder = [&](const mOrder &rawdata) {
        orders.read_from_gw(rawdata);
        wallet.calcFundsAfterOrder(orders.updated, &K.gateway->askForFees);
      };
      K.gateway->write_mTrade = [&](const mTrade &rawdata) {
        levels.stats.takerTrades.read_from_gw(rawdata);
      };
    };
    void waitAdmin() override {
      CLIENT_WELCOME
      CLIENT_CLICKME
      HOTKEYS
    };
    void run() override {
      K.handshake({
        {"gateway", K.gateway->http },
        {"gateway", K.gateway->ws   },
        {"gateway", K.gateway->fix  },
        {"autoBot", K.num("autobot")
                      ? "yes"
                      : "no"        }
      });
    };
  private:
    void calcQuotes() {
      if (broker.ready() and levels.ready() and wallet.ready()) {
        if (broker.calcQuotes()) {
          quote2orders(broker.calculon.quotes.ask);
          quote2orders(broker.calculon.quotes.bid);
        } else cancelOrders();
      }
      broker.clear();
    };
    void quote2orders(mQuote &quote) {
      const vector<mOrder*> abandoned = broker.abandon(quote);
      const unsigned int replace = K.gateway->askForReplace and !(
        quote.empty() or abandoned.empty()
      );
      for_each(
        abandoned.begin(), abandoned.end() - replace,
        [&](mOrder *const it) {
          cancelOrder(it);
        }
      );
      if (quote.empty()) return;
      if (replace)
        replaceOrder(quote.price, quote.isPong, abandoned.back());
      else placeOrder({
        quote.side,
        quote.price,
        quote.size,
        Tstamp,
        quote.isPong,
        K.gateway->randId()
      });
      monitor.orders_60s++;
    };
    void placeOrder(const mOrder &raw) {
      K.gateway->place(orders.upsert(raw));
    };
    void replaceOrder(const Price &price, const bool &isPong, mOrder *const order) {
      if (orders.replace(price, isPong, order))
        K.gateway->replace(order);
    };
    void cancelOrder(mOrder *const order) {
      if (orders.cancel(order))
        K.gateway->cancel(order);
    };
} engine;

#endif