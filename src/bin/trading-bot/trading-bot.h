#ifndef K_TRADING_BOT_H_
#define K_TRADING_BOT_H_

extern const char _www_html_index,     _www_ico_favicon,     _www_css_base,
                  _www_gzip_bomb,      _www_mp3_audio_0,     _www_css_light,
                  _www_js_client,      _www_mp3_audio_1,     _www_css_dark;

extern const  int _www_html_index_len, _www_ico_favicon_len, _www_css_base_len,
                  _www_gzip_bomb_len,  _www_mp3_audio_0_len, _www_css_light_len,
                  _www_js_client_len,  _www_mp3_audio_1_len, _www_css_dark_len;

class TradingBot: public KryptoNinja {
  public:
    static void terminal();
    TradingBot()
    {
      display   = terminal;
      margin    = {3, 6, 1, 2};
      databases = true;
      documents = {
        {"",                                  {&_www_gzip_bomb,   _www_gzip_bomb_len  }},
        {"/",                                 {&_www_html_index,  _www_html_index_len }},
        {"/js/client.min.js",                 {&_www_js_client,   _www_js_client_len  }},
        {"/css/bootstrap.min.css",            {&_www_css_base,    _www_css_base_len   }},
        {"/css/bootstrap-theme-dark.min.css", {&_www_css_dark,    _www_css_dark_len   }},
        {"/css/bootstrap-theme.min.css",      {&_www_css_light,   _www_css_light_len  }},
        {"/favicon.ico",                      {&_www_ico_favicon, _www_ico_favicon_len}},
        {"/audio/0.mp3",                      {&_www_mp3_audio_0, _www_mp3_audio_0_len}},
        {"/audio/1.mp3",                      {&_www_mp3_audio_1, _www_mp3_audio_1_len}}
      };
      arguments = { {
        {"wallet-limit", "AMOUNT", "0",                    "set AMOUNT in base currency to limit the balance,"
                                                           "\n" "otherwise the full available balance can be used"},
        {"lifetime",     "NUMBER", "0",                    "set NUMBER of minimum milliseconds to keep orders open,"
                                                           "\n" "otherwise open orders can be replaced anytime required"},
        {"matryoshka",   "URL",    "https://example.com/", "set Matryoshka link URL of the next UI"},
        {"ignore-sun",   "2",      nullptr,                "do not switch UI to light theme on daylight"},
        {"ignore-moon",  "1",      nullptr,                "do not switch UI to dark theme on moonlight"},
        {"debug-orders", "1",      nullptr,                "print detailed output about exchange messages"},
        {"debug-quotes", "1",      nullptr,                "print detailed output about quoting engine"},
        {"debug-wallet", "1",      nullptr,                "print detailed output about target base position"}
      }, [](
        unordered_map<string, string> &str,
        unordered_map<string, int>    &num,
        unordered_map<string, double> &dec
      ) {
        if (num["debug"])
          num["debug-orders"] =
          num["debug-quotes"] =
          num["debug-wallet"] = 1;
        if (num["ignore-moon"] and num["ignore-sun"])
          num["ignore-moon"] = 0;
        if (num["debug-orders"] or num["debug-quotes"])
          num["naked"] = 1;
      } };
    };
} K;

