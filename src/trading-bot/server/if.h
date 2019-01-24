#ifndef K_IF_H_
#define K_IF_H_

class TradingBot: public KryptoNinja {
  public:
    static void terminal();
    TradingBot()
    {
      display   = terminal;
      margin    = {3, 6, 1, 2};
      databases = true;
      arguments = { {
        {"wallet-limit", "AMOUNT", "0",                    "set AMOUNT in base currency to limit the balance,"
                                                           "\n" "otherwise the full available balance can be used"},
        {"client-limit", "NUMBER", "7",                    "set NUMBER of maximum concurrent UI connections"},
        {"headless",     "1",      nullptr,                "do not listen for UI connections,"
                                                           "\n" "all other UI related arguments will be ignored"},
        {"without-ssl",  "1",      nullptr,                "do not use HTTPS for UI connections (use HTTP only)"},
        {"ssl-crt",      "FILE",   "",                     "set FILE to custom SSL .crt file for HTTPS UI connections"
                                                           "\n" "(see www.akadia.com/services/ssh_test_certificate.html)"},
        {"ssl-key",      "FILE",   "",                     "set FILE to custom SSL .key file for HTTPS UI connections"
                                                           "\n" "(the passphrase MUST be removed from the .key file!)"},
        {"whitelist",    "IP",     "",                     "set IP or csv of IPs to allow UI connections,"
                                                           "\n" "alien IPs will get a zip-bomb instead"},
        {"port",         "NUMBER", "3000",                 "set NUMBER of an open port to listen for UI connections"},
        {"user",         "WORD",   "NULL",                 "set allowed WORD as username for UI connections,"
                                                           "\n" "mandatory but may be 'NULL'"},
        {"pass",         "WORD",   "NULL",                 "set allowed WORD as password for UI connections,"
                                                           "\n" "mandatory but may be 'NULL'"},
        {"lifetime",     "NUMBER", "0",                    "set NUMBER of minimum milliseconds to keep orders open,"
                                                           "\n" "otherwise open orders can be replaced anytime required"},
        {"matryoshka",   "URL",    "https://example.com/", "set Matryoshka link URL of the next UI"},
        {"ignore-sun",   "2",      nullptr,                "do not switch UI to light theme on daylight"},
        {"ignore-moon",  "1",      nullptr,                "do not switch UI to dark theme on moonlight"},
        {"autobot",      "1",      nullptr,                "automatically start trading on boot"},
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
        if (num["latency"] or !num["port"] or !num["client-limit"])
          num["headless"] = 1;
        str["B64auth"] = (!num["headless"]
          and str["user"] != "NULL" and !str["user"].empty()
          and str["pass"] != "NULL" and !str["pass"].empty()
        ) ? "Basic " + Text::B64(str["user"] + ':' + str["pass"])
          : "";
      } };
    };
} K;

class Client: public Klass {
  public:
    const unsigned int *delay = nullptr;
    string protocol  = "HTTP";
    virtual void welcome(mToClient&) = 0;
    virtual void clickme(mFromClient&, function<void(const json&)>) = 0;
} *client = nullptr;

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
#define CLIENT_WELCOME_CODE(data) client->welcome(data);
#define CLIENT_WELCOME_LIST(code) \
code( qp                       )  \
code( monitor                  )  \
code( monitor.product          )  \
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
                client->clickme(btn, [&](const json &butterfly) { fn(val); });
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
           mButtons btn;
           mMonitor monitor;
            mOrders orders;
     mQuotingParams qp;
      mMarketLevels levels;
    mWalletPosition wallet;
            mBroker broker;
    Engine()
      : monitor(K)
      , orders(monitor.product)
      , levels(monitor.product, orders, qp)
      , wallet(monitor.product, orders, qp, levels.stats.ewma.targetPositionAutoPercentage, levels.fairValue)
      , broker(monitor.product, orders, qp, levels, wallet)
    {};
    void savedQuotingParameters() {
      K.timer_ticks_factor(qp.delayUI);
      broker.calculon.dummyMM.mode("saved");
      levels.stats.ewma.calcFromHistory(qp._diffEwma);
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
      else placeOrder({
        quote.side,
        quote.price,
        quote.size,
        Tstamp,
        quote.isPong,
        gw->randId()
      });
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
    void manualCancelOrder(const RandId &orderId) {
      cancelOrder(orders.find(orderId));
    };
  protected:
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
  private:
    void placeOrder(const mOrder &raw) {
      gw->place(orders.upsert(raw));
    };
    void replaceOrder(const Price &price, const bool &isPong, mOrder *const order) {
      if (orders.replace(price, isPong, order))
        gw->replace(order);
    };
    void cancelOrder(mOrder *const order) {
      if (orders.cancel(order))
        gw->cancel(order);
    };
} *engine = nullptr;

#endif