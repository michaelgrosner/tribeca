//! \file
//! \brief Welcome user! (just a manager of portfolios).

namespace analpaper {
  struct Settings: public Sqlite::StructBackup<Settings>,
                   public Client::Broadcast<Settings>,
                   public Client::Clickable {
    string currency = "";
      bool zeroed   = true;
    private_ref:
      const KryptoNinja &K;
    public:
      Settings(const KryptoNinja &bot)
        : StructBackup(bot)
        , Broadcast(bot)
        , Clickable(bot)
        , K(bot)
      {};
      void from_json(const json &j) {
        currency = j.value("currency", K.gateway->quote);
        zeroed   = j.value("zeroed", zeroed);
        if (currency.empty()) currency = K.gateway->quote;
        K.clicked(this);
      };
      void click(const json &j) override {
        from_json(j);
        backup();
        broadcast();
      };
      mMatter about() const override {
        return mMatter::QuotingParameters;
      };
    private:
      string explain() const override {
        return "Settings";
      };
      string explainKO() const override {
        return "using default values for %";
      };
  };
  static void to_json(json &j, const Settings &k) {
    j = {
      {"currency", k.currency},
      {  "zeroed", k.zeroed  }
    };
  };
  static void from_json(const json &j, Settings &k) {
    k.from_json(j);
  };

  struct Portfolio {
    Wallet wallet;
    unordered_map<string, Price> prices;
    Price price;
  };
  static void to_json(json &j, const Portfolio &k) {
    j = {
      {"wallet", k.wallet},
      { "price", k.price }
    };
  };

  struct Portfolios: public Client::Broadcast<Portfolios> {
    Settings settings;
    unordered_map<string, Portfolio> portfolio;
    private_ref:
      const KryptoNinja &K;
    public:
      Portfolios(const KryptoNinja &bot)
        : Broadcast(bot)
        , settings(bot)
        , K(bot)
      {};
      Price calc(const string &from) const {
        if (from == settings.currency)
          return 1;
        if (portfolio.at(from).prices.find(settings.currency) != portfolio.at(from).prices.end())
          return portfolio.at(from).prices.at(settings.currency);
        else for (const auto &it : portfolio.at(from).prices)
          if (portfolio.find(it.first) != portfolio.end()) {
            if (portfolio.at(it.first).prices.find(settings.currency) != portfolio.at(it.first).prices.end())
              return it.second * portfolio.at(it.first).prices.at(settings.currency);
            else for (const auto &_it : portfolio.at(it.first).prices)
              if (portfolio.find(_it.first) != portfolio.end()
                and portfolio.at(_it.first).prices.find(settings.currency) != portfolio.at(_it.first).prices.end()
              ) return it.second * _it.second * portfolio.at(_it.first).prices.at(settings.currency);
          }
        return 0;
      };
      void calc() {
        for (auto &it : portfolio)
          portfolio.at(it.first).wallet.value = (
            portfolio.at(it.first).price = calc(it.first)
          ) * portfolio.at(it.first).wallet.total;
        if (ratelimit()) return;
        broadcast();
        K.repaint();
      };
      bool ready() const {
        const bool err = portfolio.empty();
        if (err and Tspent > 21e+3)
          K.logWar("QE", "Unable to display portfolios, missing wallet data", 3e+3);
        return !err;
      };
      mMatter about() const override {
        return mMatter::Position;
      };
    private:
      bool ratelimit() {
        return !read_soon();
      };
  };
  static void to_json(json &j, const Portfolios &k) {
    j = json::array();
    for (const auto &it : k.portfolio)
      if (it.second.price)
        j.push_back(it.second);
  };

  struct Market {
    string web;
     Price spread,
           raw_spread;
    double open;
    Amount volume,
           raw_volume;
  };
  static void to_json(json &j, const Market &k) {
    j = {
      {   "web", k.web   },
      {"spread", k.spread},
      {  "open", k.open  },
      {"volume", k.volume}
    };
  };

  struct Markets: public Client::Broadcast<Markets> {
    unordered_map<string, unordered_map<string, Market>> market;
    private_ref:
      const KryptoNinja &K;
    public:
      Markets(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      void add(const Ticker &raw) {
        market[raw.base][raw.quote] = {
          K.gateway->web(raw.base, raw.quote),
          0,
          raw.spread,
          raw.open,
          0,
          raw.volume
        };
      };
      void calc(const string &base, const string &quote, const Price &volume, const Price &spread) {
        market[base][quote].spread = spread;
        market[base][quote].volume = volume;
        if (ratelimit()) return;
        broadcast();
      };
      mMatter about() const override {
        return mMatter::MarketData;
      };
    private:
      bool ratelimit() {
        return !read_soon(1e+3);
      };
  };
  static void to_json(json &j, const Markets &k) {
    j = k.market;
  };

