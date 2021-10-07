//! \file
//! \brief Welcome user! (just a scaling bot to open/close positions while price decrements/increments).

namespace analpaper {
  struct Pongs {
    Price maxBid = 0,
          minAsk = 0;
    private:
      unordered_map<string, Price> bids,
                                   asks;
    private_ref:
      const KryptoNinja &K;
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
        else if (bids.contains(order.exchangeId)) bids.erase(order.exchangeId);
        else if (asks.contains(order.exchangeId)) asks.erase(order.exchangeId);
        maxBid = bids.empty() ? 0 : max_element(bids.begin(), bids.end(), compare)->second;
        minAsk = asks.empty() ? 0 : min_element(asks.begin(), asks.end(), compare)->second;
      };
      double limit() const {
        return K.arg<double>("wait-width");
      };
      bool limit(const System::Quote &quote) const {
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

  struct Orders: public System::Orderbook {
    Pongs pongs;
    private_ref:
      const KryptoNinja &K;
    public:
      Orders(const KryptoNinja &bot)
        : Orderbook(bot)
        , pongs(bot)
        , K(bot)
      {};
      bool purgeable(const Order &order) const override {
        return order.status == Status::Terminated
            or order.isPong;
      };
      void read_from_gw(const Order &order) {
        pongs.maxmin(order);
        if (order.orderId.empty()) return;
        last->justFilled = !order.isPong
                           and order.status == Status::Terminated
                           and order.quantity == order.totalFilled
                             ? order.quantity : 0;
        if (last->justFilled
          or (order.isPong and order.status == Status::Working)
        ) K.log("GW " + K.gateway->exchange,
            string(order.side == Side::Bid
              ? ANSI_PUKE_CYAN    + (order.isPong?"PONG":"PING") + " TRADE BUY  "
              : ANSI_PUKE_MAGENTA + (order.isPong?"PONG":"PING") + " TRADE SELL "
            )
            + K.gateway->decimal.amount.str(order.quantity)
            + " " + K.gateway->base + " at "
            + K.gateway->decimal.price.str(order.price)
            + " " + K.gateway->quote
            + " " + (order.isPong ? "(left opened)" : "(just filled)"));
        else K.repaint(true);
      };
      Price calcPongPrice(const Price &fairValue) const {
        const Price price = last->side == Side::Bid
          ? fmax(last->price + K.arg<double>("pong-width"), fairValue + K.gateway->tickPrice)
          : fmin(last->price - K.arg<double>("pong-width"), fairValue - K.gateway->tickPrice);
        if (K.arg<int>("pong-scale")) {
          if (last->side == Side::Bid) {
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
                  + " " + K.gateway->base + ANSI_PUKE_WHITE + " (from "
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
        K.repaint();
      };
      bool ready() {
        filter();
        if (!fairValue and Tspent > 21e+3)
          K.warn("QE", "Unable to calculate quote, missing market data", 10e+3);
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

  struct Wallets {
    Wallet base,
           quote;
    private_ref:
      const KryptoNinja &K;
      const Price       &fairValue;
    public:
      Wallets(const KryptoNinja &bot, const MarketLevels &l)
        : K(bot)
        , fairValue(l.fairValue)
      {};
      void read_from_gw(const Wallet &raw) {
        if      (raw.currency == K.gateway->base)  base  = raw;
        else if (raw.currency == K.gateway->quote) quote = raw;
        else return;
        calcValues();
      };
      bool ready() const {
        const bool err = base.currency.empty() or quote.currency.empty();
        if (err and Tspent > 21e+3)
          K.warn("QE", "Unable to calculate quote, missing wallet data", 3e+3);
        return !err;
      };
      void calcValues() {
        if (base.currency.empty() or quote.currency.empty() or !fairValue) return;
        base.value  = K.gateway->margin == Future::Spot
                        ? base.total + (quote.total / fairValue)
                        : base.total;
        quote.value = K.gateway->margin == Future::Spot
                        ? (base.total * fairValue) + quote.total
                        : (K.gateway->margin == Future::Inverse
                            ? base.total * fairValue
                            : base.total / fairValue
                        );
      };
  };

  struct AntonioCalculon: public System::Quotes {
    private_ref:
      const KryptoNinja  &K;
      const Pongs        &pongs;
      const MarketLevels &levels;
      const Wallets      &wallet;
    public:
      AntonioCalculon(const KryptoNinja &bot, const Pongs &p, const MarketLevels &l, const Wallets &w)
        : Quotes(bot)
        , K(bot)
        , pongs(p)
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
        else if (quote.state == QuoteState::ScaleSided)
          reason = "DISABLED " + ANSI_PUKE_WHITE
                 + "because " + (quote.side == Side::Bid ? "--bids-size" : "--asks-size")
                 + " was not set";
        else if (quote.state == QuoteState::ScalationLimit)
          reason = " PAUSED  " + ANSI_PUKE_WHITE
                 + "because the nearest pong ("
                 + K.gateway->decimal.price.str(quote.side == Side::Bid
                   ? pongs.minAsk
                   : pongs.maxBid
                 ) + " " + K.gateway->quote + ") is closer than --wait-width";
        else if (quote.state == QuoteState::DeviationLimit)
          reason = " PAUSED  " + ANSI_PUKE_WHITE
                 + "because the price deviation is lower than --wait-price";
        else if (quote.state == QuoteState::WaitingFunds)
          reason = " PAUSED  " + ANSI_PUKE_WHITE
                 + "because a pong is pending to be placed";
        else if (quote.state == QuoteState::DisabledQuotes)
          reason = "DISABLED " + ANSI_PUKE_WHITE
                 + "because an admin considered it";
        else if (quote.state == QuoteState::Disconnected)
          reason = " PAUSED  " + ANSI_PUKE_WHITE
                 + "because the exchange seems down";
        return reason;
      };
      void calcRawQuotes() override {
        bid.size = K.arg<double>("bids-size");
        ask.size = K.arg<double>("asks-size");
        bid.price = fmin(
          levels.fairValue - K.gateway->tickPrice,
          levels.fairValue - K.arg<double>("ping-width")
        );
        ask.price = fmax(
          levels.fairValue + K.gateway->tickPrice,
          levels.fairValue + K.arg<double>("ping-width")
        );
      };
      void applyQuotingParameters() override {
        debug("?"); applyScaleSide();
        debug("A"); applyPongsScalation();
        debug("B"); applyFairValueDeviation();
        debug("C"); applyBestWidth();
        debug("D"); applyRoundPrice();
        debug("E"); applyRoundSize();
        debug("F"); applyDepleted();
        debug("!");
      };
      void applyScaleSide() {
        if (!K.arg<double>("bids-size"))
          bid.skip(QuoteState::ScaleSided);
        if (!K.arg<double>("asks-size"))
          ask.skip(QuoteState::ScaleSided);
      };
      void applyPongsScalation() {
        if (pongs.limit()) {
          if (!bid.empty() and pongs.limit(bid))
            bid.skip(QuoteState::ScalationLimit);
          if (!ask.empty() and pongs.limit(ask))
            ask.skip(QuoteState::ScalationLimit);
        }
      };
      void applyFairValueDeviation() {
        if (levels.deviated.limit()) {
          if (!bid.empty() and !levels.deviated.bid)
            bid.skip(QuoteState::DeviationLimit);
          if (!ask.empty() and !levels.deviated.ask)
            ask.skip(QuoteState::DeviationLimit);
        }
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
        if (!bid.empty())
          if ((K.gateway->margin == Future::Spot
              ? wallet.quote.amount / bid.price
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * bid.price
                  : wallet.base.amount / bid.price)
              ) < bid.size
          ) bid.skip(QuoteState::DepletedFunds);
        if (!ask.empty())
          if ((K.gateway->margin == Future::Spot
              ? wallet.base.amount
              : (K.gateway->margin == Future::Inverse
                  ? wallet.base.amount * ask.price
                  : wallet.base.amount / ask.price)
              ) < ask.size
          ) ask.skip(QuoteState::DepletedFunds);
      };
  };

  struct Semaphore: public Hotkey::Keymap {
    Connectivity greenButton  = Connectivity::Disconnected,
                 greenGateway = Connectivity::Disconnected;
    private_ref:
      const KryptoNinja &K;
    public:
      Semaphore(const KryptoNinja &bot)
        : Keymap(bot, {
            {'Q', [&]() { exit(); }},
            {'q', [&]() { exit(); }},
            {'\e', [&]() { toggle(); }}
          })
        , K(bot)
      {};
      bool paused() const {
        return !(bool)greenButton;
      };
      bool offline() const {
        return !(bool)greenGateway;
      };
      void read_from_gw(const Connectivity &raw) {
        greenGateway = raw;
        switchButton();
      };
    private:
      void toggle() {
        K.gateway->adminAgreement = (Connectivity)!(bool)K.gateway->adminAgreement;
        switchButton();
      };
      void switchButton() {
        greenButton = (Connectivity)(
          (bool)greenGateway and (bool)K.gateway->adminAgreement
        );
        K.repaint();
      };
  };

  struct Broker {
          Semaphore semaphore;
    AntonioCalculon quotes;
    private:
      vector<Order> pending;
                int limit = 0;
    private_ref:
      const KryptoNinja  &K;
            Orders       &orders;
      const MarketLevels &levels;
      const      Wallets &wallet;
    public:
      Broker(const KryptoNinja &bot, Orders &o, const MarketLevels &l, const Wallets &w)
        : semaphore(bot)
        , quotes(bot, o.pongs, l, w)
        , K(bot)
        , orders(o)
        , levels(l)
        , wallet(w)
      {};
      bool ready() {
        if (semaphore.offline()) {
          quotes.offline();
          return false;
        }
        return true;
      };
      void calcQuotes() {
        if (pending.empty() and !semaphore.paused()) {
          quotes.calcQuotes();
          quote2orders(quotes.ask);
          quote2orders(quotes.bid);
        } else {
          if (semaphore.paused())
            quotes.paused();
          else quotes.pending();
          K.cancel();
        }
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
        if (orders.last
          and orders.last->justFilled
          and K.arg<double>("pong-width")
        ) pending.push_back({
            orders.last->side == Side::Bid
              ? Side::Ask
              : Side::Bid,
            orders.calcPongPrice(levels.fairValue),
            orders.last->justFilled,
            Tstamp,
            true,
            K.gateway->randId()
          });
      };
      void quit_after() {
        if (orders.last
          and orders.last->isPong
          and K.arg<int>("quit-after")
          and K.arg<int>("quit-after") == ++limit
        ) exit("CF " + ANSI_PUKE_WHITE
            + "--quit-after="
            + ANSI_HIGH_YELLOW + to_string(K.arg<int>("quit-after"))
            + ANSI_PUKE_WHITE  + " limit reached"
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
    public:
            Orders orders;
      MarketLevels levels;
           Wallets wallet;
            Broker broker;
    public:
      Engine(const KryptoNinja &bot)
        : orders(bot)
        , levels(bot, orders)
        , wallet(bot, levels)
        , broker(bot, orders, levels, wallet)
      {};
      void read(const Connectivity &rawdata) {
        broker.semaphore.read_from_gw(rawdata);
      };
      void read(const Wallet &rawdata) {
        wallet.read_from_gw(rawdata);
      };
      void read(const Levels &rawdata) {
        levels.read_from_gw(rawdata);
        wallet.calcValues();
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
        orders.zombies.purge();
      };
  };
}
