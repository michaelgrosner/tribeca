//! \file
//! \brief Welcome user! (just a working example to begin with).

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
    public:
      void read(const Levels &rawdata) {
        levels = rawdata;
        if (levels.bids.empty() or levels.asks.empty()) return;
        exit(greeting());
      };
    private:
      string greeting() {
        const Price fair = (levels.bids.cbegin()->price
                          + levels.asks.cbegin()->price) / 2;
        cout << "Hello, " << K.arg<string>("subject") << endl
             << " pssst.. 1 " << K.gateway->base << " = "
             << K.gateway->decimal.price.str(fair) << " " << K.gateway->quote
             << "." << endl;
        return "Executed " + (
          K.arg<int>("debug")
            ? string(__PRETTY_FUNCTION__)
            : K.arg<string>("title")
        ) + " OK";
      };
  };
}
