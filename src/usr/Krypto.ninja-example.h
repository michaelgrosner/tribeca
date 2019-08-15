#ifndef K_EXAMPLE_H_
#define K_EXAMPLE_H_
//! \file
//! \brief Welcome user! (just a working example to begin with).

namespace ₿::example {
  class Engine {
    public:
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
        const Price spread = levels.asks.cbegin()->price
                           - levels.bids.cbegin()->price;
        cout << "Hello, " << K.arg<string>("subject")
             << " (pssst.. current spread is: "
             << K.gateway->decimal.price.str(spread) << " " << K.gateway->quote
             << ")." << endl;
        return "Executed " + (
          K.arg<int>("debug")
            ? string(__PRETTY_FUNCTION__)
            : K.arg<string>("title")
        ) + " OK";
      };
  };
}

#endif
