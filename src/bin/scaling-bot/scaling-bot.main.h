class ScalingBot: public KryptoNinja {
  private:
    analpaper::Engine engine;
  public:
    ScalingBot()
      : engine(*this)
    {
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata); },
        [&](const Wallet       &rawdata) { engine.read(rawdata); },
        [&](const Levels       &rawdata) { engine.read(rawdata); },
        [&](const Order        &rawdata) { engine.read(rawdata); },
        [&](const unsigned int &)        { engine.timer_1s(0);   },
        [&]()                            { engine.quit();        }
      };
      arguments = {
        {
          {"order-size",   "AMOUNT", "0",      "set AMOUNT of size in base currency to place orders"},
          {"ping-width",   "AMOUNT", "0",      "set AMOUNT of price width to place pings away from fair value"},
          {"pong-width",   "AMOUNT", "0",      "set AMOUNT of price width to place pongs away from pings"},
          {"scale-asks",   "1",      nullptr,  "do not place pings on bid side (ping only at ask side)"},
          {"scale-bids",   "1",      nullptr,  "do not place pings on ask side (ping only at bid side)"},
          {"quit-after",   "NUMBER", "0",      "set NUMBER of filled pings before quit"},
          {"wait-price",   "AMOUNT", "0",      "set AMOUNT of fair value price deviation before start"},
          {"time-price",   "NUMBER", "0",      "set NUMBER of seconds to measure price deviation difference"}
        },
        [&](MutableUserArguments &args) {
          args["order-size"] = fmax(0, arg<double>("order-size"));
          args["ping-width"] = fmax(0, arg<double>("ping-width"));
          args["pong-width"] = fmax(0, arg<double>("pong-width"));
          args["quit-after"] =  max(0, arg<int>("quit-after"));
          args["wait-price"] = fmax(0, arg<double>("wait-price"));
          args["time-price"] =  max(0, arg<int>("time-price"));
          const string enabled = Ansi::b(COLOR_YELLOW) +
            (arg<int>("quit-after")
              ? to_string(arg<int>("quit-after"))
              : "Unlimited"
            ) + Ansi::r(COLOR_WHITE) + " pings enabled on";
          if (arg<int>("scale-asks") and arg<int>("scale-bids"))
            error("CF", "Invalid use of --scale-asks and --scale-bids together");
          else if (!arg<int>("scale-asks")
               and !arg<int>("scale-bids")) log("CF", enabled, "both sides");
          else if ( arg<int>("scale-asks")) log("CF", enabled, "ASK side only");
          else if ( arg<int>("scale-bids")) log("CF", enabled, "BID side only");
          Decimal opt;
          opt.precision(1e-8);
          if (arg<double>("wait-price") and !arg<int>("time-price"))
            error("CF", "Invalid use of --wait-price without --time-price");
          else log("CF", "Waiting for price deviation (" + Ansi::b(COLOR_YELLOW)
                         + opt.str(arg<double>("wait-price")) + " " + arg<string>("quote")
                         + Ansi::r(COLOR_WHITE) + " in " + Ansi::b(COLOR_YELLOW)
                         + to_string(arg<int>("time-price")) + " seconds" + Ansi::r(COLOR_WHITE) + ")");
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
