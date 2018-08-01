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

#define TRUEONCE(k) (k ? !(k = !k) : k)

namespace K {
  enum class mConnectivity: unsigned int { Disconnected, Connected };
  enum class mStatus: unsigned int { New, Working, Complete, Cancelled };
  enum class mSide: unsigned int { Bid, Ask, Both };
  enum class mTimeInForce: unsigned int { IOC, FOK, GTC };
  enum class mOrderType: unsigned int { Limit, Market };
  enum class mPingAt: unsigned int { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings };
  enum class mPongAt: unsigned int { ShortPingFair, AveragePingFair, LongPingFair, ShortPingAggressive, AveragePingAggressive, LongPingAggressive };
  enum class mQuotingMode: unsigned int { Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth };
  enum class mQuotingSafety: unsigned int { Off, PingPong, Boomerang, AK47 };
  enum class mQuoteState: unsigned int { Disconnected, Live, DisabledQuotes, MissingData, UnknownHeld, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed, UpTrendHeld, DownTrendHeld };
  enum class mFairValueModel: unsigned int { BBO, wBBO , rwBBO };
  enum class mAutoPositionMode: unsigned int { Manual, EWMA_LS, EWMA_LMS, EWMA_4 };
  enum class mPDivMode: unsigned int { Manual, Linear, Sine, SQRT, Switch};
  enum class mAPR: unsigned int { Off, Size, SizeWidth };
  enum class mSOP: unsigned int { Off, Trades, Size, TradesSize };
  enum class mSTDEV: unsigned int { Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff };

  enum class mHotkey: int { ESC = 27, Q = 81, q = 113 };

  enum class mPortal: unsigned char { Hello = '=', Kiss = '-' };
  enum class mMatter: unsigned char {
    FairValue            = 'a', Quote                = 'b', ActiveSubscription = 'c', Connectivity       = 'd', MarketData       = 'e',
    QuotingParameters    = 'f', SafetySettings       = 'g', Product            = 'h', OrderStatusReports = 'i',
    ProductAdvertisement = 'j', ApplicationState     = 'k', EWMAStats          = 'l', STDEVStats         = 'm',
    Position             = 'n', Profit               = 'o', SubmitNewOrder     = 'p', CancelOrder        = 'q', MarketTrade      = 'r',
    Trades               = 's', ExternalValuation    = 't', QuoteStatus        = 'u', TargetBasePosition = 'v', TradeSafetyValue = 'w',
    CancelAllOrders      = 'x', CleanAllClosedTrades = 'y', CleanAllTrades     = 'z', CleanTrade         = 'A',
                                WalletChart          = 'C', MarketChart        = 'D', Notepad            = 'E', MarketDataLongTerm = 'H'
  };

