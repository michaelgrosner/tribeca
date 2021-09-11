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
          {"pong-width",   "AMOUNT", "0",      "set AMOUNT of price width to place pongs away from pings,"
                                               "\n" "or leave it empty (as '0') to not place pong orders"},
          {"scale-pong",   "1",      nullptr,  "place new pongs away from last pongs instead of from pings"},
          {"scale-asks",   "1",      nullptr,  "do not place pings on bid side (ping only at ask side)"},
          {"scale-bids",   "1",      nullptr,  "do not place pings on ask side (ping only at bid side)"},
          {"quit-after",   "NUMBER", "0",      "set NUMBER of filled pings before quit"},
          {"wait-width",   "AMOUNT", "0",      "set AMOUNT of price width from last pong before start"},
          {"wait-price",   "AMOUNT", "0",      "set AMOUNT of fair value price deviation before start"},
          {"time-price",   "NUMBER", "0",      "set NUMBER of seconds to measure price deviation difference"}
        },
        [&](MutableUserArguments &args) {
          args["order-size"] = fmax(0, arg<double>("order-size"));
          args["ping-width"] = fmax(0, arg<double>("ping-width"));
          args["pong-width"] = fmax(0, arg<double>("pong-width"));
          args["scale-pong"] =  max(0, arg<int>("scale-pong"));
          args["quit-after"] =  max(0, arg<int>("quit-after"));
          args["wait-width"] = fmax(0, arg<double>("wait-width"));
          args["wait-price"] = fmax(0, arg<double>("wait-price"));
          args["time-price"] =  max(0, arg<int>("time-price"));
          if (arg<int>("scale-pong") and !arg<double>("pong-width"))
            error("CF", "Invalid use of --scale-pong without --pong-width");
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
          if (arg<double>("wait-price") and arg<double>("wait-width"))
            error("CF", "Invalid use of --wait-price and --wait-width together");
          else if (!arg<double>("wait-width"))
            log("CF", "Pongs scalation limit", "disabled");
          else
            log("CF", "Pongs scalation limit enabled (" + Ansi::b(COLOR_YELLOW) +
              opt.str(arg<double>("wait-width")) + " " + arg<string>("quote") + ")"
            );
          if (!arg<double>("wait-price") and arg<int>("time-price"))
            error("CF", "Invalid use of --time-price without --wait-price");
          else if (arg<double>("wait-price") and !arg<int>("time-price"))
            error("CF", "Invalid use of --wait-price without --time-price");
          else if (!arg<double>("wait-price"))
            log("CF", "Price deviation limit", "disabled");
          else
            log("CF", "Price deviation limit enabled (" + Ansi::b(COLOR_YELLOW) +
              opt.str(arg<double>("wait-price")) + " " + arg<string>("quote") +
              Ansi::r(COLOR_WHITE) + " in " + Ansi::b(COLOR_YELLOW) +
              (arg<int>("time-price")
                ? to_string(arg<int>("time-price"))
                : "unlimited"
              ) + " seconds" + Ansi::r(COLOR_WHITE) + ")"
            );
          if (!arg<double>("order-size"))
            error("CF", "Invalid empty --order-size value");
          else log("CF", "--order-size=", opt.str(arg<double>("order-size")));
          if (!arg<double>("ping-width"))
            error("CF", "Invalid empty --ping-width value");
          else log("CF", "--ping-width=", opt.str(arg<double>("ping-width")));
          log("CF", "--pong-width=", opt.str(arg<double>("pong-width"))
            + (arg<double>("pong-width") ? "" : Ansi::b(COLOR_RED) + " (pong orders disabled)")
          );
          if (arg<double>("pong-width")) {
            if (!arg<double>("wait-width"))
              logWar("CF", "Pong orders may overlap because --wait-width is not set");
            else if (arg<double>("wait-width") < arg<double>("pong-width") * 2)
              logWar("CF", "Pong orders may overlap because "
                           "--wait-width is smaller than the double of --pong-width");
          }
        }
      };
    };
} K;
