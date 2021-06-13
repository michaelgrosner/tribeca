//! \file
//! \brief Welcome user! (just a manager of portfolios).

namespace example {
  class Engine {
    private:
      Levels levels;
    private_ref:
      const KryptoNinja &K;
    public:
      Engine(const KryptoNinja &bot)
        : K(bot)
      {};
      void read(const Wallets &rawdata) {
      };
  };
}
