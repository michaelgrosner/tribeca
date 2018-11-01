#ifndef K_IF_H_
#define K_IF_H_

namespace K {
  struct Options: public Arguments {
    const vector<Argument> custom_long_options() const {
      return {
        {"wallet-limit", "AMOUNT", "0",                        "set AMOUNT in base currency to limit the balance,"
                                                               "\n" "otherwise the full available balance can be used"},
        {"client-limit", "NUMBER", "7",                        "set NUMBER of maximum concurrent UI connections"},
        {"naked",        "1",      0,                          "do not display CLI, print output to stdout instead"},
        {"headless",     "1",      0,                          "do not listen for UI connections,"
                                                               "\n" "all other UI related arguments will be ignored"},
        {"without-ssl",  "1",      0,                          "do not use HTTPS for UI connections (use HTTP only)"},
        {"ssl-crt",      "FILE",   "",                         "set FILE to custom SSL .crt file for HTTPS UI connections"
                                                               "\n" "(see akadia.com/services/ssh_test_certificate.html)"},
        {"ssl-key",      "FILE",   "",                         "set FILE to custom SSL .key file for HTTPS UI connections"
                                                               "\n" "(the passphrase MUST be removed from the .key file!)"},
        {"whitelist",    "IP",     "",                         "set IP or csv of IPs to allow UI connections,"
                                                               "\n" "alien IPs will get a zip-bomb instead"},
        {"port",         "NUMBER", "3000",                     "set NUMBER of an open port to listen for UI connections"},
        {"user",         "WORD",   "NULL",                     "set allowed WORD as username for UI connections,"
                                                               "\n" "mandatory but may be 'NULL'"},
        {"pass",         "WORD",   "NULL",                     "set allowed WORD as password for UI connections,"
                                                               "\n" "mandatory but may be 'NULL'"},
        {"database",     "FILE",   "",                         "set alternative PATH to database filename,"
                                                               "\n" "default PATH is '/data/db/K.*.*.*.db',"
                                                               "\n" "or use ':memory:' (see sqlite.org/inmemorydb.html)"},
        {"lifetime",     "NUMBER", "0",                        "set NUMBER of minimum milliseconds to keep orders open,"
                                                               "\n" "otherwise open orders can be replaced anytime required"},
        {"matryoshka",   "URL",    "https://www.example.com/", "set Matryoshka link URL of the next UI"},
        {"ignore-sun",   "2",      0,                          "do not switch UI to light theme on daylight"},
        {"ignore-moon",  "1",      0,                          "do not switch UI to dark theme on moonlight"},
        {"autobot",      "1",      0,                          "automatically start trading on boot"},
        {"dustybot",     "1",      0,                          "do not automatically cancel all orders on exit"},
        {"latency",      "1",      0,                          "check current HTTP latency (not from WS) and quit"},
        {"debug-orders", "1",      0,                          "print detailed output about exchange messages"},
        {"debug-quotes", "1",      0,                          "print detailed output about quoting engine"},
        {"debug-wallet", "1",      0,                          "print detailed output about target base position"}
      };
    };
    void tidy_values(
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
      if (num["latency"] or num["debug-orders"] or num["debug-quotes"])
        num["naked"] = 1;
      if (num["latency"] or !num["port"] or !num["client-limit"])
        num["headless"] = 1;
      str["B64auth"] = (!num["headless"]
        and str["user"] != "NULL" and !str["user"].empty()
        and str["pass"] != "NULL" and !str["pass"].empty()
      ) ? "Basic " + mText::oB64(str["user"] + ':' + str["pass"])
        : "";
      str["diskdata"] = "";
      if (str["database"].empty() or str["database"] == ":memory:")
        (str["database"] == ":memory:"
          ? str["diskdata"]
          : str["database"]
        ) = "/data/db/K"
          + ('.' + str["exchange"])
          +  '.' + str["base"]
          +  '.' + str["quote"]
          +  '.' + "db";
    };
  } options;

  class Screen: public Klass {
    public:
      virtual void pressme(const mHotkey&, function<void()>) = 0;
      virtual void printme(mToScreen *const) = 0;
      virtual void waitForUser() = 0;
      virtual const string stamp() = 0;
      virtual void logWar(const string&, const string&) = 0;
      virtual void logUI(const string&) = 0;
      virtual void logUIsess(const int&, const string&) = 0;
      virtual void log(const string&, const string&, const string& = "") = 0;
  } *screen = nullptr;

  class Events: public Klass {
    public:
      virtual void deferred(const function<void()>&) = 0;
  } *events = nullptr;

  class Sqlite: public Klass {
    public:
      virtual void backup(mFromDb *const) = 0;
  } *sqlite = nullptr;

  class Client: public Klass {
    public:
      uWS::Hub* socket = nullptr;
      virtual void timer_Xs() = 0;
      virtual void welcome(mToClient&) = 0;
      virtual void clickme(mFromClient&, function<void(const json&)>) = 0;
  } *client = nullptr;

