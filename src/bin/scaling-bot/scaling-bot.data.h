//! \file
//! \brief Welcome user! (just a scaling bot to open/close positions while price decrements/increments).

namespace analpaper {
  enum class QuoteState: unsigned int {
    Disconnected,  Live,        Crossed,
    MissingData,   UnknownHeld, WidthTooHigh,
    DepletedFunds, ScaleSided, ScalationLimit, DeviationLimit
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
            + to_string((int)ask.state) + ":"
            + to_string((int)bid.state) + " "
            + to_string((int)ask.isPong) + ":"
            + to_string((int)bid.isPong) + " "
            + ((json){{"ask", ask}, {"bid", bid}}).dump()
          );
      };
  };

  struct Pongs {
    unordered_map<string, Price> bids,
                                 asks;
    Price maxBid = 0,
          minAsk = 0;
    private_ref:
      const KryptoNinja  &K;
    public:
      Pongs(const KryptoNinja &bot)
        : K(bot)
      {};
      void maxmin(const Order &order) {
        if (!limit() or order.exchangeId.empty() or (
          !order.orderId.empty() and !order.isPong
        )) return;
        if (order.status == Status::Working)
          (order.side == Side::Bid ? bids : asks)[order.exchangeId] = order.price;
        else if (bids.find(order.exchangeId) != bids.end()) bids.erase(order.exchangeId);
        else if (asks.find(order.exchangeId) != asks.end()) asks.erase(order.exchangeId);
        maxBid = bids.empty() ? 0 : max_element(bids.begin(), bids.end(), compare)->second;
        minAsk = asks.empty() ? 0 : min_element(asks.begin(), asks.end(), compare)->second;
      };
      double limit() const {
        return K.arg<double>("wait-width");
      };
      bool limit(const Quote &quote) const {
        return find(quote.price, quote.side == Side::Bid ? &asks : &bids);
      };
    private:
      static bool compare(const pair<string, Price> &a, const pair<string, Price> &b) {
        return a.second < b.second;
      };
      bool find(const Price &price, const unordered_map<string, Price> *const book) const {
        return any_of(book->begin(), book->end(), [&](auto &it) {
          return abs(it.second - price) < limit();
        });
      };
  };

  struct LastOrder {
     Price price;
    Amount filled;
      Side side;
      bool isPong;
  };
  struct Orders: public Remote::Orderbook {
    Pongs     pongs;
    LastOrder last;
    private_ref:
      const KryptoNinja  &K;
    public:
      Orders(const KryptoNinja &bot)
        : Orderbook(bot, [](const Order &order) {
            return order.status == Status::Terminated
                or order.isPong;
          })
        , pongs(bot)
        , last()
        , K(bot)
      {};
      void read_from_gw(const Order &order) {
        pongs.maxmin(order);
        if (order.orderId.empty()) {
          last = {};
          return;
        }
        last = {
          order.price,
          !order.isPong
            and order.status == Status::Terminated
            and order.quantity == order.totalFilled
              ? order.quantity : 0,
          order.side,
          order.isPong
        };
        if (last.filled) K.gateway->balance();
        if (last.filled
          or (order.isPong and order.status == Status::Working)
        ) K.log("GW " + K.gateway->exchange, string(order.isPong?"PONG":"PING") + " TRADE "
            + (order.side == Side::Bid ? "BUY  " : "SELL ")
            + K.gateway->decimal.amount.str(order.quantity)
            + " " + K.gateway->base + " at "
            + K.gateway->decimal.price.str(order.price)
            + " " + K.gateway->quote
            + " " + (order.isPong ? "(left opened)" : "(just filled)"));
      };
      Price calcPongPrice(const Price &fairValue) {
        const Price price = last.side == Side::Bid
          ? fmax(last.price + K.arg<double>("pong-width"), fairValue + K.gateway->tickPrice)
          : fmin(last.price - K.arg<double>("pong-width"), fairValue - K.gateway->tickPrice);
        if (K.arg<int>("pong-scale")) {
          if (last.side == Side::Bid) {
            if (pongs.minAsk)
              return fmax(price, pongs.minAsk - K.arg<double>("pong-width"));
          } else if (pongs.maxBid)
            return fmin(price, pongs.maxBid + K.arg<double>("pong-width"));
        }
        return price;
      };
  };

  struct Deviation {
    bool bid = false,
         ask = false;
    private:
      vector<Price> fairValues;
    private_ref:
      const KryptoNinja &K;
    public:
      Deviation(const KryptoNinja &bot)
        : K(bot)
      {};
      void timer_1s(const Price &fv) {
        fairValues.push_back(fv);
        if (fairValues.size() > limit())
          fairValues.erase(fairValues.begin(), fairValues.end() - limit());
        calc(fv);
      };
      vector<Price>::size_type limit() const {
        return K.arg<int>("time-price");
      };
    private:
      void calc(const Price &current) {
        if (K.arg<int>("bids-size")) {
          const Price high = *max_element(fairValues.begin(), fairValues.end());
          reset("DOWN", high, current, high - current > K.arg<double>("wait-price"), &bid);
        }
        if (K.arg<int>("asks-size")) {
          const Price low  = *min_element(fairValues.begin(), fairValues.end());
          reset(" UP ",  low, current, current - low  > K.arg<double>("wait-price"), &ask);
        }
      };
      void reset(const string &side, const Price &from, const Price &to, const bool &next, bool *const state) {
        if (*state != next) {
          K.log("QE", "Fair value deviation " + side + (next ? " by" : " is"),
            next
              ? K.gateway->decimal.price.str(K.arg<double>("wait-price"))
                  + " " + K.gateway->base + Ansi::r(COLOR_WHITE) + " (from "
                  + K.gateway->decimal.price.str(from) + " to "
                  + K.gateway->decimal.price.str(to) + ")"
              : "OFF");
          *state = next;
        }
      };
  };
  struct MarketLevels: public Levels {
    Price fairValue = 0;
    Deviation deviated;
    private:
      Levels unfiltered;
      unordered_map<Price, Amount> filterBidOrders,
                                   filterAskOrders;
    private_ref:
      const KryptoNinja &K;
      const Orders      &orders;
    public:
      MarketLevels(const KryptoNinja &bot, const Orders &o)
        : deviated(bot)
        , K(bot)
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
      void timer_1s() {
        if (deviated.limit() and ready())
          deviated.timer_1s(fairValue);
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

  struct AntonioCalculon {
    Quotes quotes;
    QuoteState prevBidState = QuoteState::MissingData,
               prevAskState = QuoteState::MissingData;
    private:
      vector<const Order*> zombies;
    private_ref:
      const KryptoNinja  &K;
      const Pongs        &pongs;
      const MarketLevels &levels;
      const Wallets      &wallet;
    public:
      AntonioCalculon(const KryptoNinja &bot, const Pongs &p, const MarketLevels &l, const Wallets &w)
        : quotes(bot)
        , K(bot)
        , pongs(p)
        , levels(l)
        , wallet(w)
      {};
      void calcQuotes() {
        states(QuoteState::UnknownHeld);
        calcRawQuotes();
        applyQuotingParameters();
        logState(quotes.bid, &prevBidState);
        logState(quotes.ask, &prevAskState);
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
      string explainState(const Quote &quote) const {
        string reason = "";
        if (quote.state == QuoteState::Live)
          reason = "  LIVE   " + Ansi::r(COLOR_WHITE)
                 + "because of reasons (ping: "
                 + K.gateway->decimal.price.str(quote.price) + " " + K.gateway->quote
                 + ", fair value: "
                 + K.gateway->decimal.price.str(levels.fairValue) + " " + K.gateway->quote
                 +")";
        else if (quote.state == QuoteState::DepletedFunds)
          reason = "DISABLED " + Ansi::r(COLOR_WHITE)
                 + "because not enough available funds ("
                 + (quote.side == Side::Bid
                   ? K.gateway->decimal.price.str(wallet.quote.amount) + " " + K.gateway->quote
                   : K.gateway->decimal.amount.str(wallet.base.amount) + " " + K.gateway->base
                 ) + ")";
        else if (quote.state == QuoteState::ScaleSided)
          reason = "DISABLED " + Ansi::r(COLOR_WHITE)
                 + "because " + (quote.side == Side::Bid ? "--bids-size" : "--asks-size")
                 + " was not set";
        else if (quote.state == QuoteState::ScalationLimit)
          reason = "DISABLED " + Ansi::r(COLOR_WHITE)
                 + "because the nearest pong ("
                 + K.gateway->decimal.price.str(quote.side == Side::Bid
                   ? pongs.minAsk
                   : pongs.maxBid
                 ) + " " + K.gateway->quote + ") is closer than --wait-width";
        else if (quote.state == QuoteState::DeviationLimit)
          reason = "DISABLED " + Ansi::r(COLOR_WHITE)
                 + "because the price deviation is lower than --wait-price";
        return reason;
      };
      void logState(const Quote &quote, QuoteState *const prevState) {
        if (quote.state != *prevState) {
          const string reason = explainState(quote);
          if (!reason.empty())
            K.log("QP", (quote.side == Side::Bid
              ? Ansi::r(COLOR_CYAN)    + "BID"
              : Ansi::r(COLOR_MAGENTA) + "ASK"
            ) + Ansi::r(COLOR_WHITE) + " quoting", reason);
          *prevState = quote.state;
        }
      };
      bool stillAlive(const Order &order) {
        if (order.status == Status::Waiting) {
          if (Tstamp - 10e+3 > order.time) {
            zombies.push_back(&order);
            return false;
          }
        }
        return !order.manual;
      };
      void calcRawQuotes() {
        quotes.ask.isPong =
        quotes.bid.isPong = false;
        quotes.bid.size = K.arg<double>("bids-size");
        quotes.ask.size = K.arg<double>("asks-size");
        quotes.bid.price = fmin(
          levels.fairValue - K.gateway->tickPrice,
          levels.fairValue - K.arg<double>("ping-width")
        );
        quotes.ask.price = fmax(
          levels.fairValue + K.gateway->tickPrice,
          levels.fairValue + K.arg<double>("ping-width")
        );
        if (quotes.bid.price <= 0 or quotes.ask.price <= 0) {
          quotes.bid.clear(QuoteState::WidthTooHigh);
          quotes.ask.clear(QuoteState::WidthTooHigh);
          K.logWar("QP", "Negative price detected, widthPing must be lower", 3e+3);
        }
      };
      void applyQuotingParameters() {
        quotes.debug("?"); applyScaleSide();
        quotes.debug("A"); applyPongsScalation();
        quotes.debug("B"); applyFairValueDeviation();
        quotes.debug("C"); applyBestWidth();
        quotes.debug("D"); applyRoundPrice();
        quotes.debug("E"); applyRoundSize();
        quotes.debug("F"); applyDepleted();
        quotes.debug("!");
        quotes.checkCrossedQuotes();
      };
      void applyScaleSide() {
        if (!K.arg<double>("bids-size"))
          quotes.bid.clear(QuoteState::ScaleSided);
        if (!K.arg<double>("asks-size"))
          quotes.ask.clear(QuoteState::ScaleSided);
      };
      void applyPongsScalation() {
        if (pongs.limit()) {
          if (!quotes.bid.empty() and pongs.limit(quotes.bid))
            quotes.bid.clear(QuoteState::ScalationLimit);
          if (!quotes.ask.empty() and pongs.limit(quotes.ask))
            quotes.ask.clear(QuoteState::ScalationLimit);
        }
      };
      void applyFairValueDeviation() {
        if (levels.deviated.limit()) {
          if (!quotes.bid.empty() and !levels.deviated.bid)
            quotes.bid.clear(QuoteState::DeviationLimit);
          if (!quotes.ask.empty() and !levels.deviated.ask)
            quotes.ask.clear(QuoteState::DeviationLimit);
        }
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
        if (!quotes.bid.empty())
          if ((K.gateway->margin == Future::Spot
              ? wallet.quote.amount / quotes.bid.price
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * quotes.bid.price
                  : wallet.base.amount / quotes.bid.price)
              ) < quotes.bid.size
          ) quotes.bid.clear(QuoteState::DepletedFunds);
        if (!quotes.ask.empty())
          if ((K.gateway->margin == Future::Spot
              ? wallet.base.amount
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * quotes.ask.price
                  : wallet.base.amount / quotes.ask.price)
              ) < quotes.ask.size
          ) quotes.ask.clear(QuoteState::DepletedFunds);
      };
  };

  struct Broker {
    Connectivity greenGateway = Connectivity::Disconnected;
    private:
      vector<Order> pending;
      AntonioCalculon calculon;
                  int limit = 0;
    private_ref:
      const KryptoNinja  &K;
            Orders       &orders;
      const MarketLevels &levels;
      const      Wallets &wallet;
    public:
      Broker(const KryptoNinja &bot, Orders &o, const MarketLevels &l, const Wallets &w)
        : calculon(bot, o.pongs, l, w)
        , K(bot)
        , orders(o)
        , levels(l)
        , wallet(w)
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
        if (pending.empty()) {
          calculon.calcQuotes();
          quote2orders(calculon.quotes.ask);
          quote2orders(calculon.quotes.bid);
        } else K.cancel();
      };
      void timer_60s() {
        if (K.arg<int>("heartbeat") and levels.fairValue) {
          string bids, asks;
          for (auto &it : orders.open())
            (it->side == Side::Bid
              ? bids
              : asks
            ) += K.gateway->decimal.price.str(it->price) + ',';
          if (!bids.empty()) bids.pop_back(); else bids = '0';
          if (!asks.empty()) asks.pop_back(); else asks = '0';
          K.log("HB", ((json){
            {"bid|fv|ask", K.gateway->decimal.price.str(
                             levels.bids.empty()
                               ? 0 : levels.bids.cbegin()->price
                           ) + "|"
                         + K.gateway->decimal.price.str(
                             levels.fairValue
                           ) + "|"
                         + K.gateway->decimal.price.str(
                             levels.asks.empty()
                               ? 0 : levels.asks.cbegin()->price
                           )                                    },
            {"pongs", K.gateway->decimal.price.str(
                        orders.pongs.maxBid
                      ) + "|"
                    + K.gateway->decimal.price.str(
                        orders.pongs.minAsk
                      )                                         },
            {"pings", bids + "|" + asks                         }
          }).dump());
        }
      };
      void timer_1s() {
        if (!pending.empty()
          and pending.at(0).quantity < (pending.at(0).side == Side::Bid
            ? wallet.quote.amount / pending.at(0).price
            : wallet.base.amount
        )) {
          K.place(pending.at(0));
          pending.erase(pending.begin());
        }
      };
      void scale() {
        if (orders.last.filled and K.arg<double>("pong-width"))
          pending.push_back({
            orders.last.side == Side::Bid
              ? Side::Ask
              : Side::Bid,
            orders.calcPongPrice(levels.fairValue),
            orders.last.filled,
            Tstamp,
            true,
            K.gateway->randId()
          });
      };
      void quit_after() {
        if (orders.last.isPong
          and K.arg<int>("quit-after")
          and K.arg<int>("quit-after") == ++limit
        ) exit("CF " + Ansi::r(COLOR_WHITE)
            + "--quit-after="
            + Ansi::b(COLOR_YELLOW) + to_string(K.arg<int>("quit-after")) + Ansi::r(COLOR_WHITE)
            + " limit reached"
          );
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
        broker.scale();
        broker.quit_after();
      };
      void timer_1s(const unsigned int &tick) {
        levels.timer_1s();
        broker.timer_1s();
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
        broker.purge();
      };
  };
}
