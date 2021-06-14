//! \file
//! \brief Welcome user! (just a manager of portfolios).

namespace analpaper {
  class Engine {
    private:
      Levels levels;
    private_ref:
      const KryptoNinja &K;
    public:
      Engine(const KryptoNinja &bot)
        : K(bot)
      {};
      void read(const Wallet &rawdata) {
        cout << ((json)rawdata).dump() << endl;
      };
  };
}
