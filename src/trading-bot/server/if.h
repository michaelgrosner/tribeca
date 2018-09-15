#ifndef K_IF_H_
#define K_IF_H_

namespace K {
  class Screen {
    public:
      virtual void pressme(const mHotkey&, function<void()>) = 0;
      virtual void printme(mToScreen *const) = 0;
      virtual void waitForUser() = 0;
      virtual const int error(const string&, const string&, const bool& = false) = 0;
      virtual const string stamp() = 0;
      virtual void logWar(string, string, string = " Warrrrning: ") = 0;
      virtual void logUI(const string&) = 0;
      virtual void logUIsess(int, string) = 0;
      virtual void log(const string&, const string&, const string& = "") = 0;
      virtual void switchOff() = 0;
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
  code(  Q  , gw->quit                ) \
  code(  q  , gw->quit                ) \
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