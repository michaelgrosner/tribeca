#ifndef K_IF_H_
#define K_IF_H_

namespace K {
  struct Options: public Arguments {
    void default_values() {
      optstr["title"]        = "K.sh";
      optstr["matryoshka"]   = "https://www.example.com/";
      optstr["user"]         = "NULL";
      optstr["pass"]         = "NULL";
      optstr["apikey"]       = "NULL";
      optstr["secret"]       = "NULL";
      optstr["username"]     = "NULL";
      optstr["passphrase"]   = "NULL";
      optstr["http"]         = "NULL";
      optstr["wss"]          = "NULL";
      optint["port"]         = 3000;
      optint["lifetime"]     = 0;
      optint["market-limit"] = 321;
      optint["client-limit"] = 7;
      optdec["wallet-limit"] = 0;
    };
    void tidy_values() {
      if (optint["debug"])
        optint["debug-secret"] =
        optint["debug-orders"] =
        optint["debug-quotes"] =
        optint["debug-wallet"] = 1;
      if (optstr["database"].empty() or optstr["database"] == ":memory:")
        (optstr["database"] == ":memory:"
          ? optstr["diskdata"]
          : optstr["database"]
        ) = "/data/db/K"
          + ('.' + optstr["exchange"])
          +  '.' + optstr["base"]
          +  '.' + optstr["quote"]
          +  '.' + "db";
      optint["market-limit"] = max(15, optint["market-limit"]);
      if (optint["ignore-sun"] and optint["ignore-moon"]) optint["ignore-moon"] = 0;
  #ifndef _WIN32
      if (optint["latency"] or optint["debug-secret"] or optint["debug-orders"] or optint["debug-quotes"])
  #endif
        optint["naked"] = 1;
      if (optint["latency"] or !optint["port"] or !optint["client-limit"]) optint["headless"] = 1;
      if (!optint["headless"]
        and optstr["user"] != "NULL" and !optstr["user"].empty()
        and optstr["pass"] != "NULL" and !optstr["pass"].empty()
      ) optstr["B64auth"] = "Basic " + mText::oB64(optstr["user"] + ':' + optstr["pass"]);
    };
    const vector<option> custom_long_options() {
      return {
        {"debug",        no_argument,       &optint["debug"],         1},
        {"debug-secret", no_argument,       &optint["debug-secret"],  1},
        {"debug-orders", no_argument,       &optint["debug-orders"],  1},
        {"debug-quotes", no_argument,       &optint["debug-quotes"],  1},
        {"debug-wallet", no_argument,       &optint["debug-wallet"],  1},
        {"without-ssl",  no_argument,       &optint["without-ssl"],   1},
        {"ignore-sun",   no_argument,       &optint["ignore-sun"],    2},
        {"ignore-moon",  no_argument,       &optint["ignore-moon"],   1},
        {"headless",     no_argument,       &optint["headless"],      1},
        {"naked",        no_argument,       &optint["naked"],         1},
        {"autobot",      no_argument,       &optint["autobot"],       1},
        {"dustybot",     no_argument,       &optint["dustybot"],      1},
        {"latency",      no_argument,       &optint["latency"],       1},
        {"free-version", no_argument,       &optint["free-version"],  1},
        {"port",         required_argument, 0,                      999},
        {"user",         required_argument, 0,                      998},
        {"pass",         required_argument, 0,                      997},
        {"database",     required_argument, 0,                      995},
        {"apikey",       required_argument, 0,                      994},
        {"secret",       required_argument, 0,                      993},
        {"passphrase",   required_argument, 0,                      992},
        {"username",     required_argument, 0,                      991},
        {"http",         required_argument, 0,                      990},
        {"wss",          required_argument, 0,                      989},
        {"ssl-crt",      required_argument, 0,                      988},
        {"ssl-key",      required_argument, 0,                      987},
        {"whitelist",    required_argument, 0,                      986},
        {"title",        required_argument, 0,                      985},
        {"matryoshka",   required_argument, 0,                      984},
        {"lifetime",     required_argument, 0,                      983},
        {"wallet-limit", required_argument, 0,                      982},
        {"market-limit", required_argument, 0,                      981},
        {"client-limit", required_argument, 0,                      980}
      };
    };
    const string custom_help(const function<string()> &stamp) {
      return stamp() + "    --latency             - check current HTTP latency (not from WS) and quit." + '\n'
           + stamp() + "    --autobot             - automatically start trading on boot." + '\n'
           + stamp() + "    --dustybot            - do not automatically cancel all orders on exit." + '\n'
           + stamp() + "    --naked               - do not display CLI, print output to stdout instead." + '\n'
           + stamp() + "    --headless            - do not listen for UI connections," + '\n'
           + stamp() + "                            all other UI related arguments will be ignored." + '\n'
           + stamp() + "    --client-limit=NUMBER - set NUMBER of maximum concurrent UI connections." + '\n'
           + stamp() + "    --without-ssl         - do not use HTTPS for UI connections (use HTTP only)." + '\n'
           + stamp() + "    --ssl-crt=FILE        - set FILE to custom SSL .crt file for HTTPS UI connections" + '\n'
           + stamp() + "                            (see akadia.com/services/ssh_test_certificate.html)." + '\n'
           + stamp() + "    --ssl-key=FILE        - set FILE to custom SSL .key file for HTTPS UI connections" + '\n'
           + stamp() + "                            (the passphrase MUST be removed from the .key file!)." + '\n'
           + stamp() + "    --whitelist=IP        - set IP or csv of IPs to allow UI connections," + '\n'
           + stamp() + "                            alien IPs will get a zip-bomb instead." + '\n'
           + stamp() + "    --port=NUMBER         - set NUMBER of an open port to listen for UI connections." + '\n'
           + stamp() + "    --user=WORD           - set allowed WORD as username for UI connections," + '\n'
           + stamp() + "                            mandatory but may be 'NULL'." + '\n'
           + stamp() + "    --pass=WORD           - set allowed WORD as password for UI connections," + '\n'
           + stamp() + "                            mandatory but may be 'NULL'." + '\n'
           + stamp() + "    --apikey=WORD         - set (never share!) WORD as api key for trading," + '\n'
           + stamp() + "                            mandatory." + '\n'
           + stamp() + "    --secret=WORD         - set (never share!) WORD as api secret for trading," + '\n'
           + stamp() + "                            mandatory." + '\n'
           + stamp() + "    --passphrase=WORD     - set (never share!) WORD as api passphrase for trading," + '\n'
           + stamp() + "                            mandatory but may be 'NULL'." + '\n'
           + stamp() + "    --username=WORD       - set (never share!) WORD as api username for trading," + '\n'
           + stamp() + "                            mandatory but may be 'NULL'." + '\n'
           + stamp() + "    --http=URL            - set URL of api HTTP/S endpoint for trading," + '\n'
           + stamp() + "                            mandatory." + '\n'
           + stamp() + "    --wss=URL             - set URL of api SECURE WS endpoint for trading," + '\n'
           + stamp() + "                            mandatory." + '\n'
           + stamp() + "    --database=PATH       - set alternative PATH to database filename," + '\n'
           + stamp() + "                            default PATH is '/data/db/K.*.*.*.db'," + '\n'
           + stamp() + "                            any route to a filename is valid," + '\n'
           + stamp() + "                            or use ':memory:' (see sqlite.org/inmemorydb.html)." + '\n'
           + stamp() + "    --wallet-limit=AMOUNT - set AMOUNT in base currency to limit the balance," + '\n'
           + stamp() + "                            otherwise the full available balance can be used." + '\n'
           + stamp() + "    --market-limit=NUMBER - set NUMBER of maximum price levels for the orderbook," + '\n'
           + stamp() + "                            default NUMBER is '321' and the minimum is '15'." + '\n'
           + stamp() + "                            locked bots smells like '--market-limit=3' spirit." + '\n'
           + stamp() + "    --lifetime=NUMBER     - set NUMBER of minimum milliseconds to keep orders open," + '\n'
           + stamp() + "                            otherwise open orders can be replaced anytime required." + '\n'
           + stamp() + "    --debug-secret        - print (never share!) secret inputs and outputs." + '\n'
           + stamp() + "    --debug-orders        - print detailed output about exchange messages." + '\n'
           + stamp() + "    --debug-quotes        - print detailed output about quoting engine." + '\n'
           + stamp() + "    --debug-wallet        - print detailed output about target base position." + '\n'
           + stamp() + "    --debug               - print detailed output about all the (previous) things!" + '\n'
           + stamp() + "    --ignore-sun          - do not switch UI to light theme on daylight." + '\n'
           + stamp() + "    --ignore-moon         - do not switch UI to dark theme on moonlight." + '\n'
           + stamp() + "    --matryoshka=URL      - set Matryoshka link URL of the next UI." + '\n'
           + stamp() + "    --title=WORD          - set WORD as UI title to identify different bots." + '\n'
           + stamp() + "    --free-version        - work with all market levels and enable the slow XMR miner." + '\n';
    };
  } options;

  class Screen {
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

  class Events {
    public:
      virtual void deferred(const function<void()>&) = 0;
  } *events = nullptr;

  class Sqlite {
    public:
      virtual void backup(mFromDb *const) = 0;
  } *sqlite = nullptr;

  class Client {
    public:
      uWS::Hub* socket = nullptr;
      virtual void timer_Xs() = 0;
      virtual void welcome(mToClient&) = 0;
      virtual void clickme(mFromClient&, function<void(const json&)>) = 0;
  } *client = nullptr;

  class Engine {
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
  code( *gw                     )  \
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
        const bool replace = gw->askForReplace
          and !(quote.empty() or abandoned.empty());
        for_each(
          abandoned.begin(), abandoned.end() - (replace ? 1 : 0),
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