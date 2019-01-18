#ifndef K_DATA_H_
#define K_DATA_H_
//! \file
//! \brief Internal data objects.

namespace ₿ {
  enum class mQuotingMode: unsigned int {
    Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth
  };
  enum class mQuotingSafety: unsigned int {
    Off, PingPong, PingPoing, Boomerang, AK47
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

  struct mAbout {
    virtual const mMatter about() const = 0;
  };
  struct mBlob: virtual public mAbout {
    virtual const json blob() const = 0;
  };

  struct mFromClient: virtual public mAbout {
    virtual void kiss(json *const j) {};
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
    const json blob() const override {
      return *(mData*)this;
    };
    protected:
      Clock send_last_Tstamp = 0;
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
        const Clock now = Tstamp;
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

  class mFromDb: public mBlob {
    public:
      function<void()> push
#ifndef NDEBUG
      = []() { WARN("Y U NO catch sqlite push?"); }
#endif
      ;
      virtual       void   pull(const json &j) = 0;
      virtual const string increment() const { return "NULL"; };
      virtual const double limit()     const { return 0; };
      virtual const Clock  lifetime()  const { return 0; };
    protected:
      virtual const string explain()   const = 0;
      virtual       string explainOK() const = 0;
      virtual       string explainKO() const { return ""; };
      void explanation(const bool &empty) const {
        string msg = empty
          ? explainKO()
          : explainOK();
        if (msg.empty()) return;
        size_t token = msg.find("%");
        if (token != string::npos)
          msg.replace(token, 1, explain());
        if (empty)
          Print::logWar("DB", msg);
        else Print::log("DB", msg);
      };
  };
  template <typename mData> class mStructFromDb: public mFromDb {
    public:
      const json blob() const override {
        return *(mData*)this;
      };
      void pull(const json &j) override {
        if (!j.empty())
          from_json(j.at(0), *(mData*)this);
        explanation(j.empty());
      };
    protected:
      string explainOK() const override {
        return "loaded last % OK";
      };
  };
  template <typename mData> class mVectorFromDb: public mFromDb {
    public:
      vector<mData> rows;
      using reference              = typename vector<mData>::reference;
      using const_reference        = typename vector<mData>::const_reference;
      using iterator               = typename vector<mData>::iterator;
      using const_iterator         = typename vector<mData>::const_iterator;
      using reverse_iterator       = typename vector<mData>::reverse_iterator;
      using const_reverse_iterator = typename vector<mData>::const_reverse_iterator;
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
      void pull(const json &j) override {
        for (const json &it : j)
          rows.push_back(it);
        explanation(empty());
      };
      const json blob() const override {
        return back();
      };
    protected:
      const string explain() const override {
        return to_string(size());
      };
  };

