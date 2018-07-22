#ifndef K_CF_H_
#define K_CF_H_

namespace K {
  class CF {
    public:
      void main(int argc, char** argv) {
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
          {"database",     required_argument, 0,               'd'},
          {"wallet-limit", required_argument, 0,               'W'},
          {"market-limit", required_argument, 0,               'M'},
          {"client-limit", required_argument, 0,               'C'},
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
            case 'C': args.maxAdmins    = stoi(optarg);   break;
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
            case 'h': cout
              << RGREEN << PERMISSIVE_ctubio_SOFTWARE_LICENSE << '\n'
              << RGREEN << "  questions: " << RYELLOW << "https://earn.com/analpaper/" << '\n'
              << BGREEN << "K" << RGREEN << " bugkiller: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << '\n'
              << RGREEN << "  downloads: " << RYELLOW << "ssh://git@github.com/ctubio/Krypto-trading-bot" << '\n'
              << screen->stamp() << "Usage:" << BYELLOW << " ./K.sh [arguments]" << '\n'
              << screen->stamp() << "[arguments]:" << '\n'
              << screen->stamp() << RWHITE << "-h, --help                - show this help and quit." << '\n'
              << screen->stamp() << RWHITE << "    --autobot             - automatically start trading on boot." << '\n'
              << screen->stamp() << RWHITE << "    --dustybot            - do not automatically cancel all orders on exit." << '\n'
              << screen->stamp() << RWHITE << "    --naked               - do not display CLI, print output to stdout instead." << '\n'
              << screen->stamp() << RWHITE << "    --headless            - do not listen for UI connections," << '\n'
              << screen->stamp() << RWHITE << "                            all other UI related arguments will be ignored." << '\n'
              << screen->stamp() << RWHITE << "-C, --client-limit=NUMBER - set NUMBER of maximum concurrent UI connections." << '\n'
              << screen->stamp() << RWHITE << "    --without-ssl         - do not use HTTPS for UI connections (use HTTP only)." << '\n'
              << screen->stamp() << RWHITE << "-L, --whitelist=IP        - set IP or csv of IPs to allow UI connections," << '\n'
              << screen->stamp() << RWHITE << "                            alien IPs will get a zip-bomb instead." << '\n'
              << screen->stamp() << RWHITE << "    --port=NUMBER         - set NUMBER of an open port to listen for UI connections." << '\n'
              << screen->stamp() << RWHITE << "    --user=WORD           - set allowed WORD as username for UI connections," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << screen->stamp() << RWHITE << "    --pass=WORD           - set allowed WORD as password for UI connections," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << screen->stamp() << RWHITE << "    --exchange=NAME       - set exchange NAME for trading, mandatory one of:" << '\n'
              << screen->stamp() << RWHITE << "                            'COINBASE', 'BITFINEX',  'BITFINEX_MARGIN', 'HITBTC'," << '\n'
              << screen->stamp() << RWHITE << "                            'OKCOIN', 'OKEX', 'KORBIT', 'POLONIEX' or 'NULL'." << '\n'
              << screen->stamp() << RWHITE << "    --currency=PAIRS      - set currency pairs for trading (use format" << '\n'
              << screen->stamp() << RWHITE << "                            with '/' separator, like 'BTC/EUR')." << '\n'
              << screen->stamp() << RWHITE << "    --apikey=WORD         - set (never share!) WORD as api key for trading," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory." << '\n'
              << screen->stamp() << RWHITE << "    --secret=WORD         - set (never share!) WORD as api secret for trading," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory." << '\n'
              << screen->stamp() << RWHITE << "    --passphrase=WORD     - set (never share!) WORD as api passphrase for trading," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << screen->stamp() << RWHITE << "    --username=WORD       - set (never share!) WORD as api username for trading," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << screen->stamp() << RWHITE << "    --http=URL            - set URL of api HTTP/S endpoint for trading," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory." << '\n'
              << screen->stamp() << RWHITE << "    --wss=URL             - set URL of api SECURE WS endpoint for trading," << '\n'
              << screen->stamp() << RWHITE << "                            mandatory." << '\n'
              << screen->stamp() << RWHITE << "-d, --database=PATH       - set alternative PATH to database filename," << '\n'
              << screen->stamp() << RWHITE << "                            default PATH is '/data/db/K.*.*.*.db'," << '\n'
              << screen->stamp() << RWHITE << "                            any route to a filename is valid," << '\n'
              << screen->stamp() << RWHITE << "                            or use ':memory:' (see sqlite.org/inmemorydb.html)." << '\n'
              << screen->stamp() << RWHITE << "    --wallet-limit=AMOUNT - set AMOUNT in base currency to limit the balance," << '\n'
              << screen->stamp() << RWHITE << "                            otherwise the full available balance can be used." << '\n'
              << screen->stamp() << RWHITE << "    --market-limit=NUMBER - set NUMBER of maximum price levels for the orderbook," << '\n'
              << screen->stamp() << RWHITE << "                            default NUMBER is '321' and the minimum is '15'." << '\n'
              << screen->stamp() << RWHITE << "                            locked bots smells like '--market-limit=3' spirit." << '\n'
              << screen->stamp() << RWHITE << "-T, --lifetime=NUMBER     - set NUMBER of minimum milliseconds to keep orders open," << '\n'
              << screen->stamp() << RWHITE << "                            otherwise open orders can be replaced anytime required." << '\n'
              << screen->stamp() << RWHITE << "    --debug-secret        - print (never share!) secret inputs and outputs." << '\n'
              << screen->stamp() << RWHITE << "    --debug-events        - print detailed output about event handlers." << '\n'
              << screen->stamp() << RWHITE << "    --debug-orders        - print detailed output about exchange messages." << '\n'
              << screen->stamp() << RWHITE << "    --debug-quotes        - print detailed output about quoting engine." << '\n'
              << screen->stamp() << RWHITE << "    --debug-wallet        - print detailed output about target base position." << '\n'
              << screen->stamp() << RWHITE << "    --debug               - print detailed output about all the (previous) things!" << '\n'
              << screen->stamp() << RWHITE << "    --colors              - print highlighted output." << '\n'
              << screen->stamp() << RWHITE << "    --ignore-sun          - do not switch UI to light theme on daylight." << '\n'
              << screen->stamp() << RWHITE << "    --ignore-moon         - do not switch UI to dark theme on moonlight." << '\n'
              << screen->stamp() << RWHITE << "-k, --matryoshka=URL      - set Matryoshka link URL of the next UI." << '\n'
              << screen->stamp() << RWHITE << "-K, --title=WORD          - set WORD as UI title to identify different bots." << '\n'
              << screen->stamp() << RWHITE << "-x, --test-chamber=NUMBER - set release candidate NUMBER to test (ask your developer)." << '\n'
              << screen->stamp() << RWHITE << "-i, --interface=IP        - set IP to bind as outgoing network interface," << '\n'
              << screen->stamp() << RWHITE << "                            default IP is the system default network interface." << '\n'
              << screen->stamp() << RWHITE << "    --free-version        - work with all market levels and enable the slow XMR miner." << '\n'
              << screen->stamp() << RWHITE << "-v, --version             - show current build version and quit." << '\n'
              << RGREEN << "  more help: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/blob/master/MANUAL.md" << '\n'
              << BGREEN << "K" << RGREEN << " questions: " << RYELLOW << "irc://irc.freenode.net:6667/#tradingBot" << '\n'
              << RGREEN << "  home page: " << RYELLOW << "https://ca.rles-tub.io./trades" << '\n'
              << RRESET;
            case '?':
            case 'v': EXIT(EXIT_SUCCESS);
            default : abort();
          }
        if (optind < argc) {
          string argerr;
          while(optind < argc) argerr += string(" ") + argv[optind++];
          EXIT(screen->error("CF", "Invalid argument option:" + argerr));
        }
        validate();
        config();
        log();
      };
    private:
      void validate() {
        const string msg = args.validate();
        if (msg.empty()) return;
        EXIT(screen->error("CF", msg));
      };
      void config() {
        gw = Gw::config(
          args.base(),    args.quote(),
          args.exchange,  args.free,
          args.apikey,    args.secret,
          args.username,  args.passphrase,
          args.http,      args.wss,
          args.maxLevels, args.debugSecret
        );
        if (!gw)
          EXIT(screen->error("CF",
            "Unable to configure a valid gateway using --exchange="
              + args.exchange + " argument"
          ));
        screen->config();
      };
      void log() {
        if (args.inet)
          screen->log("CF", "Network Interface for outgoing traffic is", args.inet);
        for (string &it : args.warnings())
          screen->logWar("CF", it);
      };
  };
}

#endif
