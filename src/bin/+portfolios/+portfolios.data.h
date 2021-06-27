//! \file
//! \brief Welcome user! (just a manager of portfolios).

namespace analpaper {
  struct Portfolio {
    Wallet wallet;
  };
  static void to_json(json &j, const Portfolio &k) {
    j = {
      {"wallet", k.wallet}
    };
  };

  struct Wallets: public Client::Broadcast<Portfolio> {
    map<string, Portfolio> assets;
    private:
      string last;
    private_ref:
      const KryptoNinja &K;
    public:
      Wallets(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      void read_from_gw(const Wallet &raw) {
        last = raw.currency;
        assets[last].wallet = raw;
        broadcast();
        K.repaint();
      };
      bool ready() const {
        const bool err = assets.empty();
        if (err and Tspent > 21e+3)
          K.logWar("QE", "Unable to display portfolios, missing wallet data", 3e+3);
        return !err;
      };
      mMatter about() const override {
        return mMatter::Position;
      };
      json blob() const override {
        if (assets.find(last) != assets.end())
          return assets.at(last);
        else return nullptr;
      };
      json hello() override {
        json j = json::array();
        for (const auto &it : assets)
          j.push_back(it.second);
        return j;
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
          {  "webMarket", K.gateway->webMarket                        },
          {  "webOrders", K.gateway->webOrders                        },
          {  "tickPrice", K.gateway->decimal.price.stream.precision() },
          {   "tickSize", K.gateway->decimal.amount.stream.precision()},
          {  "stepPrice", K.gateway->decimal.price.step               },
          {   "stepSize", K.gateway->decimal.amount.step              },
          {    "minSize", K.gateway->minSize                          },
          {       "inet", K.arg<string>("interface")                  },
          {"environment", K.arg<string>("title")                      },
          { "matryoshka", K.arg<string>("matryoshka")                 }
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
      const KryptoNinja  &K;
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
      Wallets wallet;
       Broker broker;
    public:
      Engine(const KryptoNinja &bot)
        : wallet(bot)
        , broker(bot)
      {};
      void read(const Connectivity &rawdata) {
        broker.semaphore.read_from_gw(rawdata);
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