  struct Tickers: public Client::Clicked {
    Markets markets;
    private_ref:
      Portfolios &portfolios;
    public:
      Tickers(const KryptoNinja &bot, Portfolios &p)
        : Clicked(bot, {
            {&p.settings, [&]() { calc(); }}
          })
        , markets(bot)
        , portfolios(p)
      {};
      void read_from_gw(const Ticker &raw) {
        add(raw.base,  raw.quote,     raw.price);
        add(raw.quote, raw.base,  1 / raw.price);
        portfolios.calc();
        markets.add(raw);
        markets.calc(
          raw.base,
          raw.quote,
          raw.volume * portfolios.portfolio.at(raw.base).price,
          raw.spread * portfolios.portfolio.at(raw.quote).price
        );
      };
    private:
      void calc() {
        portfolios.calc();
        for (auto &it : markets.market)
          for (auto &_it : it.second) {
            markets.calc(
              it.first,
              _it.first,
              _it.second.raw_volume * portfolios.portfolio.at(it.first).price,
              _it.second.raw_spread * portfolios.portfolio.at(_it.first).price
            );
          }
      };
      void add(const string &base, const string &quote, const Price &price) {
        portfolios.portfolio[base].prices[quote] = price;
        if (portfolios.portfolio.at(base).wallet.currency.empty())
          portfolios.portfolio.at(base).wallet.currency = base;
      };
  };

  struct Wallets {
    private_ref:
      Portfolios &portfolios;
    public:
      Wallets(Portfolios &p)
        : portfolios(p)
      {};
      void read_from_gw(const Wallet &raw) {
        portfolios.portfolio[raw.currency].wallet = raw;
        portfolios.calc();
      };
  };

  struct Semaphore: public Client::Broadcast<Semaphore>,
                    public Hotkey::Keymap {
    Connectivity greenGateway = Connectivity::Disconnected;
    private_ref:
      const KryptoNinja &K;
    public:
      Semaphore(const KryptoNinja &bot)
        : Broadcast(bot)
        , Keymap(bot, {
            {'Q',  [&]() { exit(); }},
            {'q',  [&]() { exit(); }},
            {'\e', [&]() { exit(); }}
          })
        , K(bot)
      {};
      bool online() const {
        return (bool)greenGateway;
      };
      void read_from_gw(const Connectivity &raw) {
        if (greenGateway != raw) {
          greenGateway = raw;
          switchFlag();
        }
      };
      mMatter about() const override {
        return mMatter::Connectivity;
      };
    private:
      void switchFlag() {
        K.log("GW " + K.gateway->exchange, "Quoting state changed to",
          string(online() ? "" : "DIS") + "CONNECTED");
        broadcast();
        K.repaint();
      };
  };
  static void to_json(json &j, const Semaphore &k) {
    j = {
      {"online", k.greenGateway}
    };
  };

  struct Product: public Client::Broadcast<Product> {
    private_ref:
      const KryptoNinja &K;
    public:
      Product(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      json to_json() const {
        return {
          {   "exchange", K.gateway->exchange                         },
          {       "base", K.gateway->base                             },
          {      "quote", K.gateway->quote                            },
          {     "symbol", K.gateway->symbol                           },
          {     "margin", K.gateway->margin                           },
          {  "webMarket", K.gateway->web()                            },
          {  "webOrders", K.gateway->web(true)                        },
          {  "tickPrice", K.gateway->decimal.price.stream.precision() },
          {   "tickSize", K.gateway->decimal.amount.stream.precision()},
          {  "stepPrice", K.gateway->decimal.price.step               },
          {   "stepSize", K.gateway->decimal.amount.step              },
          {    "minSize", K.gateway->minSize                          },
          {       "inet", K.arg<string>("interface")                  },
          {"environment", K.arg<string>("title")                      },
          { "matryoshka", K.arg<string>("matryoshka")                 },
          {     "source", K_SOURCE " " K_BUILD                        }
        };
      };
      mMatter about() const override {
        return mMatter::ProductAdvertisement;
      };
  };
  static void to_json(json &j, const Product &k) {
    j = k.to_json();
  };

  struct Memory: public Client::Broadcast<Memory> {
    public:
      unsigned int orders_60s = 0;
    private:
      Product product;
    private_ref:
      const KryptoNinja &K;
    public:
      Memory(const KryptoNinja &bot)
        : Broadcast(bot)
        , product(bot)
        , K(bot)
      {};
      void timer_60s() {
        broadcast();
        orders_60s = 0;
      };
      json to_json() const {
        return {
          {  "addr", K.gateway->unlock           },
          {  "freq", orders_60s                  },
          { "theme", K.arg<int>("ignore-moon")
                       + K.arg<int>("ignore-sun")},
          {"memory", K.memSize()                 },
          {"dbsize", K.dbSize()                  }
        };
      };
      mMatter about() const override {
        return mMatter::ApplicationState;
      };
  };
  static void to_json(json &j, const Memory &k) {
    j = k.to_json();
  };

  struct Broker {
          Memory memory;
       Semaphore semaphore;
    private_ref:
      const KryptoNinja &K;
    public:
      Broker(const KryptoNinja &bot)
        : memory(bot)
        , semaphore(bot)
        , K(bot)
      {};
      bool ready() const {
        return semaphore.online();
      };
  };

  class Engine {
    public:
      Portfolios portfolios;
         Tickers ticker;
         Wallets wallet;
          Broker broker;
    public:
      Engine(const KryptoNinja &bot)
        : portfolios(bot)
        , ticker(bot, portfolios)
        , wallet(portfolios)
        , broker(bot)
      {};
      void read(const Connectivity &rawdata) {
        broker.semaphore.read_from_gw(rawdata);
      };
      void read(const Ticker &rawdata) {
        ticker.read_from_gw(rawdata);
      };
      void read(const Wallet &rawdata) {
        wallet.read_from_gw(rawdata);
      };
      void timer_1s(const unsigned int &tick) {
        if (!(tick % 60))
          broker.memory.timer_60s();
      };
  };
}
