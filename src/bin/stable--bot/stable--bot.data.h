//! \file
//! \brief Welcome user! (just a stable bot for flat markets with constant buy/sell prices).

namespace analpaper {
  struct Wallets {
    Wallet base,
           quote;
    private_ref:
      const KryptoNinja &K;
    public:
      Wallets(const KryptoNinja &bot)
        : K(bot)
      {};
      void read_from_gw(const Wallet &raw) {
        if      (raw.currency == K.gateway->base)  base  = raw;
        else if (raw.currency == K.gateway->quote) quote = raw;
      };
      bool ready() const {
        const bool err = base.currency.empty() or quote.currency.empty();
        if (err and Tspent > 21e+3)
          K.warn("QE", "Unable to calculate quote, missing wallet data", 3e+3);
        return !err;
      };
  };

  struct Orders: public System::Orderbook {
    private_ref:
      const KryptoNinja &K;
    public:
      Orders(const KryptoNinja &bot)
        : Orderbook(bot)
        , K(bot)
      {};
      void read_from_gw(const Order &order) {
        if (!order.orderId.empty() and order.justFilled)
          K.log("GW " + K.gateway->exchange,
            string(order.side == Side::Bid
              ? ANSI_PUKE_CYAN    + "TRADE BUY  "
              : ANSI_PUKE_MAGENTA + "TRADE SELL "
            )
            + K.gateway->decimal.amount.str(order.justFilled)
            + " " + K.gateway->base + " at "
            + K.gateway->decimal.price.str(order.price)
            + " " + K.gateway->quote);
      };
  };

  struct MarketLevels: public Levels {
    Price fairValue = 0;
    private:
      Levels unfiltered;
      unordered_map<Price, Amount> filterBidOrders,
                                   filterAskOrders;
    private_ref:
      const KryptoNinja &K;
      const Orders      &orders;
    public:
      MarketLevels(const KryptoNinja &bot, const Orders &o)
        : K(bot)
        , orders(o)
      {};
      void read_from_gw(const Levels &raw) {
        unfiltered.bids = raw.bids;
        unfiltered.asks = raw.asks;
        filter();
      };
      bool ready() {
        filter();
        if (!fairValue and Tspent > 21e+3)
          K.warn("QE", "Unable to calculate quote, missing market data", 10e+3);
        return fairValue;
      };
    private:
      void filter() {
        orders.resetFilters(&filterBidOrders, &filterAskOrders);
        bids = filter(unfiltered.bids, &filterBidOrders);
        asks = filter(unfiltered.asks, &filterAskOrders);
        if (bids.empty() or asks.empty())
          fairValue = 0;
        else
          fairValue = (bids.cbegin()->price
                     + asks.cbegin()->price) / 2;
      };
      vector<Level> filter(vector<Level> levels, unordered_map<Price, Amount> *const filterOrders) {
        if (!filterOrders->empty())
          for (auto it = levels.begin(); it != levels.end();) {
            for (auto it_ = filterOrders->begin(); it_ != filterOrders->end();)
              if (abs(it->price - it_->first) < K.gateway->tickPrice) {
                it->size -= it_->second;
                filterOrders->erase(it_);
                break;
              } else ++it_;
            if (it->size < K.gateway->minSize) it = levels.erase(it);
            else ++it;
            if (filterOrders->empty()) break;
          }
        return levels;
      };
  };