  struct mQuotingParams: public mStructFromDb<mQuotingParams>,
                         public mJsonToClient<mQuotingParams> {
    Price             widthPing                       = 300.0;
    double            widthPingPercentage             = 0.25;
    Price             widthPong                       = 300.0;
    double            widthPongPercentage             = 0.25;
    bool              widthPercentage                 = false;
    bool              bestWidth                       = true;
    Amount            bestWidthSize                   = 0;
    Amount            buySize                         = 0.02;
    unsigned int      buySizePercentage               = 7;
    bool              buySizeMax                      = false;
    Amount            sellSize                        = 0.01;
    unsigned int      sellSizePercentage              = 7;
    bool              sellSizeMax                     = false;
    mPingAt           pingAt                          = mPingAt::BothSides;
    mPongAt           pongAt                          = mPongAt::ShortPingFair;
    mQuotingMode      mode                            = mQuotingMode::Top;
    mQuotingSafety    safety                          = mQuotingSafety::PingPong;
    unsigned int      bullets                         = 2;
    Price             range                           = 0.5;
    double            rangePercentage                 = 5.0;
    mFairValueModel   fvModel                         = mFairValueModel::BBO;
    Amount            targetBasePosition              = 1.0;
    unsigned int      targetBasePositionPercentage    = 50;
    Amount            positionDivergence              = 0.9;
    Amount            positionDivergenceMin           = 0.4;
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
    void from_json(const json &j) {
      widthPing                       = fmax(1e-8,            j.value("widthPing", widthPing));
      widthPingPercentage             = fmin(1e+5, fmax(1e-4, j.value("widthPingPercentage", widthPingPercentage)));
      widthPong                       = fmax(1e-8,            j.value("widthPong", widthPong));
      widthPongPercentage             = fmin(1e+5, fmax(1e-4, j.value("widthPongPercentage", widthPongPercentage)));
      widthPercentage                 =                       j.value("widthPercentage", widthPercentage);
      bestWidth                       =                       j.value("bestWidth", bestWidth);
      bestWidthSize                   = fmax(0,               j.value("bestWidthSize", bestWidthSize));
      buySize                         = fmax(1e-8,            j.value("buySize", buySize));
      buySizePercentage               = fmin(1e+2, fmax(1,    j.value("buySizePercentage", buySizePercentage)));
      buySizeMax                      =                       j.value("buySizeMax", buySizeMax);
      sellSize                        = fmax(1e-8,            j.value("sellSize", sellSize));
      sellSizePercentage              = fmin(1e+2, fmax(1,    j.value("sellSizePercentage", sellSizePercentage)));
      sellSizeMax                     =                       j.value("sellSizeMax", sellSizeMax);
      pingAt                          =                       j.value("pingAt", pingAt);
      pongAt                          =                       j.value("pongAt", pongAt);
      mode                            =                       j.value("mode", mode);
      safety                          =                       j.value("safety", safety);
      bullets                         = fmin(10, fmax(1,      j.value("bullets", bullets)));
      range                           =                       j.value("range", range);
      rangePercentage                 = fmin(1e+2, fmax(1e-3, j.value("rangePercentage", rangePercentage)));
      fvModel                         =                       j.value("fvModel", fvModel);
      targetBasePosition              =                       j.value("targetBasePosition", targetBasePosition);
      targetBasePositionPercentage    = fmin(1e+2, fmax(0,    j.value("targetBasePositionPercentage", targetBasePositionPercentage)));
      positionDivergenceMin           =                       j.value("positionDivergenceMin", positionDivergenceMin);
      positionDivergenceMode          =                       j.value("positionDivergenceMode", positionDivergenceMode);
      positionDivergence              =                       j.value("positionDivergence", positionDivergence);
      positionDivergencePercentage    = fmin(1e+2, fmax(0,    j.value("positionDivergencePercentage", positionDivergencePercentage)));
      positionDivergencePercentageMin = fmin(1e+2, fmax(0,    j.value("positionDivergencePercentageMin", positionDivergencePercentageMin)));
      percentageValues                =                       j.value("percentageValues", percentageValues);
      autoPositionMode                =                       j.value("autoPositionMode", autoPositionMode);
      aggressivePositionRebalancing   =                       j.value("aggressivePositionRebalancing", aggressivePositionRebalancing);
      superTrades                     =                       j.value("superTrades", superTrades);
      tradesPerMinute                 =                       j.value("tradesPerMinute", tradesPerMinute);
      tradeRateSeconds                = fmax(0,               j.value("tradeRateSeconds", tradeRateSeconds));
      protectionEwmaWidthPing         =                       j.value("protectionEwmaWidthPing", protectionEwmaWidthPing);
      protectionEwmaQuotePrice        =                       j.value("protectionEwmaQuotePrice", protectionEwmaQuotePrice);
      protectionEwmaPeriods           = fmax(1,               j.value("protectionEwmaPeriods", protectionEwmaPeriods));
      quotingStdevProtection          =                       j.value("quotingStdevProtection", quotingStdevProtection);
      quotingStdevBollingerBands      =                       j.value("quotingStdevBollingerBands", quotingStdevBollingerBands);
      quotingStdevProtectionFactor    =                       j.value("quotingStdevProtectionFactor", quotingStdevProtectionFactor);
      quotingStdevProtectionPeriods   = fmax(1,               j.value("quotingStdevProtectionPeriods", quotingStdevProtectionPeriods));
      ewmaSensiblityPercentage        =                       j.value("ewmaSensiblityPercentage", ewmaSensiblityPercentage);
      quotingEwmaTrendProtection      =                       j.value("quotingEwmaTrendProtection", quotingEwmaTrendProtection);
      quotingEwmaTrendThreshold       =                       j.value("quotingEwmaTrendThreshold", quotingEwmaTrendThreshold);
      veryLongEwmaPeriods             = fmax(1,               j.value("veryLongEwmaPeriods", veryLongEwmaPeriods));
      longEwmaPeriods                 = fmax(1,               j.value("longEwmaPeriods", longEwmaPeriods));
      mediumEwmaPeriods               = fmax(1,               j.value("mediumEwmaPeriods", mediumEwmaPeriods));
      shortEwmaPeriods                = fmax(1,               j.value("shortEwmaPeriods", shortEwmaPeriods));
      extraShortEwmaPeriods           = fmax(1,               j.value("extraShortEwmaPeriods", extraShortEwmaPeriods));
      ultraShortEwmaPeriods           = fmax(1,               j.value("ultraShortEwmaPeriods", ultraShortEwmaPeriods));
      aprMultiplier                   =                       j.value("aprMultiplier", aprMultiplier);
      sopWidthMultiplier              =                       j.value("sopWidthMultiplier", sopWidthMultiplier);
      sopSizeMultiplier               =                       j.value("sopSizeMultiplier", sopSizeMultiplier);
      sopTradesMultiplier             =                       j.value("sopTradesMultiplier", sopTradesMultiplier);
      cancelOrdersAuto                =                       j.value("cancelOrdersAuto", cancelOrdersAuto);
      cleanPongsAuto                  =                       j.value("cleanPongsAuto", cleanPongsAuto);
      profitHourInterval              =                       j.value("profitHourInterval", profitHourInterval);
      audio                           =                       j.value("audio", audio);
      delayUI                         = fmax(0,               j.value("delayUI", delayUI));
      if (mode == mQuotingMode::Depth)
        widthPercentage = false;
    };
    void kiss(json *const j) override {
      previous = {this};
      from_json(*j);
      diff(previous);
      push();
      send();
    };
    const mMatter about() const override {
      return mMatter::QuotingParameters;
    };
    protected:
      const string explain() const override {
        return "Quoting Parameters";
      };
      string explainKO() const override {
        return "using default values for %";
      };
    private:
      struct mPreviousQParams {
        unsigned int veryLongEwmaPeriods   = 0,
                     longEwmaPeriods       = 0,
                     mediumEwmaPeriods     = 0,
                     shortEwmaPeriods      = 0,
                     extraShortEwmaPeriods = 0,
                     ultraShortEwmaPeriods = 0;
        mPreviousQParams() = default;
        mPreviousQParams(const mQuotingParams *const prev)
          : veryLongEwmaPeriods(  prev->veryLongEwmaPeriods  )
          , longEwmaPeriods(      prev->longEwmaPeriods      )
          , mediumEwmaPeriods(    prev->mediumEwmaPeriods    )
          , shortEwmaPeriods(     prev->shortEwmaPeriods     )
          , extraShortEwmaPeriods(prev->extraShortEwmaPeriods)
          , ultraShortEwmaPeriods(prev->ultraShortEwmaPeriods)
        {};
      } previous;
      void diff(const mPreviousQParams &prev) {
        _diffVLEP = prev.veryLongEwmaPeriods != veryLongEwmaPeriods;
        _diffLEP  = prev.longEwmaPeriods != longEwmaPeriods;
        _diffMEP  = prev.mediumEwmaPeriods != mediumEwmaPeriods;
        _diffSEP  = prev.shortEwmaPeriods != shortEwmaPeriods;
        _diffXSEP = prev.extraShortEwmaPeriods != extraShortEwmaPeriods;
        _diffUEP  = prev.ultraShortEwmaPeriods != ultraShortEwmaPeriods;
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
      {                  "bestWidthSize", k.bestWidthSize                  },
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
    k.from_json(j);
  };

  struct mProduct: public mJsonToClient<mProduct> {
    const Price  *minTick = nullptr;
    const Amount *minSize = nullptr;
    private_ref:
      const Option &option;
    public:
      mProduct(const Option &o)
        : option(o)
      {};
      const string title() const {
        return option.str("title");
      };
      const string matryoshka() const {
        return option.str("matryoshka");
      };
      const unsigned int lifetime() const {
        return option.num("lifetime");
      };
      const unsigned int debug(const string &k) const {
        return
#ifndef NDEBUG
        0
#else
        option.num("debug-" + k)
#endif
        ;
      };
      const double maxWallet() const {
        return
#ifndef NDEBUG
        0
#else
        option.dec("wallet-limit")
#endif
        ;
      };
      const mMatter about() const override {
        return mMatter::ProductAdvertisement;
      };
  };
  static void to_json(json &j, const mProduct &k) {
    j = {
      {   "exchange", gw->exchange                                  },
      {       "base", gw->base                                      },
      {      "quote", gw->quote                                     },
      {    "minTick", *k.minTick                                    },
      {"environment", k.title()                                     },
      { "matryoshka", k.matryoshka()                                },
      {   "homepage", "https://github.com/ctubio/Krypto-trading-bot"}
    };
  };

  struct mLastOrder {
    Price   price         = 0;
    Amount tradeQuantity  = 0;
    Side    side          = (Side)0;
    bool    isPong        = false;
    mLastOrder() = default;
    mLastOrder(const mOrder *const order, const mOrder &raw)
      : price(        order ? order->price      : 0      )
      , tradeQuantity(order ? raw.tradeQuantity : 0      )
      , side(         order ? order->side       : (Side)0)
      , isPong(       order ? order->isPong     : false  )
    {};
  };
  struct mOrders: public mJsonToClient<mOrders> {
    mLastOrder updated;
    private:
      unordered_map<RandId, mOrder> orders;
    private_ref:
      const mProduct &product;
    public:
      mOrders(const mProduct &p)
        : product(p)
      {};
      mOrder *const find(const RandId &orderId) {
        return (orderId.empty()
          or orders.find(orderId) == orders.end()
        ) ? nullptr
          : &orders.at(orderId);
      };
      mOrder *const findsert(const mOrder &raw) {
        if (raw.status == Status::Waiting and !raw.orderId.empty())
          return &(orders[raw.orderId] = raw);
        if (raw.orderId.empty() and !raw.exchangeId.empty()) {
          auto it = find_if(
            orders.begin(), orders.end(),
            [&](const pair<RandId, mOrder> &it_) {
              return raw.exchangeId == it_.second.exchangeId;
            }
          );
          if (it != orders.end())
            return &it->second;
        }
        return find(raw.orderId);
      };
      const double heldAmount(const Side &side) const {
        double held = 0;
        for (const auto &it : orders)
          if (it.second.side == side)
            held += (
              it.second.side == Side::Ask
                ? it.second.quantity
                : it.second.quantity * it.second.price
            );
        return held;
      };
      void resetFilters(
        unordered_map<Price, Amount> *const filterBidOrders,
        unordered_map<Price, Amount> *const filterAskOrders
      ) const {
        filterBidOrders->clear();
        filterAskOrders->clear();
        for (const auto &it : orders)
          (it.second.side == Side::Bid
            ? *filterBidOrders
            : *filterAskOrders
          )[it.second.price] += it.second.quantity;
      };
      const vector<mOrder*> at(const Side &side) {
        vector<mOrder*> sideOrders;
        for (auto &it : orders)
          if (side == it.second.side)
             sideOrders.push_back(&it.second);
        return sideOrders;
      };
      const vector<mOrder*> working() {
        vector<mOrder*> workingOrders;
        for (auto &it : orders)
          if (Status::Working == it.second.status
            and !it.second.disablePostOnly
          ) workingOrders.push_back(&it.second);
        return workingOrders;
      };
      const vector<mOrder> working(const bool &sorted = false) const {
        vector<mOrder> workingOrders;
        for (const auto &it : orders)
          if (Status::Working == it.second.status)
            workingOrders.push_back(it.second);
        if (sorted)
          sort(workingOrders.begin(), workingOrders.end(),
            [](const mOrder &a, const mOrder &b) {
              return a.price > b.price;
            }
          );
        return workingOrders;
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
      const bool replace(const Price &price, const bool &isPong, mOrder *const order) {
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
      void read_from_gw(const mOrder &raw) {
        if (debug()) report(&raw, " reply ");
        mOrder *const order = upsert(raw);
        updated = {order, raw};
        if (!order) return;
        if (order->status == Status::Terminated)
          purge(order);
        send();
        Print::repaint();
      };
      const mMatter about() const override {
        return mMatter::OrderStatusReports;
      };
      const bool realtime() const override {
        return !qp.delayUI;
      };
      const json blob() const override {
        return working();
      };
    private:
      void report(const mOrder *const order, const string &reason) const {
        Print::log("DEBUG OG", " " + reason + " " + (
          order
            ? order->orderId + "::" + order->exchangeId
              + " [" + to_string((int)order->status) + "]: "
              + gw->str(order->quantity) + " " + gw->base + " at price "
              + gw->str(order->price) + " " + gw->quote
            : "not found"
        ));
      };
      void report_size() const {
        Print::log("DEBUG OG", "memory " + to_string(orders.size()));
      };
      const bool debug() const {
        return product.debug("orders");
      };
  };
  static void to_json(json &j, const mOrders &k) {
    j = k.blob();
  };


  struct mMarketTakers: public mJsonToClient<mTrade> {
    vector<mTrade> trades;
            Amount takersBuySize60s  = 0,
                   takersSellSize60s = 0;
    void timer_60s() {
      takersSellSize60s = takersBuySize60s = 0;
      if (trades.empty()) return;
      for (mTrade &it : trades)
        (it.side == Side::Bid
          ? takersSellSize60s
          : takersBuySize60s
        ) += it.quantity;
      trades.clear();
    };
    void read_from_gw(const mTrade &raw) {
      trades.push_back(raw);
      send();
    };
    const mMatter about() const override {
      return mMatter::MarketTrade;
    };
    const json blob() const override {
      return trades.back();
    };
    const json hello() override {
      return trades;
    };
  };
  static void to_json(json &j, const mMarketTakers &k) {
    j = k.trades;
  };

  struct mFairLevelsPrice: public mJsonToClient<mFairLevelsPrice> {
    private_ref:
      const Price &fairValue;
    public:
      mFairLevelsPrice(const Price &f)
        : fairValue(f)
      {};
      const Price currentPrice() const {
        return fairValue;
      };
      const mMatter about() const override {
        return mMatter::FairValue;
      };
      const bool realtime() const override {
        return !qp.delayUI;
      };
      const bool send_same_blob() const override {
        return false;
      };
      const bool send_asap() const override {
        return false;
      };
  };
  static void to_json(json &j, const mFairLevelsPrice &k) {
    j = {
      {"price", k.currentPrice()}
    };
  };


  struct mStdev {
    Price fv     = 0,
          topBid = 0,
          topAsk = 0;
    mStdev() = default;
    mStdev(const Price &f, const Price &b, const Price &a)
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
    k.fv     = j.value("fv",  0.0);
    k.topBid = j.value("bid", 0.0);
    k.topAsk = j.value("ask", 0.0);
  };

  struct mStdevs: public mVectorFromDb<mStdev> {
    double top  = 0,  topMean = 0,
           fair = 0, fairMean = 0,
           bid  = 0,  bidMean = 0,
           ask  = 0,  askMean = 0;
    private_ref:
      const Price &fairValue;
    public:
      mStdevs(const Price &f)
        : fairValue(f)
      {};
      void timer_1s(const Price &topBid, const Price &topAsk) {
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
      const mMatter about() const override {
        return mMatter::STDEVStats;
      };
      const double limit() const override {
        return qp.quotingStdevProtectionPeriods;
      };
      const Clock lifetime() const override {
        return 1e+3 * limit();
      };
    protected:
      string explainOK() const override {
        return "loaded % STDEV Periods";
      };
    private:
      double calc(Price *const mean, const string &type) const {
        vector<Price> values;
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
      double calc(Price *const mean, const double &factor, const vector<Price> &values) const {
        unsigned int n = values.size();
        if (!n) return 0;
        double sum = 0;
        for (const Price &it : values) sum += it;
        *mean = sum / n;
        double sq_diff_sum = 0;
        for (const Price &it : values) {
          Price diff = it - *mean;
          sq_diff_sum += diff * diff;
        }
        double variance = sq_diff_sum / n;
        return sqrt(variance) * factor;
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

  struct mFairHistory: public mVectorFromDb<Price> {
    const mMatter about() const override {
      return mMatter::MarketDataLongTerm;
    };
    const double limit() const override {
      return 5760;
    };
    const Clock lifetime() const override {
      return 60e+3 * limit();
    };
    protected:
      string explainOK() const override {
        return "loaded % historical Fair Values";
      };
  };

  struct mEwma: public mStructFromDb<mEwma> {
    mFairHistory fairValue96h;
           Price mgEwmaVL = 0,
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
      const Price &fairValue;
    public:
      mEwma(const Price &f)
        : fairValue(f)
      {};
      void timer_60s(const Price &averageWidth) {
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
      const mMatter about() const override {
        return mMatter::EWMAStats;
      };
      const Clock lifetime() const override {
        return 60e+3 * max(qp.veryLongEwmaPeriods,
                       max(qp.longEwmaPeriods,
                       max(qp.mediumEwmaPeriods,
                       max(qp.shortEwmaPeriods,
                       max(qp.extraShortEwmaPeriods,
                           qp.ultraShortEwmaPeriods
                       )))));
      };
    protected:
      const string explain() const override {
        return "EWMA Values";
      };
      string explainKO() const override {
        return "consider to warm up some %";
      };
    private:
      void calc(Price *const mean, const unsigned int &periods, const Price &value) {
        if (*mean) {
          double alpha = 2.0 / (periods + 1);
          *mean = alpha * value + (1 - alpha) * *mean;
        } else *mean = value;
      };
      void prepareHistory() {
        fairValue96h.push_back(fairValue);
      };
      void calcFromHistory(Price *const mean, const unsigned int &periods, const string &name) {
        unsigned int n = fairValue96h.size();
        if (!n--) return;
        unsigned int x = 0;
        *mean = fairValue96h.front();
        while (n--) calc(mean, periods, fairValue96h.at(++x));
        Print::log("MG", "reloaded " + to_string(*mean) + " EWMA " + name);
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
      void calcProtections(const Price &averageWidth) {
        calc(&mgEwmaP, qp.protectionEwmaPeriods, fairValue);
        calc(&mgEwmaW, qp.protectionEwmaPeriods, averageWidth);
      };
      void calcTargetPositionAutoPercentage() {
        unsigned int max3size = min((size_t)3, fairValue96h.size());
        Price SMA3 = accumulate(fairValue96h.end() - max3size, fairValue96h.end(), Price(),
          [](Price sma3, const Price &it) { return sma3 + it; }
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
    k.mgEwmaVL = j.value("ewmaVeryLong",   0.0);
    k.mgEwmaL  = j.value("ewmaLong",       0.0);
    k.mgEwmaM  = j.value("ewmaMedium",     0.0);
    k.mgEwmaS  = j.value("ewmaShort",      0.0);
    k.mgEwmaXS = j.value("ewmaExtraShort", 0.0);
    k.mgEwmaU  = j.value("ewmaUltraShort", 0.0);
  };

  struct mMarketStats: public mJsonToClient<mMarketStats> {
               mEwma ewma;
             mStdevs stdev;
    mFairLevelsPrice fairPrice;
       mMarketTakers takerTrades;
    mMarketStats(const Price &f)
      : ewma(f)
      , stdev(f)
      , fairPrice(f)
    {};
    const mMatter about() const override {
      return mMatter::MarketChart;
    };
    const bool realtime() const override {
      return !qp.delayUI;
    };
  };
  static void to_json(json &j, const mMarketStats &k) {
    j = {
      {          "ewma", k.ewma                         },
      {    "stdevWidth", k.stdev                        },
      {     "fairValue", k.fairPrice.currentPrice()     },
      { "tradesBuySize", k.takerTrades.takersBuySize60s },
      {"tradesSellSize", k.takerTrades.takersSellSize60s}
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
          : bids.empty() or asks.empty();
      };
      void send_patch() {
        if (ratelimit()) return;
        diff();
        if (!empty()) send_now();
        unfilter();
      };
      const mMatter about() const override {
        return mMatter::MarketData;
      };
      const json hello() override {
        unfilter();
        return mToClient::hello();
      };
    private:
      const bool ratelimit() {
        return unfiltered.bids.empty() or unfiltered.asks.empty() or empty()
          or !send_soon(qp.delayUI * 1e+3);
      };
      void unfilter() {
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
          auto it_ = find_if(
            to.begin(), to.end(),
            [&](const mLevel &_it) {
              return it.price == _it.price;
            }
          );
          Amount size = 0;
          if (it_ != to.end()) {
            size = it_->size;
            to.erase(it_);
          }
          if (size != it.size)
            patch.push_back({it.price, size});
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
    unsigned int averageCount = 0;
           Price averageWidth = 0,
                 fairValue    = 0;
         mLevels unfiltered;
     mLevelsDiff diff;
    mMarketStats stats;
    private:
      unordered_map<Price, Amount> filterBidOrders,
                                   filterAskOrders;
    private_ref:
      const mProduct &product;
      const mOrders  &orders;
    public:
      mMarketLevels(const mProduct &p, const mOrders &o)
        : diff(unfiltered)
        , stats(fairValue)
        , product(p)
        , orders(o)
      {};
      const bool warn_empty() const {
        const bool err = bids.empty() or asks.empty();
        if (err) Print::logWar("QE", "Unable to calculate quote, missing market data");
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
      const Price calcQuotesWidth(bool *const superSpread) const {
        const Price widthPing = fmax(
          qp.widthPercentage
            ? qp.widthPingPercentage * fairValue / 100
            : qp.widthPing,
          qp.protectionEwmaWidthPing and stats.ewma.mgEwmaW
            ? stats.ewma.mgEwmaW
            : 0
        );
        *superSpread = asks.cbegin()->price - bids.cbegin()->price > widthPing * qp.sopWidthMultiplier;
        return widthPing;
      };
      void clear() {
        bids.clear();
        asks.clear();
      };
      const bool ready() {
        filter();
        return !(bids.empty() or asks.empty());
      };
      void read_from_gw(const mLevels &raw) {
        unfiltered.bids = raw.bids;
        unfiltered.asks = raw.asks;
        filter();
        if (stats.fairPrice.send()) Print::repaint();
        diff.send_patch();
      };
    private:
      void filter() {
        orders.resetFilters(&filterBidOrders, &filterAskOrders);
        bids = filter(unfiltered.bids, &filterBidOrders);
        asks = filter(unfiltered.asks, &filterAskOrders);
        calcFairValue();
        calcAverageWidth();
      };
      void calcAverageWidth() {
        if (bids.empty() or asks.empty()) return;
        averageWidth = (
          (averageWidth * averageCount)
            + asks.cbegin()->price
            - bids.cbegin()->price
        );
        averageWidth /= ++averageCount;
      };
      const Price resetAverageWidth() {
        averageCount = 0;
        return averageWidth;
      };
      void calcFairValue() {
        if (bids.empty() or asks.empty())
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
          fairValue = gw->dec(fairValue, abs(log10(*product.minTick)));
      };
      const vector<mLevel> filter(vector<mLevel> levels, unordered_map<Price, Amount> *const filterOrders) {
        if (!filterOrders->empty())
          for (auto it = levels.begin(); it != levels.end();) {
            for (auto it_ = filterOrders->begin(); it_ != filterOrders->end();)
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
    Amount baseValue  = 0,
           quoteValue = 0;
     Clock time       = 0;
    mProfit() = default;
    mProfit(Amount b, Amount q)
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
    k.baseValue  = j.value("baseValue",  0.0);
    k.quoteValue = j.value("quoteValue", 0.0);
    k.time       = j.value("time",  (Clock)0);
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
    const double calcDiffPercent(Amount older, Amount newer) const {
      return gw->dec(((newer - older) / newer) * 1e+2, 2);
    };
    const mMatter about() const override {
      return mMatter::Profit;
    };
    void erase() override {
      for (auto it = begin(); it != end();)
        if (it->time + lifetime() > Tstamp) ++it;
        else it = rows.erase(it);
    };
    const double limit() const override {
      return qp.profitHourInterval;
    };
    const Clock lifetime() const override {
      return 3600e+3 * limit();
    };
    protected:
      string explainOK() const override {
        return "loaded % historical Profits";
      };
  };

  struct mOrderFilled: public mTrade {
     string tradeId;
     Amount value,
            feeCharged,
            Kqty,
            Kvalue,
            Kdiff;
      Price Kprice;
      Clock Ktime;
       bool isPong,
            loadedFromDB;
  };
  static void to_json(json &j, const mOrderFilled &k) {
    j = {
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
  static void from_json(const json &j, mOrderFilled &k) {
    k.tradeId      = j.value("tradeId",     "");
    k.price        = j.value("price",      0.0);
    k.quantity     = j.value("quantity",   0.0);
    k.side         = j.value("side",   (Side)0);
    k.time         = j.value("time",  (Clock)0);
    k.value        = j.value("value",      0.0);
    k.Ktime        = j.value("Ktime", (Clock)0);
    k.Kqty         = j.value("Kqty",       0.0);
    k.Kprice       = j.value("Kprice",     0.0);
    k.Kvalue       = j.value("Kvalue",     0.0);
    k.Kdiff        = j.value("Kdiff",      0.0);
    k.feeCharged   = j.value("feeCharged", 0.0);
    k.isPong       = j.value("isPong",   false);
    k.loadedFromDB = true;
  };

  struct mTradesHistory: public mVectorFromDb<mOrderFilled>,
                         public mJsonToClient<mOrderFilled> {
    void clearAll() {
      clear_if([](iterator it) {
        return true;
      });
    };
    void clearOne(const string &tradeId) {
      clear_if([&tradeId](iterator it) {
        return it->tradeId == tradeId;
      }, true);
    };
    void clearClosed() {
      clear_if([](iterator it) {
        return it->Kqty >= it->quantity;
      });
    };
    void clearPongsAuto() {
      const Clock expire = Tstamp - (abs(qp.cleanPongsAuto) * 86400e3);
      const bool forcedClean = qp.cleanPongsAuto < 0;
      clear_if([&expire, &forcedClean](iterator it) {
        return (it->Ktime?:it->time) < expire and (
          forcedClean
          or it->Kqty >= it->quantity
        );
      });
    };
    void insert(const mLastOrder &order) {
      const Amount fee = 0;
      const Clock time = Tstamp;
      mOrderFilled filled = {
        order.side,
        order.price,
        order.tradeQuantity,
        time,
        to_string(time),
        abs(order.price * order.tradeQuantity),
        fee,
        0, 0, 0, 0, 0,
        order.isPong,
        false
      };
      Print::log("GW " + gw->exchange, string(filled.isPong?"PONG":"PING") + " TRADE "
        + (filled.side == Side::Bid ? "BUY  " : "SELL ")
        + gw->str(filled.quantity) + ' ' + gw->base + " at price "
        + gw->str(filled.price) + ' ' + gw->quote + " (value "
        + gw->str(filled.value) + ' ' + gw->quote + ")"
      );
      if (qp.safety == mQuotingSafety::Off or qp.safety == mQuotingSafety::PingPong or qp.safety == mQuotingSafety::PingPoing)
        send_push_back(filled);
      else {
        Price widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * filled.price / 100
          : qp.widthPong;
        map<Price, string> matches;
        for (mOrderFilled &it : rows)
          if (it.quantity - it.Kqty > 0
            and it.side != filled.side
            and (qp.pongAt == mPongAt::AveragePingFair
              or qp.pongAt == mPongAt::AveragePingAggressive
              or (filled.side == Side::Bid
                ? (it.price > filled.price + widthPong)
                : (it.price < filled.price - widthPong)
              )
            )
          ) matches[it.price] = it.tradeId;
        matchPong(
          matches,
          filled,
          (qp.pongAt == mPongAt::LongPingFair or qp.pongAt == mPongAt::LongPingAggressive)
            ? filled.side == Side::Ask
            : filled.side == Side::Bid
        );
      }
      if (qp.cleanPongsAuto)
        clearPongsAuto();
    };
    const mMatter about() const override {
      return mMatter::Trades;
    };
    void erase() override {
      if (crbegin()->Kqty < 0) rows.pop_back();
    };
    const json blob() const override {
      if (crbegin()->Kqty == -1) return nullptr;
      else return mVectorFromDb::blob();
    };
    const string increment() const override {
      return crbegin()->tradeId;
    };
    const json hello() override {
      for (mOrderFilled &it : rows)
        it.loadedFromDB = true;
      return rows;
    };
    protected:
      string explainOK() const override {
        return "loaded % historical Trades";
      };
    private:
      void clear_if(const function<const bool(iterator)> &fn, const bool &onlyOne = false) {
        for (auto it = begin(); it != end();)
          if (fn(it)) {
            it->Kqty = -1;
            it = send_push_erase(it);
            if (onlyOne) break;
          } else ++it;
      };
      void matchPong(const map<Price, string> &matches, mOrderFilled pong, const bool &reverse) {
        if (reverse) for (auto it = matches.crbegin(); it != matches.crend(); ++it) {
          if (!matchPong(it->second, &pong)) break;
        } else for (const auto &it : matches)
          if (!matchPong(it.second, &pong)) break;
        if (pong.quantity > 0) {
          bool eq = false;
          for (auto it = begin(); it != end(); ++it) {
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
      bool matchPong(const string &match, mOrderFilled *const pong) {
        for (auto it = begin(); it != end(); ++it) {
          if (it->tradeId != match) continue;
          Amount Kqty = fmin(pong->quantity, it->quantity - it->Kqty);
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
      void send_push_back(const mOrderFilled &row) {
        rows.push_back(row);
        push();
        if (crbegin()->Kqty < 0) rbegin()->Kqty = -2;
        send();
      };
      iterator send_push_erase(iterator it) {
        mOrderFilled row = *it;
        it = rows.erase(it);
        send_push_back(row);
        erase();
        return it;
      };
  };

  struct mRecentTrade {
     Price price    = 0;
    Amount quantity = 0;
     Clock time     = 0;
    mRecentTrade(const Price &p, const Amount &q)
      : price(p)
      , quantity(q)
      , time(Tstamp)
    {};
  };
  struct mRecentTrades {
    multimap<Price, mRecentTrade> buys,
                                  sells;
                           Amount sumBuys       = 0,
                                  sumSells      = 0;
                            Price lastBuyPrice  = 0,
                                  lastSellPrice = 0;
    void insert(const mLastOrder &order) {
      (order.side == Side::Bid
        ? lastBuyPrice
        : lastSellPrice
      ) = order.price;
      (order.side == Side::Bid
        ? buys
        : sells
      ).insert(pair<Price, mRecentTrade>(
        order.price,
        mRecentTrade(order.price, order.tradeQuantity)
      ));
    };
    void expire() {
      if (buys.size()) expire(&buys);
      if (sells.size()) expire(&sells);
      skip();
      sumBuys = sum(&buys);
      sumSells = sum(&sells);
    };
    private:
      const Amount sum(multimap<Price, mRecentTrade> *const k) const {
        Amount sum = 0;
        for (const auto &it : *k)
          sum += it.second.quantity;
        return sum;
      };
      void expire(multimap<Price, mRecentTrade> *const k) {
        Clock now = Tstamp;
        for (auto it = k->begin(); it != k->end();)
          if (it->second.time + qp.tradeRateSeconds * 1e+3 > now) ++it;
          else it = k->erase(it);
      };
      void skip() {
        while (buys.size() and sells.size()) {
          mRecentTrade &buy = buys.rbegin()->second;
          mRecentTrade &sell = sells.begin()->second;
          if (sell.price < buy.price) break;
          const Amount buyQty = buy.quantity;
          buy.quantity -= sell.quantity;
          sell.quantity -= buyQty;
          if (buy.quantity <= 0)
            buys.erase(buys.rbegin()->first);
          if (sell.quantity <= 0)
            sells.erase(sells.begin()->first);
        }
      };
  };

  struct mSafety: public mJsonToClient<mSafety> {
              double buy      = 0,
                     sell     = 0,
                     combined = 0;
               Price buyPing  = 0,
                     sellPing = 0;
              Amount buySize  = 0,
                     sellSize = 0;
       mRecentTrades recentTrades;
      mTradesHistory trades;
    private_ref:
      const Price  &fairValue;
      const Amount &baseValue,
                   &baseTotal,
                   &targetBasePosition;
    public:
      mSafety(const Price &f, const Amount &v, const Amount &t, const Amount &p)
        : fairValue(f)
        , baseValue(v)
        , baseTotal(t)
        , targetBasePosition(p)
      {};
      void timer_1s() {
        calc();
      };
      void insertTrade(const mLastOrder &order) {
        recentTrades.insert(order);
        trades.insert(order);
        calc();
      };
      void calc() {
        if (!baseValue or !fairValue) return;
        calcSizes();
        calcPrices();
        recentTrades.expire();
        if (empty()) return;
        buy  = recentTrades.sumBuys / buySize;
        sell = recentTrades.sumSells / sellSize;
        combined = (recentTrades.sumBuys + recentTrades.sumSells) / (buySize + sellSize);
        send();
      };
      const bool empty() const {
        return !baseValue or !buySize or !sellSize;
      };
      const mMatter about() const override {
        return mMatter::TradeSafetyValue;
      };
      const bool send_same_blob() const override {
        return false;
      };
    private:
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
      void calcPrices() {
        if (qp.safety == mQuotingSafety::PingPong) {
          buyPing = recentTrades.lastBuyPrice;
          sellPing = recentTrades.lastSellPrice;
        } else {
          buyPing = sellPing = 0;
          if (qp.safety == mQuotingSafety::Off) return;
          Price widthPong = qp.widthPercentage
            ? qp.widthPongPercentage * fairValue / 100
            : qp.widthPong;
          if (qp.safety == mQuotingSafety::PingPoing) {
            if (recentTrades.lastBuyPrice and fairValue > recentTrades.lastBuyPrice - widthPong)
              buyPing = recentTrades.lastBuyPrice;
            if (recentTrades.lastSellPrice and fairValue < recentTrades.lastSellPrice + widthPong)
              sellPing = recentTrades.lastSellPrice;
          } else {
            map<Price, mOrderFilled> tradesBuy;
            map<Price, mOrderFilled> tradesSell;
            for (const mOrderFilled &it: trades)
              (it.side == Side::Bid ? tradesBuy : tradesSell)[it.price] = it;
            Amount buyQty = 0,
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
        }
      };
      void matchFirstPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(true, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchBestPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(true, false, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchLastPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        matchPing(false, true, tradesSide, ping, qty, qtyMax, width, reverse);
      };
      void matchAllPing(map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width) {
        matchPing(false, false, tradesSide, ping, qty, qtyMax, width);
      };
      void matchPing(bool _near, bool _far, map<Price, mOrderFilled> *tradesSide, Price *ping, Amount *qty, Amount qtyMax, Price width, bool reverse = false) {
        int dir = width > 0 ? 1 : -1;
        if (reverse) for (auto it = tradesSide->crbegin(); it != tradesSide->crend(); ++it) {
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it->second.price, it->second.quantity, it->second.price, it->second.Kqty, reverse))
            break;
        } else for (const auto &it : *tradesSide)
          if (matchPing(_near, _far, ping, qty, qtyMax, width, dir * fairValue, dir * it.second.price, it.second.quantity, it.second.price, it.second.Kqty, reverse))
            break;
      };
      const bool matchPing(bool _near, bool _far, Price *ping, Amount *qty, Amount qtyMax, Price width, Price fv, Price price, Amount qtyTrade, Price priceTrade, Amount KqtyTrade, bool reverse) {
        if (reverse) { fv *= -1; price *= -1; width *= -1; }
        if (((!_near and !_far) or *qty < qtyMax)
          and (_far ? fv > price : true)
          and (_near ? (reverse ? fv - width : fv + width) < price : true)
          and KqtyTrade < qtyTrade
        ) {
          Amount qty_ = qtyTrade;
          if (_near or _far)
            qty_ = fmin(qtyMax - *qty, qty_);
          *ping += priceTrade * qty_;
          *qty += qty_;
        }
        return *qty >= qtyMax and (_near or _far);
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

  struct mTarget: public mStructFromDb<mTarget>,
                  public mJsonToClient<mTarget> {
    Amount targetBasePosition = 0,
           positionDivergence = 0;
    private_ref:
      const double   &targetPositionAutoPercentage;
      const mProduct &product;
      const Amount   &baseValue;
    public:
      mTarget(const double &t, const mProduct &p, const Amount &v)
        : targetPositionAutoPercentage(t)
        , product(p)
        , baseValue(v)
      {};
      void calcTargetBasePos() {
        if (warn_empty()) return;
        targetBasePosition = gw->dec(qp.autoPositionMode == mAutoPositionMode::Manual
          ? (qp.percentageValues
            ? qp.targetBasePositionPercentage * baseValue / 1e+2
            : qp.targetBasePosition)
          : targetPositionAutoPercentage * baseValue / 1e+2
        , 4);
        calcPDiv();
        if (send()) {
          push();
          if (debug()) report();
        }
      };
      const bool warn_empty() const {
        const bool err = empty();
        if (err) Print::logWar("PG", "Unable to calculate TBP, missing wallet data");
        return err;
      };
      const bool empty() const {
        return !baseValue;
      };
      const bool realtime() const override {
        return !qp.delayUI;
      };
      const mMatter about() const override {
        return mMatter::TargetBasePosition;
      };
      const bool send_same_blob() const override {
        return false;
      };
    protected:
      const string explain() const override {
        return to_string(targetBasePosition);
      };
      string explainOK() const override {
        return "loaded TBP = % " + gw->base;
      };
    private:
      void calcPDiv() {
        Amount pDiv = qp.percentageValues
          ? qp.positionDivergencePercentage * baseValue / 1e+2
          : qp.positionDivergence;
        if (qp.autoPositionMode == mAutoPositionMode::Manual or mPDivMode::Manual == qp.positionDivergenceMode)
          positionDivergence = pDiv;
        else {
          Amount pDivMin = qp.percentageValues
            ? qp.positionDivergencePercentageMin * baseValue / 1e+2
            : qp.positionDivergenceMin;
          double divCenter = 1 - abs((targetBasePosition / baseValue * 2) - 1);
          if (mPDivMode::Linear == qp.positionDivergenceMode)      positionDivergence = pDivMin + (divCenter * (pDiv - pDivMin));
          else if (mPDivMode::Sine == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sin(divCenter*M_PI_2) * (pDiv - pDivMin));
          else if (mPDivMode::SQRT == qp.positionDivergenceMode)   positionDivergence = pDivMin + (sqrt(divCenter) * (pDiv - pDivMin));
          else if (mPDivMode::Switch == qp.positionDivergenceMode) positionDivergence = divCenter < 1e-1 ? pDivMin : pDiv;
        }
        positionDivergence = gw->dec(positionDivergence, 4);
      };
      void report() const {
        Print::log("PG", "TBP: "
          + to_string((int)(targetBasePosition / baseValue * 1e+2)) + "% = " + gw->str(targetBasePosition)
          + " " + gw->base + ", pDiv: "
          + to_string((int)(positionDivergence / baseValue * 1e+2)) + "% = " + gw->str(positionDivergence)
          + " " + gw->base);
      };
      const bool debug() const {
        return product.debug("wallet");
      };
  };
  static void to_json(json &j, const mTarget &k) {
    j = {
      { "tbp", k.targetBasePosition},
      {"pDiv", k.positionDivergence}
    };
  };
  static void from_json(const json &j, mTarget &k) {
    k.targetBasePosition = j.value("tbp",  0.0);
    k.positionDivergence = j.value("pDiv", 0.0);
  };

  struct mWalletPosition: public mWallets,
                          public mJsonToClient<mWalletPosition> {
     mTarget target;
     mSafety safety;
    mProfits profits;
    private_ref:
      const mProduct &product;
      const mOrders  &orders;
      const Price   &fairValue;
    public:
      mWalletPosition(const mProduct &p, const mOrders &o, const double &t, const Price &f)
        : target(t, p, base.value)
        , safety(f, base.value, base.total, target.targetBasePosition)
        , product(p)
        , orders(o)
        , fairValue(f)
      {};
      const bool ready() const {
        return !safety.empty();
      };
      void read_from_gw(const mWallets &raw) {
        if (raw.base.currency.empty() or raw.quote.currency.empty()) return;
        base.currency = raw.base.currency;
        quote.currency = raw.quote.currency;
        mWallet::reset(raw.base.amount, raw.base.held, &base);
        mWallet::reset(raw.quote.amount, raw.quote.held, &quote);
        calcFunds();
      };
      void calcFunds() {
        calcFundsSilently();
        send();
      };
      void calcFundsAfterOrder(const mLastOrder &order, bool *const askForFees) {
        if (!order.price) return;
        calcHeldAmount(order.side);
        calcFundsSilently();
        if (order.tradeQuantity) {
          safety.insertTrade(order);
          *askForFees = true;
        }
      };
      const mMatter about() const override {
        return mMatter::Position;
      };
      const bool realtime() const override {
        return !qp.delayUI;
      };
      const bool send_asap() const override {
        return false;
      };
      const bool send_same_blob() const override {
        return false;
      };
    private:
      void calcFundsSilently() {
        if (base.currency.empty() or quote.currency.empty() or !fairValue) return;
        if (product.maxWallet()) calcMaxWallet();
        calcValues();
        calcProfits();
        target.calcTargetBasePos();
      };
      void calcHeldAmount(const Side &side) {
        const Amount heldSide = orders.heldAmount(side);
        if (side == Side::Ask and !base.currency.empty())
          mWallet::reset(base.total - heldSide, heldSide, &base);
        else if (side == Side::Bid and !quote.currency.empty())
          mWallet::reset(quote.total - heldSide, heldSide, &quote);
      };
      void calcValues() {
        base.value  = (quote.total / fairValue) + base.total;
        quote.value = (base.total * fairValue) + quote.total;
      };
      void calcProfits() {
        if (!profits.ratelimit())
          profits.push_back(mProfit(base.value, quote.value));
        base.profit  = profits.calcBaseDiff();
        quote.profit = profits.calcQuoteDiff();
      };
      void calcMaxWallet() {
        Amount maxWallet = product.maxWallet();
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
    void kiss(json *const j) override {
      if (!j->is_object() or !j->value("price", 0.0) or !j->value("quantity", 0.0))
        *j = nullptr;
    };
    const mMatter about() const override {
      return mMatter::SubmitNewOrder;
    };
  };
  struct mButtonCancelOrder: public mFromClient {
    void kiss(json *const j) override {
      *j = (j->is_object() and !j->value("orderId", "").empty())
        ? j->at("orderId").get<RandId>()
        : nullptr;
    };
    const mMatter about() const override {
      return mMatter::CancelOrder;
    };
  };
  struct mButtonCancelAllOrders: public mFromClient {
    const mMatter about() const override {
      return mMatter::CancelAllOrders;
    };
  };
  struct mButtonCleanAllClosedTrades: public mFromClient {
    const mMatter about() const override {
      return mMatter::CleanAllClosedTrades;
    };
  };
  struct mButtonCleanAllTrades: public mFromClient {
    const mMatter about() const override {
      return mMatter::CleanAllTrades;
    };
  };
  struct mButtonCleanTrade: public mFromClient {
    void kiss(json *const j) override {
      *j = (j->is_object() and !j->value("tradeId", "").empty())
        ? j->at("tradeId").get<string>()
        : nullptr;
    };
    const mMatter about() const override {
      return mMatter::CleanTrade;
    };
  };
  struct mNotepad: public mJsonToClient<mNotepad> {
    string content;
    void kiss(json *const j) override {
      if (j->is_array() and j->size() and j->at(0).is_string())
       content = j->at(0);
    };
    const mMatter about() const override {
      return mMatter::Notepad;
    };
  };
  static void to_json(json &j, const mNotepad &k) {
    j = k.content;
  };

  struct mButtons {
    mNotepad                    notepad;
    mButtonSubmitNewOrder       submit;
    mButtonCancelOrder          cancel;
    mButtonCancelAllOrders      cancelAll;
    mButtonCleanAllClosedTrades cleanTradesClosed;
    mButtonCleanAllTrades       cleanTrades;
    mButtonCleanTrade           cleanTrade;
  };

  struct mSemaphore: public mJsonToClient<mSemaphore> {
    Connectivity greenButton  = Connectivity::Disconnected,
                 greenGateway = Connectivity::Disconnected;
    private:
      Connectivity adminAgreement = Connectivity::Disconnected;
    public:
      void kiss(json *const j) override {
        if (j->is_object()
          and j->at("agree").is_number()
          and j->at("agree").get<Connectivity>() != adminAgreement
        ) toggle();
      };
      const bool paused() const {
        return !(bool)greenButton;
      };
      const bool offline() const {
        return !(bool)greenGateway;
      };
      void agree(const bool &agreement) {
        adminAgreement = (Connectivity)agreement;
      };
      void toggle() {
        agree(!(bool)adminAgreement);
        switchFlag();
      };
      void read_from_gw(const Connectivity &raw) {
        if (greenGateway != raw) {
          greenGateway = raw;
          switchFlag();
        }
      };
      const mMatter about() const override {
        return mMatter::Connectivity;
      };
    private:
      void switchFlag() {
        const Connectivity previous = greenButton;
        greenButton = (Connectivity)(
          (bool)greenGateway and (bool)adminAgreement
        );
        if (greenButton != previous)
          Print::log("GW " + gw->exchange, "Quoting state changed to",
            string(paused() ? "DIS" : "") + "CONNECTED");
        send();
        Print::repaint();
      };
  };
  static void to_json(json &j, const mSemaphore &k) {
    j = {
      { "agree", k.greenButton },
      {"online", k.greenGateway}
    };
  };

  struct mQuote: public mLevel {
    const Side        side   = (Side)0;
          mQuoteState state  = mQuoteState::MissingData;
          bool        isPong = false;
    mQuote(const Side &s)
      : side(s)
    {};
    const bool empty() const {
      return !size or !price;
    };
    void skip() {
      size = 0;
    };
    void clear(const mQuoteState &reason) {
      price = size = 0;
      state = reason;
    };
    virtual const bool deprecates(const Price&) const = 0;
    const bool checkCrossed(const mQuote &opposite) {
      if (empty()) return false;
      if (opposite.empty() or deprecates(opposite.price)) {
        state = mQuoteState::Live;
        return false;
      }
      state = mQuoteState::Crossed;
      return true;
    };
  };
  struct mQuoteBid: public mQuote {
    mQuoteBid()
      : mQuote(Side::Bid)
    {};
    const bool deprecates(const Price &higher) const override {
      return price < higher;
    };
  };
  struct mQuoteAsk: public mQuote {
    mQuoteAsk()
      : mQuote(Side::Ask)
    {};
    const bool deprecates(const Price &lower) const override {
      return price > lower;
    };
  };
  struct mQuotes {
    mQuoteBid bid;
    mQuoteAsk ask;
         bool superSpread = false;
    private_ref:
      const mProduct &product;
    public:
      mQuotes(const mProduct &p)
        : product(p)
      {};
      void checkCrossedQuotes() {
        if ((unsigned int)bid.checkCrossed(ask)
          | (unsigned int)ask.checkCrossed(bid)
        ) Print::logWar("QE", "Crossed bid/ask quotes detected, that is.. unexpected");
      };
      void debug(const string &reason) {
        if (debug())
          Print::log("DEBUG QE", reason);
      };
      void debuq(const string &step) {
        if (debug())
          Print::log("DEBUG QE", "[" + step + "] "
            + to_string((int)bid.state) + ":"
            + to_string((int)ask.state) + " "
            + ((json){{"bid", bid}, {"ask", ask}}).dump()
          );
      };
    private:
      const bool debug() const {
        return product.debug("quotes");
      };
  };

  struct mDummyMarketMaker {
    private:
      void (*calcRawQuotesFromMarket)(
        const mMarketLevels&,
        const Price&,
        const Price&,
        const Amount&,
        const Amount&,
              mQuotes&
      ) = nullptr;
    private_ref:
      const mProduct        &product;
      const mMarketLevels   &levels;
      const mWalletPosition &wallet;
            mQuotes         &quotes;
    public:
      mDummyMarketMaker(const mProduct &p, const mMarketLevels &l, const mWalletPosition &w, mQuotes &q)
        : product(p)
        , levels(l)
        , wallet(w)
        , quotes(q)
      {};
      void mode(const string &reason) {
        if (qp.mode == mQuotingMode::Top)              calcRawQuotesFromMarket = calcTopOfMarket;
        else if (qp.mode == mQuotingMode::Mid)         calcRawQuotesFromMarket = calcMidOfMarket;
        else if (qp.mode == mQuotingMode::Join)        calcRawQuotesFromMarket = calcJoinMarket;
        else if (qp.mode == mQuotingMode::InverseJoin) calcRawQuotesFromMarket = calcInverseJoinMarket;
        else if (qp.mode == mQuotingMode::InverseTop)  calcRawQuotesFromMarket = calcInverseTopOfMarket;
        else if (qp.mode == mQuotingMode::HamelinRat)  calcRawQuotesFromMarket = calcColossusOfMarket;
        else if (qp.mode == mQuotingMode::Depth)       calcRawQuotesFromMarket = calcDepthOfMarket;
        else error("QE", "Invalid quoting mode " + reason + ", consider to remove the database file");
      };
      void calcRawQuotes() const  {
        calcRawQuotesFromMarket(
          levels,
          *product.minTick,
          levels.calcQuotesWidth(&quotes.superSpread),
          wallet.safety.buySize,
          wallet.safety.sellSize,
          quotes
        );
        if (quotes.bid.price <= 0 or quotes.ask.price <= 0) {
          quotes.bid.clear(mQuoteState::WidthMustBeSmaller);
          quotes.ask.clear(mQuoteState::WidthMustBeSmaller);
          Print::logWar("QP", "Negative price detected, widthPing must be smaller");
        }
      };
    private:
      static void quoteAtTopOfMarket(const mMarketLevels &levels, const Price &minTick, mQuotes &quotes) {
        const mLevel &topBid = levels.bids.begin()->size > minTick
          ? levels.bids.at(0)
          : levels.bids.at(levels.bids.size() > 1 ? 1 : 0);
        const mLevel &topAsk = levels.asks.begin()->size > minTick
          ? levels.asks.at(0)
          : levels.asks.at(levels.asks.size() > 1 ? 1 : 0);
        quotes.bid.price = topBid.price;
        quotes.ask.price = topAsk.price;
      };
      static void calcTopOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price + minTick);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price - minTick);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcMidOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quotes.bid.price = fmax(levels.fairValue - widthPing, 0);
        quotes.ask.price = levels.fairValue + widthPing;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcJoinMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.price = fmin(levels.fairValue - widthPing / 2.0, quotes.bid.price);
        quotes.ask.price = fmax(levels.fairValue + widthPing / 2.0, quotes.ask.price);
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseJoinMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        Price mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcInverseTopOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        Price mktWidth = abs(quotes.ask.price - quotes.bid.price);
        if (mktWidth > widthPing) {
          quotes.ask.price = quotes.ask.price + widthPing;
          quotes.bid.price = quotes.bid.price - widthPing;
        }
        quotes.bid.price = quotes.bid.price + minTick;
        quotes.ask.price = quotes.ask.price - minTick;
        if (mktWidth < (2.0 * widthPing / 3.0)) {
          quotes.ask.price = quotes.ask.price + widthPing / 4.0;
          quotes.bid.price = quotes.bid.price - widthPing / 4.0;
        }
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcColossusOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &widthPing,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        quoteAtTopOfMarket(levels, minTick, quotes);
        quotes.bid.size = 0;
        quotes.ask.size = 0;
        for (const mLevel &it : levels.bids)
          if (quotes.bid.size < it.size and it.price <= quotes.bid.price) {
            quotes.bid.size = it.size;
            quotes.bid.price = it.price;
          }
        for (const mLevel &it : levels.asks)
          if (quotes.ask.size < it.size and it.price >= quotes.ask.price) {
            quotes.ask.size = it.size;
            quotes.ask.price = it.price;
          }
        if (quotes.bid.size) quotes.bid.price += minTick;
        if (quotes.ask.size) quotes.ask.price -= minTick;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
      static void calcDepthOfMarket(
        const mMarketLevels &levels,
        const Price         &minTick,
        const Price         &depth,
        const Amount        &bidSize,
        const Amount        &askSize,
              mQuotes       &quotes
      ) {
        Price bidPx = levels.bids.cbegin()->price;
        Amount bidDepth = 0;
        for (const mLevel &it : levels.bids) {
          bidDepth += it.size;
          if (bidDepth >= depth) break;
          else bidPx = it.price;
        }
        Price askPx = levels.asks.cbegin()->price;
        Amount askDepth = 0;
        for (const mLevel &it : levels.asks) {
          askDepth += it.size;
          if (askDepth >= depth) break;
          else askPx = it.price;
        }
        quotes.bid.price = bidPx;
        quotes.ask.price = askPx;
        quotes.bid.size = bidSize;
        quotes.ask.size = askSize;
      };
  };

  struct mAntonioCalculon: public mJsonToClient<mAntonioCalculon> {
                  mQuotes quotes;
        mDummyMarketMaker dummyMM;
    vector<const mOrder*> zombies;
             unsigned int countWaiting = 0,
                          countWorking = 0,
                          AK47inc      = 0;
                   string sideAPR      = "Off";
    private_ref:
      const mProduct        &product;
      const mMarketLevels   &levels;
      const mWalletPosition &wallet;
    public:
      mAntonioCalculon(const mProduct &p, const mMarketLevels &l, const mWalletPosition &w)
        : quotes(p)
        , dummyMM(p, l, w, quotes)
        , product(p)
        , levels(l)
        , wallet(w)
      {};
      vector<const mOrder*> clear() {
        send();
        states(mQuoteState::MissingData);
        countWaiting =
        countWorking = 0;
        vector<const mOrder*> zombies_;
        zombies.swap(zombies_);
        return zombies_;
      };
      void offline() {
        states(mQuoteState::Disconnected);
      };
      void paused() {
        states(mQuoteState::DisabledQuotes);
      };
      void calcQuotes() {
        states(mQuoteState::UnknownHeld);
        dummyMM.calcRawQuotes();
        applyQuotingParameters();
      };
      const bool abandon(const mOrder &order, mQuote &quote, unsigned int &bullets) {
        if (stillAlive(order)) {
          if (abs(order.price - quote.price) < *product.minTick)
            quote.skip();
          else if (order.status == Status::Waiting) {
            if (qp.safety != mQuotingSafety::AK47
              or !--bullets
            ) quote.skip();
          } else if (qp.safety != mQuotingSafety::AK47
            or quote.deprecates(order.price)
          ) {
            if (product.lifetime() and order.time + product.lifetime() > Tstamp)
              quote.skip();
            else return true;
          }
        }
        return false;
      };
      const mMatter about() const override {
        return mMatter::QuoteStatus;
      };
      const bool realtime() const override {
        return !qp.delayUI;
      };
      const bool send_same_blob() const override {
        return false;
      };
    private:
      void states(const mQuoteState &state) {
        quotes.bid.state =
        quotes.ask.state = state;
      };
      const bool stillAlive(const mOrder &order) {
        if (order.status == Status::Waiting) {
          if (Tstamp - 10e+3 > order.time) {
            zombies.push_back(&order);
            return false;
          }
          ++countWaiting;
        } else ++countWorking;
        return !order.disablePostOnly;
      };
      void applyQuotingParameters() {
        quotes.debuq("?"); applySuperTrades();
        quotes.debuq("A"); applyEwmaProtection();
        quotes.debuq("B"); applyTotalBasePosition();
        quotes.debuq("C"); applyStdevProtection();
        quotes.debuq("D"); applyAggressivePositionRebalancing();
        quotes.debuq("E"); applyAK47Increment();
        quotes.debuq("F"); applyBestWidth();
        quotes.debuq("G"); applyTradesPerMinute();
        quotes.debuq("H"); applyRoundPrice();
        quotes.debuq("I"); applyRoundSize();
        quotes.debuq("J"); applyDepleted();
        quotes.debuq("K"); applyWaitingPing();
        quotes.debuq("L"); applyEwmaTrendProtection();
        quotes.debuq("!");
        quotes.debug("totals " + ("toAsk: " + to_string(wallet.base.total))
                               + ",toBid: " + to_string(wallet.quote.total / levels.fairValue));
        quotes.checkCrossedQuotes();
      };
      void applySuperTrades() {
        if (!quotes.superSpread
          or (qp.superTrades != mSOP::Size and qp.superTrades != mSOP::TradesSize)
        ) return;
        if (!qp.buySizeMax and !quotes.bid.empty())
          quotes.bid.size = fmin(
            qp.sopSizeMultiplier * quotes.bid.size,
            (wallet.quote.amount / levels.fairValue) / 2
          );
        if (!qp.sellSizeMax and !quotes.ask.empty())
          quotes.ask.size = fmin(
            qp.sopSizeMultiplier * quotes.ask.size,
            wallet.base.amount / 2
          );
      };
      void applyEwmaProtection() {
        if (!qp.protectionEwmaQuotePrice or !levels.stats.ewma.mgEwmaP) return;
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(levels.stats.ewma.mgEwmaP, quotes.ask.price);
        if (!quotes.bid.empty())
          quotes.bid.price = fmin(levels.stats.ewma.mgEwmaP, quotes.bid.price);
      };
      void applyTotalBasePosition() {
        if (wallet.base.total < wallet.target.targetBasePosition - wallet.target.positionDivergence) {
          quotes.ask.clear(mQuoteState::TBPHeld);
          if (!quotes.bid.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = "Buy";
            if (!qp.buySizeMax)
              quotes.bid.size = fmin(
                qp.aprMultiplier * quotes.bid.size,
                wallet.target.targetBasePosition - wallet.base.total
              );
          }
        }
        else if (wallet.base.total >= wallet.target.targetBasePosition + wallet.target.positionDivergence) {
          quotes.bid.clear(mQuoteState::TBPHeld);
          if (!quotes.ask.empty() and qp.aggressivePositionRebalancing != mAPR::Off) {
            sideAPR = "Sell";
            if (!qp.sellSizeMax)
              quotes.ask.size = fmin(
                qp.aprMultiplier * quotes.ask.size,
                wallet.base.total - wallet.target.targetBasePosition
              );
          }
        }
        else sideAPR = "Off";
      };
      void applyStdevProtection() {
        if (qp.quotingStdevProtection == mSTDEV::Off or !levels.stats.stdev.fair) return;
        if (!quotes.ask.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != "Sell"
        ))
          quotes.ask.price = fmax(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.askMean )
              : levels.fairValue) + ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.ask )),
            quotes.ask.price
          );
        if (!quotes.bid.empty() and (
          qp.quotingStdevProtection == mSTDEV::OnFV
          or qp.quotingStdevProtection == mSTDEV::OnTops
          or qp.quotingStdevProtection == mSTDEV::OnTop
          or sideAPR != "Buy"
        ))
          quotes.bid.price = fmin(
            (qp.quotingStdevBollingerBands
              ? (qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fairMean : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.topMean : levels.stats.stdev.bidMean )
              : levels.fairValue) - ((qp.quotingStdevProtection == mSTDEV::OnFV or qp.quotingStdevProtection == mSTDEV::OnFVAPROff)
                ? levels.stats.stdev.fair : ((qp.quotingStdevProtection == mSTDEV::OnTops or qp.quotingStdevProtection == mSTDEV::OnTopsAPROff)
                  ? levels.stats.stdev.top : levels.stats.stdev.bid )),
            quotes.bid.price
          );
      };
      void applyAggressivePositionRebalancing() {
        if (qp.safety == mQuotingSafety::Off) return;
        const Price widthPong = qp.widthPercentage
          ? qp.widthPongPercentage * levels.fairValue / 100
          : qp.widthPong;
        if (!quotes.ask.empty() and wallet.safety.buyPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == "Sell")
            or ((qp.safety == mQuotingSafety::PingPong or qp.safety == mQuotingSafety::PingPoing)
              ? quotes.ask.price < wallet.safety.buyPing + widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) quotes.ask.price = wallet.safety.buyPing + widthPong;
          quotes.ask.isPong = quotes.ask.price >= wallet.safety.buyPing + widthPong;
        }
        if (!quotes.bid.empty() and wallet.safety.sellPing) {
          if ((qp.aggressivePositionRebalancing == mAPR::SizeWidth and sideAPR == "Buy")
            or ((qp.safety == mQuotingSafety::PingPong or qp.safety == mQuotingSafety::PingPoing)
              ? quotes.bid.price > wallet.safety.sellPing - widthPong
              : qp.pongAt == mPongAt::ShortPingAggressive
                or qp.pongAt == mPongAt::AveragePingAggressive
                or qp.pongAt == mPongAt::LongPingAggressive
            )
          ) quotes.bid.price = wallet.safety.sellPing - widthPong;
          quotes.bid.isPong = quotes.bid.price <= wallet.safety.sellPing - widthPong;
        }
      };
      void applyAK47Increment() {
        if (qp.safety != mQuotingSafety::AK47) return;
        const Price range = qp.percentageValues
          ? qp.rangePercentage * wallet.base.value / 100
          : qp.range;
        if (!quotes.bid.empty())
          quotes.bid.price -= AK47inc * range;
        if (!quotes.ask.empty())
          quotes.ask.price += AK47inc * range;
        if (++AK47inc > qp.bullets) AK47inc = 0;
      };
      void applyBestWidth() {
        if (!qp.bestWidth) return;
        const Amount bestWidthSize = (sideAPR=="Off" ? qp.bestWidthSize : 0);
        Amount depth = 0;
        if (!quotes.ask.empty())
          for (const mLevel &it : levels.asks)
            if (it.price > quotes.ask.price) {
              depth += it.size;
              if (depth < bestWidthSize) continue;
              const Price bestAsk = it.price - *product.minTick;
              if (bestAsk > levels.fairValue) {
                quotes.ask.price = bestAsk;
                break;
              }
            }
        depth = 0;
        if (!quotes.bid.empty())
          for (const mLevel &it : levels.bids)
            if (it.price < quotes.bid.price) {
              depth += it.size;
              if (depth < bestWidthSize) continue;
              const Price bestBid = it.price + *product.minTick;
              if (bestBid < levels.fairValue) {
                quotes.bid.price = bestBid;
                break;
              }
            }
      };
      void applyTradesPerMinute() {
        const double factor = (quotes.superSpread and (
          qp.superTrades == mSOP::Trades or qp.superTrades == mSOP::TradesSize
        )) ? qp.sopWidthMultiplier
           : 1;
        if (wallet.safety.sell >= qp.tradesPerMinute * factor)
          quotes.ask.clear(mQuoteState::MaxTradesSeconds);
        if (wallet.safety.buy >= qp.tradesPerMinute * factor)
          quotes.bid.clear(mQuoteState::MaxTradesSeconds);
      };
      void applyRoundPrice() {
        if (!quotes.bid.empty())
          quotes.bid.price = fmax(
            0,
            floor(quotes.bid.price / *product.minTick) * *product.minTick
          );
        if (!quotes.ask.empty())
          quotes.ask.price = fmax(
            quotes.bid.price + *product.minTick,
            ceil(quotes.ask.price / *product.minTick) * *product.minTick
          );
      };
      void applyRoundSize() {
        if (!quotes.ask.empty())
          quotes.ask.size = floor(fmax(
            fmin(
              quotes.ask.size,
              wallet.base.total
            ),
            *product.minSize
          ) / 1e-8) * 1e-8;
        if (!quotes.bid.empty())
          quotes.bid.size = floor(fmax(
            fmin(
              quotes.bid.size,
              wallet.quote.total / levels.fairValue
            ),
            *product.minSize
          ) / 1e-8) * 1e-8;
      };
      void applyDepleted() {
        if (quotes.bid.size > wallet.quote.total / levels.fairValue)
          quotes.bid.clear(mQuoteState::DepletedFunds);
        if (quotes.ask.size > wallet.base.total)
          quotes.ask.clear(mQuoteState::DepletedFunds);
      };
      void applyWaitingPing() {
        if (qp.safety == mQuotingSafety::Off) return;
        if (!quotes.ask.isPong and (
          (quotes.bid.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedBidSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::BidSide
          or qp.pingAt == mPingAt::DepletedAskSide
        )) quotes.ask.clear(mQuoteState::WaitingPing);
        if (!quotes.bid.isPong and (
          (quotes.ask.state != mQuoteState::DepletedFunds and (qp.pingAt == mPingAt::DepletedSide or qp.pingAt == mPingAt::DepletedAskSide))
          or qp.pingAt == mPingAt::StopPings
          or qp.pingAt == mPingAt::AskSide
          or qp.pingAt == mPingAt::DepletedBidSide
        )) quotes.bid.clear(mQuoteState::WaitingPing);
      };
      void applyEwmaTrendProtection() {
        if (!qp.quotingEwmaTrendProtection or !levels.stats.ewma.mgEwmaTrendDiff) return;
        if (levels.stats.ewma.mgEwmaTrendDiff > qp.quotingEwmaTrendThreshold)
          quotes.ask.clear(mQuoteState::UpTrendHeld);
        else if (levels.stats.ewma.mgEwmaTrendDiff < -qp.quotingEwmaTrendThreshold)
          quotes.bid.clear(mQuoteState::DownTrendHeld);
      };
  };
  static void to_json(json &j, const mAntonioCalculon &k) {
    j = {
      {            "bidStatus", k.quotes.bid.state},
      {            "askStatus", k.quotes.ask.state},
      {              "sideAPR", k.sideAPR         },
      {"quotesInMemoryWaiting", k.countWaiting    },
      {"quotesInMemoryWorking", k.countWorking    },
      {"quotesInMemoryZombies", k.zombies.size()  }
    };
  };

  struct mBroker {
          mSemaphore semaphore;
    mAntonioCalculon calculon;
    private_ref:
      mOrders &orders;
    public:
      mBroker(const mProduct &p, mOrders &o, const mMarketLevels &l, const mWalletPosition &w)
        : calculon(p, l, w)
        , orders(o)
      {};
      const bool ready() {
        if (semaphore.offline()) {
          calculon.offline();
          return false;
        }
        return true;
      };
      const bool calcQuotes() {
        if (semaphore.paused()) {
          calculon.paused();
          return false;
        }
        calculon.calcQuotes();
        return true;
      };
      const vector<mOrder*> abandon(mQuote &quote) {
        vector<mOrder*> abandoned;
        unsigned int bullets = qp.bullets;
        const bool all = quote.state != mQuoteState::Live;
        for (mOrder *const it : orders.at(quote.side))
          if (all or calculon.abandon(*it, quote, bullets))
            abandoned.push_back(it);
        return abandoned;
      };
      void clear() {
        for (const mOrder *const it : calculon.clear())
          orders.purge(it);
      };
  };

  struct mMonitor: public mJsonToClient<mMonitor> {
    unsigned int /* ( | L | ) */ /* more */ orders_60s /* ? */;
    const string /*  )| O |(  */  * unlock;
        mProduct /* ( | C | ) */ /* this */ product;
                 /*  )| K |(  */ /* thanks! <3 */
    private_ref:
      const Option &option;
    public:
      mMonitor(const Option &o)
        : orders_60s(0)
        , unlock(nullptr)
        , product(o)
        , option(o)
      {};
      const unsigned int memSize() const {
  #ifdef _WIN32
        return 0;
  #else
        struct rusage ru;
        return getrusage(RUSAGE_SELF, &ru) ? 0 : ru.ru_maxrss * 1e+3;
  #endif
      };
      const unsigned int dbSize() const {
        if (option.str("database") == ":memory:") return 0;
        struct stat st;
        return stat(option.str("database").data(), &st) ? 0 : st.st_size;
      };
      const unsigned int theme() const {
        return option.num("ignore-moon") + option.num("ignore-sun");
      };
      void tick_orders() {
        orders_60s++;
      };
      void timer_60s() {
        send();
        orders_60s = 0;
      };
      const mMatter about() const override {
        return mMatter::ApplicationState;
      };
  };
  static void to_json(json &j, const mMonitor &k) {
    j = {
      {     "a", *k.unlock             },
      {  "inet", Curl::inet
                   ? string(Curl::inet)
                   : ""                },
      {  "freq", k.orders_60s          },
      { "theme", k.theme()             },
      {"memory", k.memSize()           },
      {"dbsize", k.dbSize()            }
    };
  };
}

#endif