class WorldWideWeb {
  public:
    string protocol = "HTTP";
  private:
    uWS::Group<uWS::SERVER> *webui = nullptr;
    unordered_map<mMatter, function<const json()>> hello;
    unordered_map<mMatter, function<void(json&)>> kisses;
    unordered_map<mMatter, string> queue;
  private_ref:
    const unsigned int &delay;
  public:
    WorldWideWeb(const unsigned int &d)
      : delay(d)
    {};
    void listen() {
      webui = K.listen(
        protocol, K.num("port"),
        !K.num("without-ssl"), K.str("ssl-crt"), K.str("ssl-key"),
        wsMessage
      );
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
        if (K.connections) send(data);
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
};

class Engine: public Klass {
  public:
           mButtons btn;
     mQuotingParams qp;
       WorldWideWeb client;
           mMonitor monitor;
           mProduct product;
            mOrders orders;
      mMarketLevels levels;
    mWalletPosition wallet;
            mBroker broker;
  public:
    Engine()
      : qp(K)
      , client(qp.delayUI)
      , monitor(K)
      , product(K)
      , orders(K)
      , levels(K, orders, qp)
      , wallet(K, orders, qp, levels.stats.ewma.targetPositionAutoPercentage, levels.fairValue)
      , broker(K, orders, qp, levels, wallet)
    {};
  protected:
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
code( btn.notepad              )  \
code( qp                       )  \
code( monitor                  )  \
code( product                  )  \
code( orders                   )  \
code( levels.diff              )  \
code( levels.stats.takerTrades )  \
code( levels.stats.fairPrice   )  \
code( levels.stats             )  \
code( wallet.target            )  \
code( wallet.safety            )  \
code( wallet.safety.trades     )  \
code( wallet                   )  \
code( broker.semaphore         )  \
code( broker.calculon          )

#define CLIENT_CLICKME      \
        CLIENT_CLICKME_LIST \
      ( CLIENT_CLICKME_CODE )
#define CLIENT_CLICKME_CODE(btn, fn, val) \
                 client.clickme(btn, [&](const json &butterfly) { fn(val); });
#define CLIENT_CLICKME_LIST(code)                                            \
code( btn.notepad           , void                             ,           ) \
code( btn.submit            , manualSendOrder                  , butterfly ) \
code( btn.cancel            , manualCancelOrder                , butterfly ) \
code( btn.cancelAll         , cancelOrders                     ,           ) \
code( btn.cleanTrade        , wallet.safety.trades.clearOne    , butterfly ) \
code( btn.cleanTrades       , wallet.safety.trades.clearAll    ,           ) \
code( btn.cleanTradesClosed , wallet.safety.trades.clearClosed ,           ) \
code( qp                    , savedQuotingParameters           ,           ) \
code( broker.semaphore      , void                             ,           )
    void waitAdmin() override {
      HOTKEYS
      if (!K.num("headless")) client.listen();
      CLIENT_WELCOME
      CLIENT_CLICKME
    };
    void run() override {
      {
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
        K.gateway->askForCancelAll = &qp.cancelOrdersAuto;
      }
    };
  private:
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

void TradingBot::terminal() {
  const vector<mOrder> openOrders = engine.orders.working(true);
  const unsigned int previous = margin.bottom;
  margin.bottom = max((int)openOrders.size(), engine.broker.semaphore.paused() ? 0 : 2) + 1;
  const int y = getmaxy(stdscr),
            x = getmaxx(stdscr),
            yMaxLog = y - margin.bottom;
  if (margin.bottom != previous) {
    if (previous < margin.bottom) wscrl(stdlog, margin.bottom - previous);
    wresize(
      stdlog,
      y - margin.top - margin.bottom,
      x - margin.left - margin.right
    );
    if (previous > margin.bottom) wscrl(stdlog, margin.bottom - previous);
  }
  mvwvline(stdscr, 1, 1, ' ', y-1);
  mvwvline(stdscr, yMaxLog-1, 1, ' ', y-1);
  mvwhline(stdscr, yMaxLog,   1, ' ', x-1);
  int yOrders = yMaxLog;
  for (const auto &it : openOrders) {
    mvwhline(stdscr, ++yOrders, 1, ' ', x-1);
    wattron(stdscr, COLOR_PAIR(it.side == Side::Bid ? COLOR_CYAN : COLOR_MAGENTA));
    mvwaddstr(stdscr, yOrders, 1, (((it.side == Side::Bid ? "BID" : "ASK") + (" > "
      + K.gateway->str(it.quantity))) + ' ' + K.gateway->base + " at price "
      + K.gateway->str(it.price) + ' ' + K.gateway->quote + " (value "
      + K.gateway->str(abs(it.price * it.quantity)) + ' ' + K.gateway->quote + ")"
    ).data());
    wattroff(stdscr, COLOR_PAIR(it.side == Side::Bid ? COLOR_CYAN : COLOR_MAGENTA));
  }
  while (++yOrders < y) mvwhline(stdscr, yOrders, 1, ' ', x-1);
  mvwaddch(stdscr, 0, 0, ACS_ULCORNER);
  mvwhline(stdscr, 0, 1, ACS_HLINE, max(80, x));
  mvwhline(stdscr, 1, 14, ' ', max(80, x)-14);
  mvwvline(stdscr, 1, 0, ACS_VLINE, yMaxLog);
  mvwvline(stdscr, yMaxLog, 0, ACS_LTEE, y);
  mvwaddch(stdscr, y, 0, ACS_BTEE);
  mvwaddch(stdscr, 0, 12, ACS_RTEE);
  wattron(stdscr, COLOR_PAIR(COLOR_GREEN));
  const string title1 = "   " + K.str("exchange");
  const string title2 = " " + (K.num("headless")
    ? "headless"
    : "UI at " + Text::strL(engine.client.protocol) + "://" + K.wtfismyip + ":" + K.str("port")
  )  + ' ';
  wattron(stdscr, A_BOLD);
  mvwaddstr(stdscr, 0, 13, title1.data());
  wattroff(stdscr, A_BOLD);
  mvwaddstr(stdscr, 0, 13+title1.length(), title2.data());
  mvwaddch(stdscr, 0, 14, 'K' | A_BOLD);
  wattroff(stdscr, COLOR_PAIR(COLOR_GREEN));
  mvwaddch(stdscr, 0, 13+title1.length()+title2.length(), ACS_LTEE);
  mvwaddch(stdscr, 0, x-26, ACS_RTEE);
  mvwaddstr(stdscr, 0, x-25, (string(" [   ]: ") + (engine.broker.semaphore.paused() ? "Start" : "Stop?") + ", [ ]: Quit!").data());
  mvwaddch(stdscr, 0, x-9, 'q' | A_BOLD);
  wattron(stdscr, A_BOLD);
  mvwaddstr(stdscr, 0, x-23, "ESC");
  wattroff(stdscr, A_BOLD);
  mvwaddch(stdscr, 0, 7, ACS_TTEE);
  mvwaddch(stdscr, 1, 7, ACS_LLCORNER);
  mvwhline(stdscr, 1, 8, ACS_HLINE, 4);
  mvwaddch(stdscr, 1, 12, ACS_RTEE);
  wattron(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  const string baseValue  = K.gateway->str(engine.wallet.base.value),
               quoteValue = K.gateway->str(engine.wallet.quote.value);
  wattron(stdscr, A_BOLD);
  waddstr(stdscr, (" " + baseValue + ' ').data());
  wattroff(stdscr, A_BOLD);
  waddstr(stdscr, K.gateway->base.data());
  wattroff(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  wattron(stdscr, COLOR_PAIR(COLOR_GREEN));
  waddstr(stdscr, " or ");
  wattroff(stdscr, COLOR_PAIR(COLOR_GREEN));
  wattron(stdscr, COLOR_PAIR(COLOR_CYAN));
  wattron(stdscr, A_BOLD);
  waddstr(stdscr, quoteValue.data());
  wattroff(stdscr, A_BOLD);
  waddstr(stdscr, (" " + K.gateway->quote + ' ').data());
  wattroff(stdscr, COLOR_PAIR(COLOR_CYAN));
  size_t xLenValue = 14+baseValue.length()+quoteValue.length()+K.gateway->base.length()+K.gateway->quote.length()+7,
         xMaxValue = max(xLenValue+1, 18+title1.length()+title2.length());
  mvwaddch(stdscr, 0, xMaxValue, ACS_TTEE);
  mvwaddch(stdscr, 1, xMaxValue, ACS_LRCORNER);
  mvwhline(stdscr, 1, xLenValue, ACS_HLINE, xMaxValue - xLenValue);
  mvwaddch(stdscr, 1, xLenValue, ACS_LTEE);
  const int yPos = max(1, (y / 2) - 6),
            baseAmount  = round(engine.wallet.base.amount  * 10 / engine.wallet.base.value),
            baseHeld    = round(engine.wallet.base.held    * 10 / engine.wallet.base.value),
            quoteAmount = round(engine.wallet.quote.amount * 10 / engine.wallet.quote.value),
            quoteHeld   = round(engine.wallet.quote.held   * 10 / engine.wallet.quote.value);
  mvwvline(stdscr, yPos+1, x-3, ' ', 10);
  mvwvline(stdscr, yPos+1, x-4, ' ', 10);
  wattron(stdscr, COLOR_PAIR(COLOR_CYAN));
  mvwvline(stdscr, yPos+11-quoteAmount-quoteHeld, x-4, ACS_VLINE, quoteHeld);
  wattron(stdscr, A_BOLD);
  mvwvline(stdscr, yPos+11-quoteAmount, x-4, ' ' | A_REVERSE, quoteAmount);
  wattroff(stdscr, A_BOLD);
  wattroff(stdscr, COLOR_PAIR(COLOR_CYAN));
  wattron(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  mvwvline(stdscr, yPos+11-baseAmount-baseHeld, x-3, ACS_VLINE, baseHeld);
  wattron(stdscr, A_BOLD);
  mvwvline(stdscr, yPos+11-baseAmount, x-3, ' ' | A_REVERSE, baseAmount);
  wattroff(stdscr, A_BOLD);
  wattroff(stdscr, COLOR_PAIR(COLOR_MAGENTA));
  mvwaddch(stdscr, yPos, x-2, ACS_URCORNER);
  mvwaddch(stdscr, yPos+11, x-2, ACS_LRCORNER);
  mvwaddch(stdscr, yPos, x-5, ACS_ULCORNER);
  mvwaddch(stdscr, yPos+11, x-5, ACS_LLCORNER);
  mvwhline(stdscr, yPos, x-4, ACS_HLINE, 2);
  mvwhline(stdscr, yPos+11, x-4, ACS_HLINE, 2);
  mvwaddch(stdscr, yMaxLog, 0, ACS_LTEE);
  mvwhline(stdscr, yMaxLog, 1, ACS_HLINE, 3);
  mvwaddch(stdscr, yMaxLog, 4, ACS_RTEE);
  mvwaddstr(stdscr, yMaxLog, 5, "< (");
  if (engine.broker.semaphore.offline()) {
    wattron(stdscr, COLOR_PAIR(COLOR_RED));
    wattron(stdscr, A_BOLD);
    waddstr(stdscr, "DISCONNECTED");
    wattroff(stdscr, A_BOLD);
    wattroff(stdscr, COLOR_PAIR(COLOR_RED));
    waddch(stdscr, ')');
  } else {
    if (engine.broker.semaphore.paused()) {
      wattron(stdscr, COLOR_PAIR(COLOR_YELLOW));
      wattron(stdscr, A_BLINK);
      waddstr(stdscr, "press START to trade");
      wattroff(stdscr, A_BLINK);
      wattroff(stdscr, COLOR_PAIR(COLOR_YELLOW));
      waddch(stdscr, ')');
    } else {
      wattron(stdscr, COLOR_PAIR(COLOR_YELLOW));
      waddstr(stdscr, to_string(openOrders.size()).data());
      wattroff(stdscr, COLOR_PAIR(COLOR_YELLOW));
      waddstr(stdscr, ") Open Orders");
    }
    waddstr(stdscr, " while");
    wattron(stdscr, COLOR_PAIR(COLOR_GREEN));
    waddstr(stdscr, (" 1 " + K.gateway->base + " = ").data());
    wattron(stdscr, A_BOLD);
    waddstr(stdscr, K.gateway->str(engine.levels.fairValue).data());
    wattroff(stdscr, A_BOLD);
    waddstr(stdscr, (" " + K.gateway->quote).data());
    wattroff(stdscr, COLOR_PAIR(COLOR_GREEN));
    waddch(stdscr, engine.broker.semaphore.paused() ? ' ' : ':');
  }
  mvwaddch(stdscr, y-1, 0, ACS_LLCORNER);
  mvwaddstr(stdscr, 1, 2, string("|/-\\").substr(engine.monitor.orders_60s % 4, 1).data());
};

#endif
