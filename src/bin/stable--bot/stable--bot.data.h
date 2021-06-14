//! \file
//! \brief Welcome user! (just a stable bot for flat markets with constant buy/sell prices).

namespace analpaper {
  enum class QuoteState: unsigned int {
    Disconnected,  Live,        DisabledQuotes,
    MissingData,   UnknownHeld, WidthTooHigh,
    DepletedFunds, Crossed
  };

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
          K.logWar("QE", "Unable to calculate quote, missing wallet data", 3e+3);
        return !err;
      };
  };

  struct LastOrder {
     Price price;
    Amount filled;
      Side side;
      bool isPong;
  };
  struct Orders {
    LastOrder last;
    private:
      unordered_map<string, Order> orders;
    private_ref:
      const KryptoNinja  &K;
    public:
      Orders(const KryptoNinja &bot)
        : last()
        , K(bot)
      {};
      void read_from_gw(const Order &raw) {
        if (K.arg<int>("debug-orders"))
          K.log("GW " + K.gateway->exchange, "  reply: " + ((json)raw).dump());
        last = {0, 0, (Side)0, false};
        Order *const order = upsert(raw);
        if (!order) return;
        last = {
          order->price,
          raw.filled,
          order->side,
          order->isPong
        };
        if (last.filled) K.gateway->balance();
        if (order->isPong or last.filled)
          K.log("GW " + K.gateway->exchange, string("TRADE ")
            + (order->side == Side::Bid ? "BUY  " : "SELL ")
            + K.gateway->decimal.amount.str(last.filled)
            + " " + K.gateway->base + " at "
            + K.gateway->decimal.price.str(order->price)
            + " " + K.gateway->quote);
        if (order->status == Status::Terminated)
          purge(order);
        if (K.arg<int>("debug-orders"))
          K.log("GW " + K.gateway->exchange, " active: " + to_string(orders.size()));
      };
      Order *upsert(const Order &raw) {
        Order *const order = findsert(raw);
        Order::update(raw, order);
        return order;
      };
      void purge(const Order *const order) {
        orders.erase(order->orderId);
      };
      vector<Order*> at(const Side &side) {
        vector<Order*> sideOrders;
        for (auto &it : orders)
          if (side == it.second.side)
             sideOrders.push_back(&it.second);
        return sideOrders;
      };
      vector<Order*> open() {
        vector<Order*> autoOrders;
        for (auto &it : orders)
          if (!it.second.manual)
            autoOrders.push_back(&it.second);
        return autoOrders;
      };
      bool replace(const Price &price, const bool &isPong, Order *const order) {
        const bool allowed = Order::replace(price, isPong, order);
        if (allowed and K.arg<int>("debug-orders")) {
          K.log("GW " + K.gateway->exchange, "replace: " + order->orderId);
          K.log("GW " + K.gateway->exchange, " active: " + to_string(orders.size()));
        }
        return allowed;
      };
      bool cancel(Order *const order) {
        const bool allowed = Order::cancel(order);
        if (allowed and K.arg<int>("debug-orders")) {
          K.log("GW " + K.gateway->exchange, " cancel: " + order->orderId);
          K.log("GW " + K.gateway->exchange, " active: " + to_string(orders.size()));
        }
        return allowed;
      };
      void resetFilters(
        unordered_map<Price, Amount> *const filterBidOrders,
        unordered_map<Price, Amount> *const filterAskOrders
      ) const {
        filterBidOrders->clear();
        filterAskOrders->clear();
        for (const auto &it : orders)
          (it.second.side == Side::Bid
            ? *filterBidOrders
            : *filterAskOrders
          )[it.second.price] += it.second.quantity;
      };
    private:
      Order *find(const string &orderId) {
        return (orderId.empty()
          or orders.find(orderId) == orders.end()
        ) ? nullptr
          : &orders.at(orderId);
      };
      Order *findsert(const Order &raw) {
        if (raw.status == Status::Waiting and !raw.orderId.empty())
          return &(orders[raw.orderId] = raw);
        if (raw.orderId.empty() and !raw.exchangeId.empty()) {
          auto it = find_if(
            orders.begin(), orders.end(),
            [&](const pair<string, Order> &it_) {
              return raw.exchangeId == it_.second.exchangeId;
            }
          );
          if (it != orders.end())
            return &it->second;
        }
        return find(raw.orderId);
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
          K.logWar("QE", "Unable to calculate quote, missing market data", 10e+3);
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

  struct Quote: public Level {
    const Side       side   = (Side)0;
          QuoteState state  = QuoteState::MissingData;
          bool       isPong = false;
    Quote(const Side &s)
      : side(s)
    {};
    bool empty() const {
      return !size or !price;
    };
    void skip() {
      size = 0;
    };
    void clear(const QuoteState &reason) {
      price = size = 0;
      state = reason;
    };
    virtual bool deprecates(const Price&) const = 0;
    bool checkCrossed(const Quote &opposite) {
      if (empty()) return false;
      if (opposite.empty() or deprecates(opposite.price)) {
        state = QuoteState::Live;
        return false;
      }
      state = QuoteState::Crossed;
      return true;
    };
  };
  struct QuoteBid: public Quote {
    QuoteBid()
      : Quote(Side::Bid)
    {};
    bool deprecates(const Price &higher) const override {
      return price < higher;
    };
  };
  struct QuoteAsk: public Quote {
    QuoteAsk()
      : Quote(Side::Ask)
    {};
    bool deprecates(const Price &lower) const override {
      return price > lower;
    };
  };
  struct Quotes {
    QuoteBid bid;
    QuoteAsk ask;
    private_ref:
      const KryptoNinja &K;
    public:
      Quotes(const KryptoNinja &bot)
        : K(bot)
      {};
      void checkCrossedQuotes() {
        if (bid.checkCrossed(ask) or ask.checkCrossed(bid))
          K.logWar("QE", "Crossed bid/ask quotes detected, that is.. unexpected", 3e+3);
      };
      void debug(const string &step) {
        if (K.arg<int>("debug-quotes"))
          K.log("DEBUG QE", "[" + step + "] "
            + to_string((int)bid.state) + ":"
            + to_string((int)ask.state) + " "
            + ((json){{"bid", bid}, {"ask", ask}}).dump()
          );
      };
  };

  struct AntonioCalculon {
    Quotes quotes;
    private:
      vector<const Order*> zombies;
    private_ref:
      const KryptoNinja  &K;
      const MarketLevels &levels;
      const Wallets      &wallet;
    public:
      AntonioCalculon(const KryptoNinja &bot, const MarketLevels &l, const Wallets &w)
        : quotes(bot)
        , K(bot)
        , levels(l)
        , wallet(w)
      {};
      void calcQuotes() {
        states(QuoteState::UnknownHeld);
        calcRawQuotes();
        applyQuotingParameters();
      };
      vector<const Order*> purge() {
        vector<const Order*> zombies_;
        zombies.swap(zombies_);
        return zombies_;
      };
      bool abandon(const Order &order, Quote &quote) {
        if (stillAlive(order)) {
          if (order.status == Status::Waiting
            or abs(order.price - quote.price) < K.gateway->tickPrice
            or (K.arg<int>("lifetime") and order.time + K.arg<int>("lifetime") > Tstamp)
          ) quote.skip();
          else return true;
        }
        return false;
      };
      void states(const QuoteState &state) {
        quotes.bid.state =
        quotes.ask.state = state;
      };
    private:
      bool stillAlive(const Order &order) {
        if (order.status == Status::Waiting) {
          if (Tstamp - 10e+3 > order.time) {
            zombies.push_back(&order);
            return false;
          }
        }
        return true;
      };
      void calcRawQuotes() {
        quotes.bid.size =
        quotes.ask.size = K.arg<double>("order-size");
        quotes.bid.price = fmin(
          levels.fairValue - K.gateway->tickPrice,
          K.arg<double>("bid-price")
        );
        quotes.ask.price = fmax(
          levels.fairValue + K.gateway->tickPrice,
          K.arg<double>("ask-price")
        );
        if (quotes.bid.price <= 0 or quotes.ask.price <= 0) {
          quotes.bid.clear(QuoteState::WidthTooHigh);
          quotes.ask.clear(QuoteState::WidthTooHigh);
          K.logWar("QP", "Negative price detected, widthPing must be lower", 3e+3);
        }
      };
      void applyQuotingParameters() {
        quotes.debug("?"); applyStableSide();
        quotes.debug("A"); applyBestWidth();
        quotes.debug("B"); applyRoundPrice();
        quotes.debug("C"); applyRoundSize();
        quotes.debug("D"); applyDepleted();
        quotes.debug("!");
        quotes.checkCrossedQuotes();
      };
      void applyStableSide() {
        if (!K.arg<double>("bid-price"))
          quotes.bid.clear(QuoteState::DisabledQuotes);
        if (!K.arg<double>("ask-price"))
          quotes.ask.clear(QuoteState::DisabledQuotes);
      };
      void applyBestWidth() {
        if (!quotes.ask.empty())
          for (const Level &it : levels.asks)
            if (it.price > quotes.ask.price) {
              const Price bestAsk = it.price - K.gateway->tickPrice;
              if (bestAsk > quotes.ask.price)
                quotes.ask.price = bestAsk;
              break;
            }
        if (!quotes.bid.empty())
          for (const Level &it : levels.bids)
            if (it.price < quotes.bid.price) {
              const Price bestBid = it.price + K.gateway->tickPrice;
              if (bestBid < quotes.bid.price)
                quotes.bid.price = bestBid;
              break;
            }
      };
      void applyRoundPrice() {
        if (!quotes.bid.empty())
          quotes.bid.price = fmax(
            0,
            K.gateway->decimal.price.round(quotes.bid.price)
          );
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(
            quotes.bid.price + K.gateway->tickPrice,
            K.gateway->decimal.price.round(quotes.ask.price)
          );
      };
      void applyRoundSize() {
        if (!quotes.bid.empty()) {
          const Amount minBid = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.bid.price)
            : K.gateway->minSize;
          const Amount maxBid = K.gateway->margin == Future::Spot
            ? wallet.quote.total / quotes.bid.price
            : (K.gateway->margin == Future::Inverse
                ? wallet.base.amount * quotes.bid.price
                : wallet.base.amount / quotes.bid.price
            );
          quotes.bid.size = K.gateway->decimal.amount.round(
            fmax(minBid * (1.0 + K.gateway->takeFee * 1e+2), fmin(
              quotes.bid.size,
              K.gateway->decimal.amount.floor(maxBid)
            ))
          );
        }
        if (!quotes.ask.empty()) {
          const Amount minAsk = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.ask.price)
            : K.gateway->minSize;
          const Amount maxAsk = K.gateway->margin == Future::Spot
            ? wallet.base.total
            : (K.gateway->margin == Future::Inverse
                ? (quotes.bid.empty()
                  ? wallet.base.amount * quotes.ask.price
                  : quotes.bid.size)
                : wallet.base.amount / quotes.ask.price
            );
          quotes.ask.size = K.gateway->decimal.amount.round(
            fmax(minAsk * (1.0 + K.gateway->takeFee * 1e+2), fmin(
              quotes.ask.size,
              K.gateway->decimal.amount.floor(maxAsk)
            ))
          );
        }
      };
      void applyDepleted() {
        if (!quotes.bid.empty()) {
          const Amount minBid = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.bid.price)
            : K.gateway->minSize;
          if ((K.gateway->margin == Future::Spot
              ? wallet.quote.total / quotes.bid.price
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * quotes.bid.price
                  : wallet.base.amount / quotes.bid.price)
              ) < minBid
          ) quotes.bid.clear(QuoteState::DepletedFunds);
        }
        if (!quotes.ask.empty()) {
          const Amount minAsk = K.gateway->minValue
            ? fmax(K.gateway->minSize, K.gateway->minValue / quotes.ask.price)
            : K.gateway->minSize;
          if ((K.gateway->margin == Future::Spot
              ? wallet.base.total
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * quotes.ask.price
                  : wallet.base.amount / quotes.ask.price)
              ) < minAsk
          ) quotes.ask.clear(QuoteState::DepletedFunds);
        }
      };
  };

  struct Broker {
    Connectivity greenGateway = Connectivity::Disconnected;
    private:
      AntonioCalculon calculon;
                  int limit = 0;
    private_ref:
      const KryptoNinja  &K;
            Orders       &orders;
      const MarketLevels &levels;
    public:
      Broker(const KryptoNinja &bot, Orders &o, const MarketLevels &l, const Wallets &w)
        : calculon(bot, l, w)
        , K(bot)
        , orders(o)
        , levels(l)
      {};
      void read_from_gw(const Connectivity &raw) {
        const Connectivity previous = greenGateway;
        greenGateway = raw;
        if (greenGateway != previous)
          K.log("GW " + K.gateway->exchange, "Quoting state changed to",
            string(ready() ? "" : "DIS") + "CONNECTED");
        if (!(bool)greenGateway)
          calculon.states(QuoteState::Disconnected);
      };
      bool ready() const {
        return (bool)greenGateway;
      };
      void purge() {
        for (const Order *const it : calculon.purge())
          orders.purge(it);
      };
      void calcQuotes() {
        calculon.calcQuotes();
        quote2orders(calculon.quotes.ask);
        quote2orders(calculon.quotes.bid);
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
      vector<Order*> abandon(Quote &quote) {
        vector<Order*> abandoned;
        const bool all = quote.state != QuoteState::Live;
        for (Order *const it : orders.at(quote.side))
          if (all or calculon.abandon(*it, quote))
            abandoned.push_back(it);
        return abandoned;
      };
      void quote2orders(Quote &quote) {
        const vector<Order*> abandoned = abandon(quote);
        const unsigned int replace = K.gateway->askForReplace and !(
          quote.empty() or abandoned.empty()
        );
        for (
          auto it  =  abandoned.end() - replace;
               it --> abandoned.begin();
          cancelOrder(*it)
        );
        if (quote.empty()) return;
        if (replace) replaceOrder(quote.price, quote.isPong, abandoned.back());
        else placeOrder({
          quote.side,
          quote.price,
          quote.size,
          Tstamp,
          quote.isPong,
          K.gateway->randId()
        });
      };
      void placeOrder(const Order &raw) {
        K.gateway->place(orders.upsert(raw));
      };
      void replaceOrder(const Price &price, const bool &isPong, Order *const order) {
        if (orders.replace(price, isPong, order))
          K.gateway->replace(order);
      };
      void cancelOrder(Order *const order) {
        if (orders.cancel(order))
          K.gateway->cancel(order);
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
      void quit() {
        broker.quit();
      };
    private:
      void calcQuotes() {
        if (broker.ready() and levels.ready() and wallet.ready())
          broker.calcQuotes();
        broker.purge();
      };
  };
}
