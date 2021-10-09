class TradingBot: public KryptoNinja {
  public:
    tribeca::Engine engine;
  public:
    TradingBot()
      : engine(*this)
    {
      display   = { terminal };
      events    = {
        [&](const Connectivity &rawdata) { engine.read(rawdata);  },
        [&](const Wallet       &rawdata) { engine.read(rawdata);  },
        [&](const Levels       &rawdata) { engine.read(rawdata);  },
        [&](const Order        &rawdata) { engine.read(rawdata);  },
        [&](const Trade        &rawdata) { engine.read(rawdata);  },
        [&](const unsigned int &tick)    { engine.timer_1s(tick); },
        [&]()                            { engine.quit();         }
      };
      documents = {
        {"",                                  DISK_FILE(gzip_bomb)  },
        {"/",                                 DISK_FILE(html_index) },
        {"/js/client.min.js",                 DISK_FILE(js_client)  },
        {"/css/bootstrap.min.css",            DISK_FILE(css_base)   },
        {"/css/bootstrap-theme-dark.min.css", DISK_FILE(css_dark)   },
        {"/css/bootstrap-theme.min.css",      DISK_FILE(css_light)  },
        {"/font/beacons.woff2",               DISK_FILE(woff2_font) },
        {"/favicon.ico",                      DISK_FILE(ico_favicon)},
        {"/audio/0.mp3",                      DISK_FILE(mp3_audio_0)},
        {"/audio/1.mp3",                      DISK_FILE(mp3_audio_1)}
      };
      arguments = {
        {
          {"maker-fee",    "AMOUNT", "0",     "set custom percentage of maker fee, like '0.1'"},
          {"taker-fee",    "AMOUNT", "0",     "set custom percentage of taker fee, like '0.1'"},
          {"min-size",     "AMOUNT", "0",     "set custom minimum order size, like '0.01'"},
          {"leverage",     "AMOUNT", "1",     "set between '0.01' and '100' to enable isolated margin,"
                                              ANSI_NEW_LINE "or use '0' for cross margin; default AMOUNT is '1'"},
          {"wallet-limit", "AMOUNT", "0",     "set AMOUNT in base currency to limit the balance,"
                                              ANSI_NEW_LINE "otherwise the full available balance can be used"}
        },
        nullptr
      };
    };
  private:
    static string terminal();
} K;

string TradingBot::terminal() {
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
