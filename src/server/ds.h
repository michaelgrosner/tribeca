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
  enum class mExchange: unsigned int { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Ethfinex, Kraken, OkEx, Korbit, Poloniex };
  enum class mConnectivity: unsigned int { Disconnected, Connected };
  enum class mStatus: unsigned int { New, Working, Complete, Cancelled };
  enum class mSide: unsigned int { Bid, Ask, Both };
  enum class mTimeInForce: unsigned int { IOC, FOK, GTC };
  enum class mOrderType: unsigned int { Limit, Market };
  enum class mPingAt: unsigned int { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings };
  enum class mPongAt: unsigned int { ShortPingFair, AveragePingFair, LongPingFair, ShortPingAggressive, AveragePingAggressive, LongPingAggressive };
  enum class mQuotingMode: unsigned int { Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth };
  enum class mQuotingSafety: unsigned int { Off, PingPong, Boomerang, AK47 };
  enum class mQuoteState: unsigned int { Live, Disconnected, DisabledQuotes, MissingData, UnknownHeld, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed, UpTrendHeld, DownTrendHeld };
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
                                WalletChart          = 'C', MarketChart        = 'D', Notepad            = 'E', MarketDataLongTerm = 'G'
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
        int port          = 3000,   colors      = 0, debug        = 0,
            debugSecret   = 0,      debugEvents = 0, debugOrders  = 0,
            debugQuotes   = 0,      debugWallet = 0, withoutSSL   = 0,
            headless      = 0,      dustybot    = 0, lifetime     = 0,
            autobot       = 0,      naked       = 0, free         = 0,
            ignoreSun     = 0,      ignoreMoon  = 0, testChamber  = 0,
            maxAdmins     = 7,      maxLevels   = 321;
    mAmount maxWallet     = 0;
     string title         = "K.sh", matryoshka  = "https://www.example.com/",
            user          = "NULL", pass        = "NULL",
            exchange      = "NULL", currency    = "NULL",
            apikey        = "NULL", secret      = "NULL",
            username      = "NULL", passphrase  = "NULL",
            http          = "NULL", wss         = "NULL",
            database      = "",     diskdata    = "",
            whitelist     = "";
    const char *inet = nullptr;
    void tidy() {
      exchange = strU(exchange);
      currency = strU(currency);
      if (debug)
        debugSecret =
        debugEvents =
        debugOrders =
        debugQuotes =
        debugWallet = debug;
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
      if (headless) port = 0;
      else if (!port or !maxAdmins) headless = 1;
#ifdef _WIN32
      naked = 1;
#endif
    };
    string validate() {
      if (currency.find("/") == string::npos or currency.length() < 3)
        return "Invalid currency pair; must be in the format of BASE/QUOTE, like BTC/EUR";
      if (exchange.empty())
        return "Undefined exchange; the config file may have errors (there are extra spaces or double defined variables?)";
      tidy();
      return "";
    };
    vector<string> warnings() const {
      vector<string> msgs;
      if (testChamber == 1) msgs.push_back("Test Chamber #1: send new orders before cancel old");
      else if (testChamber) msgs.push_back("ignored Test Chamber #" + to_string(testChamber));
      return msgs;
    };
    string base() const {
      return currency.substr(0, currency.find("/"));
    };
    string quote() const {
      return currency.substr(1 + currency.find("/"));
    };
  } args;

  struct mAbout {
    virtual mMatter about() const = 0;
  };
  struct mDump: virtual public mAbout {
    virtual json dump() const = 0;
  };

  struct mFromClient: virtual public mAbout {
    virtual json kiss(const json &j) {
      return j;
    };
  };

  struct mToScreen {
    function<void()> refresh
#ifndef NDEBUG
    = []() { WARN("Y U NO catch screen refresh?"); }
#endif
    ;
    function<void(const string&, const string&)> print
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen print?"); }
#endif
    ;
    function<void(const string&, const string&)> warn
#ifndef NDEBUG
    = [](const string &prefix, const string &reason) { WARN("Y U NO catch screen warn?"); }
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
    virtual json hello() {
      return { dump() };
    };
    virtual bool realtime() const {
      return true;
    };
  };
  template <typename mData> struct mJsonToClient: public mToClient {
    void send() {
      if (send_asap() or send_soon())
        send_now();
    };
    virtual json dump() const {
      return *(mData*)this;
    };
    protected:
      mClock send_Tstamp = 0;
      virtual bool send_asap() const {
        return true;
      };
      bool send_soon(const int &delay = 0) {
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
    virtual   bool pull(const json &j) = 0;
    virtual string increment() const { return "NULL"; };
    virtual double limit()     const { return 0; };
    virtual mClock lifetime()  const { return 0; };
    virtual string explain()   const = 0;
    virtual string explainOK() const = 0;
    virtual string explainKO() const { return ""; };
    string explanation(const bool &loaded) {
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
    virtual json dump() const {
      return *(mData*)this;
    };
    virtual bool pull(const json &j) {
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
    virtual void erase() {
      if (size() > limit())
        rows.erase(begin(), end() - limit());
    };
    virtual void push_back(const mData &row) {
      rows.push_back(row);
      push();
      erase();
    };
    virtual bool pull(const json &j) {
      for (const json &it : j)
        rows.push_back(it);
      return !empty();
    };
    virtual json dump() const {
      return rows.back();
    };
    virtual string explain() const {
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
    bool              _matchPings                     = true;
    bool              _diffVLEP                       = false;
    bool              _diffLEP                        = false;
    bool              _diffMEP                        = false;
    bool              _diffSEP                        = false;
    bool              _diffXSEP                       = false;
    bool              _diffUEP                        = false;
    void tidy() {
      if (mode == mQuotingMode::Depth) widthPercentage = false;
    };
    void flag() {
      _matchPings = safety == mQuotingSafety::Boomerang or safety == mQuotingSafety::AK47;
    };
    void diff(const mQuotingParams &prev) {
      _diffVLEP = prev.veryLongEwmaPeriods != veryLongEwmaPeriods;
      _diffLEP = prev.longEwmaPeriods != longEwmaPeriods;
      _diffMEP = prev.mediumEwmaPeriods != mediumEwmaPeriods;
      _diffSEP = prev.shortEwmaPeriods != shortEwmaPeriods;
      _diffXSEP = prev.extraShortEwmaPeriods != extraShortEwmaPeriods;
      _diffUEP = prev.ultraShortEwmaPeriods != ultraShortEwmaPeriods;
    };
    json kiss(const json &j) {
      mQuotingParams prev = *this; // just need to copy the 6 prev.* vars above, noob
      from_json(j, *this);
      diff(prev);
      push();
      send();
      return mFromClient::kiss(j);
    };
    mMatter about() const {
      return mMatter::QuotingParameters;
    };
    string explain() const {
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
    k.flag();
  };

  struct mPair {
    mCoinId base,
            quote;
    mPair():
      base(""), quote("")
    {};
    mPair(mCoinId b, mCoinId q):
      base(b), quote(q)
    {};
  };
  static void to_json(json &j, const mPair &k) {
    j = {
      { "base", k.base },
      {"quote", k.quote}
    };
  };
  static void from_json(const json &j, mPair &k) {
    k.base  = j.value("base", "");
    k.quote = j.value("quote", "");
  };

  struct mTrade {
     string tradeId;
      mSide side;
      mPair pair;
     mPrice price,
            Kprice;
    mAmount quantity,
            value,
            Kqty,
            Kvalue,
            Kdiff,
            feeCharged;
     mClock time,
            Ktime;
       bool loadedFromDB;
    mTrade():
      tradeId(""), side((mSide)0), pair(mPair()), price(0), Kprice(0), quantity(0), value(0), Kqty(0), Kvalue(0), Kdiff(0), feeCharged(0), time(0), Ktime(0), loadedFromDB(false)
    {};
    mTrade(mPair P, mPrice p, mAmount q, mSide s, mClock t):
      tradeId(""), side(s), pair(P), price(p), Kprice(0), quantity(q), value(0), Kqty(0), Kvalue(0), Kdiff(0), feeCharged(0), time(t), Ktime(0), loadedFromDB(false)
    {};
    mTrade(string i, mPair P, mPrice p, mAmount q, mSide S, mClock t, mAmount v, mClock Kt, mAmount Kq, mPrice Kp, mAmount Kv, mAmount Kd, mAmount f, bool l):
      tradeId(i), side(S), pair(P), price(p), Kprice(Kp), quantity(q), value(v), Kqty(Kq), Kvalue(Kv), Kdiff(Kd), feeCharged(f), time(t), Ktime(Kt), loadedFromDB(l)
    {};
  };
  static void to_json(json &j, const mTrade &k) {
    if (k.tradeId.empty()) j = {
      {    "time", k.time    },
      {    "pair", k.pair    },
      {   "price", k.price   },
      {"quantity", k.quantity},
      {    "side", k.side    }
    };
    else j = {
      {     "tradeId", k.tradeId     },
      {        "time", k.time        },
      {        "pair", k.pair        },
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
      {"loadedFromDB", k.loadedFromDB},
    };
  };
  static void from_json(const json &j, mTrade &k) {
    k.tradeId      = j.value("tradeId", "");
    k.pair         = j.value("pair", json::object());
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
    k.loadedFromDB = true;
  };
  struct mTrades: public mVectorFromDb<mTrade>,
                  public mJsonToClient<mTrade> {
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
    mMatter about() const {
      return mMatter::Trades;
    };
    void erase() {
      if (crbegin()->Kqty < 0) rows.pop_back();
    };
    json dump() const {
      if (crbegin()->Kqty == -1) return nullptr;
      else return mVectorFromDb::dump();
    };
    string increment() const {
      return crbegin()->tradeId;
    };
    string explainOK() const {
      return "loaded % historical Trades";
    };
    json hello() {
      return rows;
    };
  };
  struct mTakers: public mJsonToClient<mTrade> {
    vector<mTrade> trades;
    mAmount        takersBuySize60s,
                   takersSellSize60s;
    mTakers():
      trades({}), takersBuySize60s(0), takersSellSize60s(0)
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
    mMatter about() const {
      return mMatter::MarketTrade;
    };
    json dump() const {
      return trades.back();
    };
    json hello() {
      return trades;
    };
  };
  static void to_json(json &j, const mTakers &k) {
    j = k.trades;
  };

  struct mOrder {
         mRandId orderId,
                 exchangeId;
           mPair pair;
           mSide side;
          mPrice price;
         mAmount quantity,
                 tradeQuantity;
      mOrderType type;
    mTimeInForce timeInForce;
         mStatus orderStatus;
            bool isPong,
                 preferPostOnly;
          mClock time,
                 latency,
                 _waitingCancel;
    mOrder():
      orderId(""), exchangeId(""), pair(mPair()), side((mSide)0), quantity(0), type((mOrderType)0), isPong(false), price(0), timeInForce((mTimeInForce)0), orderStatus((mStatus)0), preferPostOnly(false), tradeQuantity(0), time(0), _waitingCancel(0), latency(0)
    {};
    mOrder(mRandId o, mStatus s):
      orderId(o), exchangeId(""), pair(mPair()), side((mSide)0), quantity(0), type((mOrderType)0), isPong(false), price(0), timeInForce((mTimeInForce)0), orderStatus(s), preferPostOnly(false), tradeQuantity(0), time(0), _waitingCancel(0), latency(0)
    {};
    mOrder(mRandId o, mRandId e, mStatus s, mPrice p, mAmount q, mAmount Q):
      orderId(o), exchangeId(e), pair(mPair()), side((mSide)0), quantity(q), type((mOrderType)0), isPong(false), price(p), timeInForce((mTimeInForce)0), orderStatus(s), preferPostOnly(false), tradeQuantity(Q), time(0), _waitingCancel(0), latency(0)
    {};
    mOrder(mRandId o, mPair P, mSide S, mAmount q, mOrderType t, bool i, mPrice p, mTimeInForce F, mStatus s, bool O):
      orderId(o), exchangeId(""), pair(P), side(S), quantity(q), type(t), isPong(i), price(p), timeInForce(F), orderStatus(s), preferPostOnly(O), tradeQuantity(0), time(0), _waitingCancel(0), latency(0)
    {};
  };
  static void to_json(json &j, const mOrder &k) {
    j = {
      {       "orderId", k.orderId       },
      {    "exchangeId", k.exchangeId    },
      {          "pair", k.pair          },
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
  struct mOrders: public mJsonToClient<mOrders> {
    map<mRandId, mOrder> orders;
                 mTrades tradesHistory;
    mAmount calcHeldAmount(const mSide &side) const {
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
    mMatter about() const {
      return mMatter::OrderStatusReports;
    };
    bool realtime() const {
      return !qp.delayUI;
    };
    json dump() const {
      return working();
    };
    private:
      vector<mOrder> working() const {
        vector<mOrder> workingOrders;
        for (const map<mRandId, mOrder>::value_type &it : orders)
          if (mStatus::Working == it.second.orderStatus)
            workingOrders.push_back(it.second);
        return workingOrders;
      };
  };
  static void to_json(json &j, const mOrders &k) {
    j = k.orders;
  };

  struct mRecentTrade {
     mPrice price;
    mAmount quantity;
     mClock time;
    mRecentTrade():
      price(0), quantity(0), time(0)
    {};
    mRecentTrade(mPrice p, mAmount q):
      price(p), quantity(q), time(Tstamp)
    {};
  };
  struct mRecentTrades {
    multimap<mPrice, mRecentTrade> buys,
                                   sells;
                           mAmount sumBuys,
                                   sumSells;
                            mPrice lastBuyPrice,
                                   lastSellPrice;
    mRecentTrades():
      buys({}), sells({}), sumBuys(0), sumSells(0), lastBuyPrice(0), lastSellPrice(0)
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
      mAmount sum(multimap<mPrice, mRecentTrade> *const k) {
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

  struct mFairValue {
    mPrice fv;
    mFairValue():
      fv(0)
    {};
    mFairValue(mPrice f):
      fv(f)
    {};
  };
  static void to_json(json &j, const mFairValue &k) {
    j = {
      {"fv", k.fv}
    };
  };
  static void from_json(const json &j, mFairValue &k) {
    k.fv = j.value("fv", 0.0);
  };
  struct mFairHistory: public mVectorFromDb<mFairValue> {
    mMatter about() const {
      return mMatter::MarketDataLongTerm;
    };
    double limit() const {
      return 5760;
    };
    mClock lifetime() const {
      return 60e+3 * limit();
    };
    string explainOK() const {
      return "loaded % historical Fair Values";
    };
  };
  struct mFairLevelsPrice: public mToScreen,
                           public mJsonToClient<mFairLevelsPrice> {
    mPrice *fv;
    mFairLevelsPrice(mPrice *const f):
      fv(f)
    {};
    mMatter about() const {
      return mMatter::FairValue;
    };
    bool realtime() const {
      return !qp.delayUI;
    };
    bool ratelimit(const mPrice &prev) const {
      return *fv == prev;
    };
    void send_ratelimit(const mPrice &prev) {
      if (ratelimit(prev)) return;
      send();
      refresh();
    };
    bool send_asap() const {
      return false;
    };
  };
  static void to_json(json &j, const mFairLevelsPrice &k) {
    j = {
      {"price", *k.fv}
    };
  };

  struct mStdev: public mFairValue {
    mPrice topBid,
           topAsk;
    mStdev():
      topBid(0), topAsk(0)
    { fv = 0; };
    mStdev(mPrice f, mPrice b, mPrice a):
      topBid(b), topAsk(a)
    { fv = f; };
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
    double top,  topMean,
           fair, fairMean,
           bid,  bidMean,
           ask,  askMean;
    mStdevs():
      top(0), topMean(0), fair(0), fairMean(0), bid(0), bidMean(0), ask(0), askMean(0)
    {};
    bool pull(const json &j) {
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
    mMatter about() const {
      return mMatter::STDEVStats;
    };
    double limit() const {
      return qp.quotingStdevProtectionPeriods;
    };
    mClock lifetime() const {
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

  struct mEwma: public mToScreen,
                public mStructFromDb<mEwma> {
    mFairHistory fairValue96h;
          mPrice mgEwmaVL,
                 mgEwmaL,
                 mgEwmaM,
                 mgEwmaS,
                 mgEwmaXS,
                 mgEwmaU,
                 mgEwmaP,
                 mgEwmaW;
          double mgEwmaTrendDiff,
                 targetPositionAutoPercentage;
    mEwma():
      mgEwmaVL(0), mgEwmaL(0), mgEwmaM(0), mgEwmaS(0), mgEwmaXS(0), mgEwmaU(0), mgEwmaP(0), mgEwmaW(0), mgEwmaTrendDiff(0), targetPositionAutoPercentage(0)
    {};
    void timer_60s(const mPrice &fv, const mPrice &averageWidth) {
      prepareHistory(fv);
      calcProtections(fv, averageWidth);
      calcPositions(fv);
      calcTargetPositionAutoPercentage();
      push();
    };
    void prepareHistory(const mPrice &fv) {
      fairValue96h.push_back(mFairValue(fv));
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
      *mean = fairValue96h.begin()->fv;
      while (n--) calc(mean, periods, (fairValue96h.rbegin()+n)->fv);
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
        [](mPrice sma3, const mFairValue &it) { return sma3 + it.fv; }
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
    mMatter about() const {
      return mMatter::EWMAStats;
    };
    mClock lifetime() const {
      return 60e+3 * max(qp.veryLongEwmaPeriods,
                     max(qp.longEwmaPeriods,
                     max(qp.mediumEwmaPeriods,
                     max(qp.shortEwmaPeriods,
                     max(qp.extraShortEwmaPeriods,
                         qp.ultraShortEwmaPeriods
                     )))));
    };
    string explain() const {
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
             mTakers takerTrades;
    mMarketStats(mPrice *const f):
       ewma(mEwma()), fairPrice(mFairLevelsPrice(f)), takerTrades(mTakers())
    {};
    mMatter about() const {
      return mMatter::MarketChart;
    };
    bool realtime() const {
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
     mPrice price;
    mAmount size;
    mLevel():
      price(0), size(0)
    {};
    mLevel(mPrice p, mAmount s):
      price(p), size(s)
    {};
    void clear() {
      price = size = 0;
    };
    bool empty() const {
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
    mLevels():
      bids({}), asks({})
    {};
    mLevels(vector<mLevel> b, vector<mLevel> a):
      bids(b), asks(a)
    {};
    mPrice spread() const {
      return empty() ? 0 : asks.begin()->price - bids.begin()->price;
    };
    bool empty() const {
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
       bool patched;
    mLevels *unfiltered;
    mLevelsDiff(mLevels *c):
      patched(false), unfiltered(c)
    {};
    bool empty() const {
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
    mMatter about() const {
      return mMatter::MarketData;
    };
    json hello() {
      reset();
      return mToClient::hello();
    };
    private:
      bool ratelimit() {
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
      vector<mLevel> diff(const vector<mLevel> &from, vector<mLevel> to) {
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
    map<mPrice, mAmount> filterBidOrders,
                         filterAskOrders;
            unsigned int averageCount;
                  mPrice averageWidth,
                         fairValue;
            mMarketStats stats;
    mMarketLevels():
      diff(mLevelsDiff(&unfiltered)), filterBidOrders({}), filterAskOrders({}), averageCount(0), averageWidth(0), fairValue(0), stats(mMarketStats(&fairValue))
    {};
    void timer_1s() {
      stats.stdev.timer_1s(fairValue, bids.cbegin()->price, asks.cbegin()->price);
    };
    void timer_60s() {
      stats.takerTrades.timer_60s();
      stats.ewma.timer_60s(fairValue, resetAverageWidth());
      stats.send();
    };
    bool warn_empty() const {
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
    mPrice resetAverageWidth() {
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
    mAmount baseValue,
            quoteValue;
     mClock time;
    mProfit():
      baseValue(0), quoteValue(0), time(0)
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
    bool ratelimit() const {
      return !empty() and crbegin()->time + 21e+3 > Tstamp;
    };
    double calcBase() const {
      return calcDiffPercent(
        cbegin()->baseValue,
        crbegin()->baseValue
      );
    };
    double calcQuote() const {
      return calcDiffPercent(
        cbegin()->quoteValue,
        crbegin()->quoteValue
      );
    };
    double calcDiffPercent(mAmount older, mAmount newer) const {
      return ((newer - older) / newer) * 1e+2;
    };
    mMatter about() const {
      return mMatter::Profit;
    };
    void erase() {
      for (vector<mProfit>::iterator it = begin(); it != end();)
        if (it->time + lifetime() > Tstamp) ++it;
        else it = rows.erase(it);
    };
    double limit() const {
      return qp.profitHourInterval;
    };
    mClock lifetime() const {
      return 3600e+3 * limit();
    };
    string explainOK() const {
      return "loaded % historical Profits";
    };
  };

  struct mSafety: public mJsonToClient<mSafety> {
           double buy,
                  sell,
                  combined;
           mPrice buyPing,
                  sellPing;
          mAmount buySize,
                  sellSize;
    mRecentTrades recentTrades;
          mAmount *baseValue,
                  *baseTotal,
                  *targetBasePosition;
    mSafety(mAmount *const b, mAmount *const t, mAmount *const p):
      buy(0), sell(0), combined(0), buyPing(0), sellPing(0), buySize(0), sellSize(0), recentTrades(mRecentTrades()), baseValue(b), baseTotal(t), targetBasePosition(p)
    {};
    void timer_1s(const mMarketLevels &levels, const mTrades &tradesHistory) {
      calc(levels, tradesHistory);
    };
    void calc(const mMarketLevels &levels, const mTrades &tradesHistory) {
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
    bool empty() const {
      return !*baseValue or !buySize or !sellSize;
    };
    bool ratelimit(const double &prev_combined, const mPrice &prev_buyPing, const mPrice &prev_sellPing) const {
      return combined == prev_combined
        and buyPing == prev_buyPing
        and sellPing == prev_sellPing;
    };
    mMatter about() const {
      return mMatter::TradeSafetyValue;
    };
    private:
      void calcPrices(const mPrice &fv, const mTrades &tradesHistory) {
        if (qp.safety == mQuotingSafety::PingPong) {
          buyPing = recentTrades.lastBuyPrice;
          sellPing = recentTrades.lastSellPrice;
        } else {
          buyPing = sellPing = 0;
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
      bool matchPing(bool _near, bool _far, mPrice *ping, mAmount *qty, mAmount qtyMax, mPrice width, mPrice fv, mPrice price, mAmount qtyTrade, mPrice priceTrade, mAmount KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (((!_near and !_far) or *qty < qtyMax)
          and (_far ? fv > price : true)
          and (_near ? (reverse ? fv - width : fv + width) < price : true)
          and (!qp._matchPings or KqtyTrade < qtyTrade)
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
    mAmount targetBasePosition,
            positionDivergence;
     string sideAPR,
            sideAPRDiff;
    mSafety safety;
    mAmount *baseValue;
    mTarget(mAmount *const b, mAmount *const t):
      targetBasePosition(0), positionDivergence(0), sideAPR(""), sideAPRDiff("!="), safety(mSafety(b, t, &targetBasePosition)), baseValue(b)
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
          + " " + args.base() + ", pDiv: "
          + to_string((int)(positionDivergence / *baseValue * 1e+2)) + "% = " + str8(positionDivergence)
          + " " + args.base());
    };
    bool ratelimit(const mAmount &next) const {
      return (targetBasePosition and abs(targetBasePosition - next) < 1e-4 and sideAPR == sideAPRDiff);
    };
    bool warn_empty() const {
      const bool err = empty();
      if (err) warn("PG", "Unable to calculate TBP, missing wallet data");
      return err;
    };
    bool empty() const {
      return !baseValue or !*baseValue;
    };
    bool realtime() const {
      return !qp.delayUI;
    };
    mMatter about() const {
      return mMatter::TargetBasePosition;
    };
    string explain() const {
      return to_string(targetBasePosition);
    };
    string explainOK() const {
      return "loaded TBP = % " + args.base();
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
    mAmount amount,
            held,
            total,
            value,
            profit;
    mCoinId currency;
    mWallet():
      amount(0), held(0), total(0), value(0), profit(0), currency("")
    {};
    mWallet(mAmount a, mAmount h, mCoinId c):
      amount(a), held(h), total(0), value(0), profit(0), currency(c)
    {};
    void reset(const mAmount &a, const mAmount &h) {
      if (empty()) return;
      total = (amount = a) + (held = h);
    };
    bool empty() const {
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
    mWallets():
      base(mWallet()), quote(mWallet())
    {};
    mWallets(mWallet b, mWallet q):
      base(b), quote(q)
    {};
    bool empty() const {
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
      target(mTarget(&base.value, &base.total)), profits(mProfits())
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
    bool ratelimit(const mWallet &prevBase, const mWallet &prevQuote) const {
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
    mMatter about() const {
      return mMatter::Position;
    };
    bool realtime() const {
      return !qp.delayUI;
    };
    bool send_asap() const {
      return false;
    };
  };

  struct mQuote {
    mLevel bid,
           ask;
      bool isBidPong,
           isAskPong;
    mQuote():
      bid(mLevel()), ask(mLevel()), isBidPong(false), isAskPong(false)
    {};
    mQuote(mLevel b, mLevel a):
      bid(b), ask(a), isBidPong(false), isAskPong(false)
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

  struct mQuoteStatus: public mJsonToClient<mQuoteStatus> {
     mQuoteState bidStatus,
                 askStatus;
    unsigned int quotesInMemoryNew,
                 quotesInMemoryWorking,
                 quotesInMemoryDone;
    mQuoteStatus():
      bidStatus((mQuoteState)0), askStatus((mQuoteState)0), quotesInMemoryNew(0), quotesInMemoryWorking(0), quotesInMemoryDone(0)
    {};
    mMatter about() const {
      return mMatter::QuoteStatus;
    };
    bool realtime() const {
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

  struct mButtonSubmitNewOrder: public mFromClient {
    mMatter about() const {
      return mMatter::SubmitNewOrder;
    };
  };
  struct mButtonCancelOrder: public mFromClient {
    json kiss(const json &j) {
      json butterfly;
      if (j.is_object() and j["orderId"].is_string())
        butterfly = j["orderId"];
      return mFromClient::kiss(butterfly);
    };
    mMatter about() const {
      return mMatter::CancelOrder;
    };
  };
  struct mButtonCancelAllOrders: public mFromClient {
    mMatter about() const {
      return mMatter::CancelAllOrders;
    };
  };
  struct mButtonCleanAllClosedTrades: public mFromClient {
    mMatter about() const {
      return mMatter::CleanAllClosedTrades;
    };
  };
  struct mButtonCleanAllTrades: public mFromClient {
    mMatter about() const {
      return mMatter::CleanAllTrades;
    };
  };
  struct mButtonCleanTrade: public mFromClient {
    json kiss(const json &j) {
      json butterfly;
      if (j.is_object() and j["tradeId"].is_string())
        butterfly = j["tradeId"];
      return mFromClient::kiss(butterfly);
    };
    mMatter about() const {
      return mMatter::CleanTrade;
    };
  };
  struct mButtons {
    mButtonSubmitNewOrder       submitNewOrder;
    mButtonCancelOrder          cancelOrder;
    mButtonCancelAllOrders      cancelAllOrders;
    mButtonCleanAllClosedTrades cleanAllClosedTrades;
    mButtonCleanAllTrades       cleanAllTrades;
    mButtonCleanTrade           cleanTrade;
  };

  struct mNotepad: public mJsonToClient<mNotepad> {
    string content;
    mNotepad():
      content("")
    {};
    json kiss(const json &j) {
      if (j.is_array() and j.size())
        content = j.at(0);
      return mFromClient::kiss(j);
    };
    mMatter about() const {
      return mMatter::Notepad;
    };
  };
  static void to_json(json &j, const mNotepad &k) {
    j = k.content;
  };

  struct mSemaphore: public mJsonToClient<mSemaphore> {
    mConnectivity greenButton,
                  greenGateway;
    mSemaphore():
      greenButton((mConnectivity)0), greenGateway((mConnectivity)0)
    {};
    json kiss(const json &j) {
      json butterfly;
      if (j.is_object() and j["state"].is_number())
        butterfly = j["state"];
      return mFromClient::kiss(butterfly);
    };
    mMatter about() const {
      return mMatter::Connectivity;
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
    static string oHmac256(string p, string s, bool hex = false) {
      unsigned char* digest;
      digest = HMAC(EVP_sha256(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
      char k_[SHA256_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA256_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return hex ? oHex(k_) : k_;
    };
    static string oHmac512(string p, string s) {
      unsigned char* digest;
      digest = HMAC(EVP_sha512(), s.data(), s.length(), (unsigned char*)p.data(), p.length(), NULL, NULL);
      char k_[SHA512_DIGEST_LENGTH*2+1];
      for (unsigned int i = 0; i < SHA512_DIGEST_LENGTH; i++) sprintf(&k_[i*2], "%02x", (unsigned int)digest[i]);
      return k_;
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
      if (reply.empty() or (reply[0] != '{' and reply[0] != '[')) reply = "{}";
      return json::parse(reply);
    };
    static size_t curl_write(void *buf, size_t size, size_t nmemb, void *up) {
      ((string*)up)->append((char*)buf, size * nmemb);
      return size * nmemb;
    };
    static json xfer(const string &url, long timeout = 13) {
      return curl_perform(url, [&](CURL *curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
      }, timeout == 13);
    };
    static json xfer(const string &url, string p) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
      });
    };
    static json xfer(const string &url, string t, bool auth) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        if (!t.empty()) h_ = curl_slist_append(h_, ("Authorization: Bearer " + t).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
      });
    };
    static json xfer(const string &url, bool p, string a, string s, string n) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, ("API-Key: " + a).data());
        h_ = curl_slist_append(h_, ("API-Sign: " + s).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, n.data());
      });
    };
    static json xfer(const string &url, bool a, string p, string s) {
      return curl_perform(url, [&](CURL *curl) {
        if (a) {
          curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, s.data());
        }
        curl_easy_setopt(curl, CURLOPT_USERPWD, p.data());
      });
    };
    static json xfer(const string &url, string p, string s, bool post) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, ("X-Signature: " + s).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
      });
    };
    static json xfer(const string &url, string p, string a, string s) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
        h_ = curl_slist_append(h_, ("Key: " + a).data());
        h_ = curl_slist_append(h_, ("Sign: " + s).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
      });
    };
    static json xfer(const string &url, string p, string a, string s, bool post) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, ("X-BFX-APIKEY: " + a).data());
        h_ = curl_slist_append(h_, ("X-BFX-PAYLOAD: " + p).data());
        h_ = curl_slist_append(h_, ("X-BFX-SIGNATURE: " + s).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
      });
    };
    static json xfer(const string &url, string p, string a, string s, bool post, bool auth) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, "Content-Type: application/x-www-form-urlencoded");
        if (!a.empty()) h_ = curl_slist_append(h_, ("Authorization: Bearer " + a).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p.data());
      });
    };
    static json xfer(const string &url, string t, string a, string s, string p, bool d = false) {
      return curl_perform(url, [&](CURL *curl) {
        struct curl_slist *h_ = NULL;
        h_ = curl_slist_append(h_, ("CB-ACCESS-KEY: " + a).data());
        h_ = curl_slist_append(h_, ("CB-ACCESS-SIGN: " + s).data());
        h_ = curl_slist_append(h_, ("CB-ACCESS-TIMESTAMP: " + t).data());
        h_ = curl_slist_append(h_, ("CB-ACCESS-PASSPHRASE: " + p).data());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, h_);
        if (d) curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      });
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

  static const class mCommand {
    private:
      string output(const string &cmd) const {
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
      string uname() const {
        return output("uname -srvm");
      };
      string ps() const {
        return output("ps -p" + to_string(::getpid()) + " -orss | tail -n1");
      };
      string netstat() const {
        return output("netstat -anp 2>/dev/null | grep " + to_string(args.port));
      };
      void stunnel(const bool &reboot) const {
        int k = system("pkill stunnel || :");
        if (reboot) k = system("stunnel etc/stunnel.conf");
      };
      bool git() const {
        return
#ifdef NDEBUG
          access(".git", F_OK) != -1
#else
          false
#endif
        ;
      };
      void fetch() const {
        if (git()) int k = system("git fetch");
      };
      string changelog() const {
        return git()
          ? output("git --no-pager log --graph --oneline @..@{u}")
          : "";
      };
      bool deprecated() const {
        return git()
          ? output("git rev-parse @") != output("git rev-parse @{u}")
          : false;
      };
  } cmd;

  struct mProduct: public mJsonToClient<mProduct> {
    mExchange exchange;
        mPair pair;
       mPrice *minTick;
    mProduct():
      exchange((mExchange)0), pair(mPair()), minTick(nullptr)
    {};
    mMatter about() const {
      return mMatter::ProductAdvertisement;
    };
  };
  static void to_json(json &j, const mProduct &k) {
    j = {
      {   "exchange", k.exchange                                    },
      {       "pair", k.pair                                        },
      {    "minTick", *k.minTick                                    },
      {"environment", args.title                                    },
      { "matryoshka", args.matryoshka                               },
      {   "homepage", "https://github.com/ctubio/Krypto-trading-bot"}
    };
  };

  struct mMonitor: public mJsonToClient<mMonitor> {
    unsigned int /* ( | L | ) */ /* more */ orders_60s; /* ? */
          string /*  )| O |(  */ unlock;
        mProduct /* ( | C | ) */ /* this */ product;
    mMonitor():  /*  )| K |(  */ /* thanks! <3 */
       orders_60s(0), unlock(""), product(mProduct())
    {};
    function<unsigned int()> dbSize = []() { return 0; };
    unsigned int memSize() const {
      string ps = cmd.ps();
      ps.erase(remove(ps.begin(), ps.end(), ' '), ps.end());
      return ps.empty() ? 0 : stoi(ps) * 1e+3;
    };
    void fromGw(const mExchange &exchange, const mPair &pair, mPrice *const minTick) {
      product.exchange = exchange;
      product.pair     = pair;
      product.minTick  = minTick;
    };
    void tick_orders() {
      orders_60s++;
    };
    void timer_60s() {
      send();
      orders_60s = 0;
    };
    mMatter about() const {
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