  static          bool operator! (mConnectivity k_)                   { return !(unsigned int)k_; };
  static mConnectivity operator* (mConnectivity _k, mConnectivity k_) { return (mConnectivity)((unsigned int)_k * (unsigned int)k_); };

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
#ifdef _WIN32
        naked = 1;
#endif
      };
  } args;

  struct mAbout {
    virtual const mMatter about() const = 0;
  };
  struct mDump: virtual public mAbout {
    virtual const json dump() const = 0;
  };

  struct mFromClient: virtual public mAbout {
    virtual const json kiss(const json &j) {
      return j;
    };
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

  struct mToClient: public mDump,
                    public mFromClient  {
    function<void()> send
#ifndef NDEBUG
    = []() { WARN("Y U NO catch client send?"); }
#endif
    ;
    virtual const json hello() {
      return { dump() };
    };
    virtual const bool realtime() const {
      return true;
    };
  };
  template <typename mData> struct mJsonToClient: public mToClient {
    void send() {
      if (send_asap() or send_soon())
        send_now();
    };
    virtual const json dump() const {
      return *(mData*)this;
    };
    protected:
      mClock send_Tstamp = 0;
      virtual const bool send_asap() const {
        return true;
      };
      const bool send_soon(const int &delay = 0) {
        if (send_Tstamp + max(369, delay) > Tstamp) return false;
        send_Tstamp = Tstamp;
        return true;
      };
      void send_now() const {
        if (mToClient::send)
          mToClient::send();
      };
  };

  struct mFromDb: public mDump {
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
    virtual const json dump() const {
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
    virtual const json dump() const {
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
    const json kiss(const json &j) {
      mQuotingParams prev = *this; // just need to copy the 6 prev.* vars above, noob
      from_json(j, *this);
      diff(prev);
      push();
      send();
      return j;
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
           mSide side           = (mSide)0;
          mPrice price          = 0;
         mAmount quantity       = 0,
                 tradeQuantity  = 0;
      mOrderType type           = (mOrderType)0;
    mTimeInForce timeInForce    = (mTimeInForce)0;
         mStatus orderStatus    = (mStatus)0;
            bool isPong         = false,
                 preferPostOnly = false;
          mClock time           = 0,
                 latency        = 0,
                 _waitingCancel = 0;
    mOrder()
    {};
    mOrder(mRandId o, mStatus s):
      orderId(o), orderStatus(s)
    {};
    mOrder(mRandId o, mRandId e, mStatus s, mPrice p, mAmount q, mAmount Q):
      orderId(o), exchangeId(e), quantity(q), price(p), orderStatus(s), tradeQuantity(Q)
    {};
    mOrder(mRandId o, mSide S, mPrice p, mAmount q, mOrderType t, bool i, mTimeInForce F):
      orderId(o), side(S), price(p), quantity(q), type(t), isPong(i), timeInForce(F), orderStatus(mStatus::New), preferPostOnly(true)
    {};
    void update(const mOrder &raw) {
      orderStatus = raw.orderStatus;
      if (!raw.exchangeId.empty()) exchangeId = raw.exchangeId;
      if (raw.price)               price      = raw.price;
      if (raw.quantity)            quantity   = raw.quantity;
      if (raw.time)                time       = raw.time;
      if (raw.latency)             latency    = raw.latency;
      if (!time)                   time       = Tstamp;
      if (!latency and orderStatus == mStatus::Working)
                                   latency    = Tstamp - time;
      if (latency)                 time       = Tstamp;
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
    k.type           = j.value("orderType", "") == "Limit"
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
    mTrade(mPrice p, mAmount q, mSide s, mClock t):
      side(s), price(p), quantity(q), time(t)
    {};
    mTrade(string i, mPrice p, mAmount q, mSide S, bool P, mClock t, mAmount v, mClock Kt, mAmount Kq, mPrice Kp, mAmount Kv, mAmount Kd, mAmount f, bool l):
      tradeId(i), side(S), price(p), Kprice(Kp), quantity(q), value(v), Kqty(Kq), Kvalue(Kv), Kdiff(Kd), feeCharged(f), time(t), Ktime(Kt), isPong(P), loadedFromDB(l)
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
    mMarketTakers()
    {};
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
    void send_push_back(const mTrade &row) {
      trades.push_back(row);
      send();
    };
    const mMatter about() const {
      return mMatter::MarketTrade;
    };
    const json dump() const {
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
    void clearOne(const json &butterfly) {
      if (!butterfly.is_string()) return;
      const string &tradeId = butterfly.get<string>();
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
    const json dump() const {
      if (crbegin()->Kqty == -1) return nullptr;
      else return mVectorFromDb::dump();
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
    mRecentTrade()
    {};
    mRecentTrade(mPrice p, mAmount q):
      price(p), quantity(q), time(Tstamp)
    {};
  };
  struct mRecentTrades {
    multimap<mPrice, mRecentTrade> buys,
                                   sells;
                           mAmount sumBuys       = 0,
                                   sumSells      = 0;
                            mPrice lastBuyPrice  = 0,
                                   lastSellPrice = 0;
    mRecentTrades()
    {};
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
    const mPrice *const fv;
    mFairLevelsPrice(const mPrice *const f):
      fv(f)
    {};
    const mMatter about() const {
      return mMatter::FairValue;
    };
    const bool realtime() const {
      return !qp.delayUI;
    };
    const bool ratelimit(const mPrice &prev) const {
      return *fv == prev;
    };
    void send_ratelimit(const mPrice &prev) {
      if (ratelimit(prev)) return;
      send();
      refresh();
    };
    const bool send_asap() const {
      return false;
    };
  };
  static void to_json(json &j, const mFairLevelsPrice &k) {
    j = {
      {"price", *k.fv}
    };
  };

  struct mStdev {
    mPrice fv     = 0,
           topBid = 0,
           topAsk = 0;
    mStdev()
    {};
    mStdev(mPrice f, mPrice b, mPrice a):
      fv(f), topBid(b), topAsk(a)
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
    double top  = 0,  topMean = 0,
           fair = 0, fairMean = 0,
           bid  = 0,  bidMean = 0,
           ask  = 0,  askMean = 0;
    mStdevs()
    {};
    const bool pull(const json &j) {
      const bool loaded = mVectorFromDb::pull(j);
      if (loaded) calc();
      return loaded;
    };
    void timer_1s(const mPrice &fv, const mPrice &topBid, const mPrice &topAsk) {
      push_back(mStdev(fv, topBid, topAsk));
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
    mEwma()
    {};
    void timer_60s(const mPrice &fv, const mPrice &averageWidth) {
      prepareHistory(fv);
      calcProtections(fv, averageWidth);
      calcPositions(fv);
      calcTargetPositionAutoPercentage();
      push();
    };
    void prepareHistory(const mPrice &fv) {
      fairValue96h.push_back(fv);
    };
    void calc(mPrice *const mean, const unsigned int &periods, const mPrice &value) {
      if (*mean) {
        double alpha = 2.0 / (periods + 1);
        *mean = alpha * value + (1 - alpha) * *mean;
      } else *mean = value;
    };
    void calcFromHistory(mPrice *const mean, const unsigned int &periods, const string &name) {
      unsigned int n = fairValue96h.size();
      if (!n) return;
      unsigned int x = 0;
      *mean = fairValue96h.front();
      while (n--) calc(mean, periods, fairValue96h.at(++x));
      print("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
    };
    void calcFromHistory() {
      if (TRUEONCE(qp._diffVLEP)) calcFromHistory(&mgEwmaVL, qp.veryLongEwmaPeriods,   "VeryLong");
      if (TRUEONCE(qp._diffLEP))  calcFromHistory(&mgEwmaL,  qp.longEwmaPeriods,       "Long");
      if (TRUEONCE(qp._diffMEP))  calcFromHistory(&mgEwmaM,  qp.mediumEwmaPeriods,     "Medium");
      if (TRUEONCE(qp._diffSEP))  calcFromHistory(&mgEwmaS,  qp.shortEwmaPeriods,      "Short");
      if (TRUEONCE(qp._diffXSEP)) calcFromHistory(&mgEwmaXS, qp.extraShortEwmaPeriods, "ExtraShort");
      if (TRUEONCE(qp._diffUEP))  calcFromHistory(&mgEwmaU,  qp.ultraShortEwmaPeriods, "UltraShort");
    };
    void calcProtections(const mPrice &fv, const mPrice &averageWidth) {
      calc(&mgEwmaP, qp.protectionEwmaPeriods, fv);
      calc(&mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
    };
    void calcPositions(const mPrice &fv) {
      calc(&mgEwmaVL, qp.veryLongEwmaPeriods,   fv);
      calc(&mgEwmaL,  qp.longEwmaPeriods,       fv);
      calc(&mgEwmaM,  qp.mediumEwmaPeriods,     fv);
      calc(&mgEwmaS,  qp.shortEwmaPeriods,      fv);
      calc(&mgEwmaXS, qp.extraShortEwmaPeriods, fv);
      calc(&mgEwmaU,  qp.ultraShortEwmaPeriods, fv);
      if (mgEwmaXS and mgEwmaU)
        mgEwmaTrendDiff = ((mgEwmaU * 1e+2) / mgEwmaXS) - 1e+2;
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
    mMarketStats(const mPrice *const f):
       fairPrice(f)
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
      {     "fairValue", *k.fairPrice.fv                },
      { "tradesBuySize", k.takerTrades.takersBuySize60s },
      {"tradesSellSize", k.takerTrades.takersSellSize60s}
    };
  };

  struct mLevel {
     mPrice price = 0;
    mAmount size  = 0;
    mLevel()
    {};
    mLevel(mPrice p, mAmount s):
      price(p), size(s)
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
    mLevels(vector<mLevel> b, vector<mLevel> a):
      bids(b), asks(a)
    {};
    mPrice spread() const {
      return empty() ? 0 : asks.begin()->price - bids.begin()->price;
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
  struct mQuote {
    mLevel bid,
           ask;
      bool isBidPong = false,
           isAskPong = false;
    mQuote(mLevel b, mLevel a):
      bid(b), ask(a)
    {};
    mQuote(mLevel b, mLevel a, bool bP, bool aP):
      bid(b), ask(a), isBidPong(bP), isAskPong(aP)
    {};
  };
  static void to_json(json &j, const mQuote &k) {
    j = {
      {"bid", k.bid},
      {"ask", k.ask}
    };
  };
  struct mDummyMarketLevels: public mToScreen {
    const vector<mLevel> *const bids      = nullptr,
                         *const asks      = nullptr;
            const mPrice *const fairValue = nullptr;
    mDummyMarketLevels(const vector<mLevel> *const b, const vector<mLevel> *const a, const mPrice *const f):
      bids(b), asks(a), fairValue(f)
    {};
  };
  struct mDummyMarketMaker: public mToScreen {
    private:
      mDummyMarketLevels levels;
      mQuote (*calcRawQuoteFromMarket)(
        const mDummyMarketLevels&,
        const mPrice&,
        const mPrice&,
        const mAmount&,
        const mAmount&
      ) = nullptr;
    public:
      mDummyMarketMaker(const vector<mLevel> *const b, const vector<mLevel> *const a, const mPrice *const f):
        levels(b, a, f)
      {};
      void reset(const string &reason) {
        if (qp.mode == mQuotingMode::Top)              calcRawQuoteFromMarket = calcTopOfMarket;
        else if (qp.mode == mQuotingMode::Mid)         calcRawQuoteFromMarket = calcMidOfMarket;
        else if (qp.mode == mQuotingMode::Join)        calcRawQuoteFromMarket = calcJoinMarket;
        else if (qp.mode == mQuotingMode::InverseJoin) calcRawQuoteFromMarket = calcInverseJoinMarket;
        else if (qp.mode == mQuotingMode::InverseTop)  calcRawQuoteFromMarket = calcInverseTopOfMarket;
        else if (qp.mode == mQuotingMode::HamelinRat)  calcRawQuoteFromMarket = calcColossusOfMarket;
        else if (qp.mode == mQuotingMode::Depth)       calcRawQuoteFromMarket = calcDepthOfMarket;
        else EXIT(error("QE", "Invalid quoting mode "
          + reason + ", consider to remove the database file"));
      };
      mQuote calcRawQuote(const mPrice &minTick, const mPrice &price, const mAmount &bidSize, const mAmount &askSize) {
        return calcRawQuoteFromMarket(levels, minTick, price, bidSize, askSize);
      };
    private:
      static mQuote quoteAtTopOfMarket(const mDummyMarketLevels &levels, const mPrice &minTick) {
        mLevel topBid = levels.bids->begin()->size > minTick
          ? levels.bids->at(0)
          : levels.bids->at(levels.bids->size() > 1 ? 1 : 0);
        mLevel topAsk = levels.asks->begin()->size > minTick
          ? levels.asks->at(0)
          : levels.asks->at(levels.asks->size() > 1 ? 1 : 0);
        return mQuote(topBid, topAsk);
      };
      static mQuote calcJoinMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket(levels, minTick);
        k.bid.price = fmin(*levels.fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(*levels.fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      static mQuote calcTopOfMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket(levels, minTick);
        k.bid.price = k.bid.price + minTick;
        k.ask.price = k.ask.price - minTick;
        k.bid.price = fmin(*levels.fairValue - widthPing / 2.0, k.bid.price);
        k.ask.price = fmax(*levels.fairValue + widthPing / 2.0, k.ask.price);
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      static mQuote calcInverseJoinMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket(levels, minTick);
        mPrice mktWidth = abs(k.ask.price - k.bid.price);
        if (mktWidth > widthPing) {
          k.ask.price = k.ask.price + widthPing;
          k.bid.price = k.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k.ask.price = k.ask.price + widthPing / 4.0;
          k.bid.price = k.bid.price - widthPing / 4.0;
        }
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      static mQuote calcInverseTopOfMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket(levels, minTick);
        mPrice mktWidth = abs(k.ask.price - k.bid.price);
        if (mktWidth > widthPing) {
          k.ask.price = k.ask.price + widthPing;
          k.bid.price = k.bid.price - widthPing;
        }
        k.bid.price = k.bid.price + minTick;
        k.ask.price = k.ask.price - minTick;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          k.ask.price = k.ask.price + widthPing / 4.0;
          k.bid.price = k.bid.price - widthPing / 4.0;
        }
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      static mQuote calcMidOfMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        return mQuote(
          mLevel(fmax(*levels.fairValue - widthPing, 0), bidSize),
          mLevel(*levels.fairValue + widthPing, askSize)
        );
      };
      static mQuote calcColossusOfMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mPrice &widthPing, const mAmount &bidSize, const mAmount &askSize) {
        mQuote k = quoteAtTopOfMarket(levels, minTick);
        k.bid.size = 0;
        k.ask.size = 0;
        for (const mLevel &it : *levels.bids)
          if (k.bid.size < it.size and it.price <= k.bid.price) {
            k.bid.size = it.size;
            k.bid.price = it.price;
          }
        for (const mLevel &it : *levels.asks)
          if (k.ask.size < it.size and it.price >= k.ask.price) {
            k.ask.size = it.size;
            k.ask.price = it.price;
          }
        if (k.bid.size) k.bid.price += minTick;
        if (k.ask.size) k.ask.price -= minTick;
        k.bid.size = bidSize;
        k.ask.size = askSize;
        return k;
      };
      static mQuote calcDepthOfMarket(const mDummyMarketLevels &levels, const mPrice &minTick, const mAmount &depth, const mAmount &bidSize, const mAmount &askSize) {
        mPrice bidPx = levels.bids->begin()->price;
        mAmount bidDepth = 0;
        for (const mLevel &it : *levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        mPrice askPx = levels.asks->begin()->price;
        mAmount askDepth = 0;
        for (const mLevel &it : *levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        return mQuote(
          mLevel(bidPx, bidSize),
          mLevel(askPx, askSize)
        );
      };
  };
  struct mLevelsDiff: public mLevels,
                      public mJsonToClient<mLevelsDiff> {
                    bool patched;
    const mLevels *const unfiltered;
    mLevelsDiff(const mLevels *const c):
      patched(false), unfiltered(c)
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
        return unfiltered->empty() or empty()
          or !send_soon(qp.delayUI * 1e+3);
      };
      void reset() {
        bids = unfiltered->bids;
        asks = unfiltered->asks;
        patched = false;
      };
      void diff() {
        bids = diff(bids, unfiltered->bids);
        asks = diff(asks, unfiltered->asks);
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
       mDummyMarketMaker dummyMM;
            mMarketStats stats;
    map<mPrice, mAmount> filterBidOrders,
                         filterAskOrders;
            unsigned int averageCount = 0;
                  mPrice averageWidth = 0,
                         fairValue    = 0;
    mMarketLevels():
      diff(&unfiltered), dummyMM(&bids, &asks, &fairValue), stats(&fairValue)
    {};
    void timer_1s() {
      stats.stdev.timer_1s(fairValue, bids.cbegin()->price, asks.cbegin()->price);
    };
    void timer_60s() {
      stats.takerTrades.timer_60s();
      stats.ewma.timer_60s(fairValue, resetAverageWidth());
      stats.send();
    };
    const bool warn_empty() const {
      const bool err = empty();
      if (err) stats.fairPrice.warn("QE", "Unable to calculate quote, missing market data");
      return err;
    };
    void calcFairValue(const mPrice &minTick) {
      mPrice prev = fairValue;
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
      if (fairValue) fairValue = round(fairValue / minTick) * minTick;
      stats.fairPrice.send_ratelimit(prev);
    };
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
    void reset(const mLevels &next) {
      bids = unfiltered.bids = next.bids;
      asks = unfiltered.asks = next.asks;
    };
    void filter(vector<mLevel> *const levels, map<mPrice, mAmount> orders) {
      for (vector<mLevel>::iterator it = levels->begin(); it != levels->end();) {
        for (map<mPrice, mAmount>::iterator it_ = orders.begin(); it_ != orders.end();)
          if (it->price == it_->first) {
            it->size -= it_->second;
            orders.erase(it_);
            break;
          } else ++it_;
        if (!it->size) it = levels->erase(it);
        else ++it;
        if (orders.empty()) break;
      }
    };
    void send_reset_filter(const mLevels &next, const mPrice &minTick) {
      reset(next);
      if (!filterBidOrders.empty()) filter(&bids, filterBidOrders);
      if (!filterAskOrders.empty()) filter(&asks, filterAskOrders);
      calcFairValue(minTick);
      calcAverageWidth();
      diff.send_reset();
    };
  };

  struct mProfit {
    mAmount baseValue  = 0,
            quoteValue = 0;
     mClock time       = 0;
    mProfit()
    {};
    mProfit(mAmount b, mAmount q):
      baseValue(b), quoteValue(q), time(Tstamp)
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
    const double calcBase() const {
      return calcDiffPercent(
        cbegin()->baseValue,
        crbegin()->baseValue
      );
    };
    const double calcQuote() const {
      return calcDiffPercent(
        cbegin()->quoteValue,
        crbegin()->quoteValue
      );
    };
    const double calcDiffPercent(mAmount older, mAmount newer) const {
      return ((newer - older) / newer) * 1e+2;
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
    const mAmount *const baseValue          = nullptr,
                  *const baseTotal          = nullptr,
                  *const targetBasePosition = nullptr;
    mSafety(const mAmount *const b, const mAmount *const t, const mAmount *const p):
      baseValue(b), baseTotal(t), targetBasePosition(p)
    {};
    void timer_1s(const mMarketLevels &levels, const mTradesCompleted &tradesHistory) {
      calc(levels, tradesHistory);
    };
    void calc(const mMarketLevels &levels, const mTradesCompleted &tradesHistory) {
      if (!*baseValue or levels.empty()) return;
      double prev_combined = combined;
      mPrice prev_buyPing  = buyPing,
             prev_sellPing = sellPing;
      calcSizes();
      calcPrices(levels.fairValue, tradesHistory);
      recentTrades.reset();
      if (empty()) return;
      buy  = recentTrades.sumBuys / buySize;
      sell = recentTrades.sumSells / sellSize;
      combined = (recentTrades.sumBuys + recentTrades.sumSells) / (buySize + sellSize);
      if (!ratelimit(prev_combined, prev_buyPing, prev_sellPing))
        send();
    };
    const bool empty() const {
      return !*baseValue or !buySize or !sellSize;
    };
    const bool ratelimit(const double &prev_combined, const mPrice &prev_buyPing, const mPrice &prev_sellPing) const {
      return combined == prev_combined
        and buyPing == prev_buyPing
        and sellPing == prev_sellPing;
    };
    const mMatter about() const {
      return mMatter::TradeSafetyValue;
    };
    private:
      void calcPrices(const mPrice &fv, const mTradesCompleted &tradesHistory) {
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
            ? qp.widthPongPercentage * fv / 100
            : qp.widthPong;
          mAmount buyQty = 0,
                  sellQty = 0;
          if (qp.pongAt == mPongAt::ShortPingFair or qp.pongAt == mPongAt::ShortPingAggressive) {
            matchBestPing(fv, &tradesBuy, &buyPing, &buyQty, sellSize, widthPong, true);
            matchBestPing(fv, &tradesSell, &sellPing, &sellQty, buySize, widthPong);
            if (!buyQty) matchFirstPing(fv, &tradesBuy, &buyPing, &buyQty, sellSize, widthPong*-1, true);
            if (!sellQty) matchFirstPing(fv, &tradesSell, &sellPing, &sellQty, buySize, widthPong*-1);
          } else if (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive) {
            matchLastPing(fv, &tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
            matchLastPing(fv, &tradesSell, &sellPing, &sellQty, buySize, widthPong, true);
          } else if (qp.pongAt == mPongAt::AveragePingFair or qp.pongAt == mPongAt::AveragePingAggressive) {
            matchAllPing(fv, &tradesBuy, &buyPing, &buyQty, sellSize, widthPong);
            matchAllPing(fv, &tradesSell, &sellPing, &sellQty, buySize, widthPong);
          }
          if (buyQty) buyPing /= buyQty;
          if (sellQty) sellPing /= sellQty;
        }
      };
      void matchFirstPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, true, fv, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(true, false, fv, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        matchPing(false, true, fv, trades, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width) {
        matchPing(false, false, fv, trades, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, mPrice fv, map<mPrice, mTrade> *trades, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (map<mPrice, mTrade>::reverse_iterator it = trades->rbegin(); it != trades->rend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fv, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (map<mPrice, mTrade>::iterator it = trades->begin(); it != trades->end(); ++it)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fv, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
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
            ? qp.sellSizePercentage * *baseValue / 1e+2
            : qp.sellSize;
        buySize = qp.percentageValues
          ? qp.buySizePercentage * *baseValue / 1e+2
          : qp.buySize;
        if (qp.aggressivePositionRebalancing == mAPR::Off) return;
        if (qp.buySizeMax)
          buySize = fmax(buySize, *targetBasePosition - *baseTotal);
        if (qp.sellSizeMax)
          sellSize = fmax(sellSize, *baseTotal - *targetBasePosition);
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
     string sideAPR,
            sideAPRDiff = "!=";
    mSafety safety;
    const mAmount *const baseValue = nullptr;
    mTarget(const mAmount *const b, const mAmount *const t):
      safety(b, t, &targetBasePosition), baseValue(b)
    {};
    void calcPDiv() {
      mAmount pDiv = qp.percentageValues
        ? qp.positionDivergencePercentage * *baseValue / 1e+2
        : qp.positionDivergence;
      if (qp.autoPositionMode == mAutoPositionMode::Manual or mPDivMode::Manual == qp.positionDivergenceMode)
        positionDivergence = pDiv;
      else {
        mAmount pDivMin = qp.percentageValues
          ? qp.positionDivergencePercentageMin * *baseValue / 1e+2
          : qp.positionDivergenceMin;
        double divCenter = 1 - abs((targetBasePosition / *baseValue * 2) - 1);
        if (mPDivMode::Linear == qp.positionDivergenceMode)      positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
        else if (mPDivMode::Sine == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
        else if (mPDivMode::SQRT == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
        else if (mPDivMode::Switch == qp.positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
      }
    };
    void calcTargetBasePos(const double &targetPositionAutoPercentage) { // PRETTY_DEBUG plz
      if (warn_empty()) return;
      mAmount next = qp.autoPositionMode == mAutoPositionMode::Manual
        ? (qp.percentageValues
          ? qp.targetBasePositionPercentage * *baseValue / 1e+2
          : qp.targetBasePosition)
        : targetPositionAutoPercentage * *baseValue / 1e+2;
      if (ratelimit(next)) return;
      targetBasePosition = next;
      sideAPRDiff = sideAPR;
      calcPDiv();
      push();
      send();
      if (args.debugWallet)
        print("PG", "TBP: "
          + to_string((int)(targetBasePosition / *baseValue * 1e+2)) + "% = " + str8(targetBasePosition)
          + " " + args.base + ", pDiv: "
          + to_string((int)(positionDivergence / *baseValue * 1e+2)) + "% = " + str8(positionDivergence)
          + " " + args.base);
    };
    const bool ratelimit(const mAmount &next) const {
      return (targetBasePosition and abs(targetBasePosition - next) < 1e-4 and sideAPR == sideAPRDiff);
    };
    const bool warn_empty() const {
      const bool err = empty();
      if (err) warn("PG", "Unable to calculate TBP, missing wallet data");
      return err;
    };
    const bool empty() const {
      return !baseValue or !*baseValue;
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
  };
  static void to_json(json &j, const mTarget &k) {
    j = {
      {    "tbp", k.targetBasePosition},
      {   "pDiv", k.positionDivergence},
      {"sideAPR", k.sideAPR           }
    };
  };
  static void from_json(const json &j, mTarget &k) {
    k.targetBasePosition = j.value("tbp", 0.0);
    k.positionDivergence = j.value("pDiv", 0.0);
    k.sideAPR            = j.value("sideAPR", "");
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
    mWallet(mAmount a, mAmount h, mCoinId c):
      amount(a), held(h), currency(c)
    {};
    void reset(const mAmount &a, const mAmount &h) {
      if (empty()) return;
      total = (amount = a) + (held = h);
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
    mWallets(mWallet b, mWallet q):
      base(b), quote(q)
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
    mWalletPosition():
      target(&base.value, &base.total)
    {};
    void reset(const mWallets &next, const mMarketLevels &levels) {
      if (next.empty()) return;
      mWallet prevBase = base,
              prevQuote = quote;
      base.currency = next.base.currency;
      quote.currency = next.quote.currency;
      base.reset(next.base.amount, next.base.held);
      quote.reset(next.quote.amount, next.quote.held);
      send_ratelimit(levels, prevBase, prevQuote);
    };
    void reset(const mSide &side, const mAmount &nextHeldAmount, const mMarketLevels &levels) {
      mWallet prevBase = base,
              prevQuote = quote;
      if (side == mSide::Ask)
        base.reset(base.total - nextHeldAmount, nextHeldAmount);
      else quote.reset(quote.total - nextHeldAmount, nextHeldAmount);
      send_ratelimit(levels, prevBase, prevQuote);
    };
    void calcValues(const mPrice &fv) {
      if (!fv) return;
      if (args.maxWallet) calcMaxWallet(fv);
      base.value = quote.total / fv + base.total;
      quote.value = base.total * fv + quote.total;
      calcProfits();
    };
    void calcProfits() {
      if (!profits.ratelimit())
        profits.push_back(mProfit(base.value, quote.value));
      base.profit  = profits.calcBase();
      quote.profit = profits.calcQuote();
    };
    void calcMaxWallet(const mPrice &fv) {
      mAmount maxWallet = args.maxWallet;
      maxWallet -= quote.held / fv;
      if (maxWallet > 0 and quote.amount / fv > maxWallet) {
        quote.amount = maxWallet * fv;
        maxWallet = 0;
      } else maxWallet -= quote.amount / fv;
      maxWallet -= base.held;
      if (maxWallet > 0 and base.amount > maxWallet)
        base.amount = maxWallet;
    };
    void send_ratelimit(const mMarketLevels &levels) {
      send_ratelimit(levels, base, quote);
    };
    void send_ratelimit(const mMarketLevels &levels, const mWallet &prevBase, const mWallet &prevQuote) {
      if (empty() or levels.empty()) return;
      calcValues(levels.fairValue);
      target.calcTargetBasePos(levels.stats.ewma.targetPositionAutoPercentage);
      if (!ratelimit(prevBase, prevQuote))
        send();
    };
    const bool ratelimit(const mWallet &prevBase, const mWallet &prevQuote) const {
      return (abs(base.value - prevBase.value) < 2e-6
        and abs(quote.value - prevQuote.value) < 2e-2
        and abs(base.amount - prevBase.amount) < 2e-6
        and abs(quote.amount - prevQuote.amount) < 2e-2
        and abs(base.held - prevBase.held) < 2e-6
        and abs(quote.held - prevQuote.held) < 2e-2
        and abs(base.profit - prevBase.profit) < 2e-2
        and abs(quote.profit - prevQuote.profit) < 2e-2
      );
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
  };

  struct mButtonSubmitNewOrder: public mFromClient {
    const mMatter about() const {
      return mMatter::SubmitNewOrder;
    };
  };
  struct mButtonCancelOrder: public mFromClient {
    const json kiss(const json &j) {
      json butterfly;
      if (j.is_object() and j.at("orderId").is_string())
        butterfly = j.at("orderId");
      return butterfly;
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
    const json kiss(const json &j) {
      json butterfly;
      if (j.is_object() and j.at("tradeId").is_string())
        butterfly = j.at("tradeId");
      return butterfly;
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

  struct mBroker: public mToScreen,
                  public mJsonToClient<mBroker> {
    map<mRandId, mOrder> orders;
        mTradesCompleted tradesHistory;
    mOrder *const find(const mRandId &orderId) {
      return (orderId.empty()
        or orders.find(orderId) == orders.end()
      ) ? nullptr
        : &orders.at(orderId);
    };
    mOrder *const findsert(const mOrder &raw) {
      if (raw.orderStatus == mStatus::New and !raw.orderId.empty())
        return &(orders[raw.orderId] = raw);
      if (raw.orderId.empty() and !raw.exchangeId.empty()) {
        map<mRandId, mOrder>::iterator it = find_if(
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
    mOrder *const upsert(const mOrder &raw, const bool &place = true) {
      mOrder *const order = findsert(raw);
      if (!order) return nullptr;
      order->update(raw);
      if (debug()) {
        report(order);
        report_size();
        if (place) report_place(order);
      }
      return order;
    };
    void upsert(const mOrder &raw, mWalletPosition *const wallet, const mMarketLevels &levels, bool *const askForFees) {
      if (debug()) report(raw);
      mOrder *const order = upsert(raw, false);
      if (!order) return;
      if (raw.tradeQuantity)
        tradesHistory.insert(order, raw.tradeQuantity);
      mSide  lastSide  = order->side;
      mPrice lastPrice = order->price;
      if (mStatus::Cancelled == order->orderStatus
        or mStatus::Complete == order->orderStatus
      ) erase(order->orderId);
      if (raw.orderStatus == mStatus::New) return;
      wallet->reset(lastSide, calcHeldAmount(lastSide), levels);
      if (raw.tradeQuantity) {
        wallet->target.safety.recentTrades.insert(lastSide, lastPrice, raw.tradeQuantity);
        wallet->target.safety.calc(levels, tradesHistory);
        *askForFees = true;
      }
      send();
      refresh();
    };
    const bool replace(mOrder *const toReplace, const mPrice &price, const bool &isPong) {
      if (!toReplace
        or toReplace->exchangeId.empty()
      ) return false;
      toReplace->price  = price;
      toReplace->isPong = isPong;
      if (debug()) report_replace(toReplace);
      return true;
    };
    const bool cancel(mOrder *const toCancel) {
      if (!toCancel
        or toCancel->exchangeId.empty()
        or toCancel->_waitingCancel + 3e+3 > Tstamp
      ) return false;
      toCancel->_waitingCancel = Tstamp;
      if (debug()) report_cancel(toCancel);
      return true;
    };
    void erase(const mRandId &orderId) {
      if (debug()) print("DEBUG OG", "remove " + orderId);
      map<mRandId, mOrder>::iterator it = orders.find(orderId);
      if (it != orders.end()) orders.erase(it);
      if (debug()) report_size();
    };
    const mAmount calcHeldAmount(const mSide &side) const {
      return accumulate(orders.begin(), orders.end(), mAmount(),
        [&](mAmount held, const map<mRandId, mOrder>::value_type &it) {
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
      for (map<mRandId, mOrder>::value_type &it : orders)
        if (mStatus::Working == it.second.orderStatus and (side == mSide::Both
          or (side == it.second.side and it.second.preferPostOnly)
        )) workingOrders.push_back(&it.second);
      return workingOrders;
    };
    const vector<mOrder> working(const bool &sorted = false) const {
      vector<mOrder> workingOrders;
      for (const map<mRandId, mOrder>::value_type &it : orders)
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
    const json dump() const {
      return working();
    };
    private:
      const bool debug() const {
        return args.debugOrders;
      };
      void report_size() const {
        print("DEBUG OG", "memory " + to_string(orders.size()));
      };
      void report_replace(mOrder *const order) const {
        print("DEBUG OG", "update "
          + ((order->side == mSide::Bid ? "BID" : "ASK")
          + (" id " + order->orderId)) + ":  at price "
          + str8(order->price) + " " + args.quote);
      };
      void report_cancel(mOrder *const order) const {
        print("DEBUG OG", "cancel " + (
          (order->side == mSide::Bid ? "BID id " : "ASK id ")
          + order->orderId
        ) + "::"
          + order->exchangeId
        );
      };
      void report_place(mOrder *const order) const {
        print("DEBUG OG", " place "
          + ((order->side == mSide::Bid ? "BID id " : "ASK id ") + order->orderId) + ": "
          + str8(order->quantity) + " " + args.base + " at price "
          + str8(order->price) + " " + args.quote);
      };
      void report(mOrder *const order) const {
        print("DEBUG OG", " saved "
          + ((order->side == mSide::Bid ? "BID id " : "ASK id ") + order->orderId)
          + "::" + order->exchangeId + " [" + to_string((int)order->orderStatus) + "]: "
          + str8(order->quantity) + " " + args.base + " at price "
          + str8(order->price) + " " + args.quote);
      };
      void report(const mOrder &raw) const {
        print("DEBUG OG", "reply  " + raw.orderId + "::" + raw.exchangeId
          + " [" + to_string((int)raw.orderStatus) + "]: "
          + str8(raw.quantity) + "/" + str8(raw.tradeQuantity) + " at price "
          + str8(raw.price));
      };
  };
  static void to_json(json &j, const mBroker &k) {
    j = k.dump();
  };

  struct mQuoteStatus: public mJsonToClient<mQuoteStatus> {
     mQuoteState bidStatus             = mQuoteState::Disconnected,
                 askStatus             = mQuoteState::Disconnected;
    unsigned int quotesInMemoryNew     = 0,
                 quotesInMemoryWorking = 0,
                 quotesInMemoryDone    = 0;
    const mMatter about() const {
      return mMatter::QuoteStatus;
    };
    const bool realtime() const {
      return !qp.delayUI;
    };
  };
  static void to_json(json &j, const mQuoteStatus &k) {
    j = {
      {            "bidStatus", k.bidStatus            },
      {            "askStatus", k.askStatus            },
      {    "quotesInMemoryNew", k.quotesInMemoryNew    },
      {"quotesInMemoryWorking", k.quotesInMemoryWorking},
      {   "quotesInMemoryDone", k.quotesInMemoryDone   }
    };
  };

  struct mNotepad: public mJsonToClient<mNotepad> {
    string content;
    const json kiss(const json &j) {
      if (j.is_array() and j.size())
        content = j.at(0);
      return j;
    };
    const mMatter about() const {
      return mMatter::Notepad;
    };
  };
  static void to_json(json &j, const mNotepad &k) {
    j = k.content;
  };

  struct mSemaphore: public mToScreen,
                     public mJsonToClient<mSemaphore> {
    mConnectivity *const adminAgreement = (mConnectivity*)&args.autobot;
    mConnectivity greenButton           = mConnectivity::Disconnected,
                  greenGateway          = mConnectivity::Disconnected;
    const json kiss(const json &j) {
      if (j.is_object() and j.at("state").is_number())
        agree(j.at("state").get<mConnectivity>());
      return j;
    };
    const bool online(const mConnectivity &raw) {
      if (greenGateway != raw) {
        greenGateway = raw;
        send_refresh();
      }
      return !!greenGateway;
    };
    void toggle() {
      *adminAgreement = (mConnectivity)!*adminAgreement;
      send_refresh();
    };
    const mMatter about() const {
      return mMatter::Connectivity;
    };
    private:
      void agree(const mConnectivity &raw) {
        if (*adminAgreement != raw) {
          *adminAgreement = raw;
          send_refresh();
        }
      };
      void send_refresh() {
        const mConnectivity k = *adminAgreement * greenGateway;
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
    static unsigned long long int64() {
      static random_device rd;
      static mt19937_64 gen(rd());
      return uniform_int_distribution<unsigned long long>()(gen);
    };
    static string int45Id() {
      return to_string(int64()).substr(0, 10);
    };
    static string int32Id() {
      return to_string(int64()).substr(0,  8);
    };
    static string char16Id() {
      char s[16];
      for (unsigned int i = 0; i < 16; ++i)
        s[i] = numsAz[int64() % (sizeof(numsAz) - 1)];
      return string(s, 16);
    };
    static string uuid36Id() {
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
    static string uuid32Id() {
      string uuid = uuid36Id();
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

  struct mProduct: public mJsonToClient<mProduct> {
    const mPrice *minTick = nullptr;
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

  struct mMonitor: public mJsonToClient<mMonitor> {
    unsigned int /* ( | L | ) */ /* more */ orders_60s /* ? */;
          string /*  )| O |(  */    unlock;
        mProduct /* ( | C | ) */ /* this */ product;
                 /*  )| K |(  */ /* thanks! <3 */
    mMonitor():
      orders_60s(0)
    {};
    function<const unsigned int()> dbSize = []() {
      return 0;
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
