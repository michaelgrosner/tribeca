//! \file
//! \brief Welcome user! (just a manager of portfolios).

namespace analpaper {
  struct Wallets {
    map<string, Wallet> wallet;
    private_ref:
      const KryptoNinja &K;
    public:
      Wallets(const KryptoNinja &bot)
        : K(bot)
      {};
      void read_from_gw(const Wallet &raw) {
        wallet[raw.currency] = raw;
      };
      bool ready() const {
        const bool err = wallet.empty();
        if (err and Tspent > 21e+3)
          K.logWar("QE", "Unable to display portfolios, missing wallet data", 3e+3);
        return !err;
      };
  };

  class Engine {
    private:
      Wallets wallet;
    public:
      Engine(const KryptoNinja &bot)
        : wallet(bot)
      {};
      void read(const Wallet &rawdata) {
        wallet.read_from_gw(rawdata);
      };
  };
}
