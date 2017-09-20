#ifndef K_CF_H_
#define K_CF_H_

#ifndef K_BUILD
#define K_BUILD "0"
#endif

#ifndef K_STAMP
#define K_STAMP "0"
#endif

namespace K {
  static int argPort = 3000,
             argColors = 0,
             argDebugOrders = 0,
             argDebugQuotes = 0,
             argHeadless = 0,
             argAutobot = 0;
  static string argTitle = "K.sh",
                argExchange = "NULL",
                argUser = "NULL",
                argPass = "NULL",
                argMatryoshka = "https://www.example.com/",
                argDatabase = "",
                argCurrency = "NULL",
                argTarget = "NULL",
                argApikey = "NULL",
                argSecret = "NULL",
                argUsername = "NULL",
                argPassphrase = "NULL",
                argHttp = "NULL",
                argWs = "NULL",
                argWss = "NULL";
  static double argEwmaShort = 0,
                argEwmaMedium = 0,
                argEwmaLong = 0;
  static Gw* gw;
  static Gw* gW;
  class CF {
    public:
      static void main(int argc, char** argv) {
        cout << BGREEN << "K" << RGREEN << " build " << K_BUILD << " " << K_STAMP << "." << BRED << endl;
        int k;
        while (true) {
          int i = 0;
          static struct option args[] = {
            {"help",         no_argument,       0,               'h'},
            {"colors",       no_argument,       &argColors,        1},
            {"debug-orders", no_argument,       &argDebugOrders,   1},
            {"debug-quotes", no_argument,       &argDebugQuotes,   1},
            {"headless",     no_argument,       &argHeadless,      1},
            {"autobot",      no_argument,       &argAutobot,       1},
            {"matryoshka",   required_argument, 0,               'k'},
            {"exchange",     required_argument, 0,               'e'},
            {"currency",     required_argument, 0,               'c'},
            {"target",       required_argument, 0,               'T'},
            {"apikey",       required_argument, 0,               'A'},
            {"secret",       required_argument, 0,               'S'},
            {"passphrase",   required_argument, 0,               'X'},
            {"username",     required_argument, 0,               'U'},
            {"http",         required_argument, 0,               'H'},
            {"wss",          required_argument, 0,               'W'},
            {"ws",           required_argument, 0,               'w'},
            {"title",        required_argument, 0,               'K'},
            {"port",         required_argument, 0,               'P'},
            {"user",         required_argument, 0,               'u'},
            {"pass",         required_argument, 0,               'p'},
            {"database",     required_argument, 0,               'd'},
            {"ewma-short",   required_argument, 0,               's'},
            {"ewma-medium",  required_argument, 0,               'm'},
            {"ewma-long",    required_argument, 0,               'l'},
            {"version",      no_argument,       0,               'v'},
            {0,              0,                 0,                 0}
          };
          k = getopt_long(argc, argv, "hvd:l:m:s:p:u:v:c:e:k:P:K:w:W:H:U:X:S:A:T:", args, &i);
          if (k == -1) break;
          switch (k) {
            case 0: break;
            case 'P': argPort = stoi(optarg); break;
            case 'T': argTarget = string(optarg); break;
            case 'A': argApikey = string(optarg); break;
            case 'S': argSecret = string(optarg); break;
            case 'U': argUsername = string(optarg); break;
            case 'X': argPassphrase = string(optarg); break;
            case 'H': argHttp = string(optarg); break;
            case 'W': argWss = string(optarg); break;
            case 'w': argWs = string(optarg); break;
            case 'e': argExchange = string(optarg); break;
            case 'c': argCurrency = string(optarg); break;
            case 'd': argDatabase = string(optarg); break;
            case 'k': argMatryoshka = string(optarg); break;
            case 'K': argTitle = string(optarg); break;
            case 'u': argUser = string(optarg); break;
            case 'p': argPass = string(optarg); break;
            case 's': argEwmaShort = stod(optarg); break;
            case 'm': argEwmaMedium = stod(optarg); break;
            case 'l': argEwmaLong = stod(optarg); break;
            case 'h': cout
              << RGREEN << "This is free software: the quoting engine and UI are open source," << endl << "feel free to hack both as you need." << endl
              << RGREEN << "This is non-free software: the exchange integrations are licensed" << endl << "by and under the law of my grandma, feel free to crack all." << endl
              << BGREEN << "  questions: " << RYELLOW << "https://21.co/analpaper/" << endl
              << BGREEN << "K" << RGREEN << " bugkiller: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << endl
              << BGREEN << "  downloads: " << RYELLOW << "ssh://git@github.com/ctubio/Krypto-trading-bot" << endl;
            case '?': cout
              << FN::uiT() << "Usage:" << BYELLOW << " ./K.sh [arguments]" << endl
              << FN::uiT() << "[arguments]:" << endl
              << FN::uiT() << RWHITE << "-h, --help               - show this help and quit." << endl
              << FN::uiT() << RWHITE << "    --autobot            - automatically start trading on boot." << endl
              << FN::uiT() << RWHITE << "    --headless           - do not listen for UI connections (overwrites '-P')." << endl
              << FN::uiT() << RWHITE << "-P, --port=NUMBER        - set NUMBER of an open port to listen for UI connections." << endl
              << FN::uiT() << RWHITE << "-u, --user=WORD          - set allowed WORD as username for UI connections," << endl
              << FN::uiT() << RWHITE << "                           mandatory but may be 'NULL'." << endl
              << FN::uiT() << RWHITE << "-p, --pass=WORD          - set allowed WORD as password for UI connections," << endl
              << FN::uiT() << RWHITE << "                           mandatory but may be 'NULL'." << endl
              << FN::uiT() << RWHITE << "-e, --exchange=NAME      - set exchange NAME for trading, mandatory one of:" << endl
              << FN::uiT() << RWHITE << "                           'COINBASE', 'BITFINEX', 'HITBTC', 'OKCOIN'," << endl
              << FN::uiT() << RWHITE << "                           'KORBIT', 'POLONIEX' or 'NULL'." << endl
              << FN::uiT() << RWHITE << "-c, --currency=PAIRS     - set currency pairs for trading (use format" << endl
              << FN::uiT() << RWHITE << "                           with '/' separator, like 'BTC/EUR')." << endl
              << FN::uiT() << RWHITE << "-T, --target=NAME        - set orders destination (like '--exchange')," << endl
              << FN::uiT() << RWHITE << "                           a value of NULL generates fake orders only." << endl
              << FN::uiT() << RWHITE << "-A, --apikey=WORD        - set (never share!) WORD as api key for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory." << endl
              << FN::uiT() << RWHITE << "-S, --secret=WORD        - set (never share!) WORD as api secret for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory." << endl
              << FN::uiT() << RWHITE << "-X, --passphrase=WORD    - set (never share!) WORD as api passphrase for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory but may be 'NULL'." << endl
              << FN::uiT() << RWHITE << "-U, --username=WORD      - set (never share!) WORD as api username for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory but may be 'NULL'." << endl
              << FN::uiT() << RWHITE << "-H, --http=URL           - set URL of api HTTP/S endpoint for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory." << endl
              << FN::uiT() << RWHITE << "-W, --wss=URL            - set URL of api SECURE WS endpoint for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory." << endl
              << FN::uiT() << RWHITE << "-w, --ws=URL             - set URL of api PUBLIC WS endpoint for trading," << endl
              << FN::uiT() << RWHITE << "                           mandatory but may be 'NULL'." << endl
              << FN::uiT() << RWHITE << "-d, --database=PATH      - set alternative PATH to database filename," << endl
              << FN::uiT() << RWHITE << "                           default PATH is '/data/db/K.*.*.*.db'," << endl
              << FN::uiT() << RWHITE << "                           any route to a filename is valid," << endl
              << FN::uiT() << RWHITE << "                           or use ':memory:' (sqlite.org/inmemorydb.html)." << endl
              << FN::uiT() << RWHITE << "-s, --ewma-short=PRICE   - set initial ewma short value," << endl
              << FN::uiT() << RWHITE << "                           overwrites the value from the database." << endl
              << FN::uiT() << RWHITE << "-m, --ewma-medium=PRICE  - set initial ewma medium value," << endl
              << FN::uiT() << RWHITE << "                           overwrites the value from the database." << endl
              << FN::uiT() << RWHITE << "-l, --ewma-long=PRICE    - set initial ewma long value," << endl
              << FN::uiT() << RWHITE << "                           overwrites the value from the database." << endl
              << FN::uiT() << RWHITE << "    --debug-orders       - print detailed output about exchange messages." << endl
              << FN::uiT() << RWHITE << "    --debug-quotes       - print detailed output about quoting engine." << endl
              << FN::uiT() << RWHITE << "    --colors             - print highlighted output." << endl
              << FN::uiT() << RWHITE << "-k, --matryoshka=URL     - set Matryoska link URL of the next UI." << endl
              << FN::uiT() << RWHITE << "-K, --title=WORD         - set WORD as UI title to identify different bots." << endl
              << FN::uiT() << RWHITE << "-v, --version            - show current build version and quit." << endl
              << BGREEN << "  more help: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/blob/master/MANUAL.md" << endl
              << BGREEN << "K" << RGREEN << " questions: " << RYELLOW << "irc://irc.domirc.net:6667/##tradingBot" << endl
              << BGREEN << "  home page: " << RYELLOW << "https://ca.rles-tub.io./trades" << endl;
              exit(EXIT_SUCCESS);
              break;
            case 'v': exit(EXIT_SUCCESS);
            default: abort();
          }
        }
        if (optind < argc) {
          cout << FN::uiT() << "ARG" << RRED << " Warrrrning:" << BRED << " non-option ARGV-elements: ";
          while(optind < argc) cout << argv[optind++];
          cout << endl;
        }
        cout << RWHITE;
        if (!argColors) {
          RBLACK[0] = 0; RRED[0]    = 0; RGREEN[0] = 0; RYELLOW[0] = 0;
          RBLUE[0]  = 0; RPURPLE[0] = 0; RCYAN[0]  = 0; RWHITE[0]  = 0;
          BBLACK[0] = 0; BRED[0]    = 0; BGREEN[0] = 0; BYELLOW[0] = 0;
          BBLUE[0]  = 0; BPURPLE[0] = 0; BCYAN[0]  = 0; BWHITE[0]  = 0;
        }
        if (argExchange == "") cout << FN::uiT() << "CF" << RRED << " Warrrrning:" << BRED << " Settings not loaded because the config file was not found, reading ENVIRONMENT vars instead." << endl;
      };
      static void api() {
        gw = Gw::E(cfExchange());
        gw->name = argExchange;
        gw->base = cfBase();
        gw->quote = cfQuote();
        gw->apikey = argApikey;
        gw->secret = argSecret;
        gw->user = argUsername;
        gw->pass = argPassphrase;
        gw->http = argHttp;
        gw->ws = argWss;
        gw->wS = argWs;
        cfExchange(gw->config());
        gW = (argTarget == "NULL") ? Gw::E(mExchange::Null) : gw;
      };
      static int cfBase() {
        string k_ = argCurrency;
        string k = k_.substr(0, k_.find("/"));
        if (k == k_) { cout << FN::uiT() << "CF" << RRED << " Errrror:" << BRED << " Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR." << endl; exit(1); }
        return S2mC(k);
      };
      static int cfQuote() {
        string k_ = argCurrency;
        string k = k_.substr(k_.find("/")+1);
        if (k == k_) { cout << FN::uiT() << "CF" << RRED << " Errrror:" << BRED << " Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR." << endl; exit(1); }
        return S2mC(k);
      };
      static mExchange cfExchange() {
        string k = FN::S2l(argExchange);
        if (k == "coinbase") return mExchange::Coinbase;
        else if (k == "okcoin") return mExchange::OkCoin;
        else if (k == "bitfinex") return mExchange::Bitfinex;
        else if (k == "poloniex") return mExchange::Poloniex;
        else if (k == "korbit") return mExchange::Korbit;
        else if (k == "hitbtc") return mExchange::HitBtc;
        else if (k == "null") return mExchange::Null;
        cout << FN::uiT() << "CF" << RRED << " Errrror:" << BRED << " Invalid configuration value \"" << k << "\" as EXCHANGE. See https://github.com/ctubio/Krypto-trading-bot/tree/master/etc#configuration-options for more information." << endl;
        exit(1);
      };
      static int S2mC(string k) {
        k = FN::S2u(k);
        for (unsigned i=0; i<mCurrency.size(); ++i) if (mCurrency[i] == k) return i;
        cout << FN::uiT() << "CF" << RRED << " Errrror:" << BRED << " Use of missing \"" << k << "\" currency." << endl;
        exit(1);
      };
      static mConnectivity autoStart() {
        return argAutobot ? mConnectivity::Connected : mConnectivity::Disconnected;
      };
    private:
      static void cfExchange(mExchange e) {
        if (e == mExchange::Coinbase) {
          system("test -n \"`/bin/pidof stunnel`\" && kill -9 `/bin/pidof stunnel`");
          system("stunnel etc/K-stunnel.conf");
          json k = FN::wJet(string(gw->http).append("/products/").append(gw->symbol));
          gw->minTick = stod(k.value("quote_increment", "0"));
          gw->minSize = stod(k.value("base_min_size", "0"));
        } else if (e == mExchange::HitBtc) {
          json k = FN::wJet(string(gw->http).append("/api/1/public/symbols"));
          if (k.find("symbols") != k.end())
            for (json::iterator it = k["symbols"].begin(); it != k["symbols"].end(); ++it)
              if (it->value("symbol", "") == gw->symbol) {
                gw->minTick = stod(it->value("step", "0"));
                gw->minSize = stod(it->value("lot", "0"));
                break;
              }
        } else if (e == mExchange::Bitfinex) {
          json k = FN::wJet(string(gw->http).append("/pubticker/").append(gw->symbol));
          if (k.find("last_price") != k.end()) {
            stringstream price_;
            price_ << scientific << stod(k.value("last_price", "0"));
            string _price_ = price_.str();
            for (string::iterator it=_price_.begin(); it!=_price_.end();)
              if (*it == '+' or *it == '-') break; else it = _price_.erase(it);
            stringstream os(string("1e").append(to_string(stod(_price_)-4)));
            os >> gw->minTick;
            gw->minSize = 0.01;
          }
        } else if (e == mExchange::OkCoin) {
          gw->minTick = "btc" == gw->symbol.substr(0,3) ? 0.01 : 0.001;
          gw->minSize = 0.01;
        } else if (e == mExchange::Korbit) {
          json k = FN::wJet(string(gw->http).append("/constants"));
          if (k.find(gw->symbol.substr(0,3).append("TickSize")) != k.end()) {
            gw->minTick = k.value(gw->symbol.substr(0,3).append("TickSize"), 0.0);
            gw->minSize = 0.015;
          }
        } else if (e == mExchange::Poloniex) {
          json k = FN::wJet(string(gw->http).append("/public?command=returnTicker"));
          if (k.find(gw->symbol) != k.end()) {
            istringstream os(string("1e-").append(to_string(6-k[gw->symbol]["last"].get<string>().find("."))));
            os >> gw->minTick;
            gw->minSize = 0.01;
          }
        } else if (e == mExchange::Null) {
          gw->minTick = 0.01;
          gw->minSize = 0.01;
        }
        if (!gw->minTick) { cout << FN::uiT() << "CF" << RRED << " Errrror:" << BRED << " Unable to fetch data from " << argExchange << " symbol \"" << gw->symbol << "\"." << endl; exit(1); }
        else { cout << FN::uiT() << "GW " << RWHITE << argExchange << " allows client IP." << endl; }
        cout << FN::uiT() << "GW " << RWHITE << setprecision(8) << fixed << argExchange << ":" << endl
          << "- autoBot: " << (autoStart() == mConnectivity::Connected ? "yes" : "no") << endl
          << "- pair: " << gw->symbol << endl
          << "- minTick: " << gw->minTick << endl
          << "- minSize: " << gw->minSize << endl
          << "- makeFee: " << gw->makeFee << endl
          << "- takeFee: " << gw->takeFee << endl;
      };
  };
}

#endif