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
      transform(exchange.begin(), exchange.end(), exchange.begin(), ::toupper);
      transform(currency.begin(), currency.end(), currency.begin(), ::toupper);
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
    function<void()> refresh;
    function<void(const string&, const string&)> print;
    function<void(const string&, const string&)> warn;
  };

  struct mToClient: public mDump,
                    public mFromClient  {
    function<void()> send;
    virtual json hello() {
      return { dump() };
    };
    virtual bool realtime() const {
      return true;
    };
  };
  template <typename mData> struct mJsonToClient: public mToClient {
    void send() const {
      if (mToClient::send) mToClient::send();
    };
    virtual json dump() const {
      return *(mData*)this;
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
      std::size_t token = msg.find("%");
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
    iterator                 begin()       { return rows.begin(); };
    const_iterator          cbegin() const { return rows.cbegin(); };
    iterator                   end()       { return rows.end(); };
    reverse_iterator        rbegin()       { return rows.rbegin(); };
    const_reverse_iterator crbegin() const { return rows.crbegin(); };
    reverse_iterator          rend()       { return rows.rend(); };
    bool                     empty() const { return rows.empty(); };
    size_t                    size() const { return rows.size(); };
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

  struct mWallet {
    mAmount amount,
            held;
    mCoinId currency;
    mWallet():
      amount(0), held(0), currency("")
    {};
    mWallet(mAmount a, mAmount h, mCoinId c):
      amount(a), held(h), currency(c)
    {};
    void reset(const mAmount &a, const mAmount &h) {
      if (empty()) return;
      amount = a;
      held = h;
    };
    bool empty() const {
      return currency.empty();
    };
  };
  static void to_json(json &j, const mWallet &k) {
    j = {
      {  "amount", k.amount  },
      {    "held", k.held    },
      {"currency", k.currency}
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
      return ((newer - older) / newer) * 100;
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
    mSafety():
      buy(0), sell(0), combined(0), buyPing(0), sellPing(0), buySize(0), sellSize(0)
    {};
    mSafety(double b, double s, double c, mPrice bP, mPrice sP, mPrice bS, mPrice sS):
      buy(b), sell(s), combined(c), buyPing(bP), sellPing(sP), buySize(bS), sellSize(sS)
    {};
    bool empty() const {
      return !buySize and !sellSize;
    };
    bool ratelimit(const mSafety &next) const {
      return (
        combined == next.combined
        and buyPing == next.buyPing
        and sellPing == next.sellPing
      );
    };
    void send_ratelimit(const mSafety &next) {
      bool limited = ratelimit(next);
      buy = next.buy;
      sell = next.sell;
      combined = next.combined;
      buyPing = next.buyPing;
      sellPing = next.sellPing;
      buySize = next.buySize;
      sellSize = next.sellSize;
      if (!limited) send();
    };
    mMatter about() const {
      return mMatter::TradeSafetyValue;
    };
  };
  static void to_json(json &j, const mSafety &k) {
    j = {
      {     "buy", k.buy              },
      {    "sell", k.sell             },
      {"combined", k.combined         },
      { "buyPing", fmax(0, k.buyPing) },
      {"sellPing", fmax(0, k.sellPing)}
    };
  };

  struct mPosition: public mToScreen,
                    public mJsonToClient<mPosition> {
    mAmount baseAmount,
            quoteAmount,
            _quoteAmountValue,
            baseHeldAmount,
            quoteHeldAmount,
            _baseTotal,
            _quoteTotal,
            baseValue,
            quoteValue;
     double profitBase,
            profitQuote;
      mPair pair;
    mPosition():
      baseAmount(0), quoteAmount(0), _quoteAmountValue(0), baseHeldAmount(0), quoteHeldAmount(0), _baseTotal(0), _quoteTotal(0), baseValue(0), quoteValue(0), profitBase(0), profitQuote(0), pair(mPair())
    {};
    mPosition(mAmount bA, mAmount qA, mAmount qAV, mAmount bH, mAmount qH, mAmount bT, mAmount qT, mAmount bV, mAmount qV, mAmount bP, mAmount qP, mPair p):
      baseAmount(bA), quoteAmount(qA), _quoteAmountValue(qAV), baseHeldAmount(bH), quoteHeldAmount(qH), _baseTotal(bT), _quoteTotal(qT), baseValue(bV), quoteValue(qV), profitBase(bP), profitQuote(qP), pair(p)
    {};
    bool empty() const {
      return !baseValue;
    };
    bool warn_empty() const {
      const bool err = empty();
      if (err) warn("PG", "Unable to calculate TBP, missing wallet data");
      return err;
    };
    bool ratelimit(const mPosition &next) const {
      return (!next.empty()
        and abs(baseValue - next.baseValue) < 2e-6
        and abs(quoteValue - next.quoteValue) < 2e-2
        and abs(baseAmount - next.baseAmount) < 2e-6
        and abs(quoteAmount - next.quoteAmount) < 2e-2
        and abs(baseHeldAmount - next.baseHeldAmount) < 2e-6
        and abs(quoteHeldAmount - next.quoteHeldAmount) < 2e-2
        and abs(profitBase - next.profitBase) < 2e-2
        and abs(profitQuote - next.profitQuote) < 2e-2
      );
    };
    void send_ratelimit(const mPosition &next) {
      bool limited = ratelimit(next);
      baseAmount = next.baseAmount;
      quoteAmount = next.quoteAmount;
      _quoteAmountValue = next._quoteAmountValue;
      baseHeldAmount = next.baseHeldAmount;
      quoteHeldAmount = next.quoteHeldAmount;
      _baseTotal = next._baseTotal;
      _quoteTotal = next._quoteTotal;
      baseValue = next.baseValue;
      quoteValue = next.quoteValue;
      profitBase = next.profitBase;
      profitQuote = next.profitQuote;
      pair = next.pair;
      if (!limited) {
        send();
        refresh();
      }
    };
    mMatter about() const {
      return mMatter::Position;
    };
    bool realtime() const {
      return !qp.delayUI;
    };
  };
  static void to_json(json &j, const mPosition &k) {
    j = {
      {     "baseAmount", k.baseAmount     },
      {    "quoteAmount", k.quoteAmount    },
      { "baseHeldAmount", k.baseHeldAmount },
      {"quoteHeldAmount", k.quoteHeldAmount},
      {      "baseValue", k.baseValue      },
      {     "quoteValue", k.quoteValue     },
      {     "profitBase", k.profitBase     },
      {    "profitQuote", k.profitQuote    },
      {           "pair", k.pair           }
    };
  };

  struct mTarget: public mStructFromDb<mTarget>,
                  public mJsonToClient<mTarget> {
    mAmount targetBasePosition,
            positionDivergence;
     string sideAPR;
     string base;
    mTarget():
      targetBasePosition(0), positionDivergence(0), sideAPR(""), base("")
    {};
    void send_push() const {
      push();
      send();
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
    mFairLevelsPrice(mPrice *f):
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
    void send_ratelimit(const mPrice &prev) const {
      if (ratelimit(prev)) return;
      send();
      refresh();
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
    void calcFromHistory(mPrice *mean, const unsigned int &periods, const string &name) {
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
      mPrice SMA3 = accumulate(
        fairValue96h.end() - max3size,
        fairValue96h.end(),
        0, [](mFairValue a, mFairValue b) { return a.fv + b.fv; }
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
      tradeId(""), pair(mPair()), price(0), quantity(0), side((mSide)0), time(0), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(mPair P, mPrice p, mAmount q, mSide s, mClock t):
      tradeId(""), pair(P), price(p), quantity(q), side(s), time(t), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(string i, mPair P, mPrice p, mAmount q, mSide S, mClock t, mAmount v, mClock Kt, mAmount Kq, mPrice Kp, mAmount Kv, mAmount Kd, mAmount f, bool l):
      tradeId(i), pair(P), price(p), quantity(q), side(S), time(t), value(v), Ktime(Kt), Kqty(Kq), Kprice(Kp), Kvalue(Kv), Kdiff(Kd), feeCharged(f), loadedFromDB(l)
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

  struct mMarketStats: public mJsonToClient<mMarketStats> {
               mEwma ewma;
             mStdevs stdev;
    mFairLevelsPrice fairPrice;
             mTakers takerTrades;
    mMarketStats(mPrice *f):
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
    vector<mOrder> working() const {
      vector<mOrder> workingOrders;
      for (const map<mRandId, mOrder>::value_type &it : orders)
        if (mStatus::Working == it.second.orderStatus)
          workingOrders.push_back(it.second);
      return workingOrders;
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
  };
  static void to_json(json &j, const mOrders &k) {
    j = k.orders;
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
     mClock T_369ms;
    mLevels *unfiltered;
       bool patched;
    mLevelsDiff(mLevels *c):
      T_369ms(0), unfiltered(c), patched(false)
    {};
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
    void diff() {
      bids = diff(bids, unfiltered->bids);
      asks = diff(asks, unfiltered->asks);
      patched = true;
    };
    void reset() {
      bids = unfiltered->bids;
      asks = unfiltered->asks;
      patched = false;
    };
    bool empty() const {
      return patched
        ? bids.empty() and asks.empty()
        : mLevels::empty();
    };
    bool ratelimit() const {
      return unfiltered->empty() or empty()
        or T_369ms + max(369.0, qp.delayUI * 1e+3) > Tstamp;
    };
    void send_reset() {
      if (ratelimit()) return;
      T_369ms = Tstamp;
      diff();
      if (!empty()) send();
      reset();
    };
    mMatter about() const {
      return mMatter::MarketData;
    };
    json hello() {
      reset();
      return mToClient::hello();
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
      diff(mLevelsDiff(&unfiltered)), filterBidOrders({}), filterAskOrders({}), averageCount(0), averageWidth(0), stats(mMarketStats(&fairValue))
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
    void calcFairValue() {
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
    void send_reset_filter(const mLevels &next) {
      reset(next);
      if (!filterBidOrders.empty()) filter(&bids, filterBidOrders);
      if (!filterAskOrders.empty()) filter(&asks, filterAskOrders);
      calcFairValue();
      calcAverageWidth();
      diff.send_reset();
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
        system("pkill stunnel || :");
        if (reboot) system("stunnel etc/stunnel.conf");
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
        if (git()) system("git fetch");
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
