#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  class CF: public kLass {
    public:
      CF() {
        screen = new SH();
      }
    protected:
      void load(int argc, char** argv) {
        static const struct option opts[] = {
          {"help",         no_argument,       0,               'h'},
          {"colors",       no_argument,       &args.colors,      1},
          {"debug",        no_argument,       &args.debug,       1},
          {"debug-secret", no_argument,       &args.debugSecret, 1},
          {"debug-events", no_argument,       &args.debugEvents, 1},
          {"debug-orders", no_argument,       &args.debugOrders, 1},
          {"debug-quotes", no_argument,       &args.debugQuotes, 1},
          {"debug-wallet", no_argument,       &args.debugWallet, 1},
          {"without-ssl",  no_argument,       &args.withoutSSL,  1},
          {"ignore-sun",   no_argument,       &args.ignoreSun,   2},
          {"ignore-moon",  no_argument,       &args.ignoreMoon,  1},
          {"headless",     no_argument,       &args.headless,    1},
          {"naked",        no_argument,       &args.naked,       1},
          {"autobot",      no_argument,       &args.autobot,     1},
          {"dustybot",     no_argument,       &args.dustybot,    1},
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
          {"interface",    required_argument, 0,               'i'},
          {"free-version", no_argument,       &args.free,        1},
          {"version",      no_argument,       0,               'v'},
          {0,              0,                 0,                 0}
        };
        int k = 0;
        while (++k)
          switch (k = getopt_long(argc, argv, "hvd:i:k:x:K:L:M:T:W:", opts, NULL)) {
            case -1 :
            case  0 : break;
            case 'x': args.testChamber  = stoi(optarg);   break;
            case 'M': args.maxLevels    = stoi(optarg);   break;
            case 'T': args.lifetime     = stoi(optarg);   break;
            case 999: args.port         = stoi(optarg);   break;
            case 998: args.user         = string(optarg); break;
            case 997: args.pass         = string(optarg); break;
            case 996: args.exchange     = string(optarg); break;
            case 995: args.currency     = string(optarg); break;
            case 994: args.apikey       = string(optarg); break;
            case 993: args.secret       = string(optarg); break;
            case 992: args.passphrase   = string(optarg); break;
            case 991: args.username     = string(optarg); break;
            case 990: args.http         = string(optarg); break;
            case 989: args.wss          = string(optarg); break;
            case 'd': args.database     = string(optarg); break;
            case 'k': args.matryoshka   = string(optarg); break;
            case 'K': args.title        = string(optarg); break;
            case 'L': args.whitelist    = string(optarg); break;
            case 'i': args.inet         = strdup(optarg); break;
            case 'W': args.maxWallet    = stod(optarg);   break;
            case 905: args.ewmaUShort   = stod(optarg);   break;
            case 904: args.ewmaXShort   = stod(optarg);   break;
            case 903: args.ewmaShort    = stod(optarg);   break;
            case 902: args.ewmaMedium   = stod(optarg);   break;
            case 901: args.ewmaLong     = stod(optarg);   break;
            case 900: args.ewmaVeryLong = stod(optarg);   break;
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
              << ((SH*)screen)->stamp() << RWHITE << "-i, --interface=IP        - set IP to bind as outgoing network interface," << '\n'
              << ((SH*)screen)->stamp() << RWHITE << "                            default IP is the system default network interface." << '\n'
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
        if (args.currency.find("/") == string::npos or args.currency.length() < 3)
          exit(_redAlert_("CF", "Invalid currency pair; must be in the format of BASE/QUOTE, like BTC/EUR"));
        if (args.exchange.empty())
          exit(_redAlert_("CF", "Undefined exchange; the config file may have errors (there are extra spaces or double defined variables?)"));
        tidy();
      };
      void run() {
#ifndef _WIN32
        ((SH*)screen)->config(base(), quote());
#endif
        gw = Gw::config(
          base(),         quote(),
          args.exchange,  args.free,
          args.apikey,    args.secret,
          args.username,  args.passphrase,
          args.http,      args.wss,
          args.maxLevels, args.debugSecret
        );
        if (!gw)
          exit(_redAlert_("CF", string("Unable to load a valid gateway using --exchange=")
            + args.exchange + " argument"));
        if (args.inet)
          ((SH*)screen)->log("CF", "Network Interface for outgoing traffic is", args.inet);
        if (args.testChamber == 1)
          ((SH*)screen)->logWar("CF", "Test Chamber #1: send new orders before cancel old");
        else if (args.testChamber)
          ((SH*)screen)->logWar("CF", string("ignored Test Chamber #") + to_string(args.testChamber));
      };
    private:
      inline mCoinId base() {
        return FN::strU(args.currency.substr(0, args.currency.find("/")));
      };
      inline mCoinId quote() {
        return FN::strU(args.currency.substr(args.currency.find("/") + 1));
      };
      inline void tidy() {
        args.exchange = FN::strU(args.exchange);
        if (args.debug)
          args.debugSecret =
          args.debugEvents =
          args.debugOrders =
          args.debugQuotes =
          args.debugWallet = args.debug;
        if (!args.colors)
          RBLACK[0] = RRED[0]    = RGREEN[0] = RYELLOW[0] =
          RBLUE[0]  = RPURPLE[0] = RCYAN[0]  = RWHITE[0]  =
          BBLACK[0] = BRED[0]    = BGREEN[0] = BYELLOW[0] =
          BBLUE[0]  = BPURPLE[0] = BCYAN[0]  = BWHITE[0]  = RRESET[0] = 0;
        if (args.database.empty() or args.database == ":memory:")
          (args.database == ":memory:"
            ? args.diskdata
            : args.database
          ) = string("/data/db/K")
            + '.' + args.exchange
            + '.' + base()
            + '.' + quote()
            + '.' + "db";
        args.maxLevels = args.maxLevels
          ? max(15, args.maxLevels)
          : 321;
        if (args.user == "NULL") args.user.clear();
        if (args.pass == "NULL") args.pass.clear();
        if (args.ignoreSun and args.ignoreMoon) args.ignoreMoon = 0;
        if (args.headless) args.port = 0;
        else if (!args.port) args.headless = 1;
      };
  };
}

#endif
