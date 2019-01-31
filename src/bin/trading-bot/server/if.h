#ifndef K_IF_H_
#define K_IF_H_

class Client: public Klass {
  public:
    string protocol = "HTTP";
    const unsigned int *delay = nullptr;
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
           mProduct product;
            mOrders orders;
     mQuotingParams qp;
      mMarketLevels levels;
    mWalletPosition wallet;
            mBroker broker;
    Engine()
      : monitor(K)
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
} *engine = nullptr;

#endif