  class Engine: public Klass {
#define SQLITE_BACKUP      \
        SQLITE_BACKUP_LIST \
      ( SQLITE_BACKUP_CODE )
#define SQLITE_BACKUP_CODE(data)         sqlite->backup(&data);
#define SQLITE_BACKUP_LIST(code)         \
  code( qp                             ) \
  code( wallet.target                  ) \
  code( wallet.safety.trades           ) \
  code( wallet.profits                 ) \
  code( levels.stats.ewma.fairValue96h ) \
  code( levels.stats.ewma              ) \
  code( levels.stats.stdev             )

#define SCREEN_PRINTME      \
        SCREEN_PRINTME_LIST \
      ( SCREEN_PRINTME_CODE )
#define SCREEN_PRINTME_CODE(data)  screen->printme(&data);
#define SCREEN_PRINTME_LIST(code)  \
  code( orders                  )  \
  code( wallet.target           )  \
  code( wallet.safety.trades    )  \
  code( levels.stats.fairPrice  )  \
  code( levels.stats.ewma       )  \
  code( broker.semaphore        )  \
  code( broker.calculon.quotes  )  \
  code( broker.calculon.dummyMM )

#define SCREEN_PRESSME      \
        SCREEN_PRESSME_LIST \
      ( SCREEN_PRESSME_CODE )
#define SCREEN_PRESSME_CODE(key, fn)    screen->pressme(mHotkey::key, [&]() { fn(); });
#define SCREEN_PRESSME_LIST(code)       \
  code(  Q  , exit                    ) \
  code(  q  , exit                    ) \
  code( ESC , broker.semaphore.toggle )

#define CLIENT_WELCOME      \
        CLIENT_WELCOME_LIST \
      ( CLIENT_WELCOME_CODE )
#define CLIENT_WELCOME_CODE(data)  client->welcome(data);
#define CLIENT_WELCOME_LIST(code)  \
  code( qp                       ) \
  code( monitor                  ) \
  code( monitor.product          ) \
  code( orders                   ) \
  code( wallet.target            ) \
  code( wallet.safety            ) \
  code( wallet.safety.trades     ) \
  code( wallet                   ) \
  code( levels.diff              ) \
  code( levels.stats.takerTrades ) \
  code( levels.stats.fairPrice   ) \
  code( levels.stats             ) \
  code( broker.semaphore         ) \
  code( broker.calculon          ) \
  code( btn.notepad              )

#define CLIENT_CLICKME      \
        CLIENT_CLICKME_LIST \
      ( CLIENT_CLICKME_CODE )
#define CLIENT_CLICKME_CODE(btn, fn, val) \
                  client->clickme(btn, [&](const json &butterfly) { fn(val); });
#define CLIENT_CLICKME_LIST(code)                                              \
  code( qp                    , savedQuotingParameters           ,           ) \
  code( broker.semaphore      , void                             ,           ) \
  code( btn.notepad           , void                             ,           ) \
  code( btn.submit            , manualSendOrder                  , butterfly ) \
  code( btn.cancel            , manualCancelOrder                , butterfly ) \
  code( btn.cancelAll         , cancelOrders                     ,           ) \
  code( btn.cleanTrade        , wallet.safety.trades.clearOne    , butterfly ) \
  code( btn.cleanTradesClosed , wallet.safety.trades.clearClosed ,           ) \
  code( btn.cleanTrades       , wallet.safety.trades.clearAll    ,           )
    public:
             mButtons btn;
             mMonitor monitor;
              mOrders orders;
        mMarketLevels levels;
      mWalletPosition wallet;
              mBroker broker;
      Engine()
        : levels(orders, monitor.product)
        , wallet(orders, levels.stats.ewma.targetPositionAutoPercentage, levels.fairValue)
        , broker(orders, monitor.product, levels, wallet)
      {};
      void savedQuotingParameters() {
        broker.calculon.dummyMM.mode("saved");
        levels.stats.ewma.calcFromHistory();
      };
      void timer_1s(const unsigned int &tick) {
        if (levels.warn_empty()) return;
        levels.timer_1s();
        if (!(tick % 60)) {
          levels.timer_60s();
          monitor.timer_60s();
        }
        wallet.safety.timer_1s();
        calcQuotes();
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
        const unsigned int replace = gw->askForReplace and !(
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
        else placeOrder(mOrder(
          gw->randId(), quote.side, quote.price, quote.size, quote.isPong
        ));
        monitor.tick_orders();
      };
      void cancelOrders() {
        for (mOrder *const it : orders.working())
          cancelOrder(it);
      };
      void manualSendOrder(mOrder raw) {
        raw.orderId = gw->randId();
        placeOrder(raw);
      };
      void manualCancelOrder(const mRandId &orderId) {
        cancelOrder(orders.find(orderId));
      };
    private:
      void placeOrder(const mOrder &raw) {
        gw->place(orders.upsert(raw));
      };
      void replaceOrder(const mPrice &price, const bool &isPong, mOrder *const order) {
        if (orders.replace(price, isPong, order))
          gw->replace(order);
      };
      void cancelOrder(mOrder *const order) {
        if (orders.cancel(order))
          gw->cancel(order);
      };
  } *engine = nullptr;
}

#endif