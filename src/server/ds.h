#ifndef K_DS_H_
#define K_DS_H_

#define mClock  unsigned long long
#define mPrice  double
#define mAmount double
#define mRandId string
#define mCoinId string

#define Tclock  chrono::system_clock::now()
#define Tstamp  chrono::duration_cast<chrono::milliseconds>( \
                  Tclock.time_since_epoch()                  \
                ).count()

#define numsAz "0123456789"                 \
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
               "abcdefghijklmnopqrstuvwxyz"

#ifndef M_PI_2
#define M_PI_2 1.5707963267948965579989817342720925807952880859375
#endif

#define  public_ref public
#define private_ref private

#define TRUEONCE(k) (k ? !(k = !k) : k)

#define ROUND(k, x) (round((k) / x) * x)

namespace K {
  enum class mConnectivity: unsigned int {
    Disconnected, Connected
  };
  enum class mStatus: unsigned int {
    Waiting, Working, Terminated
  };
  enum class mSide: unsigned int {
    Bid, Ask, Both
  };
  enum class mTimeInForce: unsigned int {
    IOC, FOK, GTC
  };
  enum class mOrderType: unsigned int {
    Limit, Market
  };
  enum class mQuotingMode: unsigned int {
    Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth
  };
  enum class mQuotingSafety: unsigned int {
    Off, PingPong, Boomerang, AK47
  };
  enum class mQuoteState: unsigned int {
    Disconnected,  Live,             DisabledQuotes,
    MissingData,   UnknownHeld,      WidthMustBeSmaller,
    TBPHeld,       MaxTradesSeconds, WaitingPing,
    DepletedFunds, Crossed,
    UpTrendHeld,   DownTrendHeld
  };
  enum class mFairValueModel: unsigned int {
    BBO, wBBO, rwBBO
  };
  enum class mAutoPositionMode: unsigned int {
    Manual, EWMA_LS, EWMA_LMS, EWMA_4
  };
  enum class mPDivMode: unsigned int {
    Manual, Linear, Sine, SQRT, Switch
  };
  enum class mAPR: unsigned int {
    Off, Size, SizeWidth
  };
  enum class mSOP: unsigned int {
    Off, Trades, Size, TradesSize
  };
  enum class mSTDEV: unsigned int {
    Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff
  };
  enum class mPingAt: unsigned int {
    BothSides,    BidSide,         AskSide,
    DepletedSide, DepletedBidSide, DepletedAskSide,
    StopPings
  };
  enum class mPongAt: unsigned int {
    ShortPingFair,       AveragePingFair,       LongPingFair,
    ShortPingAggressive, AveragePingAggressive, LongPingAggressive
  };

  enum class mHotkey: unsigned int {
    ESC = 27,
     Q  = 81,
     q  = 113
  };

  enum class mPortal: unsigned char {
    Hello = '=',
    Kiss  = '-'
  };
  enum class mMatter: unsigned char {
    FairValue            = 'a', Quote                = 'b', ActiveSubscription = 'c', Connectivity       = 'd',
    MarketData           = 'e', QuotingParameters    = 'f', SafetySettings     = 'g', Product            = 'h',
    OrderStatusReports   = 'i', ProductAdvertisement = 'j', ApplicationState   = 'k', EWMAStats          = 'l',
    STDEVStats           = 'm', Position             = 'n', Profit             = 'o', SubmitNewOrder     = 'p',
    CancelOrder          = 'q', MarketTrade          = 'r', Trades             = 's', ExternalValuation  = 't',
    QuoteStatus          = 'u', TargetBasePosition   = 'v', TradeSafetyValue   = 'w', CancelAllOrders    = 'x',
    CleanAllClosedTrades = 'y', CleanAllTrades       = 'z', CleanTrade         = 'A',
    WalletChart          = 'C', MarketChart          = 'D', Notepad            = 'E',
                                MarketDataLongTerm   = 'H'
  };

  static          bool operator ! (mConnectivity k_)                   { return !(unsigned int)k_; };
  static mConnectivity operator * (mConnectivity _k, mConnectivity k_) { return (mConnectivity)((unsigned int)_k * (unsigned int)k_); };

  static string strX(const double &d, const unsigned int &X) { stringstream ss; ss << setprecision(X) << fixed << d; return ss.str(); };
  static string str8(const double &d) { return strX(d, 8); };
  static string strL(string s) { transform(s.begin(), s.end(), s.begin(), ::tolower); return s; };
  static string strU(string s) { transform(s.begin(), s.end(), s.begin(), ::toupper); return s; };

  static char RBLACK[] = "\033[0;30m", RRED[]    = "\033[0;31m", RGREEN[] = "\033[0;32m", RYELLOW[] = "\033[0;33m",
              RBLUE[]  = "\033[0;34m", RPURPLE[] = "\033[0;35m", RCYAN[]  = "\033[0;36m", RWHITE[]  = "\033[0;37m",
              BBLACK[] = "\033[1;30m", BRED[]    = "\033[1;31m", BGREEN[] = "\033[1;32m", BYELLOW[] = "\033[1;33m",
              BBLUE[]  = "\033[1;34m", BPURPLE[] = "\033[1;35m", BCYAN[]  = "\033[1;36m", BWHITE[]  = "\033[1;37m",
              RRESET[] = "\033[0m";

