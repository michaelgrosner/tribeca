class ScalingBot: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    ScalingBot()
      : engine(*this)
    {
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata); },
        [&](const Wallets      &rawdata) { engine.read(rawdata); },
        [&](const Levels       &rawdata) { engine.read(rawdata); },
        [&](const Order        &rawdata) { engine.read(rawdata); },
        [&]()                            { engine.quit();        }
      };
      arguments = {
        {
          {"quit-after",   "NUMBER", "0",      "set NUMBER of filled orders before quit"},
          {"order-size",   "AMOUNT", "0",      "set size AMOUNT in base currency to place orders"},
          {"ping-width",   "AMOUNT", "0",      "set price width AMOUNT to place pings away from fair value"},
          {"pong-width",   "AMOUNT", "0",      "set price width AMOUNT to place pongs away from pings"},
          {"scale-asks",   "1",      nullptr,  "do not place pings on bid side (ping only at ask side)"},
          {"scale-bids",   "1",      nullptr,  "do not place pings on ask side (ping only at bid side)"},
          {"debug-orders", "1",      nullptr,  "print detailed output about exchange messages"},
          {"debug-quotes", "1",      nullptr,  "print detailed output about quoting engine"},
        },
        [&](MutableUserArguments &args) {
          if (arg<int>("debug"))
            args["debug-orders"] =
            args["debug-quotes"] = 1;
          const string enabled = Ansi::b(COLOR_YELLOW) +
            (arg<int>("quit-after")
              ? to_string(arg<int>("quit-after"))
              : "unlimited"
            ) + Ansi::r(COLOR_WHITE) + " pings enabled on";
          if (arg<int>("scale-asks") and arg<int>("scale-bids"))
            error("CF", "Invalid use of --scale-asks and --scale-bids together");
          else if (!arg<int>("scale-asks") and !arg<int>("scale-bids"))
            log("CF", enabled, "both sides");
          else if (arg<int>("scale-asks"))
            log("CF", enabled, "ask side");
          else if (arg<int>("scale-bids"))
            log("CF", enabled, "bid side");
          Decimal opt;
          opt.precision(1e-8);
          if (!arg<double>("order-size"))
            error("CF", "Invalid empty --order-size value");
          else log("CF", "--order-size=", opt.str(arg<double>("order-size")));
          if (!arg<double>("ping-width"))
            error("CF", "Invalid empty --ping-width value");
          else log("CF", "--ping-width=", opt.str(arg<double>("ping-width")));
          if (!arg<double>("pong-width"))
            error("CF", "Invalid empty --pong-width value");
          else log("CF", "--pong-width=", opt.str(arg<double>("pong-width")));
        }
      };
    };
} K;