  struct AntonioCalculon: public System::Quotes {
    private_ref:
      const KryptoNinja  &K;
      const MarketLevels &levels;
      const Wallets      &wallet;
    public:
      AntonioCalculon(const KryptoNinja &bot, const MarketLevels &l, const Wallets &w)
        : Quotes(bot)
        , K(bot)
        , levels(l)
        , wallet(w)
      {};
    private:
      string explainState(const System::Quote &quote) const override {
        string reason = "";
        if (quote.state == QuoteState::Live)
          reason = "  LIVE   " + ANSI_PUKE_WHITE
                 + "because of reasons (ping: "
                 + K.gateway->decimal.price.str(quote.price) + " " + K.gateway->quote
                 + ", fair value: "
                 + K.gateway->decimal.price.str(levels.fairValue) + " " + K.gateway->quote
                 +")";
        else if (quote.state == QuoteState::DepletedFunds)
          reason = " PAUSED  " + ANSI_PUKE_WHITE
                 + "because not enough available funds ("
                 + (quote.side == Side::Bid
                   ? K.gateway->decimal.price.str(wallet.quote.amount) + " " + K.gateway->quote
                   : K.gateway->decimal.amount.str(wallet.base.amount) + " " + K.gateway->base
                 ) + ")";
        else if (quote.state == QuoteState::DisabledQuotes)
          reason = "DISABLED " + ANSI_PUKE_WHITE
                 + "because " + (quote.side == Side::Bid ? "--bid-price" : "--ask-price")
                 + " was not set";
        else if (quote.state == QuoteState::Disconnected)
          reason = " PAUSED  " + ANSI_PUKE_WHITE
                 + "because the exchange seems down";
        return reason;
      };
      void calcRawQuotes() override {
        bid.size =
        ask.size = K.arg<double>("order-size");
        bid.price = fmin(
          levels.fairValue - K.gateway->tickPrice,
          K.arg<double>("bid-price")
        );
        ask.price = fmax(
          levels.fairValue + K.gateway->tickPrice,
          K.arg<double>("ask-price")
        );
      };
      void applyQuotingParameters() override {
        debug("?"); applyStableSide();
        debug("A"); applyBestWidth();
        debug("B"); applyRoundPrice();
        debug("C"); applyRoundSize();
        debug("D"); applyDepleted();
        debug("!");
      };
      void applyStableSide() {
        if (!K.arg<double>("bid-price"))
          bid.skip(QuoteState::DisabledQuotes);
        if (!K.arg<double>("ask-price"))
          ask.skip(QuoteState::DisabledQuotes);
      };
      void applyBestWidth() {
        if (!ask.empty())
          for (const Level &it : levels.asks)
            if (it.price > ask.price) {
              const Price bestAsk = it.price - K.gateway->tickPrice;
              if (bestAsk > ask.price)
                ask.price = bestAsk;
              break;
            }
        if (!bid.empty())
          for (const Level &it : levels.bids)
            if (it.price < bid.price) {
              const Price bestBid = it.price + K.gateway->tickPrice;
              if (bestBid < bid.price)
                bid.price = bestBid;
              break;
            }
      };
      void applyRoundPrice() {
        if (!bid.empty())
          bid.price = fmax(
            0,
            K.gateway->decimal.price.round(bid.price)
          );
        if (!ask.empty())
          ask.price = fmax(
            bid.price + K.gateway->tickPrice,
            K.gateway->decimal.price.round(ask.price)
          );
      };
      void applyRoundSize() {
        if (!bid.empty()) {
          const Amount minBid = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / bid.price)
            : K.gateway->minSize;
          const Amount maxBid = K.gateway->margin == Future::Spot
            ? wallet.quote.total / bid.price
            : (K.gateway->margin == Future::Inverse
                ? wallet.base.amount * bid.price
                : wallet.base.amount / bid.price
            );
          bid.size = K.gateway->decimal.amount.round(
            fmax(minBid * (1.0 + K.gateway->takeFee * 1e+2), fmin(
              bid.size,
              K.gateway->decimal.amount.floor(maxBid)
            ))
          );
        }
        if (!ask.empty()) {
          const Amount minAsk = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / ask.price)
            : K.gateway->minSize;
          const Amount maxAsk = K.gateway->margin == Future::Spot
            ? wallet.base.total
            : (K.gateway->margin == Future::Inverse
                ? (bid.empty()
                  ? wallet.base.amount * ask.price
                  : bid.size)
                : wallet.base.amount / ask.price
            );
          ask.size = K.gateway->decimal.amount.round(
            fmax(minAsk * (1.0 + K.gateway->takeFee * 1e+2), fmin(
              ask.size,
              K.gateway->decimal.amount.floor(maxAsk)
            ))
          );
        }
      };
      void applyDepleted() {
        if (!bid.empty()) {
          const Amount minBid = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / bid.price)
            : K.gateway->minSize;
          if ((K.gateway->margin == Future::Spot
              ? wallet.quote.total / bid.price
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * bid.price
                  : wallet.base.amount / bid.price)
              ) < minBid
          ) bid.skip(QuoteState::DepletedFunds);
        }
        if (!ask.empty()) {
          const Amount minAsk = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / ask.price)
            : K.gateway->minSize;
          if ((K.gateway->margin == Future::Spot
              ? wallet.base.total
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * ask.price
                  : wallet.base.amount / ask.price)
              ) < minAsk
          ) ask.skip(QuoteState::DepletedFunds);
        }
      };
  };

  struct Broker {
       Connectivity greenGateway = Connectivity::Disconnected;
    AntonioCalculon quotes;
    private_ref:
      const KryptoNinja  &K;
            Orders       &orders;
      const MarketLevels &levels;
    public:
      Broker(const KryptoNinja &bot, Orders &o, const MarketLevels &l, const Wallets &w)
        : quotes(bot, l, w)
        , K(bot)
        , orders(o)
        , levels(l)
      {};
      void read_from_gw(const Connectivity &raw) {
        greenGateway = raw;
        if (!(bool)greenGateway)
          quotes.offline();
      };
      bool ready() const {
        return (bool)greenGateway;
      };
      void calcQuotes() {
        quotes.calcQuotes();
        quote2orders(quotes.ask);
        quote2orders(quotes.bid);
      };
      void timer_60s() {
        if (K.arg<int>("heartbeat") and levels.fairValue)
          K.log("HB", ((json){
            {"bid|fv|ask", K.gateway->decimal.price.str(levels.fairValue)
                         + "|"
                         + K.gateway->decimal.price.str(
                             levels.bids.empty() ? 0 : levels.bids.begin()->price)
                         + "|"
                         + K.gateway->decimal.price.str(
                             levels.asks.empty() ? 0 : levels.asks.begin()->price)}
          }).dump());
      };
      void quit() {
        unsigned int n = 0;
        for (Order *const it : orders.open()) {
          K.gateway->cancel(it);
          n++;
        }
        if (n)
          K.log("QE", "Canceled " + to_string(n) + " open order" + string(n != 1, 's') + " before quit");
      };
    private:
      bool abandon(const Order &order, System::Quote &quote) {
        if (orders.zombies.stillAlive(order)) {
          if (order.status == Status::Waiting
            or abs(order.price - quote.price) < K.gateway->tickPrice
            or (K.arg<int>("lifetime") and order.time + K.arg<int>("lifetime") > Tstamp)
          ) quote.skip();
          else return true;
        }
        return false;
      };
      vector<Order*> abandon(System::Quote &quote) {
        vector<Order*> abandoned;
        const bool all = quote.state != QuoteState::Live;
        for (Order *const it : orders.at(quote.side))
          if (all or abandon(*it, quote))
            abandoned.push_back(it);
        return abandoned;
      };
      void quote2orders(System::Quote &quote) {
        const vector<Order*> abandoned = abandon(quote);
        const unsigned int replace = K.gateway->askForReplace and !(
          quote.empty() or abandoned.empty()
        );
        for (
          auto it  =  abandoned.end() - replace;
               it --> abandoned.begin();
          K.cancel(*it)
        );
        if (quote.empty()) return;
        if (replace) K.replace(quote.price, quote.isPong, abandoned.back());
        else K.place({
          quote.side,
          quote.price,
          quote.size,
          Tstamp,
          quote.isPong,
          K.gateway->randId()
        });
      };
  };

  class Engine {
    private:
            Orders orders;
      MarketLevels levels;
           Wallets wallet;
            Broker broker;
    public:
      Engine(const KryptoNinja &bot)
        : orders(bot)
        , levels(bot, orders)
        , wallet(bot)
        , broker(bot, orders, levels, wallet)
      {};
      void read(const Connectivity &rawdata) {
        broker.read_from_gw(rawdata);
      };
      void read(const Wallet &rawdata) {
        wallet.read_from_gw(rawdata);
      };
      void read(const Levels &rawdata) {
        levels.read_from_gw(rawdata);
        calcQuotes();
      };
      void read(const Order &rawdata) {
        orders.read_from_gw(rawdata);
      };
      void timer_1s(const unsigned int &tick) {
        if (!(tick % 60))
          broker.timer_60s();
        calcQuotes();
      };
      void quit() {
        broker.quit();
      };
    private:
      void calcQuotes() {
        if (broker.ready() and levels.ready() and wallet.ready())
          broker.calcQuotes();
        orders.zombies.purge();
      };
  };
}