  static struct mArgs {
        int port        = 3000,   colors      = 0, debug        = 0,
            debugSecret = 0,      debugEvents = 0, debugOrders  = 0,
            debugQuotes = 0,      debugWallet = 0, debugSqlite  = 0,
            headless    = 0,      dustybot    = 0, lifetime     = 0,
            autobot     = 0,      naked       = 0, free         = 0,
            ignoreSun   = 0,      ignoreMoon  = 0, testChamber  = 0,
            maxAdmins   = 7,      latency     = 0, maxLevels   = 321,
            withoutSSL  = 0;
    mAmount maxWallet   = 0;
     string title       = "K.sh", matryoshka  = "https://www.example.com/",
            user        = "NULL", pass        = "NULL",
            exchange    = "NULL", currency    = "NULL",
            apikey      = "NULL", secret      = "NULL",
            username    = "NULL", passphrase  = "NULL",
            http        = "NULL", wss         = "NULL",
            database    = "",     diskdata    = "",
            whitelist   = "",
            base        = "",     quote       = "";
    const char *inet = nullptr;
    const string main(int argc, char** argv) {
      static const struct option opts[] = {
        {"help",         no_argument,       0,               'h'},
        {"colors",       no_argument,       &colors,           1},
        {"debug",        no_argument,       &debug,            1},
        {"debug-secret", no_argument,       &debugSecret,      1},
        {"debug-events", no_argument,       &debugEvents,      1},
        {"debug-orders", no_argument,       &debugOrders,      1},
        {"debug-quotes", no_argument,       &debugQuotes,      1},
        {"debug-wallet", no_argument,       &debugWallet,      1},
        {"debug-sqlite", no_argument,       &debugSqlite,      1},
        {"without-ssl",  no_argument,       &withoutSSL,       1},
        {"ignore-sun",   no_argument,       &ignoreSun,        2},
        {"ignore-moon",  no_argument,       &ignoreMoon,       1},
        {"headless",     no_argument,       &headless,         1},
        {"naked",        no_argument,       &naked,            1},
        {"autobot",      no_argument,       &autobot,          1},
        {"dustybot",     no_argument,       &dustybot,         1},
        {"latency",      no_argument,       &latency,          1},
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
        {"free-version", no_argument,       &free,             1},
        {"version",      no_argument,       0,               'v'},
        {0,              0,                 0,                 0}
      };
      int k = 0;
      while (++k)
        switch (k = getopt_long(argc, argv, "hvd:i:k:x:K:L:M:T:W:", opts, NULL)) {
          case -1 :
          case  0 : break;
          case 'x': testChamber  = stoi(optarg);   break;
          case 'M': maxLevels    = stoi(optarg);   break;
          case 'C': maxAdmins    = stoi(optarg);   break;
          case 'T': lifetime     = stoi(optarg);   break;
          case 999: port         = stoi(optarg);   break;
          case 998: user         = string(optarg); break;
          case 997: pass         = string(optarg); break;
          case 996: exchange     = string(optarg); break;
          case 995: currency     = string(optarg); break;
          case 994: apikey       = string(optarg); break;
          case 993: secret       = string(optarg); break;
          case 992: passphrase   = string(optarg); break;
          case 991: username     = string(optarg); break;
          case 990: http         = string(optarg); break;
          case 989: wss          = string(optarg); break;
          case 'd': database     = string(optarg); break;
          case 'k': matryoshka   = string(optarg); break;
          case 'K': title        = string(optarg); break;
          case 'L': whitelist    = string(optarg); break;
          case 'i': inet         = strdup(optarg); break;
          case 'W': maxWallet    = stod(optarg);   break;
          case 'h': {
            const vector<string> stamp = {
              " \\__/  \\__/ ", " | (   .    ", "   __   \\__/  ",
              " /  \\__/  \\ ", " |  `.  `.  ", "  /  \\        ",
              " \\__/  \\__/ ", " |    )   ) ", "  \\__/   __   ",
              " /  \\__/  \\ ", " |  ,'  ,'  ", "        /  \\  "
            };
                  unsigned int y = Tstamp;
            const unsigned int x = !(y % 2)
                                 + !(y % 21);
            cout
              << RGREEN << PERMISSIVE_analpaper_SOFTWARE_LICENSE << '\n'
              << RGREEN << "  questions: " << RYELLOW << "https://earn.com/analpaper/" << '\n'
              << BGREEN << "K" << RGREEN << " bugkiller: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/issues/new" << '\n'
              << RGREEN << "  downloads: " << RYELLOW << "ssh://git@github.com/ctubio/Krypto-trading-bot" << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << "Usage:" << BYELLOW << " ./K.sh [arguments]" << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << "[arguments]:" << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-h, --help                - show this help and quit." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --latency             - check current HTTP latency (not from WS) and quit." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --autobot             - automatically start trading on boot." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --dustybot            - do not automatically cancel all orders on exit." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --naked               - do not display CLI, print output to stdout instead." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --headless            - do not listen for UI connections," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            all other UI related arguments will be ignored." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-C, --client-limit=NUMBER - set NUMBER of maximum concurrent UI connections." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --without-ssl         - do not use HTTPS for UI connections (use HTTP only)." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-L, --whitelist=IP        - set IP or csv of IPs to allow UI connections," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            alien IPs will get a zip-bomb instead." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --port=NUMBER         - set NUMBER of an open port to listen for UI connections." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --user=WORD           - set allowed WORD as username for UI connections," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --pass=WORD           - set allowed WORD as password for UI connections," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --exchange=NAME       - set exchange NAME for trading, mandatory one of:" << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            'COINBASE', 'BITFINEX',  'BITFINEX_MARGIN', 'HITBTC'," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            'OKCOIN', 'OKEX', 'KORBIT', 'POLONIEX' or 'NULL'." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --currency=PAIRS      - set currency pairs for trading (use format" << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            with '/' separator, like 'BTC/EUR')." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --apikey=WORD         - set (never share!) WORD as api key for trading," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --secret=WORD         - set (never share!) WORD as api secret for trading," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --passphrase=WORD     - set (never share!) WORD as api passphrase for trading," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --username=WORD       - set (never share!) WORD as api username for trading," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory but may be 'NULL'." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --http=URL            - set URL of api HTTP/S endpoint for trading," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --wss=URL             - set URL of api SECURE WS endpoint for trading," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            mandatory." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-d, --database=PATH       - set alternative PATH to database filename," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            default PATH is '/data/db/K.*.*.*.db'," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            any route to a filename is valid," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            or use ':memory:' (see sqlite.org/inmemorydb.html)." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --wallet-limit=AMOUNT - set AMOUNT in base currency to limit the balance," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            otherwise the full available balance can be used." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --market-limit=NUMBER - set NUMBER of maximum price levels for the orderbook," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            default NUMBER is '321' and the minimum is '15'." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            locked bots smells like '--market-limit=3' spirit." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-T, --lifetime=NUMBER     - set NUMBER of minimum milliseconds to keep orders open," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            otherwise open orders can be replaced anytime required." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug-secret        - print (never share!) secret inputs and outputs." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug-sqlite        - print detailed output about database queries." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug-events        - print detailed output about event handlers." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug-orders        - print detailed output about exchange messages." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug-quotes        - print detailed output about quoting engine." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug-wallet        - print detailed output about target base position." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --debug               - print detailed output about all the (previous) things!" << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --colors              - print highlighted output." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --ignore-sun          - do not switch UI to light theme on daylight." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --ignore-moon         - do not switch UI to dark theme on moonlight." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-k, --matryoshka=URL      - set Matryoshka link URL of the next UI." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-K, --title=WORD          - set WORD as UI title to identify different bots." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-x, --test-chamber=NUMBER - set release candidate NUMBER to test (ask your developer)." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-i, --interface=IP        - set IP to bind as outgoing network interface," << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "                            default IP is the system default network interface." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "    --free-version        - work with all market levels and enable the slow XMR miner." << '\n'
              << BWHITE << stamp.at(((--y%4)*3)+x) << RWHITE << "-v, --version             - show current build version and quit." << '\n'
              << RGREEN << "  more help: " << RYELLOW << "https://github.com/ctubio/Krypto-trading-bot/blob/master/MANUAL.md" << '\n'
              << BGREEN << "K" << RGREEN << " questions: " << RYELLOW << "irc://irc.freenode.net:6667/#tradingBot" << '\n'
              << RGREEN << "  home page: " << RYELLOW << "https://ca.rles-tub.io./trades" << '\n'
              << RRESET;
          }
          case '?':
          case 'v': EXIT(EXIT_SUCCESS);
          default : abort();
        }
      if (optind < argc) {
        string argerr = "Invalid argument option:";
        while(optind < argc) argerr += string(" ") + argv[optind++];
        return argerr;
      }
      if (currency.find("/") == string::npos or currency.length() < 3)
        return "Invalid currency pair; must be in the format of BASE/QUOTE, like BTC/EUR";
      if (exchange.empty())
        return "Undefined exchange; the config file may have errors (there are extra spaces or double defined variables?)";
      tidy();
      return "";
    };
    private:
      void tidy() {
        exchange = strU(exchange);
        currency = strU(currency);
        base  = currency.substr(0, currency.find("/"));
        quote = currency.substr(1+ currency.find("/"));
        if (debug)
          debugSecret =
          debugEvents =
          debugOrders =
          debugQuotes =
          debugWallet =
          debugSqlite = debug;
        if (!colors)
          RBLACK[0] = RRED[0]    = RGREEN[0] = RYELLOW[0] =
          RBLUE[0]  = RPURPLE[0] = RCYAN[0]  = RWHITE[0]  =
          BBLACK[0] = BRED[0]    = BGREEN[0] = BYELLOW[0] =
          BBLUE[0]  = BPURPLE[0] = BCYAN[0]  = BWHITE[0]  = RRESET[0] = 0;
        if (database.empty() or database == ":memory:")
          (database == ":memory:"
            ? diskdata
            : database
          ) = "/data/db/K"
            + ('.' + exchange)
            +  '.' + string(currency).replace(currency.find("/"), 1, ".")
            +  '.' + "db";
        maxLevels = max(15, maxLevels);
        if (user == "NULL") user.clear();
        if (pass == "NULL") pass.clear();
        if (ignoreSun and ignoreMoon) ignoreMoon = 0;
        if (latency) naked = headless = 1;
        if (headless) port = 0;
        else if (!port or !maxAdmins) headless = 1;
        if (debugSecret
          or debugEvents
          or debugOrders
          or debugQuotes
          or debugSqlite
        ) naked = 1;
#ifdef _WIN32
        naked = 1;
#endif
      };
  } args;

  struct mAbout {
    virtual const mMatter about() const = 0;
  };
  struct mBlob: virtual public mAbout {
    virtual const json blob() const = 0;
  };

  struct mFromClient: virtual public mAbout {
    virtual void kiss(json *const j) {};
  };

  struct mToScreen {
    function<void(const string&, const string&)> print
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen print?"); }
#endif
    ;
    function<void(const string&, const string&, const string&)> focus
#ifndef NDEBUG
    = [](const string &prefix, const string &reason, const string &highlight) { WARN("Y U NO catch screen focus?"); }
#endif
    ;
    function<void(const string&, const string&)> warn
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen warn?"); }
#endif
    ;
    function<const int(const string&, const string&)> error
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen error?"); return 0; }
#endif
    ;
    function<void()> refresh
#ifndef NDEBUG
    = []() { WARN("Y U NO catch screen refresh?"); }
#endif
    ;
  };

  struct mToClient: public mBlob,
                    public mFromClient {
    function<void()> send
#ifndef NDEBUG
    = []() { WARN("Y U NO catch client send?"); }
#endif
    ;
    virtual const json hello() {
      return { blob() };
    };
    virtual const bool realtime() const {
      return true;
    };
  };
  template <typename mData> struct mJsonToClient: public mToClient {
    virtual const bool send() {
      if ((send_asap() or send_soon())
        and (send_same_blob() or diff_blob())
      ) {
        send_now();
        return true;
      }
      return false;
    };
    virtual const json blob() const {
      return *(mData*)this;
    };
    protected:
      mClock send_last_Tstamp = 0;
      string send_last_blob;
      virtual const bool send_same_blob() const {
        return true;
      };
      const bool diff_blob() {
        const string last_blob = send_last_blob;
        return (send_last_blob = blob().dump()) != last_blob;
      };
      virtual const bool send_asap() const {
        return true;
      };
      const bool send_soon(const int &delay = 0) {
        const mClock now = Tstamp;
        if (send_last_Tstamp + max(369, delay) > now)
          return false;
        send_last_Tstamp = now;
        return true;
      };
      void send_now() const {
        if (mToClient::send)
          mToClient::send();
      };
  };

  struct mFromDb: public mBlob {
    function<void()> push;
    virtual const bool pull(const json &j) = 0;
    virtual const string increment() const { return "NULL"; };
    virtual const double limit()     const { return 0; };
    virtual const mClock lifetime()  const { return 0; };
    virtual const string explain()   const = 0;
    virtual       string explainOK() const = 0;
    virtual       string explainKO() const { return ""; };
    const string explanation(const bool &loaded) const {
      string msg = loaded
        ? explainOK()
        : explainKO();
      size_t token = msg.find("%");
      if (token != string::npos)
        msg.replace(token, 1, explain());
      return msg;
    };
  };
  template <typename mData> struct mStructFromDb: public mFromDb {
    virtual const json blob() const {
      return *(mData*)this;
    };
    virtual const bool pull(const json &j) {
      if (j.empty()) return false;
      from_json(j.at(0), *(mData*)this);
      return true;
    };
    virtual string explainOK() const {
      return "loaded last % OK";
    };
  };
  template <typename mData> struct mVectorFromDb: public mFromDb {
    vector<mData> rows;
    typedef typename vector<mData>::reference                           reference;
    typedef typename vector<mData>::const_reference               const_reference;
    typedef typename vector<mData>::iterator                             iterator;
    typedef typename vector<mData>::const_iterator                 const_iterator;
    typedef typename vector<mData>::reverse_iterator             reverse_iterator;
    typedef typename vector<mData>::const_reverse_iterator const_reverse_iterator;
    iterator                 begin()       noexcept { return rows.begin(); };
    const_iterator           begin() const noexcept { return rows.begin(); };
    const_iterator          cbegin() const noexcept { return rows.cbegin(); };
    iterator                   end()       noexcept { return rows.end(); };
    const_iterator             end() const noexcept { return rows.end(); };
    reverse_iterator        rbegin()       noexcept { return rows.rbegin(); };
    const_reverse_iterator crbegin() const noexcept { return rows.crbegin(); };
    reverse_iterator          rend()       noexcept { return rows.rend(); };
    bool                     empty() const noexcept { return rows.empty(); };
    size_t                    size() const noexcept { return rows.size(); };
    reference                front()                { return rows.front(); };
    const_reference          front() const          { return rows.front(); };
    reference                 back()                { return rows.back(); };
    const_reference           back() const          { return rows.back(); };
    reference                   at(size_t n)        { return rows.at(n); };
    const_reference             at(size_t n) const  { return rows.at(n); };
    virtual void erase() {
      if (size() > limit())
        rows.erase(begin(), end() - limit());
    };
    virtual void push_back(const mData &row) {
      rows.push_back(row);
      push();
      erase();
    };
    virtual const bool pull(const json &j) {
      for (const json &it : j)
        rows.push_back(it);
      return !empty();
    };
    virtual const json blob() const {
      return back();
    };
    virtual const string explain() const {
      return to_string(size());
    };
  };

  static struct mQuotingParams: public mStructFromDb<mQuotingParams>,
                                public mJsonToClient<mQuotingParams> {
    mPrice            widthPing                       = 2.0;
    double            widthPingPercentage             = 0.25;
    mPrice            widthPong                       = 2.0;
    double            widthPongPercentage             = 0.25;
    bool              widthPercentage                 = false;
    bool              bestWidth                       = true;
    mAmount           buySize                         = 0.02;
    unsigned int      buySizePercentage               = 7;
    bool              buySizeMax                      = false;
    mAmount           sellSize                        = 0.01;
    unsigned int      sellSizePercentage              = 7;
    bool              sellSizeMax                     = false;
    mPingAt           pingAt                          = mPingAt::BothSides;
    mPongAt           pongAt                          = mPongAt::ShortPingFair;
    mQuotingMode      mode                            = mQuotingMode::Top;
    mQuotingSafety    safety                          = mQuotingSafety::Boomerang;
    unsigned int      bullets                         = 2;
    mPrice            range                           = 0.5;
    double            rangePercentage                 = 5.0;
    mFairValueModel   fvModel                         = mFairValueModel::BBO;
    mAmount           targetBasePosition              = 1.0;
    unsigned int      targetBasePositionPercentage    = 50;
    mAmount           positionDivergence              = 0.9;
    mAmount           positionDivergenceMin           = 0.4;
    unsigned int      positionDivergencePercentage    = 21;
    unsigned int      positionDivergencePercentageMin = 10;
    mPDivMode         positionDivergenceMode          = mPDivMode::Manual;
    bool              percentageValues                = false;
    mAutoPositionMode autoPositionMode                = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing   = mAPR::Off;
    mSOP              superTrades                     = mSOP::Off;
    double            tradesPerMinute                 = 0.9;
    unsigned int      tradeRateSeconds                = 3;
    bool              protectionEwmaWidthPing         = false;
    bool              protectionEwmaQuotePrice        = true;
    unsigned int      protectionEwmaPeriods           = 200;
    mSTDEV            quotingStdevProtection          = mSTDEV::Off;
    bool              quotingStdevBollingerBands      = false;
    double            quotingStdevProtectionFactor    = 1.0;
    unsigned int      quotingStdevProtectionPeriods   = 1200;
    double            ewmaSensiblityPercentage        = 0.5;
    bool              quotingEwmaTrendProtection      = false;
    double            quotingEwmaTrendThreshold       = 2.0;
    unsigned int      veryLongEwmaPeriods             = 400;
    unsigned int      longEwmaPeriods                 = 200;
    unsigned int      mediumEwmaPeriods               = 100;
    unsigned int      shortEwmaPeriods                = 50;
    unsigned int      extraShortEwmaPeriods           = 12;
    unsigned int      ultraShortEwmaPeriods           = 3;
    double            aprMultiplier                   = 2;
    double            sopWidthMultiplier              = 2;
    double            sopSizeMultiplier               = 2;
    double            sopTradesMultiplier             = 2;
    bool              cancelOrdersAuto                = false;
    double            cleanPongsAuto                  = 0.0;
    double            profitHourInterval              = 0.5;
    bool              audio                           = false;
    unsigned int      delayUI                         = 7;
    bool              _diffVLEP                       = false;
    bool              _diffLEP                        = false;
    bool              _diffMEP                        = false;
    bool              _diffSEP                        = false;
    bool              _diffXSEP                       = false;
    bool              _diffUEP                        = false;
    void tidy() {
      if (mode == mQuotingMode::Depth) widthPercentage = false;
    };
    void diff(const mQuotingParams &prev) {
      _diffVLEP = prev.veryLongEwmaPeriods != veryLongEwmaPeriods;
      _diffLEP = prev.longEwmaPeriods != longEwmaPeriods;
      _diffMEP = prev.mediumEwmaPeriods != mediumEwmaPeriods;
      _diffSEP = prev.shortEwmaPeriods != shortEwmaPeriods;
      _diffXSEP = prev.extraShortEwmaPeriods != extraShortEwmaPeriods;
      _diffUEP = prev.ultraShortEwmaPeriods != ultraShortEwmaPeriods;
    };
    void kiss(json *const j) {
      mQuotingParams prev = *this; // just need to copy the 6 prev.* vars above, noob
      from_json(*j, *this);
      diff(prev);
      push();
      send();
    };
    const mMatter about() const {
      return mMatter::QuotingParameters;
    };
    const string explain() const {
      return "Quoting Parameters";
    };
    string explainKO() const {
      return "using default values for %";
    };
  } qp;
  static void to_json(json &j, const mQuotingParams &k) {
    j = {
      {                      "widthPing", k.widthPing                      },
      {            "widthPingPercentage", k.widthPingPercentage            },
      {                      "widthPong", k.widthPong                      },
      {            "widthPongPercentage", k.widthPongPercentage            },
      {                "widthPercentage", k.widthPercentage                },
      {                      "bestWidth", k.bestWidth                      },
      {                        "buySize", k.buySize                        },
      {              "buySizePercentage", k.buySizePercentage              },
      {                     "buySizeMax", k.buySizeMax                     },
      {                       "sellSize", k.sellSize                       },
      {             "sellSizePercentage", k.sellSizePercentage             },
      {                    "sellSizeMax", k.sellSizeMax                    },
      {                         "pingAt", k.pingAt                         },
      {                         "pongAt", k.pongAt                         },
      {                           "mode", k.mode                           },
      {                         "safety", k.safety                         },
      {                        "bullets", k.bullets                        },
      {                          "range", k.range                          },
      {                "rangePercentage", k.rangePercentage                },
      {                        "fvModel", k.fvModel                        },
      {             "targetBasePosition", k.targetBasePosition             },
      {   "targetBasePositionPercentage", k.targetBasePositionPercentage   },
      {             "positionDivergence", k.positionDivergence             },
      {   "positionDivergencePercentage", k.positionDivergencePercentage   },
      {          "positionDivergenceMin", k.positionDivergenceMin          },
      {"positionDivergencePercentageMin", k.positionDivergencePercentageMin},
      {         "positionDivergenceMode", k.positionDivergenceMode         },
      {               "percentageValues", k.percentageValues               },
      {               "autoPositionMode", k.autoPositionMode               },
      {  "aggressivePositionRebalancing", k.aggressivePositionRebalancing  },
      {                    "superTrades", k.superTrades                    },
      {                "tradesPerMinute", k.tradesPerMinute                },
      {               "tradeRateSeconds", k.tradeRateSeconds               },
      {        "protectionEwmaWidthPing", k.protectionEwmaWidthPing        },
      {       "protectionEwmaQuotePrice", k.protectionEwmaQuotePrice       },
      {          "protectionEwmaPeriods", k.protectionEwmaPeriods          },
      {         "quotingStdevProtection", k.quotingStdevProtection         },
      {     "quotingStdevBollingerBands", k.quotingStdevBollingerBands     },
      {   "quotingStdevProtectionFactor", k.quotingStdevProtectionFactor   },
      {  "quotingStdevProtectionPeriods", k.quotingStdevProtectionPeriods  },
      {       "ewmaSensiblityPercentage", k.ewmaSensiblityPercentage       },
      {     "quotingEwmaTrendProtection", k.quotingEwmaTrendProtection     },
      {      "quotingEwmaTrendThreshold", k.quotingEwmaTrendThreshold      },
      {            "veryLongEwmaPeriods", k.veryLongEwmaPeriods            },
      {                "longEwmaPeriods", k.longEwmaPeriods                },
      {              "mediumEwmaPeriods", k.mediumEwmaPeriods              },
      {               "shortEwmaPeriods", k.shortEwmaPeriods               },
      {          "extraShortEwmaPeriods", k.extraShortEwmaPeriods          },
      {          "ultraShortEwmaPeriods", k.ultraShortEwmaPeriods          },
      {                  "aprMultiplier", k.aprMultiplier                  },
      {             "sopWidthMultiplier", k.sopWidthMultiplier             },
      {              "sopSizeMultiplier", k.sopSizeMultiplier              },
      {            "sopTradesMultiplier", k.sopTradesMultiplier            },
      {               "cancelOrdersAuto", k.cancelOrdersAuto               },
      {                 "cleanPongsAuto", k.cleanPongsAuto                 },
      {             "profitHourInterval", k.profitHourInterval             },
      {                          "audio", k.audio                          },
      {                        "delayUI", k.delayUI                        }
    };
  };
  static void from_json(const json &j, mQuotingParams &k) {
    k.widthPing                       = fmax(1e-8,            j.value("widthPing", k.widthPing));
    k.widthPingPercentage             = fmin(1e+2, fmax(1e-3, j.value("widthPingPercentage", k.widthPingPercentage)));
    k.widthPong                       = fmax(1e-8,            j.value("widthPong", k.widthPong));
    k.widthPongPercentage             = fmin(1e+2, fmax(1e-3, j.value("widthPongPercentage", k.widthPongPercentage)));
    k.widthPercentage                 =                       j.value("widthPercentage", k.widthPercentage);
    k.bestWidth                       =                       j.value("bestWidth", k.bestWidth);
    k.buySize                         = fmax(1e-8,            j.value("buySize", k.buySize));
    k.buySizePercentage               = fmin(1e+2, fmax(1,    j.value("buySizePercentage", k.buySizePercentage)));
    k.buySizeMax                      =                       j.value("buySizeMax", k.buySizeMax);
    k.sellSize                        = fmax(1e-8,            j.value("sellSize", k.sellSize));
    k.sellSizePercentage              = fmin(1e+2, fmax(1,    j.value("sellSizePercentage", k.sellSizePercentage)));
    k.sellSizeMax                     =                       j.value("sellSizeMax", k.sellSizeMax);
    k.pingAt                          =                       j.value("pingAt", k.pingAt);
    k.pongAt                          =                       j.value("pongAt", k.pongAt);
    k.mode                            =                       j.value("mode", k.mode);
    k.safety                          =                       j.value("safety", k.safety);
    k.bullets                         = fmin(10, fmax(1,      j.value("bullets", k.bullets)));
    k.range                           =                       j.value("range", k.range);
    k.rangePercentage                 = fmin(1e+2, fmax(1e-3, j.value("rangePercentage", k.rangePercentage)));
    k.fvModel                         =                       j.value("fvModel", k.fvModel);
    k.targetBasePosition              =                       j.value("targetBasePosition", k.targetBasePosition);
    k.targetBasePositionPercentage    = fmin(1e+2, fmax(0,    j.value("targetBasePositionPercentage", k.targetBasePositionPercentage)));
    k.positionDivergenceMin           =                       j.value("positionDivergenceMin", k.positionDivergenceMin);
    k.positionDivergenceMode          =                       j.value("positionDivergenceMode", k.positionDivergenceMode);
    k.positionDivergence              =                       j.value("positionDivergence", k.positionDivergence);
    k.positionDivergencePercentage    = fmin(1e+2, fmax(0,    j.value("positionDivergencePercentage", k.positionDivergencePercentage)));
    k.positionDivergencePercentageMin = fmin(1e+2, fmax(0,    j.value("positionDivergencePercentageMin", k.positionDivergencePercentageMin)));
    k.percentageValues                =                       j.value("percentageValues", k.percentageValues);
    k.autoPositionMode                =                       j.value("autoPositionMode", k.autoPositionMode);
    k.aggressivePositionRebalancing   =                       j.value("aggressivePositionRebalancing", k.aggressivePositionRebalancing);
    k.superTrades                     =                       j.value("superTrades", k.superTrades);
    k.tradesPerMinute                 =                       j.value("tradesPerMinute", k.tradesPerMinute);
    k.tradeRateSeconds                = fmax(0,               j.value("tradeRateSeconds", k.tradeRateSeconds));
    k.protectionEwmaWidthPing         =                       j.value("protectionEwmaWidthPing", k.protectionEwmaWidthPing);
    k.protectionEwmaQuotePrice        =                       j.value("protectionEwmaQuotePrice", k.protectionEwmaQuotePrice);
    k.protectionEwmaPeriods           = fmax(1,               j.value("protectionEwmaPeriods", k.protectionEwmaPeriods));
    k.quotingStdevProtection          =                       j.value("quotingStdevProtection", k.quotingStdevProtection);
    k.quotingStdevBollingerBands      =                       j.value("quotingStdevBollingerBands", k.quotingStdevBollingerBands);
    k.quotingStdevProtectionFactor    =                       j.value("quotingStdevProtectionFactor", k.quotingStdevProtectionFactor);
    k.quotingStdevProtectionPeriods   = fmax(1,               j.value("quotingStdevProtectionPeriods", k.quotingStdevProtectionPeriods));
    k.ewmaSensiblityPercentage        =                       j.value("ewmaSensiblityPercentage", k.ewmaSensiblityPercentage);
    k.quotingEwmaTrendProtection      =                       j.value("quotingEwmaTrendProtection", k.quotingEwmaTrendProtection);
    k.quotingEwmaTrendThreshold       =                       j.value("quotingEwmaTrendThreshold", k.quotingEwmaTrendThreshold);
    k.veryLongEwmaPeriods             = fmax(1,               j.value("veryLongEwmaPeriods", k.veryLongEwmaPeriods));
    k.longEwmaPeriods                 = fmax(1,               j.value("longEwmaPeriods", k.longEwmaPeriods));
    k.mediumEwmaPeriods               = fmax(1,               j.value("mediumEwmaPeriods", k.mediumEwmaPeriods));
    k.shortEwmaPeriods                = fmax(1,               j.value("shortEwmaPeriods", k.shortEwmaPeriods));
    k.extraShortEwmaPeriods           = fmax(1,               j.value("extraShortEwmaPeriods", k.extraShortEwmaPeriods));
    k.ultraShortEwmaPeriods           = fmax(1,               j.value("ultraShortEwmaPeriods", k.ultraShortEwmaPeriods));
    k.aprMultiplier                   =                       j.value("aprMultiplier", k.aprMultiplier);
    k.sopWidthMultiplier              =                       j.value("sopWidthMultiplier", k.sopWidthMultiplier);
    k.sopSizeMultiplier               =                       j.value("sopSizeMultiplier", k.sopSizeMultiplier);
    k.sopTradesMultiplier             =                       j.value("sopTradesMultiplier", k.sopTradesMultiplier);
    k.cancelOrdersAuto                =                       j.value("cancelOrdersAuto", k.cancelOrdersAuto);
    k.cleanPongsAuto                  =                       j.value("cleanPongsAuto", k.cleanPongsAuto);
    k.profitHourInterval              =                       j.value("profitHourInterval", k.profitHourInterval);
    k.audio                           =                       j.value("audio", k.audio);
    k.delayUI                         = fmax(0,               j.value("delayUI", k.delayUI));
    k.tidy();
  };

  struct mOrder {
         mRandId orderId,
                 exchangeId;
         mStatus orderStatus    = mStatus::Waiting;
           mSide side           = (mSide)0;
          mPrice price          = 0;
         mAmount quantity       = 0,
                 tradeQuantity  = 0;
      mOrderType type           = mOrderType::Limit;
    mTimeInForce timeInForce    = mTimeInForce::GTC;
            bool isPong         = false,
                 preferPostOnly = true;
          mClock time           = 0,
                 latency        = 0;
    mOrder()
    {};
    mOrder(const mRandId &o, const mSide &S, const mPrice &p, const mAmount &q, const bool &i)
      : orderId(o)
      , side(S)
      , price(p)
      , quantity(q)
      , isPong(i)
      , time(Tstamp)
    {};
    mOrder(const mRandId &o, const mRandId &e, const mStatus &s, const mPrice &p, const mAmount &q, const mAmount &Q)
      : orderId(o)
      , exchangeId(e)
      , orderStatus(s)
      , price(p)
      , quantity(q)
      , tradeQuantity(Q)
      , time(Tstamp)
    {};
    static void update(const mOrder &raw, mOrder *const order) {
      if (!order) return;
      if ((                        order->orderStatus = raw.orderStatus
      ) == mStatus::Working)       order->latency     = Tstamp - order->time;
                                   order->time        = raw.time;
      if (!raw.exchangeId.empty()) order->exchangeId  = raw.exchangeId;
      if (raw.price)               order->price       = raw.price;
      if (raw.quantity)            order->quantity    = raw.quantity;
    };
    static const bool replace(const mPrice &price, const bool &isPong, mOrder *const order) {
      if (!order
        or order->exchangeId.empty()
      ) return false;
      order->price  = price;
      order->isPong = isPong;
      order->time   = Tstamp;
      return true;
    };
    static const bool cancel(mOrder *const order) {
      if (!order
        or order->exchangeId.empty()
        or order->orderStatus == mStatus::Waiting
      ) return false;
      order->orderStatus = mStatus::Waiting;
      order->time        = Tstamp;
      return true;
    };
  };
  static void to_json(json &j, const mOrder &k) {
    j = {
      {       "orderId", k.orderId       },
      {    "exchangeId", k.exchangeId    },
      {          "side", k.side          },
      {      "quantity", k.quantity      },
      {          "type", k.type          },
      {        "isPong", k.isPong        },
      {         "price", k.price         },
      {   "timeInForce", k.timeInForce   },
      {   "orderStatus", k.orderStatus   },
      {"preferPostOnly", k.preferPostOnly},
      {          "time", k.time          },
      {       "latency", k.latency       }
    };
  };
  static void from_json(const json &j, mOrder &k) {
    k.price          = j.value("price", 0.0);
    k.quantity       = j.value("quantity", 0.0);
    k.side           = j.value("side", "") == "Bid"
                         ? mSide::Bid
                         : mSide::Ask;
    k.type           = j.value("type", "") == "Limit"
                         ? mOrderType::Limit
                         : mOrderType::Market;
    k.timeInForce    = j.value("timeInForce", "") == "GTC"
                         ? mTimeInForce::GTC
                         : (j.value("timeInForce", "") == "FOK"
                           ? mTimeInForce::FOK
                           : mTimeInForce::IOC);
    k.isPong         = false;
    k.preferPostOnly = false;
  };

  struct mTrade {
     string tradeId;
      mSide side         = (mSide)0;
     mPrice price        = 0,
            Kprice       = 0;
    mAmount quantity     = 0,
            value        = 0,
            Kqty         = 0,
            Kvalue       = 0,
            Kdiff        = 0,
            feeCharged   = 0;
     mClock time         = 0,
            Ktime        = 0;
       bool isPong       = false,
            loadedFromDB = false;
    mTrade()
    {};
    mTrade(const mPrice p, const mAmount q, const mSide s, const mClock t)
      : side(s)
      , price(p)
      , quantity(q)
      , time(t)
    {};
    mTrade(const string &i, const mPrice &p, const mAmount &q, const mSide &S, const bool &P, const mClock &t, const mAmount &v, const mClock &Kt, const mAmount &Kq, const mPrice &Kp, const mAmount &Kv, const mAmount &Kd, const mAmount &f, const bool &l)
      : tradeId(i)
      , side(S)
      , price(p)
      , Kprice(Kp)
      , quantity(q)
      , value(v)
      , Kqty(Kq)
      , Kvalue(Kv)
      , Kdiff(Kd)
      , feeCharged(f)
      , time(t)
      , Ktime(Kt)
      , isPong(P)
      , loadedFromDB(l)
    {};
  };
  static void to_json(json &j, const mTrade &k) {
    if (k.tradeId.empty()) j = {
      {    "time", k.time    },
      {   "price", k.price   },
      {"quantity", k.quantity},
      {    "side", k.side    }
    };
    else j = {
      {     "tradeId", k.tradeId     },
      {        "time", k.time        },
      {       "price", k.price       },
      {    "quantity", k.quantity    },
      {        "side", k.side        },
      {       "value", k.value       },
      {       "Ktime", k.Ktime       },
      {        "Kqty", k.Kqty        },
      {      "Kprice", k.Kprice      },
      {      "Kvalue", k.Kvalue      },
      {       "Kdiff", k.Kdiff       },
      {  "feeCharged", k.feeCharged  },
      {      "isPong", k.isPong      },
      {"loadedFromDB", k.loadedFromDB}
    };
  };
  static void from_json(const json &j, mTrade &k) {
    k.tradeId      = j.value("tradeId", "");
    k.price        = j.value("price", 0.0);
    k.quantity     = j.value("quantity", 0.0);
    k.side         = j.value("side", (mSide)0);
    k.time         = j.value("time", (mClock)0);
    k.value        = j.value("value", 0.0);
    k.Ktime        = j.value("Ktime", (mClock)0);
    k.Kqty         = j.value("Kqty", 0.0);
    k.Kprice       = j.value("Kprice", 0.0);
    k.Kvalue       = j.value("Kvalue", 0.0);
    k.Kdiff        = j.value("Kdiff", 0.0);
    k.feeCharged   = j.value("feeCharged", 0.0);
    k.isPong       = j.value("isPong", false);
    k.loadedFromDB = true;
  };
  struct mMarketTakers: public mJsonToClient<mTrade> {
    vector<mTrade> trades;
    mAmount        takersBuySize60s  = 0,
                   takersSellSize60s = 0;
    void timer_60s() {
      takersSellSize60s = takersBuySize60s = 0;
      if (trades.empty()) return;
      for (mTrade &it : trades)
        (it.side == mSide::Bid
          ? takersSellSize60s
          : takersBuySize60s
        ) += it.quantity;
      trades.clear();
    };
    void read_from_gw(const mTrade &raw) {
      trades.push_back(raw);
      send();
    };
    const mMatter about() const {
      return mMatter::MarketTrade;
    };
    const json blob() const {
      return trades.back();
    };
    const json hello() {
      return trades;
    };
  };
  static void to_json(json &j, const mMarketTakers &k) {
    j = k.trades;
  };
  struct mTradesCompleted: public mToScreen,
                           public mVectorFromDb<mTrade>,
                           public mJsonToClient<mTrade> {
    void clearAll() {
      clear_if([](iterator it) {
        return true;
      });
    };
    void clearOne(const string &tradeId) {
      if (tradeId.empty()) return;
      clear_if([&](iterator it) {
        return it->tradeId == tradeId;
      }, true);
    };
    void clearClosed() {
      clear_if([](iterator it) {
        return it->Kqty >= it->quantity;
      });
    };
    void clearPongsAuto() {
      const mClock expire = Tstamp - (abs(qp.cleanPongsAuto) * 86400e3);
      clear_if([&](iterator it) {
        return (it->Ktime?:it->time) < expire and (
          qp.cleanPongsAuto < 0
          or it->Kqty >= it->quantity
        );
      });
    };
    void insert(mOrder *const o, const double &tradeQuantity) {
      mAmount fee = 0;
      mTrade trade(
        to_string(Tstamp),
        o->price,
        tradeQuantity,
        o->side,
        o->isPong,
        o->time,
        abs(o->price * tradeQuantity),
        0, 0, 0, 0, 0, fee, false
      );
      print("GW " + args.exchange, string(trade.isPong?"PONG":"PING") + " TRADE "
        + (trade.side == mSide::Bid ? "BUY  " : "SELL ")
        + str8(trade.quantity) + ' ' + args.base + " at price "
        + str8(trade.price) + ' ' + args.quote + " (value "
        + str8(trade.value) + ' ' + args.quote + ")"
      );
      if (qp.safety == mQuotingSafety::Off or qp.safety == mQuotingSafety::PingPong)
        send_push_back(trade);
      else {
        mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * trade.price / 100
          : qp.widthPong;
        map<mPrice, string> matches;
        for (mTrade &it : rows)
          if (it.quantity - it.Kqty > 0
            and it.side != trade.side
            and (qp.pongAt == mPongAt::AveragePingFair
              or qp.pongAt == mPongAt::AveragePingAggressive
              or (trade.side == mSide::Bid
                ? (it.price > trade.price + widthPong)
                : (it.price < trade.price - widthPong)
              )
            )
          ) matches[it.price] = it.tradeId;
        matchPong(
          matches,
          trade,
          (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive)
            ? trade.side == mSide::Ask
            : trade.side == mSide::Bid
        );
      }
      if (qp.cleanPongsAuto)
        clearPongsAuto();
    };
    const mMatter about() const {
      return mMatter::Trades;
    };
    void erase() {
      if (crbegin()->Kqty < 0) rows.pop_back();
    };
    const json blob() const {
      if (crbegin()->Kqty == -1) return nullptr;
      else return mVectorFromDb::blob();
    };
    const string increment() const {
      return crbegin()->tradeId;
    };
    string explainOK() const {
      return "loaded % historical Trades";
    };
    const json hello() {
      for (mTrade &it : rows)
        it.loadedFromDB = true;
      return rows;
    };
    private:
      void clear_if(const function<const bool(iterator)> &fn, const bool &onlyOne = false) {
        for (iterator it = begin(); it != end();)
          if (fn(it)) {
            it->Kqty = -1;
            it = send_push_erase(it);
            if (onlyOne) break;
          } else ++it;
      };
      void matchPong(map<mPrice, string> matches, mTrade pong, bool reverse) {
        if (reverse) for (map<mPrice, string>::reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (map<mPrice, string>::iterator it = matches.begin(); it != matches.end(); ++it)
          if (!matchPong(it->second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (iterator it = begin(); it != end(); ++it) {
            if (it->price!=pong.price or it->side!=pong.side or it->quantity<=it->Kqty) continue;
            eq = true;
            it->time = pong.time;
            it->quantity = it->quantity + pong.quantity;
            it->value = it->value + pong.value;
            it->isPong = false;
            it->loadedFromDB = false;
            it = send_push_erase(it);
            break;
          }
          if (!eq) {
            send_push_back(pong);
          }
        }
      };
      bool matchPong(string match, mTrade* pong) {
        for (iterator it = begin(); it != end(); ++it) {
          if (it->tradeId != match) continue;
          mAmount Kqty = fmin(pong->quantity, it->quantity - it->Kqty);
          it->Ktime = pong->time;
          it->Kprice = ((Kqty*pong->price) + (it->Kqty*it->Kprice)) / (it->Kqty+Kqty);
          it->Kqty = it->Kqty + Kqty;
          it->Kvalue = abs(it->Kqty*it->Kprice);
          pong->quantity = pong->quantity - Kqty;
          pong->value = abs(pong->price*pong->quantity);
          if (it->quantity<=it->Kqty)
            it->Kdiff = abs(it->quantity * it->price - it->Kqty * it->Kprice);
          it->isPong = true;
          it->loadedFromDB = false;
          it = send_push_erase(it);
          break;
        }
        return pong->quantity > 0;
      };
      void send_push_back(const mTrade &row) {
        rows.push_back(row);
        push();
        if (crbegin()->Kqty < 0) rbegin()->Kqty = -2;
        send();
      };
      iterator send_push_erase(iterator it) {
        mTrade row = *it;
        it = rows.erase(it);
        send_push_back(row);
        erase();
        return it;
      };
  };

  struct mRecentTrade {
     mPrice price    = 0;
    mAmount quantity = 0;
     mClock time     = 0;
    mRecentTrade(const mPrice &p, const mAmount &q)
      : price(p)
      , quantity(q)
      , time(Tstamp)
    {};
  };
  struct mRecentTrades {
    multimap<mPrice, mRecentTrade> buys,
                                   sells;
                           mAmount sumBuys       = 0,
                                   sumSells      = 0;
                            mPrice lastBuyPrice  = 0,
                                   lastSellPrice = 0;
    void insert(const mSide &side, const mPrice &price, const mAmount &quantity) {
      const bool bidORask = side == mSide::Bid;
      (bidORask
        ? lastBuyPrice
        : lastSellPrice
      ) = price;
      (bidORask
        ? buys
        : sells
      ).insert(pair<mPrice, mRecentTrade>(price, mRecentTrade(price, quantity)));
    };
    void reset() {
      if (buys.size()) expire(&buys);
      if (sells.size()) expire(&sells);
      skip();
      sumBuys = sum(&buys);
      sumSells = sum(&sells);
    };
    private:
      const mAmount sum(multimap<mPrice, mRecentTrade> *const k) const {
        mAmount sum = 0;
        for (multimap<mPrice, mRecentTrade>::value_type &it : *k)
          sum += it.second.quantity;
        return sum;
      };
      void expire(multimap<mPrice, mRecentTrade> *const k) {
        mClock now = Tstamp;
        for (multimap<mPrice, mRecentTrade>::iterator it = k->begin(); it != k->end();)
          if (it->second.time + qp.tradeRateSeconds * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      void skip() {
        while (buys.size() and sells.size()) {
          mRecentTrade &buy = buys.rbegin()->second;
          mRecentTrade &sell = sells.begin()->second;
          if (sell.price < buy.price) break;
          const mAmount buyQty = buy.quantity;
          buy.quantity -= sell.quantity;
          sell.quantity -= buyQty;
          if (buy.quantity <= 0)
            buys.erase(buys.rbegin()->first);
          if (sell.quantity <= 0)
            sells.erase(sells.begin()->first);
        }
      };
  };

  struct mFairLevelsPrice: public mToScreen,
                           public mJsonToClient<mFairLevelsPrice> {
    public_ref:
      const mPrice &fairValue;
    public:
      mFairLevelsPrice(const mPrice &f)
        : fairValue(f)
      {};
      const mMatter about() const {
        return mMatter::FairValue;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      void send_refresh() {
        if (send()) refresh();
      };
      const bool send_same_blob() const {
        return false;
      };
      const bool send_asap() const {
        return false;
      };
  };
  static void to_json(json &j, const mFairLevelsPrice &k) {
    j = {
      {"price", k.fairValue}
    };
  };

  struct mStdev {
    mPrice fv     = 0,
           topBid = 0,
           topAsk = 0;
    mStdev()
    {};
    mStdev(const mPrice &f, const mPrice &b, const mPrice &a)
      : fv(f)
      , topBid(b)
      , topAsk(a)
    {};
  };
  static void to_json(json &j, const mStdev &k) {
    j = {
      { "fv", k.fv    },
      {"bid", k.topBid},
      {"ask", k.topAsk}
    };
  };
  static void from_json(const json &j, mStdev &k) {
    k.fv = j.value("fv", 0.0);
    k.topBid = j.value("bid", 0.0);
    k.topAsk = j.value("ask", 0.0);
  };
  struct mStdevs: public mVectorFromDb<mStdev> {
    private_ref:
      const mPrice &fairValue;
    public:
      mStdevs(const mPrice &f)
        : fairValue(f)
      {};
    double top  = 0,  topMean = 0,
           fair = 0, fairMean = 0,
           bid  = 0,  bidMean = 0,
           ask  = 0,  askMean = 0;
    const bool pull(const json &j) {
      const bool loaded = mVectorFromDb::pull(j);
      if (loaded) calc();
      return loaded;
    };
    void timer_1s(const mPrice &topBid, const mPrice &topAsk) {
      push_back(mStdev(fairValue, topBid, topAsk));
      calc();
    };
    void calc() {
      if (size() < 2) return;
      fair = calc(&fairMean, "fv");
      bid  = calc(&bidMean, "bid");
      ask  = calc(&askMean, "ask");
      top  = calc(&topMean, "top");
    };
    double calc(mPrice *const mean, const string &type) const {
      vector<mPrice> values;
      for (const mStdev &it : rows)
        if (type == "fv")
          values.push_back(it.fv);
        else if (type == "bid")
          values.push_back(it.topBid);
        else if (type == "ask")
          values.push_back(it.topAsk);
        else if (type == "top") {
          values.push_back(it.topBid);
          values.push_back(it.topAsk);
        }
      return calc(mean, qp.quotingStdevProtectionFactor, values);
    };
    double calc(mPrice *const mean, const double &factor, const vector<mPrice> &values) const {
      unsigned int n = values.size();
      if (!n) return 0;
      double sum = 0;
      for (const mPrice &it : values) sum += it;
      *mean = sum / n;
      double sq_diff_sum = 0;
      for (const mPrice &it : values) {
        mPrice diff = it - *mean;
        sq_diff_sum += diff * diff;
      }
      double variance = sq_diff_sum / n;
      return sqrt(variance) * factor;
    };
    const mMatter about() const {
      return mMatter::STDEVStats;
    };
    const double limit() const {
      return qp.quotingStdevProtectionPeriods;
    };
    const mClock lifetime() const {
      return 1e+3 * limit();
    };
    string explainOK() const {
      return "loaded % STDEV Periods";
    };
  };
  static void to_json(json &j, const mStdevs &k) {
    j = {
      {      "fv", k.fair    },
      {  "fvMean", k.fairMean},
      {    "tops", k.top     },
      {"topsMean", k.topMean },
      {     "bid", k.bid     },
      { "bidMean", k.bidMean },
      {     "ask", k.ask     },
      { "askMean", k.askMean }
    };
  };

  struct mFairHistory: public mVectorFromDb<mPrice> {
    const mMatter about() const {
      return mMatter::MarketDataLongTerm;
    };
    const double limit() const {
      return 5760;
    };
    const mClock lifetime() const {
      return 60e+3 * limit();
    };
    string explainOK() const {
      return "loaded % historical Fair Values";
    };
  };

  struct mEwma: public mToScreen,
                public mStructFromDb<mEwma> {
    mFairHistory fairValue96h;
          mPrice mgEwmaVL = 0,
                 mgEwmaL  = 0,
                 mgEwmaM  = 0,
                 mgEwmaS  = 0,
                 mgEwmaXS = 0,
                 mgEwmaU  = 0,
                 mgEwmaP  = 0,
                 mgEwmaW  = 0;
          double mgEwmaTrendDiff              = 0,
                 targetPositionAutoPercentage = 0;
    private_ref:
      const mPrice &fairValue;
    public:
      mEwma(const mPrice &f)
        : fairValue(f)
      {};
      void timer_60s(const mPrice &averageWidth) {
        prepareHistory();
        calcProtections(averageWidth);
        calcPositions();
        calcTargetPositionAutoPercentage();
        push();
      };
      void calcFromHistory() {
        if (TRUEONCE(qp._diffVLEP)) calcFromHistory(&mgEwmaVL, qp.veryLongEwmaPeriods,   "VeryLong");
        if (TRUEONCE(qp._diffLEP))  calcFromHistory(&mgEwmaL,  qp.longEwmaPeriods,       "Long");
        if (TRUEONCE(qp._diffMEP))  calcFromHistory(&mgEwmaM,  qp.mediumEwmaPeriods,     "Medium");
        if (TRUEONCE(qp._diffSEP))  calcFromHistory(&mgEwmaS,  qp.shortEwmaPeriods,      "Short");
        if (TRUEONCE(qp._diffXSEP)) calcFromHistory(&mgEwmaXS, qp.extraShortEwmaPeriods, "ExtraShort");
        if (TRUEONCE(qp._diffUEP))  calcFromHistory(&mgEwmaU,  qp.ultraShortEwmaPeriods, "UltraShort");
      };
      const mMatter about() const {
        return mMatter::EWMAStats;
      };
      const mClock lifetime() const {
        return 60e+3 * max(qp.veryLongEwmaPeriods,
                       max(qp.longEwmaPeriods,
                       max(qp.mediumEwmaPeriods,
                       max(qp.shortEwmaPeriods,
                       max(qp.extraShortEwmaPeriods,
                           qp.ultraShortEwmaPeriods
                       )))));
      };
      const string explain() const {
        return "EWMA Values";
      };
      string explainKO() const {
        return "consider to warm up some %";
      };
    private:
      void calc(mPrice *const mean, const unsigned int &periods, const mPrice &value) {
        if (*mean) {
          double alpha = 2.0 / (periods + 1);
          *mean = alpha * value + (1 - alpha) * *mean;
        } else *mean = value;
      };
      void prepareHistory() {
        fairValue96h.push_back(fairValue);
      };
      void calcFromHistory(mPrice *const mean, const unsigned int &periods, const string &name) {
        unsigned int n = fairValue96h.size();
        if (!n) return;
        unsigned int x = 0;
        *mean = fairValue96h.front();
        while (n--) calc(mean, periods, fairValue96h.at(++x));
        print("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
      };
      void calcPositions() {
        calc(&mgEwmaVL, qp.veryLongEwmaPeriods,   fairValue);
        calc(&mgEwmaL,  qp.longEwmaPeriods,       fairValue);
        calc(&mgEwmaM,  qp.mediumEwmaPeriods,     fairValue);
        calc(&mgEwmaS,  qp.shortEwmaPeriods,      fairValue);
        calc(&mgEwmaXS, qp.extraShortEwmaPeriods, fairValue);
        calc(&mgEwmaU,  qp.ultraShortEwmaPeriods, fairValue);
        if (mgEwmaXS and mgEwmaU)
          mgEwmaTrendDiff = ((mgEwmaU * 1e+2) / mgEwmaXS) - 1e+2;
      };
      void calcProtections(const mPrice &averageWidth) {
        calc(&mgEwmaP, qp.protectionEwmaPeriods, fairValue);
        calc(&mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
      };
      void calcTargetPositionAutoPercentage() {
        unsigned int max3size = min((size_t)3, fairValue96h.size());
        mPrice SMA3 = accumulate(fairValue96h.end() - max3size, fairValue96h.end(), mPrice(),
          [](mPrice sma3, const mPrice &it) { return sma3 + it; }
        ) / max3size;
        double targetPosition = 0;
        if (qp.autoPositionMode == mAutoPositionMode::EWMA_LMS) {
          double newTrend = ((SMA3 * 1e+2 / mgEwmaL) - 1e+2);
          double newEwmacrossing = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2);
          targetPosition = ((newTrend + newEwmacrossing) / 2) * (1 / qp.ewmaSensiblityPercentage);
        } else if (qp.autoPositionMode == mAutoPositionMode::EWMA_LS)
          targetPosition = ((mgEwmaS * 1e+2 / mgEwmaL) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        else if (qp.autoPositionMode == mAutoPositionMode::EWMA_4) {
          if (mgEwmaL < mgEwmaVL) targetPosition = -1;
          else targetPosition = ((mgEwmaS * 1e+2 / mgEwmaM) - 1e+2) * (1 / qp.ewmaSensiblityPercentage);
        }
        targetPositionAutoPercentage = ((1 + max(-1.0, min(1.0, targetPosition))) / 2) * 1e+2;
      };
  };
  static void to_json(json &j, const mEwma &k) {
    j = {
      {  "ewmaVeryLong", k.mgEwmaVL       },
      {      "ewmaLong", k.mgEwmaL        },
      {    "ewmaMedium", k.mgEwmaM        },
      {     "ewmaShort", k.mgEwmaS        },
      {"ewmaExtraShort", k.mgEwmaXS       },
      {"ewmaUltraShort", k.mgEwmaU        },
      {     "ewmaQuote", k.mgEwmaP        },
      {     "ewmaWidth", k.mgEwmaW        },
      { "ewmaTrendDiff", k.mgEwmaTrendDiff}
    };
  };
  static void from_json(const json &j, mEwma &k) {
    k.mgEwmaVL = j.value("ewmaVeryLong", 0.0);
    k.mgEwmaL  = j.value("ewmaLong", 0.0);
    k.mgEwmaM  = j.value("ewmaMedium", 0.0);
    k.mgEwmaS  = j.value("ewmaShort", 0.0);
    k.mgEwmaXS = j.value("ewmaExtraShort", 0.0);
    k.mgEwmaU  = j.value("ewmaUltraShort", 0.0);
  };

  struct mMarketStats: public mJsonToClient<mMarketStats> {
               mEwma ewma;
             mStdevs stdev;
    mFairLevelsPrice fairPrice;
       mMarketTakers takerTrades;
    mMarketStats(const mPrice &f)
      : ewma(f)
      , stdev(f)
      , fairPrice(f)
    {};
    const mMatter about() const {
      return mMatter::MarketChart;
    };
    const bool realtime() const {
      return !qp.delayUI;
    };
  };
  static void to_json(json &j, const mMarketStats &k) {
    j = {
      {          "ewma", k.ewma                         },
      {    "stdevWidth", k.stdev                        },
      {     "fairValue", k.fairPrice.fairValue          },
      { "tradesBuySize", k.takerTrades.takersBuySize60s },
      {"tradesSellSize", k.takerTrades.takersSellSize60s}
    };
  };

  struct mProduct: public mJsonToClient<mProduct> {
    const mPrice  *minTick = nullptr;
    const mAmount *minSize = nullptr;
    mProduct()
    {};
    const mMatter about() const {
      return mMatter::ProductAdvertisement;
    };
  };
  static void to_json(json &j, const mProduct &k) {
    j = {
      {   "exchange", args.exchange                                 },
      {       "base", args.base                                     },
      {      "quote", args.quote                                    },
      {    "minTick", *k.minTick                                    },
      {"environment", args.title                                    },
      { "matryoshka", args.matryoshka                               },
      {   "homepage", "https://github.com/ctubio/Krypto-trading-bot"}
    };
  };

  struct mLevel {
     mPrice price = 0;
    mAmount size  = 0;
    mLevel()
    {};
    mLevel(const mPrice &p, const mAmount &s)
      : price(p)
      , size(s)
    {};
    void clear() {
      price = size = 0;
    };
    const bool empty() const {
      return !price or !size;
    };
  };
  static void to_json(json &j, const mLevel &k) {
    j = {
      {"price", k.price}
    };
    if (k.size) j["size"] = k.size;
  };
  struct mLevels {
    vector<mLevel> bids,
                   asks;
    mLevels()
    {};
    mLevels(const vector<mLevel> &b, const vector<mLevel> &a)
      : bids(b)
      , asks(a)
    {};
    const mPrice spread() const {
      return empty()
        ? 0
        : asks.cbegin()->price - bids.cbegin()->price;
    };
    const bool empty() const {
      return bids.empty() or asks.empty();
    };
    void clear() {
      bids.clear();
      asks.clear();
    };
  };
  static void to_json(json &j, const mLevels &k) {
    j = {
      {"bids", k.bids},
      {"asks", k.asks}
    };
  };
  struct mLevelsDiff: public mLevels,
                      public mJsonToClient<mLevelsDiff> {
      bool patched = false;
    private_ref:
      const mLevels &unfiltered;
    public:
      mLevelsDiff(const mLevels &u)
        : unfiltered(u)
      {};
      const bool empty() const {
        return patched
          ? bids.empty() and asks.empty()
          : mLevels::empty();
      };
      void send_reset() {
        if (ratelimit()) return;
        diff();
        if (!empty()) send_now();
        reset();
      };
      const mMatter about() const {
        return mMatter::MarketData;
      };
      const json hello() {
        reset();
        return mToClient::hello();
      };
    private:
      const bool ratelimit() {
        return unfiltered.empty() or empty()
          or !send_soon(qp.delayUI * 1e+3);
      };
      void reset() {
        bids = unfiltered.bids;
        asks = unfiltered.asks;
        patched = false;
      };
      void diff() {
        bids = diff(bids, unfiltered.bids);
        asks = diff(asks, unfiltered.asks);
        patched = true;
      };
      vector<mLevel> diff(const vector<mLevel> &from, vector<mLevel> to) const {
        vector<mLevel> patch;
        for (const mLevel &it : from) {
          vector<mLevel>::iterator it_ = find_if(
            to.begin(), to.end(),
            [&](const mLevel &_it) {
              return it.price == _it.price;
            }
          );
          mAmount size = 0;
          if (it_ != to.end()) {
            size = it_->size;
            to.erase(it_);
          }
          if (size != it.size)
            patch.push_back(mLevel(it.price, size));
        }
        if (!to.empty())
          patch.insert(patch.end(), to.begin(), to.end());
        return patch;
      };
  };
  static void to_json(json &j, const mLevelsDiff &k) {
    to_json(j, (mLevels)k);
    if (k.patched)
      j["diff"] = true;
  };
  struct mMarketLevels: public mLevels {
         mLevels unfiltered;
     mLevelsDiff diff;
    mMarketStats stats;
    unsigned int averageCount = 0;
          mPrice averageWidth = 0,
                 fairValue    = 0;
    private:
      unordered_map<mPrice, mAmount> filterBidOrders,
                                     filterAskOrders;
    private_ref:
      const mProduct                       &product;
      const unordered_map<mRandId, mOrder> &orders;
    public:
      mMarketLevels(const mProduct &p, const unordered_map<mRandId, mOrder> &o)
        : diff(unfiltered)
        , stats(fairValue)
        , product(p)
        , orders(o)
      {};
      const bool warn_empty() const {
        const bool err = empty();
        if (err) stats.fairPrice.warn("QE", "Unable to calculate quote, missing market data");
        return err;
      };
      void timer_1s() {
        stats.stdev.timer_1s(bids.cbegin()->price, asks.cbegin()->price);
      };
      void timer_60s() {
        stats.takerTrades.timer_60s();
        stats.ewma.timer_60s(resetAverageWidth());
        stats.send();
      };
      const mPrice calcQuotesWidth(bool *const superSpread) const {
        const mPrice widthPing = fmax(
          qp.widthPercentage
            ? qp.widthPingPercentage * fairValue / 100
            : qp.widthPing,
          qp.protectionEwmaWidthPing and stats.ewma.mgEwmaW
            ? stats.ewma.mgEwmaW
            : 0
        );
        *superSpread = spread() > widthPing * qp.sopWidthMultiplier;
        return widthPing;
      };
      const bool filter() {
        calcFilterOrders();
        bids = filter(unfiltered.bids, &filterBidOrders);
        asks = filter(unfiltered.asks, &filterAskOrders);
        calcFairValue();
        calcAverageWidth();
        return !empty();
      };
      void read_from_gw(const mLevels &raw) {
        unfiltered.bids = raw.bids;
        unfiltered.asks = raw.asks;
        filter();
        stats.fairPrice.send_refresh();
        diff.send_reset();
      };
    private:
      void calcAverageWidth() {
        if (empty()) return;
        averageWidth = (
          (averageWidth * averageCount)
            + asks.cbegin()->price
            - bids.cbegin()->price
        );
        averageWidth /= ++averageCount;
      };
      const mPrice resetAverageWidth() {
        averageCount = 0;
        return averageWidth;
      };
      void calcFairValue() {
        if (empty())
          fairValue = 0;
        else if (qp.fvModel == mFairValueModel::BBO)
          fairValue = (asks.cbegin()->price
                     + bids.cbegin()->price) / 2;
        else if (qp.fvModel == mFairValueModel::wBBO)
          fairValue = (
            bids.cbegin()->price * bids.cbegin()->size
          + asks.cbegin()->price * asks.cbegin()->size
          ) / (asks.cbegin()->size
             + bids.cbegin()->size
        );
        else
          fairValue = (
            bids.cbegin()->price * asks.cbegin()->size
          + asks.cbegin()->price * bids.cbegin()->size
          ) / (asks.cbegin()->size
             + bids.cbegin()->size
        );
        if (fairValue)
          fairValue = ROUND(fairValue, *product.minTick);
      };
      void calcFilterOrders() {
        filterBidOrders.clear();
        filterAskOrders.clear();
        for (const unordered_map<mRandId, mOrder>::value_type &it : orders)
          (it.second.side == mSide::Bid
            ? filterBidOrders
            : filterAskOrders
          )[it.second.price] += it.second.quantity;
      };
      const vector<mLevel> filter(vector<mLevel> levels, unordered_map<mPrice, mAmount> *const filterOrders) {
        if (!filterOrders->empty())
          for (vector<mLevel>::iterator it = levels.begin(); it != levels.end();) {
            for (unordered_map<mPrice, mAmount>::iterator it_ = filterOrders->begin(); it_ != filterOrders->end();)
              if (abs(it->price - it_->first) < *product.minTick) {
                it->size -= it_->second;
                filterOrders->erase(it_);
                break;
              } else ++it_;
            if (it->size < *product.minSize) it = levels.erase(it);
            else ++it;
            if (filterOrders->empty()) break;
          }
        return levels;
      };
  };

  struct mProfit {
    mAmount baseValue  = 0,
            quoteValue = 0;
     mClock time       = 0;
    mProfit()
    {};
    mProfit(mAmount b, mAmount q)
      : baseValue(b)
      , quoteValue(q)
      , time(Tstamp)
    {};
  };
  static void to_json(json &j, const mProfit &k) {
    j = {
      { "baseValue", k.baseValue },
      {"quoteValue", k.quoteValue},
      {      "time", k.time      }
    };
  };
  static void from_json(const json &j, mProfit &k) {
    k.baseValue  = j.value("baseValue", 0.0);
    k.quoteValue = j.value("quoteValue", 0.0);
    k.time       = j.value("time", (mClock)0);
  };
  struct mProfits: public mVectorFromDb<mProfit> {
    const bool ratelimit() const {
      return !empty() and crbegin()->time + 21e+3 > Tstamp;
    };
    const double calcBaseDiff() const {
      return calcDiffPercent(
        cbegin()->baseValue,
        crbegin()->baseValue
      );
    };
    const double calcQuoteDiff() const {
      return calcDiffPercent(
        cbegin()->quoteValue,
        crbegin()->quoteValue
      );
    };
    const double calcDiffPercent(mAmount older, mAmount newer) const {
      return ROUND(((newer - older) / newer) * 1e+2, 1e-2);
    };
    const mMatter about() const {
      return mMatter::Profit;
    };
    void erase() {
      for (vector<mProfit>::iterator it = begin(); it != end();)
        if (it->time + lifetime() > Tstamp) ++it;
        else it = rows.erase(it);
    };
    const double limit() const {
      return qp.profitHourInterval;
    };
    const mClock lifetime() const {
      return 3600e+3 * limit();
    };
    string explainOK() const {
      return "loaded % historical Profits";
    };
  };

  struct mSafety: public mJsonToClient<mSafety> {
           double buy      = 0,
                  sell     = 0,
                  combined = 0;
           mPrice buyPing  = 0,
                  sellPing = 0;
          mAmount buySize  = 0,
                  sellSize = 0;
    mRecentTrades recentTrades;
    private_ref:
      const mPrice  &fairValue;
      const mAmount &baseValue,
                    &baseTotal,
                    &targetBasePosition;
    public:
      mSafety(const mPrice &f, const mAmount &v, const mAmount &t, const mAmount &p)
        : fairValue(f)
        , baseValue(v)
        , baseTotal(t)
        , targetBasePosition(p)
      {};
      void calc(const mTradesCompleted &tradesHistory) {
        if (!baseValue or !fairValue) return;
        calcSizes();
        calcPrices(tradesHistory);
        recentTrades.reset();
        if (empty()) return;
        buy  = recentTrades.sumBuys / buySize;
        sell = recentTrades.sumSells / sellSize;
        combined = (recentTrades.sumBuys + recentTrades.sumSells) / (buySize + sellSize);
        send();
      };
      const bool empty() const {
        return !baseValue or !buySize or !sellSize;
      };
      const mMatter about() const {
        return mMatter::TradeSafetyValue;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void calcPrices(const mTradesCompleted &tradesHistory) {
        if (qp.safety == mQuotingSafety::PingPong) {
          buyPing = recentTrades.lastBuyPrice;
          sellPing = recentTrades.lastSellPrice;
        } else {
          buyPing = sellPing = 0;
          if (qp.safety == mQuotingSafety::Off) return;
          map<mPrice, mTrade> tradesBuy;
          map<mPrice, mTrade> tradesSell;
          for (const mTrade &it: tradesHistory)
            (it.side == mSide::Bid ? tradesBuy : tradesSell)[it.price] = it;
          mPrice widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * fairValue / 100
            : qp.widthPong;
          mAmount buyQty = 0,
                  sellQty = 0;
          if (qp.pongAt == mPongAt::ShortPingFair or qp.pongAt == mPongAt::ShortPingAggressive) {
            matchBestPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
            matchBestPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
            if (!buyQty) matchFirstPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
            if (!sellQty) matchFirstPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
          } else if (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) {
            matchLastPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
            matchLastPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
          } else if (qp.pongAt == mPongAt::AveragePingFair or qp.pongAt == mPongAt::AveragePingAggressive) {
            matchAllPing(&tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
            matchAllPing(&tradesSell, &sellPing, &sellQty, buySize, widthPong);
          }
          if (buyQty) buyPing /= buyQty;
          if (sellQty) sellPing /= sellQty;
        }
      };
      void matchFirstPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, false, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(false, true, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width) {
        matchPing(false, false, trades, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<mPrice, mTrade>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<mPrice, mTrade>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
      };
      const bool matchPing(bool _near, bool _far, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, mPrice fv, mPrice price, mAmount qtyTrade, mPrice priceTrade, mAmount KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (((!_near and !_far) or *qty < qtyMax)
          and (_far ? fv > price : true)
          and (_near ? (reverse ? fv - width : fv + width) < price : true)
          and KqtyTrade < qtyTrade
        ) {
          mAmount qty_ = qtyTrade;
          if (_near or _far)
            qty_ = fmin(qtyMax - *qty, qty_);
          *ping += priceTrade * qty_;
          *qty += qty_;
        }
        return *qty >= qtyMax and (_near or _far);
      };
      void calcSizes() {
        sellSize = qp.percentageValues
            ? qp.sellSizePercentage * baseValue / 1e+2
            : qp.sellSize;
        buySize = qp.percentageValues
          ? qp.buySizePercentage * baseValue / 1e+2
          : qp.buySize;
        if (qp.aggressivePositionRebalancing == mAPR::Off) return;
        if (qp.buySizeMax)
          buySize = fmax(buySize, targetBasePosition - baseTotal);
        if (qp.sellSizeMax)
          sellSize = fmax(sellSize, baseTotal - targetBasePosition);
      };
  };
  static void to_json(json &j, const mSafety &k) {
    j = {
      {     "buy", k.buy     },
      {    "sell", k.sell    },
      {"combined", k.combined},
      { "buyPing", k.buyPing },
      {"sellPing", k.sellPing}
    };
  };

  struct mTarget: public mToScreen,
                  public mStructFromDb<mTarget>,
                  public mJsonToClient<mTarget> {
    mAmount targetBasePosition = 0,
            positionDivergence = 0;
    mSafety safety;
    private_ref:
      const double  &targetPositionAutoPercentage;
      const mAmount &baseValue;
    public:
      mTarget(const mPrice &f, const double &p, const mAmount &v, const mAmount &t)
        : safety(f, v, t, targetBasePosition)
        , targetPositionAutoPercentage(p)
        , baseValue(v)
      {};
      void calcTargetBasePos() {                             // PRETTY_DEBUG plz
        if (warn_empty()) return;
        targetBasePosition = ROUND(qp.autoPositionMode == mAutoPositionMode::Manual
          ? (qp.percentageValues
            ? qp.targetBasePositionPercentage * baseValue / 1e+2
            : qp.targetBasePosition)
          : targetPositionAutoPercentage * baseValue / 1e+2
        , 1e-4);
        calcPDiv();
        if (send()) {
          push();
          if (debug())
            report();
        }
      };
      const bool warn_empty() const {
        const bool err = empty();
        if (err) warn("PG", "Unable to calculate TBP, missing wallet data");
        return err;
      };
      const bool empty() const {
        return !baseValue;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const mMatter about() const {
        return mMatter::TargetBasePosition;
      };
      const string explain() const {
        return to_string(targetBasePosition);
      };
      string explainOK() const {
        return "loaded TBP = % " + args.base;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void calcPDiv() {
        mAmount pDiv = qp.percentageValues
          ? qp.positionDivergencePercentage * baseValue / 1e+2
          : qp.positionDivergence;
        if (qp.autoPositionMode == mAutoPositionMode::Manual or mPDivMode::Manual == qp.positionDivergenceMode)
          positionDivergence = pDiv;
        else {
          mAmount pDivMin = qp.percentageValues
            ? qp.positionDivergencePercentageMin * baseValue / 1e+2
            : qp.positionDivergenceMin;
          double divCenter = 1 - abs((targetBasePosition / baseValue * 2) - 1);
          if (mPDivMode::Linear == qp.positionDivergenceMode)      positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
          else if (mPDivMode::Sine == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
          else if (mPDivMode::SQRT == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
          else if (mPDivMode::Switch == qp.positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
        }
        positionDivergence = ROUND(positionDivergence, 1e-4);
      };
      void report() const {
        print("PG", "TBP: "
          + to_string((int)(targetBasePosition / baseValue * 1e+2)) + "% = " + str8(targetBasePosition)
          + " " + args.base + ", pDiv: "
          + to_string((int)(positionDivergence / baseValue * 1e+2)) + "% = " + str8(positionDivergence)
          + " " + args.base);
      };
      const bool debug() const {
        return args.debugWallet;
      };
  };
  static void to_json(json &j, const mTarget &k) {
    j = {
      { "tbp", k.targetBasePosition},
      {"pDiv", k.positionDivergence}
    };
  };
  static void from_json(const json &j, mTarget &k) {
    k.targetBasePosition = j.value("tbp", 0.0);
    k.positionDivergence = j.value("pDiv", 0.0);
  };

  struct mWallet {
    mAmount amount = 0,
            held   = 0,
            total  = 0,
            value  = 0,
            profit = 0;
    mCoinId currency;
    mWallet()
    {};
    mWallet(const mAmount &a, const mAmount &h, const mCoinId &c)
      : amount(a)
      , held(h)
      , currency(c)
    {};
    void reset(const mAmount &a, const mAmount &h) {
      if (empty()) return;
      total = (amount = ROUND(a, 1e-8))
            + (held   = ROUND(h , 1e-8));
    };
    const bool empty() const {
      return currency.empty();
    };
  };
  static void to_json(json &j, const mWallet &k) {
    j = {
      {"amount", k.amount},
      {  "held", k.held  },
      { "value", k.value },
      {"profit", k.profit}
    };
  };
  struct mWallets {
    mWallet base,
            quote;
    mWallets()
    {};
    mWallets(const mWallet &b, const mWallet &q)
      : base(b)
      , quote(q)
    {};
    const bool empty() const {
      return base.empty() or quote.empty();
    };
  };
  static void to_json(json &j, const mWallets &k) {
    j = {
      { "base", k.base },
      {"quote", k.quote}
    };
  };
  struct mWalletPosition: public mWallets,
                          public mJsonToClient<mWalletPosition> {
     mTarget target;
    mProfits profits;
    private_ref:
      const mPrice &fairValue;
    public:
      mWalletPosition(const mPrice &f, const double &t)
        : target(f, t, base.value, base.total)
        , fairValue(f)
      {};
      void read_from_gw(const mWallets &raw) {
        if (raw.empty()) return;
        base.currency = raw.base.currency;
        quote.currency = raw.quote.currency;
        base.reset(raw.base.amount, raw.base.held);
        quote.reset(raw.quote.amount, raw.quote.held);
        send_ratelimit();
      };
      void reset(const mSide &side, const mAmount &nextHeldAmount) {
        if (side == mSide::Ask)
          base.reset(base.total - nextHeldAmount, nextHeldAmount);
        else quote.reset(quote.total - nextHeldAmount, nextHeldAmount);
        send_ratelimit();
      };
      void send_ratelimit() {
        if (empty() or !fairValue) return;
        calcValues();
        target.calcTargetBasePos();
        send();
      };
      const mMatter about() const {
        return mMatter::Position;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const bool send_asap() const {
        return false;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void calcValues() {
        if (args.maxWallet) calcMaxWallet();
        base.value = ROUND(quote.total / fairValue + base.total, 1e-8);
        quote.value = ROUND(base.total * fairValue + quote.total, 1e-8);
        calcProfits();
      };
      void calcProfits() {
        if (!profits.ratelimit())
          profits.push_back(mProfit(base.value, quote.value));
        base.profit  = profits.calcBaseDiff();
        quote.profit = profits.calcQuoteDiff();
      };
      void calcMaxWallet() {
        mAmount maxWallet = args.maxWallet;
        maxWallet -= quote.held / fairValue;
        if (maxWallet > 0 and quote.amount / fairValue > maxWallet) {
          quote.amount = maxWallet * fairValue;
          maxWallet = 0;
        } else maxWallet -= quote.amount / fairValue;
        maxWallet -= base.held;
        if (maxWallet > 0 and base.amount > maxWallet)
          base.amount = maxWallet;
      };
  };

  struct mButtonSubmitNewOrder: public mFromClient {
    void kiss(json *const j) {
      if (!j->is_object() or !j->value("price", 0.0) or !j->value("quantity", 0.0))
        *j = nullptr;
    };
    const mMatter about() const {
      return mMatter::SubmitNewOrder;
    };
  };
  struct mButtonCancelOrder: public mFromClient {
    void kiss(json *const j) {
      *j = (j->is_object() and !j->value("orderId", "").empty())
        ? j->at("orderId").get<mRandId>()
        : nullptr;
    };
    const mMatter about() const {
      return mMatter::CancelOrder;
    };
  };
  struct mButtonCancelAllOrders: public mFromClient {
    const mMatter about() const {
      return mMatter::CancelAllOrders;
    };
  };
  struct mButtonCleanAllClosedTrades: public mFromClient {
    const mMatter about() const {
      return mMatter::CleanAllClosedTrades;
    };
  };
  struct mButtonCleanAllTrades: public mFromClient {
    const mMatter about() const {
      return mMatter::CleanAllTrades;
    };
  };
  struct mButtonCleanTrade: public mFromClient {
    void kiss(json *const j) {
      *j = (j->is_object() and !j->value("tradeId", "").empty())
        ? j->at("tradeId").get<string>()
        : nullptr;
    };
    const mMatter about() const {
      return mMatter::CleanTrade;
    };
  };
  struct mButtons {
    mButtonSubmitNewOrder       submit;
    mButtonCancelOrder          cancel;
    mButtonCancelAllOrders      cancelAll;
    mButtonCleanAllClosedTrades cleanTradesClosed;
    mButtonCleanAllTrades       cleanTrades;
    mButtonCleanTrade           cleanTrade;
  };

  struct mSemaphore: public mToScreen,
                     public mJsonToClient<mSemaphore> {
    mConnectivity greenButton           = mConnectivity::Disconnected,
                  greenGateway          = mConnectivity::Disconnected;
    private_ref:
      mConnectivity &adminAgreement = (mConnectivity&)args.autobot;
    public:
      void kiss(json *const j) {
        if (j->is_object()
          and j->at("state").is_number()
          and j->at("state").get<mConnectivity>() != adminAgreement
        ) toggle();
      };
      const bool read_from_gw(const mConnectivity &raw) {
        if (greenGateway != raw) {
          greenGateway = raw;
          send_refresh();
        }
        return !!greenGateway;
      };
      void toggle() {
        adminAgreement = (mConnectivity)!adminAgreement;
        send_refresh();
      };
      const mMatter about() const {
        return mMatter::Connectivity;
      };
    private:
      void send_refresh() {
        const mConnectivity k = adminAgreement * greenGateway;
        if (greenButton != k) {
          greenButton = k;
          focus("GW " + args.exchange, "Quoting state changed to",
            string(!greenButton ? "DIS" : "") + "CONNECTED");
        }
        send();
        refresh();
      };
  };
  static void to_json(json &j, const mSemaphore &k) {
    j = {
      { "state", k.greenButton },
      {"status", k.greenGateway}
    };
  };

  struct mQuote: public mLevel {
    mQuoteState state  = mQuoteState::MissingData;
           bool isPong = false;
    void clear(const mQuoteState &reason) {
      mLevel::clear();
      state = reason;
    };
  };
  struct mQuotes: public mToScreen {
    mQuote bid,
           ask;
      bool superSpread = false;
    mQuotes()
    {};
    mQuotes(const mQuote &b, const mQuote &a)
      : bid(b)
      , ask(a)
    {};
    void checkCrossedQuotes() {
      bid.state = checkCrossedQuotes(mSide::Bid);
      ask.state = checkCrossedQuotes(mSide::Ask);
    };
    void debug(const string &reason) {
      if (debug())
        print("DEBUG QE", reason);
    };
    void debuq(const string &step) {
      if (debug())
        print("DEBUG QE", "[" + step + "] "
          + to_string((int)bid.state) + ":"
          + to_string((int)ask.state) + " "
          + ((json*)this)->dump()
        );
    };
    private:
      mQuoteState checkCrossedQuotes(const mSide &side) {
        bool cross = false;
        if (side == mSide::Bid) {
          if (bid.empty()) return bid.state;
          if (ask.empty()) return mQuoteState::Live;
          cross = bid.price >= ask.price;
        } else {
          if (ask.empty()) return ask.state;
          if (bid.empty()) return mQuoteState::Live;
          cross = ask.price <= bid.price;
        }
        if (cross) {
          warn("QE", "Cross bid/ask quotes detected, that is.. unexpected");
          return mQuoteState::Crossed;
        } else return mQuoteState::Live;
      };
      const bool debug() const {
        return args.debugQuotes;
      };
  };
  static void to_json(json &j, const mQuotes &k) {
    j = {
      {"bid", k.bid},
      {"ask", k.ask}
    };
  };

  struct mDummyMarketMaker: public mToScreen {
    private:
      void (*calcRawQuotesFromMarket)(
        const mMarketLevels&,
        const mPrice&,
        const mPrice&,
        const mAmount&,
        const mAmount&,
              mQuotes&
      ) = nullptr;
    private_ref:
      const mProduct        &product;
      const mWalletPosition &wallet;
      const mMarketLevels   &levels;
            mQuotes         &nextQuotes;
    public:
      mDummyMarketMaker(const mProduct &p, const mWalletPosition &w, const mMarketLevels &l, mQuotes &q)
        : product(p)
        , wallet(w)
        , levels(l)
        , nextQuotes(q)
      {};
      void reset(const string &reason) {
        if (qp.mode == mQuotingMode::Top)              calcRawQuotesFromMarket = calcTopOfMarket;
        else if (qp.mode == mQuotingMode::Mid)         calcRawQuotesFromMarket = calcMidOfMarket;
        else if (qp.mode == mQuotingMode::Join)        calcRawQuotesFromMarket = calcJoinMarket;
        else if (qp.mode == mQuotingMode::InverseJoin) calcRawQuotesFromMarket = calcInverseJoinMarket;
        else if (qp.mode == mQuotingMode::InverseTop)  calcRawQuotesFromMarket = calcInverseTopOfMarket;
        else if (qp.mode == mQuotingMode::HamelinRat)  calcRawQuotesFromMarket = calcColossusOfMarket;
        else if (qp.mode == mQuotingMode::Depth)       calcRawQuotesFromMarket = calcDepthOfMarket;
        else EXIT(error("QE", "Invalid quoting mode "
          + reason + ", consider to remove the database file"));
      };
      void calcRawQuotes() const  {
        calcRawQuotesFromMarket(
          levels,
          *product.minTick,
          levels.calcQuotesWidth(&nextQuotes.superSpread),
          wallet.target.safety.buySize,
          wallet.target.safety.sellSize,
          nextQuotes
        );
        if (nextQuotes.bid.price <= 0 or nextQuotes.ask.price <= 0) {
          nextQuotes.bid.clear(mQuoteState::WidthMustBeSmaller);
          nextQuotes.ask.clear(mQuoteState::WidthMustBeSmaller);
          warn("QP", "Negative price detected, widthPing must be smaller");
        }
      };
    private:
      static void quoteAtTopOfMarket(const mMarketLevels &levels, const mPrice &minTick, mQuotes &nextQuotes) {
        const mLevel &topBid = levels.bids.begin()->size > minTick
          ? levels.bids.at(0)
          : levels.bids.at(levels.bids.size() > 1 ? 1 : 0);
        const mLevel &topAsk = levels.asks.begin()->size > minTick
          ? levels.asks.at(0)
          : levels.asks.at(levels.asks.size() > 1 ? 1 : 0);
        nextQuotes.bid.price = topBid.price;
        nextQuotes.ask.price = topAsk.price;
      };
      static void calcTopOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        quoteAtTopOfMarket(levels, minTick, nextQuotes);
        nextQuotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, nextQuotes.bid.price + minTick);
        nextQuotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, nextQuotes.ask.price - minTick);
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
      static void calcMidOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        nextQuotes.bid.price = fmax(levels.fairValue - widthPing, 0);
        nextQuotes.ask.price = levels.fairValue + widthPing;
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
      static void calcJoinMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        quoteAtTopOfMarket(levels, minTick, nextQuotes);
        nextQuotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, nextQuotes.bid.price);
        nextQuotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, nextQuotes.ask.price);
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
      static void calcInverseJoinMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        quoteAtTopOfMarket(levels, minTick, nextQuotes);
        mPrice mktWidth = abs(nextQuotes.ask.price - nextQuotes.bid.price);
        if (mktWidth > widthPing) {
          nextQuotes.ask.price = nextQuotes.ask.price + widthPing;
          nextQuotes.bid.price = nextQuotes.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          nextQuotes.ask.price = nextQuotes.ask.price + widthPing / 4.0;
          nextQuotes.bid.price = nextQuotes.bid.price - widthPing / 4.0;
        }
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
      static void calcInverseTopOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        quoteAtTopOfMarket(levels, minTick, nextQuotes);
        mPrice mktWidth = abs(nextQuotes.ask.price - nextQuotes.bid.price);
        if (mktWidth > widthPing) {
          nextQuotes.ask.price = nextQuotes.ask.price + widthPing;
          nextQuotes.bid.price = nextQuotes.bid.price - widthPing;
        }
        nextQuotes.bid.price = nextQuotes.bid.price + minTick;
        nextQuotes.ask.price = nextQuotes.ask.price - minTick;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          nextQuotes.ask.price = nextQuotes.ask.price + widthPing / 4.0;
          nextQuotes.bid.price = nextQuotes.bid.price - widthPing / 4.0;
        }
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
      static void calcColossusOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &widthPing,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        quoteAtTopOfMarket(levels, minTick, nextQuotes);
        nextQuotes.bid.size = 0;
        nextQuotes.ask.size = 0;
        for (const mLevel &it : levels.bids)
          if (nextQuotes.bid.size < it.size and it.price <= nextQuotes.bid.price) {
            nextQuotes.bid.size = it.size;
            nextQuotes.bid.price = it.price;
          }
        for (const mLevel &it : levels.asks)
          if (nextQuotes.ask.size < it.size and it.price >= nextQuotes.ask.price) {
            nextQuotes.ask.size = it.size;
            nextQuotes.ask.price = it.price;
          }
        if (nextQuotes.bid.size) nextQuotes.bid.price += minTick;
        if (nextQuotes.ask.size) nextQuotes.ask.price -= minTick;
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
      static void calcDepthOfMarket(
        const mMarketLevels &levels,
        const mPrice        &minTick,
        const mPrice        &depth,
        const mAmount       &bidSize,
        const mAmount       &askSize,
              mQuotes       &nextQuotes
      ) {
        mPrice bidPx = levels.bids.cbegin()->price;
        mAmount bidDepth = 0;
        for (const mLevel &it : levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        mPrice askPx = levels.asks.cbegin()->price;
        mAmount askDepth = 0;
        for (const mLevel &it : levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        nextQuotes.bid.price = bidPx;
        nextQuotes.ask.price = askPx;
        nextQuotes.bid.size = bidSize;
        nextQuotes.ask.size = askSize;
      };
  };

  struct mAntonioCalculon: public mJsonToClient<mAntonioCalculon> {
              mQuotes nextQuotes;
    mDummyMarketMaker dummyMM;
         unsigned int countWaiting = 0,
                      countWorking = 0,
                      AK47inc      = 0;
               string sideAPR      = "Off";
    private_ref:
      const mProduct        &product;
      const mWalletPosition &wallet;
      const mMarketLevels   &levels;
    public:
      mAntonioCalculon(const mProduct &p, const mWalletPosition &w, const mMarketLevels &l)
        : dummyMM(p, w, l, nextQuotes)
        , product(p)
        , wallet(w)
        , levels(l)
      {};
      void calcQuotes() {
        dummyMM.calcRawQuotes();
        applyQuotingParameters();
      };
      void reset(const mQuoteState &state) {
        nextQuotes.bid.state =
        nextQuotes.ask.state = state;
      };
      void reset() {
        reset(mQuoteState::MissingData);
        countWaiting =
        countWorking = 0;
      };
      const mMatter about() const {
        return mMatter::QuoteStatus;
      };
      const bool realtime() const {
        return !qp.delayUI;
      };
      const bool send_same_blob() const {
        return false;
      };
    private:
      void applyQuotingParameters() {
        nextQuotes.debuq("?"); applySuperTrades();
        nextQuotes.debuq("A"); applyEwmaProtection();
        nextQuotes.debuq("B"); applyTotalBasePosition();
        nextQuotes.debuq("C"); applyStdevProtection();
        nextQuotes.debuq("D"); applyAggressivePositionRebalancing();
        nextQuotes.debuq("E"); applyAK47Increment();
        nextQuotes.debuq("F"); applyBestWidth();
        nextQuotes.debuq("G"); applyTradesPerMinute();
        nextQuotes.debuq("H"); applyRoundPrice();
        nextQuotes.debuq("I"); applyRoundSize();
        nextQuotes.debuq("J"); applyDepleted();
        nextQuotes.debuq("K"); applyWaitingPing();
        nextQuotes.debuq("L"); applyEwmaTrendProtection();
        nextQuotes.debuq("!");
        nextQuotes.debug("totals " + ("toAsk: " + to_string(wallet.base.total))
                                   + ",toBid: " + to_string(wallet.quote.total / levels.fairValue));
        nextQuotes.checkCrossedQuotes();
      };
      void applySuperTrades() {
        if (!nextQuotes.superSpread
          or (qp.superTrades != mSOP::Size and qp.superTrades != mSOP::TradesSize)
        ) return;
        if (!qp.buySizeMax and !nextQuotes.bid.empty())
          nextQuotes.bid.size = fmin(
            qp.sopSizeMultiplier * nextQuotes.bid.size,
            (wallet.quote.amount / levels.fairValue) / 2
          );
        if (!qp.sellSizeMax and !nextQuotes.ask.empty())
          nextQuotes.ask.size = fmin(
            qp.sopSizeMultiplier * nextQuotes.ask.size,
            wallet.base.amount / 2
          );
      };
      void applyEwmaProtection() {
        if (!qp.protectionEwmaQuotePrice or !levels.stats.ewma.mgEwmaP) return;
        if (!nextQuotes.ask.empty())
          nextQuotes.ask.price = fmax(levels.stats.ewma.mgEwmaP, nextQuotes.ask.price);
        if (!nextQuotes.bid.empty())
          nextQuotes.bid.price = fmin(levels.stats.ewma.mgEwmaP, nextQuotes.bid.price);
      };
      void applyTotalBasePosition() {
        if (wallet.base.total < wallet.target.targetBasePosition - wallet.target.positionDivergence) {
          nextQuotes.ask.clear(mQuoteState::TBPHeld);
          if (!nextQuotes.bid.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = "Buy";
            if (!qp.buySizeMax) nextQuotes.bid.size = fmin(qp.aprMultiplier * nextQuotes.bid.size, fmin(wallet.target.targetBasePosition - wallet.base.total, (wallet.quote.amount / levels.fairValue) / 2));
          }
        }
        else if (wallet.base.total >= wallet.target.targetBasePosition + wallet.target.positionDivergence) {
          nextQuotes.bid.clear(mQuoteState::TBPHeld);
          if (!nextQuotes.ask.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = "Sell";
            if (!qp.sellSizeMax) nextQuotes.ask.size = fmin(qp.aprMultiplier * nextQuotes.ask.size, fmin(wallet.base.total - wallet.target.targetBasePosition, wallet.base.amount / 2));
          }
        }
        else sideAPR = "Off";
      };
      void applyStdevProtection() {
        if (qp.quotingStdevProtection == mSTDEV::Off or !levels.stats.stdev.fair) return;
        if (!nextQuotes.ask.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != "Sell"
        ))
          nextQuotes.ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.askMean )
              : levels.fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.ask )),
            nextQuotes.ask.price
          );
        if (!nextQuotes.bid.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != "Buy"
        ))
          nextQuotes.bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.bidMean )
              : levels.fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.bid )),
            nextQuotes.bid.price
          );
      };
      void applyAggressivePositionRebalancing() {
        if (qp.safety == mQuotingSafety::Off) return;
        const mPrice widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * levels.fairValue / 100
          : qp.widthPong;
        const mPrice &safetyBuyPing = wallet.target.safety.buyPing;
        if (!nextQuotes.ask.empty() and safetyBuyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == "Sell")
            or (qp.safety == mQuotingSafety::PingPong
              ? nextQuotes.ask.price < safetyBuyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) nextQuotes.ask.price = safetyBuyPing + widthPong;
          nextQuotes.ask.isPong = nextQuotes.ask.price >= safetyBuyPing + widthPong;
        }
        const mPrice &safetysellPing = wallet.target.safety.sellPing;
        if (!nextQuotes.bid.empty() and safetysellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == "Buy")
            or (qp.safety == mQuotingSafety::PingPong
              ? nextQuotes.bid.price > safetysellPing - widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) nextQuotes.bid.price = safetysellPing - widthPong;
          nextQuotes.bid.isPong = nextQuotes.bid.price <= safetysellPing - widthPong;
        }
      };
      void applyAK47Increment() {
        if (qp.safety != mQuotingSafety::AK47) return;
        const mPrice range = qp.percentageValues
          ? qp.rangePercentage * wallet.base.value / 100
          : qp.range;
        if (!nextQuotes.bid.empty())
          nextQuotes.bid.price -= AK47inc * range;
        if (!nextQuotes.ask.empty())
          nextQuotes.ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyBestWidth() {
        if (!qp.bestWidth) return;
        if (!nextQuotes.ask.empty())
          for (const mLevel &it : levels.asks)
            if (it.price > nextQuotes.ask.price) {
              const mPrice bestAsk = it.price - *product.minTick;
              if (bestAsk > levels.fairValue) {
                nextQuotes.ask.price = bestAsk;
                break;
              }
            }
        if (!nextQuotes.bid.empty())
          for (const mLevel &it : levels.bids)
            if (it.price < nextQuotes.bid.price) {
              const mPrice bestBid = it.price + *product.minTick;
              if (bestBid < levels.fairValue) {
                nextQuotes.bid.price = bestBid;
                break;
              }
            }
      };
      void applyTradesPerMinute() {
        const double factor = (nextQuotes.superSpread and (
          qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize
        )) ? qp.sopWidthMultiplier
           : 1;
        if (wallet.target.safety.sell >= qp.tradesPerMinute * factor)
          nextQuotes.ask.clear(mQuoteState::MaxTradesSeconds);
        if (wallet.target.safety.buy >= qp.tradesPerMinute * factor)
          nextQuotes.bid.clear(mQuoteState::MaxTradesSeconds);
      };
      void applyRoundPrice() {
        if (!nextQuotes.bid.empty())
          nextQuotes.bid.price = fmax(
            0,
            floor(nextQuotes.bid.price / *product.minTick) * *product.minTick
          );
        if (!nextQuotes.ask.empty())
          nextQuotes.ask.price = fmax(
            nextQuotes.bid.price + *product.minTick,
            ceil(nextQuotes.ask.price / *product.minTick) * *product.minTick
          );
      };
      void applyRoundSize() {
        if (!nextQuotes.ask.empty())
          nextQuotes.ask.size = floor(fmax(
            fmin(
              nextQuotes.ask.size,
              wallet.base.total
            ),
            *product.minSize
          ) / 1e-8) * 1e-8;
        if (!nextQuotes.bid.empty())
          nextQuotes.bid.size = floor(fmax(
            fmin(
              nextQuotes.bid.size,
              wallet.quote.total / levels.fairValue
            ),
            *product.minSize
          ) / 1e-8) * 1e-8;
      };
      void applyDepleted() {
        if (nextQuotes.bid.size > wallet.quote.total / levels.fairValue)
          nextQuotes.bid.clear(mQuoteState::DepletedFunds);
        if (nextQuotes.ask.size > wallet.base.total)
          nextQuotes.ask.clear(mQuoteState::DepletedFunds);
      };
      void applyWaitingPing() {
        if (qp.safety == mQuotingSafety::Off) return;
        if (!nextQuotes.ask.isPong and (
          (nextQuotes.bid.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::BidSide
          or qp.pingAt == mPingAt::DepletedAskSide
        )) nextQuotes.ask.clear(mQuoteState::WaitingPing);
        if (!nextQuotes.bid.isPong and (
          (nextQuotes.ask.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::AskSide
          or qp.pingAt == mPingAt::DepletedBidSide
        )) nextQuotes.bid.clear(mQuoteState::WaitingPing);
      };
      void applyEwmaTrendProtection() {
        if (!qp.quotingEwmaTrendProtection or !levels.stats.ewma.mgEwmaTrendDiff) return;
        if (levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold)
          nextQuotes.ask.clear(mQuoteState::UpTrendHeld);
        else if (levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold)
          nextQuotes.bid.clear(mQuoteState::DownTrendHeld);
      };
  };
  static void to_json(json &j, const mAntonioCalculon &k) {
    j = {
      {            "bidStatus", k.nextQuotes.bid.state},
      {            "askStatus", k.nextQuotes.ask.state},
      {              "sideAPR", k.sideAPR             },
      {"quotesInMemoryWaiting", k.countWaiting        },
      {"quotesInMemoryWorking", k.countWorking        }
    };
  };

  struct mBroker: public mToScreen,
                  public mJsonToClient<mBroker> {
    unordered_map<mRandId, mOrder> orders;
             vector<const mOrder*> zombies;
                  mTradesCompleted tradesHistory;
                        mSemaphore semaphore;
                  mAntonioCalculon calculon;
    mBroker(const mProduct &p, const mWalletPosition &w, const mMarketLevels &l)
      : calculon(p, w, l)
    {};
    mOrder *const find(const mRandId &orderId) {
      return (orderId.empty()
        or orders.find(orderId) == orders.end()
      ) ? nullptr
        : &orders.at(orderId);
    };
    mOrder *const findsert(const mOrder &raw) {
      if (raw.orderStatus == mStatus::Waiting and !raw.orderId.empty())
        return &(orders[raw.orderId] = raw);
      if (raw.orderId.empty() and !raw.exchangeId.empty()) {
        unordered_map<mRandId, mOrder>::iterator it = find_if(
          orders.begin(), orders.end(),
          [&](const pair<mRandId, mOrder> &it_) {
            return raw.exchangeId == it_.second.exchangeId;
          }
        );
        if (it != orders.end())
          return &it->second;
      }
      return find(raw.orderId);
    };
    mOrder *const upsert(const mOrder &raw) {
      mOrder *const order = findsert(raw);
      mOrder::update(raw, order);
      if (debug()) {
        report(order, " saved ");
        report_size();
      }
      return order;
    };
    const bool replace(const mPrice &price, const bool &isPong, mOrder *const order) {
      const bool allowed = mOrder::replace(price, isPong, order);
      if (debug()) report(order, "replace");
      return allowed;
    };
    const bool cancel(mOrder *const order) {
      const bool allowed = mOrder::cancel(order);
      if (debug()) report(order, "cancel ");
      return allowed;
    };
    void purge(const mOrder *const order) {
      if (debug()) report(order, " purge ");
      orders.erase(order->orderId);
      if (debug()) report_size();
    };
    void purge() {
      for (const mOrder *const it : zombies)
        purge(it);
      zombies.clear();
    };
    const bool stillAlive(const mOrder &order) {
      if (order.orderStatus == mStatus::Waiting) {
        if (Tstamp - 10e+3 > order.time) {
          zombies.push_back(&order);
          return false;
        }
        ++calculon.countWaiting;
      } else ++calculon.countWorking;
      return order.preferPostOnly;
    };
    void read_from_gw(const mOrder &raw, mWalletPosition *const wallet, bool *const askForFees) {
      if (debug()) report(&raw, " reply ");
      if (raw.orderStatus == mStatus::Waiting)
        EXIT(error("OG", "Dataflow error (exchanges do not send waiting status!)"));
      mOrder *const order = upsert(raw);
      if (!order
        or order->orderStatus == mStatus::Waiting
      ) return;
      if (raw.tradeQuantity)
        tradesHistory.insert(order, raw.tradeQuantity);
      const mSide  lastSide  = order->side;
      const mPrice lastPrice = order->price;
      if (order->orderStatus == mStatus::Terminated)
        purge(order);
      wallet->reset(lastSide, calcHeldAmount(lastSide));
      if (raw.tradeQuantity) {
        wallet->target.safety.recentTrades.insert(lastSide, lastPrice, raw.tradeQuantity);
        wallet->target.safety.calc(tradesHistory);
        *askForFees = true;
      }
      send();
      refresh();
    };
    const mAmount calcHeldAmount(const mSide &side) const {
      return accumulate(orders.begin(), orders.end(), mAmount(),
        [&](mAmount held, const unordered_map<mRandId, mOrder>::value_type &it) {
          if (it.second.side == side and it.second.orderStatus == mStatus::Working)
            return held + (it.second.side == mSide::Ask
              ? it.second.quantity
              : it.second.quantity * it.second.price
            );
          else return held;
        }
      );
    };
    vector<mOrder*> working(const mSide &side = mSide::Both) {
      vector<mOrder*> workingOrders;
      for (unordered_map<mRandId, mOrder>::value_type &it : orders)
        if (mStatus::Working == it.second.orderStatus and (side == mSide::Both
          or (side == it.second.side and it.second.preferPostOnly)
        )) workingOrders.push_back(&it.second);
      return workingOrders;
    };
    const vector<mOrder> working(const bool &sorted = false) const {
      vector<mOrder> workingOrders;
      for (const unordered_map<mRandId, mOrder>::value_type &it : orders)
        if (mStatus::Working == it.second.orderStatus)
          workingOrders.push_back(it.second);
      if (sorted)
        sort(workingOrders.begin(), workingOrders.end(),
          [](const mOrder &a, const mOrder &b) {
            return a.price > b.price;
          }
        );
      return workingOrders;
    };
    const mMatter about() const {
      return mMatter::OrderStatusReports;
    };
    const bool realtime() const {
      return !qp.delayUI;
    };
    const json blob() const {
      return working();
    };
    private:
      void report(const mOrder *const order, const string &reason) const {
        print("DEBUG OG", " " + reason + " " + (
          order
            ? (order->side == mSide::Bid ? "BID id " : "ASK id ")
              + order->orderId + "::" + order->exchangeId
              + " [" + to_string((int)order->orderStatus) + "]: "
              + str8(order->quantity) + " " + args.base + " at price "
              + str8(order->price) + " " + args.quote
            : "not found"
        ));
      };
      void report_size() const {
        print("DEBUG OG", "memory " + to_string(orders.size()));
      };
      const bool debug() const {
        return args.debugOrders;
      };
  };
  static void to_json(json &j, const mBroker &k) {
    j = k.blob();
  };

  struct mNotepad: public mJsonToClient<mNotepad> {
    string content;
    void kiss(json *const j) {
      if (j->is_array() and j->size() and j->at(0).is_string())
        content = j->at(0);
    };
    const mMatter about() const {
      return mMatter::Notepad;
    };
  };
  static void to_json(json &j, const mNotepad &k) {
    j = k.content;
  };

  struct mText {
    static string oZip(string k) {
      z_stream zs;
      if (inflateInit2(&zs, -15) != Z_OK) return "";
      zs.next_in = (Bytef*)k.data();
      zs.avail_in = k.size();
      int ret;
      char outbuffer[32768];
      string k_;
      do {
        zs.avail_out = 32768;
        zs.next_out = (Bytef*)outbuffer;
        ret = inflate(&zs, Z_SYNC_FLUSH);
        if (k_.size() < zs.total_out)
          k_.append(outbuffer, zs.total_out - k_.size());
      } while (ret == Z_OK);
      inflateEnd(&zs);
      if (ret != Z_STREAM_END) return "";
      return k_;
    };
    static string oHex(string k) {
     unsigned int len = k.length();
      string k_;
      for (unsigned int i=0; i < len; i+=2) {
        string byte = k.substr(i, 2);
        char chr = (char)(int)strtol(byte.data(), NULL, 16);
        k_.push_back(chr);
      }
      return k_;
    };
    static string oB64(string k) {
      BIO *bio, *b64;
      BUF_MEM *bufferPtr;
      b64 = BIO_new(BIO_f_base64());
      bio = BIO_new(BIO_s_mem());
      bio = BIO_push(b64, bio);
      BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
      BIO_write(bio, k.data(), k.length());
      BIO_flush(bio);
      BIO_get_mem_ptr(bio, &bufferPtr);
      BIO_set_close(bio, BIO_NOCLOSE);
      BIO_free_all(bio);
      return string(bufferPtr->data, bufferPtr->length);
    };
    static string oB64decode(string k) {
      BIO *bio, *b64;
      char buffer[k.length()];
      b64 = BIO_new(BIO_f_base64());
      bio = BIO_new_mem_buf(k.data(), k.length());
      bio = BIO_push(b64, bio);
      BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
      BIO_set_close(bio, BIO_NOCLOSE);
      int len = BIO_read(bio, buffer, k.length());
      BIO_free_all(bio);
      return string(buffer, len);
    };
    static string oMd5(string k) {
      unsigned char digest[MD5_DIGEST_LENGTH];
      MD5((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
      char k_[16*2+1];
      for (unsigned int i = 0; i < 16; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return strU(k_);
    };
    static string oSha256(string k) {
      unsigned char digest[SHA256_DIGEST_LENGTH];
      SHA256((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
      char k_[SHA256_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return k_;
    };
    static string oSha512(string k) {
      unsigned char digest[SHA512_DIGEST_LENGTH];
      SHA512((unsigned char*)k.data(), k.length(), (unsigned char*)&digest);
      char k_[SHA512_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return k_;
    };
    static string oHmac1(string p, string s, bool hex = false) {
      unsigned char* digest;
      digest = HMAC(EVP_sha1(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
      char k_[SHA_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return hex ? oHex(k_) : k_;
    };
    static string oHmac256(string p, string s, bool hex = false) {
      unsigned char* digest;
      digest = HMAC(EVP_sha256(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
      char k_[SHA256_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return hex ? oHex(k_) : k_;
    };
    static string oHmac512(string p, string s, bool hex = false) {
      unsigned char* digest;
      digest = HMAC(EVP_sha512(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
      char k_[SHA512_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return hex ? oHex(k_) : k_;
    };
    static string oHmac384(string p, string s) {
      unsigned char* digest;
      digest = HMAC(EVP_sha384(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
      char k_[SHA384_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA384_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return k_;
    };
  };

  struct mREST {
    static json xfer(const string &url, const long &timeout = 13) {
      return curl_perform(url, [&](CURL *curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
      }, timeout == 13);
    };
    static json xfer(const string &url, const string &post) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.data());
      });
    };
    static json curl_perform(const string &url, function<void(CURL *curl)> curl_setopt, bool debug = true) {
      string reply;
      CURL *curl = curl_easy_init();
      if (curl) {
        curl_setopt(curl);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "K");
        curl_easy_setopt(curl, CURLOPT_CAINFO, "etc/cabundle.pem");
        curl_easy_setopt(curl, CURLOPT_INTERFACE, args.inet);
        curl_easy_setopt(curl, CURLOPT_URL, url.data());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &reply);
        CURLcode r = curl_easy_perform(curl);
        if (debug and r != CURLE_OK)
          reply = string("{\"error\":\"CURL Error: ") + curl_easy_strerror(r) + "\"}";
        curl_easy_cleanup(curl);
      }
      if (reply.empty() or (reply.at(0) != '{' and reply.at(0) != '[')) reply = "{}";
      return json::parse(reply);
    };
    private:
      static size_t curl_write(void *buf, size_t size, size_t nmemb, void *up) {
        ((string*)up)->append((char*)buf, size * nmemb);
        return size * nmemb;
      };
  };

  struct mRandom {
    static const unsigned long long int64() {
      static random_device rd;
      static mt19937_64 gen(rd());
      return uniform_int_distribution<unsigned long long>()(gen);
    };
    static const mRandId int45Id() {
      return to_string(int64()).substr(0, 10);
    };
    static const mRandId int32Id() {
      return to_string(int64()).substr(0,  8);
    };
    static const mRandId char16Id() {
      char s[16];
      for (unsigned int i = 0; i < 16; ++i)
        s[i] = numsAz[int64() % (sizeof(numsAz) - 1)];
      return string(s, 16);
    };
    static const mRandId uuid36Id() {
      string uuid = string(36, ' ');
      unsigned long long rnd = int64();
      unsigned long long rnd_ = int64();
      uuid[8] = '-';
      uuid[13] = '-';
      uuid[18] = '-';
      uuid[23] = '-';
      uuid[14] = '4';
      for (unsigned int i=0;i<36;i++)
        if (i != 8 && i != 13 && i != 18 && i != 14 && i != 23) {
          if (rnd <= 0x02) rnd = 0x2000000 + (rnd_ * 0x1000000) | 0;
          rnd >>= 4;
          uuid[i] = numsAz[(i == 19) ? ((rnd & 0xf) & 0x3) | 0x8 : rnd & 0xf];
        }
      return strL(uuid);
    };
    static const mRandId uuid32Id() {
      mRandId uuid = uuid36Id();
      uuid.erase(remove(uuid.begin(), uuid.end(), '-'), uuid.end());
      return uuid;
    }
  };

  struct mCommand {
    private:
      static string output(const string &cmd) {
        string data;
        FILE *stream = popen((cmd + " 2>&1").data(), "r");
        if (stream) {
          const int max_buffer = 256;
          char buffer[max_buffer];
          while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL)
              data += buffer;
          pclose(stream);
        }
        return data;
      };
    public:
      static string uname() {
        return output("uname -srvm");
      };
      static string ps() {
        return output("ps -p" + to_string(::getpid()) + " -orss | tail -n1");
      };
      static string netstat() {
        return output("netstat -anp 2>/dev/null | grep " + to_string(args.port));
      };
      static void stunnel(const bool &reboot) {
        int k = system("pkill stunnel || :");
        if (reboot) k = system("stunnel etc/stunnel.conf");
      };
      static bool git() {
        return
#ifdef NDEBUG
          access(".git", F_OK) != -1
#else
          false
#endif
        ;
      };
      static void fetch() {
        if (git()) int k = system("git fetch");
      };
      static string changelog() {
        return git()
          ? output("git --no-pager log --graph --oneline @..@{u}")
          : "";
      };
      static bool deprecated() {
        return git()
          ? output("git rev-parse @") != output("git rev-parse @{u}")
          : false;
      };
  };

  struct mMonitor: public mJsonToClient<mMonitor> {
    unsigned int /* ( | L | ) */ /* more */ orders_60s /* ? */;
          string /*  )| O |(  */    unlock;
        mProduct /* ( | C | ) */ /* this */ product;
                 /*  )| K |(  */ /* thanks! <3 */
    mMonitor()
      : orders_60s(0)
    {};
    const unsigned int dbSize() const {
      if (args.database == ":memory:") return 0;
      struct stat st;
      return stat(args.database.data(), &st) ? 0 : st.st_size;
    };
    const unsigned int memSize() const {
      string ps = mCommand::ps();
      ps.erase(remove(ps.begin(), ps.end(), ' '), ps.end());
      return ps.empty() ? 0 : stoi(ps) * 1e+3;
    };
    void tick_orders() {
      orders_60s++;
    };
    void timer_60s() {
      send();
      orders_60s = 0;
    };
    const mMatter about() const {
      return mMatter::ApplicationState;
    };
  };
  static void to_json(json &j, const mMonitor &k) {
    j = {
      {     "a", k.unlock                        },
      {  "inet", string(args.inet ?: "")         },
      {  "freq", k.orders_60s                    },
      { "theme", args.ignoreMoon + args.ignoreSun},
      {"memory", k.memSize()                     },
      {"dbsize", k.dbSize()                      }
    };
  };
}

#endif
