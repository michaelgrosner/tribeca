class StableBot: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    StableBot()
      : engine(*this)
    {
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata); },
        [&](const Wallet       &rawdata) { engine.read(rawdata); },
        [&](const Levels       &rawdata) { engine.read(rawdata); },
        [&](const Order        &rawdata) { engine.read(rawdata); },
        [&]()                            { engine.quit();        }
      };
      arguments = {
        {
          {"order-size", "AMOUNT", "0", "set AMOUNT of size in base currency to place orders"},
          {"ask-price",  "AMOUNT", "0", "set AMOUNT of price to place ask orders"},
          {"bid-price",  "AMOUNT", "0", "set AMOUNT of price to place bid orders"}
        },
        [&](MutableUserArguments &args) {
          args["order-size"] = fmax(0, arg<double>("order-size"));
          args["ask-price"] = fmax(0, arg<double>("ask-price"));
          args["bid-price"] = fmax(0, arg<double>("bid-price"));
          if (!arg<double>("ask-price") and !arg<double>("bid-price"))
            error("CF", "Invalid empty --ask-price or --bid-price value");
          else if (arg<double>("ask-price")
               and arg<double>("bid-price")) log("CF", "Orders enabled on", "both sides");
          else if (arg<double>("ask-price")) log("CF", "Orders enabled on", "ASK side only");
          else if (arg<double>("bid-price")) log("CF", "Orders enabled on", "BID side only");
          Decimal opt;
          opt.precision(1e-8);
          if (!arg<double>("order-size"))
            error("CF", "Invalid empty --order-size value");
          else log("CF", "--order-size=", opt.str(arg<double>("order-size")));
          if (arg<double>("ask-price"))
            log("CF", "--ask-price=", opt.str(arg<double>("ask-price")));
          if (arg<double>("bid-price"))
            log("CF", "--bid-price=", opt.str(arg<double>("bid-price")));
        }
      };
    };
} K;
