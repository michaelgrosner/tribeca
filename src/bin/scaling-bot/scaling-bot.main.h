class ScalingBot: public KryptoNinja {
  public:
    analpaper::Engine engine;
  public:
    ScalingBot()
      : engine(*this)
    {
      display   = { terminal };
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata);  },
        [&](const Wallet       &rawdata) { engine.read(rawdata);  },
        [&](const Levels       &rawdata) { engine.read(rawdata);  },
        [&](const Order        &rawdata) { engine.read(rawdata);  },
        [&](const unsigned int &tick)    { engine.timer_1s(tick); },
        [&]()                            { engine.quit();         }
      };
      arguments = {
        {
          {"bids-size",    "AMOUNT", "0",      "set AMOUNT of size in base currency to place ping bid orders,"
                                               ANSI_NEW_LINE "or leave it unset to not place ping bid orders"},
          {"asks-size",    "AMOUNT", "0",      "set AMOUNT of size in base currency to place ping ask orders,"
                                               ANSI_NEW_LINE "or leave it unset to not place ping ask orders"},
          {"ping-width",   "AMOUNT", "0",      "set AMOUNT of price width to place pings away from fair value"},
          {"pong-width",   "AMOUNT", "0",      "set AMOUNT of price width to place pongs away from pings,"
                                               ANSI_NEW_LINE "or leave it unset to not place pong orders"},
          {"pong-scale",   "1",      nullptr,  "place new pongs away from last pongs instead of from pings"},
          {"quit-after",   "NUMBER", "0",      "set NUMBER of filled pings before quit"},
          {"wait-width",   "AMOUNT", "0",      "set AMOUNT of price width from last pong before start"},
          {"wait-price",   "AMOUNT", "0",      "set AMOUNT of fair value price deviation before start"},
          {"time-price",   "NUMBER", "0",      "set NUMBER of seconds to measure price deviation difference"}
        },
        [&](MutableUserArguments &args) {
          args["bids-size"]  = fmax(0, arg<double>("bids-size"));
          args["asks-size"]  = fmax(0, arg<double>("asks-size"));
          args["ping-width"] = fmax(0, arg<double>("ping-width"));
          args["pong-width"] = fmax(0, arg<double>("pong-width"));
          args["pong-scale"] =  max(0, arg<int>("pong-scale"));
          args["quit-after"] =  max(0, arg<int>("quit-after"));
          args["wait-width"] = fmax(0, arg<double>("wait-width"));
          args["wait-price"] = fmax(0, arg<double>("wait-price"));
          args["time-price"] =  max(0, arg<int>("time-price"));
          if (arg<int>("pong-scale") and !arg<double>("pong-width"))
            error("CF", "Invalid use of --pong-scale without --pong-width");
          const string enabled = ANSI_HIGH_YELLOW +
            (arg<int>("quit-after")
              ? to_string(arg<int>("quit-after"))
              : "Unlimited"
            ) + ANSI_PUKE_WHITE + " pings enabled on";
          if (!arg<double>("bids-size") and !arg<double>("asks-size"))
            error("CF", "Invalid empty --bids-size or --asks-size value");
          else if (arg<double>("bids-size")
               and arg<double>("asks-size")) log("CF", enabled, "both sides");
          else if ( !arg<double>("bids-size")) log("CF", enabled, "ASK side only");
          else if ( !arg<double>("asks-size")) log("CF", enabled, "BID side only");
          Decimal opt;
          opt.precision(1e-8);
          if (arg<double>("wait-price") and arg<double>("wait-width"))
            error("CF", "Invalid use of --wait-price and --wait-width together");
          else if (!arg<double>("wait-width"))
            log("CF", "Pongs scalation limit", "disabled");
          else
            log("CF", "Pongs scalation limit enabled (" + ANSI_HIGH_YELLOW +
              opt.str(arg<double>("wait-width")) + " " + arg<string>("quote") + ")"
            );
          if (!arg<double>("wait-price") and arg<int>("time-price"))
            error("CF", "Invalid use of --time-price without --wait-price");
          else if (arg<double>("wait-price") and !arg<int>("time-price"))
            error("CF", "Invalid use of --wait-price without --time-price");
          else if (!arg<double>("wait-price"))
            log("CF", "Price deviation limit", "disabled");
          else
            log("CF", "Price deviation limit enabled (" + ANSI_HIGH_YELLOW +
              opt.str(arg<double>("wait-price")) + " " + arg<string>("quote") +
              ANSI_PUKE_WHITE + " in " + ANSI_HIGH_YELLOW +
              (arg<int>("time-price")
                ? to_string(arg<int>("time-price"))
                : "unlimited"
              ) + " seconds" + ANSI_PUKE_WHITE + ")"
            );
          if (arg<double>("bids-size"))
            log("CF", "--bids-size=", opt.str(arg<double>("bids-size")));
          if (arg<double>("asks-size"))
            log("CF", "--asks-size=", opt.str(arg<double>("asks-size")));
          if (!arg<double>("ping-width"))
            error("CF", "Invalid empty --ping-width value");
          else log("CF", "--ping-width=", opt.str(arg<double>("ping-width")));
          log("CF", "--pong-width=", opt.str(arg<double>("pong-width"))
            + (arg<double>("pong-width") ? "" : ANSI_HIGH_RED + " (pong orders disabled)")
          );
          if (arg<double>("pong-width")) {
            if (!arg<double>("wait-width"))
              warn("CF", "Pong orders may overlap because --wait-width is not set");
            else if (arg<double>("wait-width") < arg<double>("pong-width") * 2)
              warn("CF", "Pong orders may overlap because "
                         "--wait-width is smaller than the double of --pong-width");
          }
        }
      };
    };
  private:
    static string terminal();
} K;

