//! \file
//! \brief Welcome user! (just a manager of portfolios).

namespace analpaper {
  struct Wallets: public Client::Broadcast<Wallets> {
    map<string, Wallet> assets;
    private_ref:
      const KryptoNinja &K;
    public:
      Wallets(const KryptoNinja &bot)
        : Broadcast(bot)
        , K(bot)
      {};
      void read_from_gw(const Wallet &raw) {
        assets[raw.currency] = raw;
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
  };
  static void to_json(json &j, const Wallets &k) {
    for (const auto &it : k.assets)
      j[it.first] = {
        {  "amount", it.second.amount},
        {    "held", it.second.held  },
        {   "value", it.second.value },
        {  "profit", it.second.profit}
      };
  };

  struct Broker: public Client::Broadcast<Broker>,
                 public Hotkey::Keymap {
    Connectivity greenGateway = Connectivity::Disconnected;
    private_ref:
      const KryptoNinja  &K;
    public:
      Broker(const KryptoNinja &bot)
        : Broadcast(bot)
        , Keymap(bot, {
            {'Q',  [&]() { exit(); }},
            {'q',  [&]() { exit(); }},
            {'\e', [&]() { exit(); }}
          })
        , K(bot)
      {};
      void read_from_gw(const Connectivity &raw) {
        const Connectivity previous = greenGateway;
        greenGateway = raw;
        if (greenGateway != previous)
          K.log("GW " + K.gateway->exchange, "Quoting state changed to",
            string(ready() ? "" : "DIS") + "CONNECTED");
        broadcast();
        K.repaint();
      };
      bool ready() const {
        return (bool)greenGateway;
      };
      mMatter about() const override {
        return mMatter::Connectivity;
      };
  };
  static void to_json(json &j, const Broker &k) {
    j = {
      {"online", k.greenGateway}
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
        broker.read_from_gw(rawdata);
      };
      void read(const Wallet &rawdata) {
        wallet.read_from_gw(rawdata);
      };
  };
}
