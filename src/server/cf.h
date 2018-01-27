#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  class CF: public kLass {
    public:
       int argPort         = 3000,   argColors       = 0, argDebug        = 0,
           argDebugSecret  = 0,      argDebugEvents  = 0, argDebugOrders  = 0,
           argDebugQuotes  = 0,      argDebugWallet  = 0, argWithoutSSL   = 0,
           argMaxLevels    = 0,      argHeadless     = 0, argDustybot     = 0,
           argAutobot      = 0,      argNaked        = 0, argFree         = 0,
           argIgnoreSun    = 0,      argIgnoreMoon   = 0;
    mPrice argEwmaShort    = 0,      argEwmaMedium   = 0,
           argEwmaLong     = 0,      argEwmaVeryLong = 0;
    string argTitle        = "K.sh", argMatryoshka   = "https://www.example.com/",
           argUser         = "NULL", argPass         = "NULL",
           argExchange     = "NULL", argCurrency     = "NULL",
           argApikey       = "NULL", argSecret       = "NULL",
           argUsername     = "NULL", argPassphrase   = "NULL",
           argHttp         = "NULL", argWss          = "NULL",
           argDatabase     = "",     argDiskdata     = "",
           argWhitelist    = "";
    protected:
      void load(int argc, char** argv) {
        screen = new SH();
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
          {"whitelist",    required_argument, 0,               'L'},
          {"matryoshka",   required_argument, 0,               'k'},
          {"exchange",     required_argument, 0,               'e'},
          {"currency",     required_argument, 0,               'c'},
          {"apikey",       required_argument, 0,               'A'},
          {"secret",       required_argument, 0,               'S'},
          {"passphrase",   required_argument, 0,               'X'},
          {"username",     required_argument, 0,               'U'},
          {"http",         required_argument, 0,               'H'},
          {"wss",          required_argument, 0,               'W'},
          {"title",        required_argument, 0,               'K'},
          {"port",         required_argument, 0,               'P'},
          {"user",         required_argument, 0,               'u'},
          {"pass",         required_argument, 0,               'p'},
          {"ewma-short",   required_argument, 0,               's'},
          {"ewma-medium",  required_argument, 0,               'm'},
          {"ewma-long",    required_argument, 0,               'l'},
          {"ewma-verylong",required_argument, 0,               'V'},
          {"database",     required_argument, 0,               'd'},
          {"market-limit", required_argument, 0,               'M'},
          {"free-version", no_argument,       &argFree,          1},
          {"version",      no_argument,       0,               'v'},
          {0,              0,                 0,                 0}
        };
        int k = 0;
        while (++k)
          switch (k = getopt_long(argc, argv, "hvc:d:e:k:l:m:s:p:u:v:A:H:K:M:P:S:U:W:X:", args, NULL)) {
            case -1 :
            case  0 : break;
            case 'P': argPort         = stoi(optarg);   break;
            case 'M': argMaxLevels    = stoi(optarg);   break;
            case 'A': argApikey       = string(optarg); break;
            case 'S': argSecret       = string(optarg); break;
            case 'U': argUsername     = string(optarg); break;
            case 'X': argPassphrase   = string(optarg); break;
            case 'H': argHttp         = string(optarg); break;
            case 'W': argWss          = string(optarg); break;
            case 'e': argExchange     = string(optarg); break;
            case 'c': argCurrency     = string(optarg); break;
            case 'd': argDatabase     = string(optarg); break;
            case 'k': argMatryoshka   = string(optarg); break;
            case 'K': argTitle        = string(optarg); break;
            case 'u': argUser         = string(optarg); break;
            case 'p': argPass         = string(optarg); break;
            case 'L': argWhitelist    = string(optarg); break;
            case 's': argEwmaShort    = stod(optarg);   break;
            case 'm': argEwmaMedium   = stod(optarg);   break;
            case 'l': argEwmaLong     = stod(optarg);   break;
            case 'V': argEwmaVeryLong = stod(optarg);   break;
            case 'h': cout
              << RGREEN << "This is free software: the quoting engine and UI are open source,"
                        << '\n' << "feel free to hack both as you need." << '\n'
              << RGREEN << "This is non-free software: the exchange integrations are licensed"
                        << '\n' << "by and under the law of my grandma, feel free to crack all." << '\n'
              << RGREEN << "  questions: " << RYELLOW << "https://earn.com/analpaper/" << '\n'
              << BGREEN << "K" << RGREEN << " bugkiller: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << '\n'
              << RGREEN << "  downloads: " << RYELLOW << "ssh://git@github.com/ctubio/Krypto-trading-bot" << '\n';
            case '?': cout
              << ((SH*)screen)->uiT() << "Usage:" << BYELLOW << " ./K.sh [arguments]" << '\n'
              << ((SH*)screen)->uiT() << "[arguments]:" << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-h, --help                - show this help and quit." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --autobot             - automatically start trading on boot." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --dustybot            - do not automatically cancel all orders on exit." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --naked               - do not display CLI, print output to stdout instead." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --headless            - do not listen for UI connections," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            ignores '--without-ssl', '--whitelist' and '--port'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --without-ssl         - do not use HTTPS for UI connections (use HTTP only)." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-L, --whitelist=IP        - set IP or csv of IPs to allow UI connections," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            alien IPs will get a zip-bomb instead." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-P, --port=NUMBER         - set NUMBER of an open port to listen for UI connections." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-u, --user=WORD           - set allowed WORD as username for UI connections," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-p, --pass=WORD           - set allowed WORD as password for UI connections," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-e, --exchange=NAME       - set exchange NAME for trading, mandatory one of:" << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            'COINBASE', 'BITFINEX',  'BITFINEX_MARGIN', 'HITBTC'," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            'OKCOIN', 'OKEX', 'KORBIT', 'POLONIEX' or 'NULL'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-c, --currency=PAIRS      - set currency pairs for trading (use format" << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            with '/' separator, like 'BTC/EUR')." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-A, --apikey=WORD         - set (never share!) WORD as api key for trading," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-S, --secret=WORD         - set (never share!) WORD as api secret for trading," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-X, --passphrase=WORD     - set (never share!) WORD as api passphrase for trading," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-U, --username=WORD       - set (never share!) WORD as api username for trading," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-H, --http=URL            - set URL of api HTTP/S endpoint for trading," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-W, --wss=URL             - set URL of api SECURE WS endpoint for trading," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            mandatory." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-d, --database=PATH       - set alternative PATH to database filename," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            default PATH is '/data/db/K.*.*.*.db'," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            any route to a filename is valid," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            or use ':memory:' (see sqlite.org/inmemorydb.html)." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-s, --ewma-short=PRICE    - set initial ewma short value," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-m, --ewma-medium=PRICE   - set initial ewma medium value," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-l, --ewma-long=PRICE     - set initial ewma long value," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-V, --ewma-verylong=PRICE - set initial ewma verylong value," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            overwrites the value from the database." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-M, --market-limit=NUMBER - set NUMBER of maximum price levels for the orderbook," << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            default NUMBER is '321' and the minimum is '15'." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "                            locked bots smells like '--market-limit=3' spirit." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --debug-secret        - print (never share!) secret inputs and outputs." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --debug-events        - print detailed output about event handlers." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --debug-orders        - print detailed output about exchange messages." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --debug-quotes        - print detailed output about quoting engine." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --debug-wallet        - print detailed output about target base position." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --debug               - print detailed output about all the (previous) things!" << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --colors              - print highlighted output." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --ignore-sun          - do not switch UI to light theme on daylight." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --ignore-moon         - do not switch UI to dark theme on moonlight." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-k, --matryoshka=URL      - set Matryoshka link URL of the next UI." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-K, --title=WORD          - set WORD as UI title to identify different bots." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "    --free-version        - work with all market levels but slowdown with 21 XMR hash." << '\n'
              << ((SH*)screen)->uiT() << RWHITE << "-v, --version             - show current build version and quit." << '\n'
              << RGREEN << "  more help: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/blob/master/MANUAL.md" << '\n'
              << BGREEN << "K" << RGREEN << " questions: " << RYELLOW << "irc://irc.domirc.net:6667/##tradingBot" << '\n'
              << RGREEN << "  home page: " << RYELLOW << "https://ca.rles-tub.io./trades" << '\n';
              exit(EXIT_SUCCESS);
              break;
            case 'v': exit(EXIT_SUCCESS);
            default: abort();
          }
        if (optind < argc) {
          cout << "ARG" << RRED << " Errrror:" << BRED << " non-option ARGV-elements: ";
          while(optind < argc) cout << argv[optind++];
          cout << '\n';
          exit(EXIT_SUCCESS);
        }
        if (argExchange.empty()) {
          cout << "ARG" << RRED << " Errrror:" << BRED
               << " Missing mandatory argument \"--exchange\", at least." << '\n';
          exit(EXIT_SUCCESS);
        }
        if (argCurrency.find("/") == string::npos) {
          cout << "ARG" << RRED << " Errrror:" << BRED
               << " Invalid currency pair; must be in the format of BASE/QUOTE, like BTC/EUR." << '\n';
          exit(EXIT_SUCCESS);
        }
        tidy();
      };
      void run() {
        gw = Gw::config(
          base(),       quote(),
          argExchange,  argFree,
          argApikey,    argSecret,
          argUsername,  argPassphrase,
          argHttp,      argWss,
          argMaxLevels, argDebugSecret,
          ((SH*)screen)->config(
            argNaked,    argColors,
            argExchange, argCurrency
          )
        );
      };
    private:
      inline mCoinId base() {
        return FN::S2u(argCurrency.substr(0, argCurrency.find("/")));
      };
      inline mCoinId quote() {
        return FN::S2u(argCurrency.substr(argCurrency.find("/")+1));
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
          BBLUE[0]  = BPURPLE[0] = BCYAN[0]  = BWHITE[0]  = argColors;
        if (argDatabase.empty() or argDatabase == ":memory:")
          (argDatabase == ":memory:"
            ? argDiskdata
            : argDatabase
          ) = string("/data/db/K")
            + '.' + FN::S2u(argExchange)
            + '.' + base()
            + '.' + quote()
            + '.' + "db";
        argMaxLevels = argMaxLevels
          ? max(15, argMaxLevels)
          : 321;
        if (argUser == "NULL") argUser.clear();
        if (argPass == "NULL") argPass.clear();
        if (argIgnoreSun and argIgnoreMoon) argIgnoreSun = 0;
      };
  };
}

#endif