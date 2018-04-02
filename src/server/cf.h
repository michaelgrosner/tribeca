#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  class CF: public kLass {
    public:
        int argPort         = 3000,   argColors       = 0, argDebug        = 0,
            argDebugSecret  = 0,      argDebugEvents  = 0, argDebugOrders  = 0,
            argDebugQuotes  = 0,      argDebugWallet  = 0, argWithoutSSL   = 0,
            argHeadless     = 0,      argDustybot     = 0, argLifetime     = 0,
            argAutobot      = 0,      argNaked        = 0, argFree         = 0,
            argIgnoreSun    = 0,      argIgnoreMoon   = 0, argMaxLevels    = 0,
            argTestChamber  = 0;
    mAmount argMaxWallet    = 0;
     mPrice argEwmaUShort   = 0,      argEwmaXShort   = 0, argEwmaShort    = 0,
            argEwmaMedium   = 0,      argEwmaLong     = 0, argEwmaVeryLong = 0;
     string argTitle        = "K.sh", argMatryoshka   = "https://www.example.com/",
            argUser         = "NULL", argPass         = "NULL",
            argExchange     = "NULL", argCurrency     = "NULL",
            argApikey       = "NULL", argSecret       = "NULL",
            argUsername     = "NULL", argPassphrase   = "NULL",
            argHttp         = "NULL", argWss          = "NULL",
            argDatabase     = "",     argDiskdata     = "",
            argWhitelist    = "";
    public:
      CF() {
        screen = new SH();
      }
    protected:
      void load(int argc, char** argv) {
        static const struct option args[] = {
          {"help",         no_argument,       0,               'h'},
          {"colors",       no_argument,       &argColors,        1},
          {"debug",        no_argument,       &argDebug,         1},
          {"debug-secret", no_argument,       &argDebugSecret,   1},
          {"debug-events", no_argument,       &argDebugEvents,   1},
          {"debug-orders", no_argument,       &argDebugOrders,   1},
          {"debug-quotes", no_argument,       &argDebugQuotes,   1},
          {"debug-wallet", no_argument,       &argDebugWallet,   1},
          {"without-ssl",  no_argument,       &argWithoutSSL,    1},
          {"ignore-sun",   no_argument,       &argIgnoreSun,     2},
          {"ignore-moon",  no_argument,       &argIgnoreMoon,    1},
          {"headless",     no_argument,       &argHeadless,      1},
          {"naked",        no_argument,       &argNaked,         1},
          {"autobot",      no_argument,       &argAutobot,       1},
          {"dustybot",     no_argument,       &argDustybot,      1},
          {"lifetime",     required_argument, 0,               'T'},
          {"whitelist",    required_argument, 0,               'L'},
          {"port",         required_argument, 0,               999},
          {"user",         required_argument, 0,               998},
          {"pass",         required_argument, 0,               997},
          {"exchange",     required_argument, 0,               996},
          {"currency",     required_argument, 0,               995},
          {"apikey",       required_argument, 0,               994},
          {"secret",       required_argument, 0,               993},
          {"passphrase",   required_argument, 0,               992},
          {"username",     required_argument, 0,               991},
          {"http",         required_argument, 0,               990},
          {"wss",          required_argument, 0,               989},
          {"title",        required_argument, 0,               'K'},
          {"matryoshka",   required_argument, 0,               'k'},
          {"ewma-ultra",   required_argument, 0,               905},
          {"ewma-micro",   required_argument, 0,               904},
          {"ewma-short",   required_argument, 0,               903},
          {"ewma-medium",  required_argument, 0,               902},
          {"ewma-long",    required_argument, 0,               901},
          {"ewma-verylong",required_argument, 0,               900},
          {"database",     required_argument, 0,               'd'},
          {"wallet-limit", required_argument, 0,               'W'},
          {"market-limit", required_argument, 0,               'M'},
          {"test-chamber", required_argument, 0,               'x'},
          {"free-version", no_argument,       &argFree,          1},
          {"version",      no_argument,       0,               'v'},
          {0,              0,                 0,                 0}
        };
        int k = 0;
        while (++k)
          switch (k = getopt_long(argc, argv, "hvd:k:x:K:L:M:T:W:", args, NULL)) {
            case -1 :
            case  0 : break;
            case 'x': argTestChamber  = stoi(optarg);   break;
            case 'M': argMaxLevels    = stoi(optarg);   break;
            case 'T': argLifetime     = stoi(optarg);   break;
            case 999: argPort         = stoi(optarg);   break;
            case 998: argUser         = string(optarg); break;
            case 997: argPass         = string(optarg); break;
            case 996: argExchange     = string(optarg); break;
            case 995: argCurrency     = string(optarg); break;
            case 994: argApikey       = string(optarg); break;
            case 993: argSecret       = string(optarg); break;
            case 992: argPassphrase   = string(optarg); break;
            case 991: argUsername     = string(optarg); break;
            case 990: argHttp         = string(optarg); break;
            case 989: argWss          = string(optarg); break;
            case 'd': argDatabase     = string(optarg); break;
            case 'k': argMatryoshka   = string(optarg); break;
            case 'K': argTitle        = string(optarg); break;
            case 'L': argWhitelist    = string(optarg); break;
            case 'W': argMaxWallet    = stod(optarg);   break;
            case 905: argEwmaUShort   = stod(optarg);   break;
            case 904: argEwmaXShort   = stod(optarg);   break;
            case 903: argEwmaShort    = stod(optarg);   break;
            case 902: argEwmaMedium   = stod(optarg);   break;
            case 901: argEwmaLong     = stod(optarg);   break;
            case 900: argEwmaVeryLong = stod(optarg);   break;
            case 'h': cout
              << RGREEN << "This is free software: the quoting engine and UI are open source,"
                        << '\n' << "feel free to hack both as you need." << '\n'
              << RGREEN << "This is non-free software: the exchange integrations are licensed"
                        << '\n' << "by and under the law of my grandma, feel free to crack all." << '\n'
              << RGREEN << "  questions: " << RYELLOW << "https://earn.com/analpaper/" << '\n'
              << BGREEN << "K" << RGREEN << " bugkiller: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << '\n'
              << RGREEN << "  downloads: " << RYELLOW << "ssh://git@github.com/ctubio/Krypto-trading-bot" << '\n'
              << ((SH*)screen)->stamp() << "Usage:" << BYELLOW << " ./K.sh [arguments]" << '\n'
              << ((SH*)screen)->stamp() << "[arguments]:" << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-h, --help                - show this help and quit." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --autobot             - automatically start trading on boot." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --dustybot            - do not automatically cancel all orders on exit." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --naked               - do not display CLI, print output to stdout instead." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --headless            - do not listen for UI connections," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            ignores '--without-ssl', '--whitelist' and '--port'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --without-ssl         - do not use HTTPS for UI connections (use HTTP only)." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-L, --whitelist=IP        - set IP or csv of IPs to allow UI connections," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            alien IPs will get a zip-bomb instead." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --port=NUMBER         - set NUMBER of an open port to listen for UI connections." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --user=WORD           - set allowed WORD as username for UI connections," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --pass=WORD           - set allowed WORD as password for UI connections," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --exchange=NAME       - set exchange NAME for trading, mandatory one of:" << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            'COINBASE', 'BITFINEX',  'BITFINEX_MARGIN', 'HITBTC'," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            'OKCOIN', 'OKEX', 'KORBIT', 'POLONIEX' or 'NULL'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --currency=PAIRS      - set currency pairs for trading (use format" << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            with '/' separator, like 'BTC/EUR')." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --apikey=WORD         - set (never share!) WORD as api key for trading," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --secret=WORD         - set (never share!) WORD as api secret for trading," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --passphrase=WORD     - set (never share!) WORD as api passphrase for trading," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --username=WORD       - set (never share!) WORD as api username for trading," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --http=URL            - set URL of api HTTP/S endpoint for trading," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --wss=URL             - set URL of api SECURE WS endpoint for trading," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-d, --database=PATH       - set alternative PATH to database filename," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            default PATH is '/data/db/K.*.*.*.db'," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            any route to a filename is valid," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            or use ':memory:' (see sqlite.org/inmemorydb.html)." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ewma-ultra=PRICE    - set initial ewma ultra short value," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ewma-micro=PRICE    - set initial ewma micro short value," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ewma-short=PRICE    - set initial ewma short value," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ewma-medium=PRICE   - set initial ewma medium value," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ewma-long=PRICE     - set initial ewma long value," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ewma-verylong=PRICE - set initial ewma verylong value," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --wallet-limit=AMOUNT - set AMOUNT in base currency to limit the balance," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            otherwise the full available balance can be used." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --market-limit=NUMBER - set NUMBER of maximum price levels for the orderbook," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            default NUMBER is '321' and the minimum is '15'." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            locked bots smells like '--market-limit=3' spirit." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-T, --lifetime=NUMBER     - set NUMBER of minimum milliseconds to keep orders open," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            otherwise open orders can be replaced anytime required." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --debug-secret        - print (never share!) secret inputs and outputs." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --debug-events        - print detailed output about event handlers." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --debug-orders        - print detailed output about exchange messages." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --debug-quotes        - print detailed output about quoting engine." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --debug-wallet        - print detailed output about target base position." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --debug               - print detailed output about all the (previous) things!" << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --colors              - print highlighted output." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ignore-sun          - do not switch UI to light theme on daylight." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --ignore-moon         - do not switch UI to dark theme on moonlight." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-k, --matryoshka=URL      - set Matryoshka link URL of the next UI." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-K, --title=WORD          - set WORD as UI title to identify different bots." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-x, --test-chamber=NUMBER - set release candidate NUMBER to test (ask your developer)." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "    --free-version        - work with all market levels and enable the slow XMR miner." << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "-v, --version             - show current build version and quit." << '\n'
              << RGREEN << "  more help: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/blob/master/MANUAL.md" << '\n'
              << BGREEN << "K" << RGREEN << " questions: " << RYELLOW << "irc://irc.freenode.net:6667/#tradingBot" << '\n'
              << RGREEN << "  home page: " << RYELLOW << "https://ca.rles-tub.io./trades" << '\n'
              << RRESET;
            case '?':
            case 'v': exit(EXIT_SUCCESS);
            default : abort();
          }
        if (optind < argc) {
          string argerr;
          while(optind < argc) argerr += string(" ") + argv[optind++];
          exit(_redAlert_("CF", string("Invalid argument option:") + argerr));
        }
        if (argCurrency.find("/") == string::npos)
          exit(_redAlert_("CF", "Invalid currency pair; must be in the format of BASE/QUOTE, like BTC/EUR"));
        tidy();
      };
      void run() {
#ifndef _WIN32
        if (!argNaked)
          ((SH*)screen)->config(
            base(),       quote(),
            argExchange,  argColors,
            argPort
          );
#endif
        gw = Gw::config(
          base(),       quote(),
          argExchange,  argFree,
          argApikey,    argSecret,
          argUsername,  argPassphrase,
          argHttp,      argWss,
          argMaxLevels, argDebugSecret,
          chambers()
        );
        if (!gw)
          exit(_redAlert_("CF", string("Unable to load a valid gateway using --exchange=")
            + argExchange + " argument"));
      };
    private:
      inline mCoinId base() {
        return FN::strU(argCurrency.substr(0, argCurrency.find("/")));
      };
      inline mCoinId quote() {
        return FN::strU(argCurrency.substr(argCurrency.find("/") + 1));
      };
      inline int chambers() {
        if (argTestChamber == 1)
          ((SH*)screen)->logWar("CF", "Test Chamber #1: GDAX send new before cancel old");
        else if (argTestChamber)
          ((SH*)screen)->logWar("CF", string("ignored Test Chamber #") + to_string(argTestChamber));
        return argTestChamber;
      };
      inline void tidy() {
        if (argDebug)
          argDebugSecret =
          argDebugEvents =
          argDebugOrders =
          argDebugQuotes =
          argDebugWallet = argDebug;
        if (!argColors)
          RBLACK[0] = RRED[0]    = RGREEN[0] = RYELLOW[0] =
          RBLUE[0]  = RPURPLE[0] = RCYAN[0]  = RWHITE[0]  =
          BBLACK[0] = BRED[0]    = BGREEN[0] = BYELLOW[0] =
          BBLUE[0]  = BPURPLE[0] = BCYAN[0]  = BWHITE[0]  = RRESET[0] = 0;
        if (argDatabase.empty() or argDatabase == ":memory:")
          (argDatabase == ":memory:"
            ? argDiskdata
            : argDatabase
          ) = string("/data/db/K")
            + '.' + FN::strU(argExchange)
            + '.' + base()
            + '.' + quote()
            + '.' + "db";
        argMaxLevels = argMaxLevels
          ? max(15, argMaxLevels)
          : 321;
        if (argUser == "NULL") argUser.clear();
        if (argPass == "NULL") argPass.clear();
        if (argIgnoreSun and argIgnoreMoon) argIgnoreMoon = 0;
        if (argHeadless) argPort = 0;
        else if (!argPort) argHeadless = 1;
      };
  };
}

#endif