string ScalingBot::terminal() {
  const string baseValue  = K.gateway->decimal.funds.str(K.engine.wallet.base.value),
               quoteValue = K.gateway->decimal.price.str(K.engine.wallet.quote.value);
  const string coins = ANSI_HIGH_MAGENTA + baseValue
                     + ANSI_PUKE_MAGENTA + ' ' + K.gateway->base
                     + ANSI_PUKE_GREEN   + " or "
                     + ANSI_HIGH_CYAN    + quoteValue
                     + ANSI_PUKE_CYAN    + ' ' + K.gateway->quote
                     + ANSI_PUKE_WHITE   + " ├";
  const string quit = "┤ [" + ANSI_HIGH_WHITE
                    + "ESC" + ANSI_PUKE_WHITE + "]: "
                    + (K.engine.broker.semaphore.paused()
                      ? "Start"
                      : "Stop?"
                    ) + "!"
                    + ", [" + ANSI_HIGH_WHITE
                    +  "q"  + ANSI_PUKE_WHITE + "]: Quit!";
  const string title = K.arg<string>("exchange")
                     + ANSI_PUKE_GREEN
                     + ' ' + (K.arg<int>("headless")
                       ? "headless"
                       : "UI at " + K.location()
                     ) + ' ';
  const string space = string(fmax(0,
    coins.length()
    - title.length()
    - ANSI_SYMBOL_SIZE(1)
    - ANSI_COLORS_SIZE(5)
  ), ' ');
  const string top = "┌───────┐ K │ "
                   + ANSI_HIGH_GREEN + title + space
                   + ANSI_PUKE_WHITE + "├";
  string top_line;
  for (
    unsigned int i = fmax(0,
      K.display.width
      - 1
      - top.length()
      - quit.length()
      + ANSI_SYMBOL_SIZE(12)
      + ANSI_COLORS_SIZE(7)
    );
    i --> 0;
    top_line += "─"
  );
  string coins_line;
  for (
    unsigned int i = fmax(0,
      title.length()
      + space.length()
      - coins.length()
      + ANSI_SYMBOL_SIZE(1)
      + ANSI_COLORS_SIZE(5)
    );
    i --> 0;
    coins_line += "─"
  );
  const vector<Order> openOrders = K.engine.orders.working(true);
  unsigned int orders = openOrders.size();
  unsigned int rows = 0;
  string data = ANSI_PUKE_WHITE
              + (openOrders.empty() and K.engine.broker.semaphore.paused()
                ? "└"
                : "├"
              ) + "───┤< (";
  if (K.engine.broker.semaphore.offline()) {
    data += ANSI_HIGH_RED    + "DISCONNECTED"
          + ANSI_PUKE_WHITE  + ")"
          + ANSI_END_LINE;
  } else {
    if (K.engine.broker.semaphore.paused())
      data += ANSI_WAVE_YELLOW + "press START to trade"
            + ANSI_PUKE_WHITE  + ")";
    else
      data += ANSI_PUKE_YELLOW + to_string(orders)
            + ANSI_PUKE_WHITE  + ") Open Orders";
    data += " while "
          + ANSI_PUKE_GREEN
          + "1 " + K.gateway->base
          + " = "
          + ANSI_HIGH_GREEN
          + K.gateway->decimal.price.str(K.engine.levels.fairValue)
          + ANSI_PUKE_GREEN
          + " " + K.gateway->quote
          + (K.engine.broker.semaphore.paused() ? ' ' : ':')
          + ANSI_END_LINE;
    for (const auto &it : openOrders) {
      data += ANSI_PUKE_WHITE + "├"
            + (it.side == Side::Bid ? ANSI_PUKE_CYAN + "BID" : ANSI_PUKE_MAGENTA + "ASK")
            + " > "
            + K.gateway->decimal.amount.str(it.quantity)
            + ' ' + K.gateway->base + " at price "
            + K.gateway->decimal.price.str(it.price)
            + ' ' + K.gateway->quote + " (value "
            + K.gateway->decimal.price.str(abs(it.price * it.quantity))
            + ' ' + K.gateway->quote + ")"
            + ANSI_END_LINE;
      ++rows;
    }
    if (!K.engine.broker.semaphore.paused())
      while (orders < 2) {
        data += ANSI_PUKE_WHITE + "├"
             + ANSI_END_LINE;
        ++orders;
        ++rows;
      }
  }
  return ANSI_PUKE_WHITE
    + top + top_line + quit
    + ANSI_END_LINE
    + "│  " + K.spin() + "  └───┤ " + coins + coins_line + "┘"
    + ANSI_END_LINE
    + K.logs(rows + 4, "│ ")
    + data;
};
