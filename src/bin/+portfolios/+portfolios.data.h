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

  struct Portfolio  {
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

  struct Portfolios: public Client::Broadcast<Portfolios>,
                     public Client::Clicked {
    Settings settings;
    unordered_map<string, Portfolio> portfolio;
    private_ref:
      const KryptoNinja &K;
    public:
      Portfolios(const KryptoNinja &bot)
        : Broadcast(bot)
        , Clicked(bot, {
            {&settings, [&]() { calc(); }}
          })
        , settings(bot)
        , K(bot)
      {};
      Price calc(const string &base) const {
        if (base == settings.currency)
          return 1;
        if (portfolio.at(base).prices.find(settings.currency) != portfolio.at(base).prices.end())
          return portfolio.at(base).prices.at(settings.currency);
        else for (const auto &it : portfolio.at(base).prices)
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
      if (!it.second.prices.empty())
        j.push_back(it.second);
  };

  struct Links: public Client::Broadcast<Links> {
    unordered_map<string, unordered_map<string, string>> link;
    private_ref:
      const KryptoNinja &K;
    public:
      Links(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      void add(const string &base, const string &quote) {
        link[base][quote] =
        link[quote][base] = K.gateway->web(base, quote);
        broadcast();
      };
      bool read_same_blob() const override {
        return false;
      };
      mMatter about() const override {
        return mMatter::Links;
      };
  };
  static void to_json(json &j, const Links &k) {
    j = k.link;
  };

  struct Tickers {
    Links links;
    private_ref:
      Portfolios  &portolios;
    public:
      Tickers(const KryptoNinja &bot, Portfolios &p)
        : links(bot)
        , portolios(p)
      {};
      void read_from_gw(const Ticker &raw) {
        add(raw.base,  raw.quote,     raw.price);
        add(raw.quote, raw.base,  1 / raw.price);
        portolios.calc();
        links.add(raw.base, raw.quote);
      };
    private:
      void add(const string &base, const string &quote, const Price &price) {
        portolios.portfolio[base].prices[quote] = price;
        if (portolios.portfolio.at(base).wallet.currency.empty())
          portolios.portfolio.at(base).wallet.currency = base;
      };
  };

  struct Wallets {
    private_ref:
      Portfolios &portolios;
    public:
      Wallets(Portfolios &p)
        : portolios(p)
      {};
      void read_from_gw(const Wallet &raw) {
        portolios.portfolio[raw.currency].wallet = raw;
        portolios.calc();
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
