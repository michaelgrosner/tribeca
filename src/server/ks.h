#ifndef K_KS_H_
#define K_KS_H_

#define mClock  unsigned long long
#define mPrice  double
#define mAmount double
#define mRandId string
#define mCoinId string

#ifndef M_PI_2
#define M_PI_2 1.5707963267948965579989817342720925807952880859375
#endif

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
  enum class mFairValueModel: unsigned int { BBO, wBBO };
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
    ProductAdvertisement = 'j', ApplicationState     = 'k', Notepad            = 'l',
    Position             = 'n',                             SubmitNewOrder     = 'p', CancelOrder        = 'q', MarketTrade      = 'r',
    Trades               = 's', ExternalValuation    = 't', QuoteStatus        = 'u', TargetBasePosition = 'v', TradeSafetyValue = 'w',
    CancelAllOrders      = 'x', CleanAllClosedTrades = 'y', CleanAllTrades     = 'z', CleanTrade         = 'A',
    TradesChart          = 'B', WalletChart          = 'C', EWMAChart          = 'D', MarketDataLongTerm = 'G'
  };
  static          bool operator! (mConnectivity k_)                   { return !(unsigned int)k_; };
  static mConnectivity operator* (mConnectivity _k, mConnectivity k_) { return (mConnectivity)((unsigned int)_k * (unsigned int)k_); };
  static struct mQuotingParams {
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
    bool empty() {
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
    bool empty() {
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
    mProfit(mAmount b, mAmount q, mClock t):
      baseValue(b), quoteValue(q), time(t)
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
  struct mSafety {
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
    bool empty() {
      return !buySize and !sellSize;
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
  struct mPosition {
    mAmount baseAmount,
            quoteAmount,
            _quoteAmountValue,
            baseHeldAmount,
            quoteHeldAmount,
            _baseTotal,
            _quoteTotal,
            baseValue,
            quoteValue,
            profitBase,
            profitQuote;
      mPair pair;
    mPosition():
      baseAmount(0), quoteAmount(0), _quoteAmountValue(0), baseHeldAmount(0), quoteHeldAmount(0), _baseTotal(0), _quoteTotal(0), baseValue(0), quoteValue(0), profitBase(0), profitQuote(0), pair(mPair())
    {};
    mPosition(mAmount bA, mAmount qA, mAmount qAV, mAmount bH, mAmount qH, mAmount bT, mAmount qT, mAmount bV, mAmount qV, mAmount bP, mAmount qP, mPair p):
      baseAmount(bA), quoteAmount(qA), _quoteAmountValue(qAV), baseHeldAmount(bH), quoteHeldAmount(qH), _baseTotal(bT), _quoteTotal(qT), baseValue(bV), quoteValue(qV), profitBase(bP), profitQuote(qP), pair(p)
    {};
    bool empty() {
      return !baseValue;
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
    mTrade(mPrice p, mAmount q, mSide s):
      tradeId(""), pair(mPair()), price(p), quantity(q), side(s), time(0), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
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
    k.loadedFromDB = j.value("loadedFromDB", false);
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
  struct mLevel {
     mPrice price;
    mAmount size;
    mLevel():
      price(0), size(0)
    {};
    mLevel(mPrice p, mAmount s):
      price(p), size(s)
    {};
    inline void clear() {
      price = size = 0;
    };
    inline bool empty() {
      return !price or !size;
    };
  };
  static void to_json(json &j, const mLevel &k) {
    if (k.size)
      j = {
        {"price", k.price},
        { "size", k.size }
      };
    else
      j = {
        {"price", k.price}
      };
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
    inline mPrice spread() {
      return empty() ? 0 : asks.begin()->price - bids.begin()->price;
    };
    inline bool empty() {
      return bids.empty() or asks.empty();
    };
  };
  static void to_json(json &j, const mLevels &k) {
    j = {
      {"bids", k.bids},
      {"asks", k.asks}
    };
  };
  struct mLevelsDiff: public mLevels {
    inline vector<mLevel> diff(const vector<mLevel> &from, vector<mLevel> to) {
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
    inline json diff(const mLevels &to) {
      bids = diff(bids, to.bids);
      asks = diff(asks, to.asks);
      json patch = *this;
      reset(to);
      return patch;
    }
    inline mLevels reset(const mLevels &from) {
      bids = from.bids;
      asks = from.asks;
      return from;
    };
  };
  static void to_json(json &j, const mLevelsDiff &k) {
    j = {
      {"bids", k.bids},
      {"asks", k.asks},
      {"diff", true}
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
  struct mQuoteStatus {
     mQuoteState bidStatus,
                 askStatus;
    unsigned int quotesInMemoryNew,
                 quotesInMemoryWorking,
                 quotesInMemoryDone;
    mQuoteStatus():
      bidStatus((mQuoteState)0), askStatus((mQuoteState)0), quotesInMemoryNew(0), quotesInMemoryWorking(0), quotesInMemoryDone(0)
    {};
    mQuoteStatus(mQuoteState b, mQuoteState a, unsigned int n, unsigned int w, unsigned int d):
      bidStatus(b), askStatus(a), quotesInMemoryNew(n), quotesInMemoryWorking(w), quotesInMemoryDone(d)
    {};
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
  static struct mArgs {
        int port          = 3000,   colors      = 0, debug        = 0,
            debugSecret   = 0,      debugEvents = 0, debugOrders  = 0,
            debugQuotes   = 0,      debugWallet = 0, withoutSSL   = 0,
            headless      = 0,      dustybot    = 0, lifetime     = 0,
            autobot       = 0,      naked       = 0, free         = 0,
            ignoreSun     = 0,      ignoreMoon  = 0, maxLevels    = 0,
            maxAdmins     = 7,      testChamber = 0;
    mAmount maxWallet     = 0;
     mPrice ewmaUShort    = 0,      ewmaXShort  = 0, ewmaShort    = 0,
            ewmaMedium    = 0,      ewmaLong    = 0, ewmaVeryLong = 0;
     string title         = "K.sh", matryoshka  = "https://www.example.com/",
            user          = "NULL", pass        = "NULL",
            exchange      = "NULL", currency    = "NULL",
            apikey        = "NULL", secret      = "NULL",
            username      = "NULL", passphrase  = "NULL",
            http          = "NULL", wss         = "NULL",
            database      = "",     diskdata    = "",
            whitelist     = "";
    const char *inet = nullptr;
  } args;
  static struct mMemory {
    function<json(const mMatter&)> select;
    void insert(
      const mMatter &table            ,
      const json    &cell             ,
      const bool    &rm       = true  ,
      const string  &updateId = "NULL",
      const mClock  &rmOlder  = 0
    ) {
      delete_insert(table, cell, rm, updateId, rmOlder);
    };
    function<void(const mMatter&, const json&, const bool&, const string&, const mClock&)> delete_insert;
    function<unsigned int()> size = []() { return 0; };
  } sqlite;
  static struct mClient {
    function<void()> timer_Xs   = []() {},
                     timer_60s  = []() {};
    function<void(const mMatter&, function<void(json*)>*)>       welcome = [](const mMatter &type, function<void(json*)> *fn) {};
    function<void(const mMatter&, function<void(const json&)>*)> clickme = [](const mMatter &type, function<void(const json&)> *fn) {};
    function<void(const mMatter&, const json&)>                  send;
  } client;
  static struct mEngine {
    function<void()> timer_1s,
                     calcQuote,
                     calcQuoteAfterSavedParams;
        unsigned int orders_60s = 0;
       mConnectivity greenButton  = mConnectivity::Disconnected,
                     greenGateway = mConnectivity::Disconnected;
  } engine;
  static class Gw {
    public:
      virtual string A() = 0;
      uWS::Hub *hub = nullptr;
      static Gw *config(mCoinId, mCoinId, string, int, string, string, string, string, string, string, int, int);
      function<void(const string&)> log,
                                    reconnect;
      function<void(mOrder)>        evDataOrder;
      function<void(mTrade)>        evDataTrade;
      function<void(mLevels)>       evDataLevels;
      function<void(mWallets)>      evDataWallet;
      function<void(mConnectivity)> evConnectOrder,
                                    evConnectMarket;
      function<void(mRandId, string)> replace;
      virtual void place(mRandId, mSide, string, string, mOrderType, mTimeInForce, bool, mClock) = 0,
                   cancel(mRandId, mRandId) = 0,
                   close() = 0;
      mRandId (*randId)() = nullptr;
      mExchange exchange = (mExchange)0;
            int version  = 0, maxLevel = 0,
                debug    = 0;
         mPrice minTick  = 0;
        mAmount makeFee  = 0, takeFee  = 0,
                minSize  = 0;
        mCoinId base     = "", quote   = "";
         string name     = "", symbol  = "",
                apikey   = "", secret  = "",
                user     = "", pass    = "",
                ws       = "", http    = "";
      bool refreshWallet = false,
                   async = false;
      virtual bool asyncWs() = 0;
      inline bool waitForData() {
        return waitFor(replyOrders, evDataOrder)
             | waitFor(replyLevels, evDataLevels)
             | waitFor(replyTrades, evDataTrade)
             | waitFor(replyWallets, evDataWallet)
             | waitFor(replyCancelAll, evDataOrder);
      };
      function<bool()> wallet = [&]() { return !(async_wallet() or !askFor(replyWallets, [&]() { return sync_wallet(); })); };
      function<bool()> levels = [&]() { return askFor(replyLevels, [&]() { return sync_levels(); }); };
      function<bool()> trades = [&]() { return askFor(replyTrades, [&]() { return sync_trades(); }); };
      function<bool()> orders = [&]() { return askFor(replyOrders, [&]() { return sync_orders(); }); };
      function<bool()> cancelAll = [&]() { return askFor(replyCancelAll, [&]() { return sync_cancelAll(); }); };
      virtual vector<mOrder> sync_cancelAll() = 0;
    protected:
      virtual bool async_wallet() { return false; };
      virtual vector<mWallets> sync_wallet() { return vector<mWallets>(); };
      virtual vector<mLevels> sync_levels() { return vector<mLevels>(); };
      virtual vector<mTrade> sync_trades() { return vector<mTrade>(); };
      virtual vector<mOrder> sync_orders() { return vector<mOrder>(); };
      future<vector<mWallets>> replyWallets;
      future<vector<mLevels>> replyLevels;
      future<vector<mTrade>> replyTrades;
      future<vector<mOrder>> replyOrders;
      future<vector<mOrder>> replyCancelAll;
      template<typename mData, typename syncFn> inline bool askFor(
              future<vector<mData>> &reply,
        const syncFn                &read
      ) {
        bool waiting = reply.valid();
        if (!waiting) {
          reply = ::async(launch::async, read);
          waiting = true;
        }
        return waiting;
      };
      template<typename mData> inline unsigned int waitFor(
              future<vector<mData>> &reply,
        const function<void(mData)> &write
      ) {
        bool waiting = reply.valid();
        if (waiting and reply.wait_for(chrono::nanoseconds(0))==future_status::ready) {
          for (mData &it : reply.get()) write(it);
          waiting = false;
        }
        return waiting;
      };
  } *gw;
  class Klass {
    protected:
      Klass *events = nullptr,
            *broker = nullptr,
            *market = nullptr,
            *wallet = nullptr;
      virtual void load() {};
      virtual void waitData() {};
      virtual void waitTime() {};
      virtual void waitUser() {};
      virtual void run() {};
    public:
      inline void wait() {
        load();
        waitData();
        waitTime();
        waitUser();
        run();
      };
      inline void evLink(Klass &k) { events = &k; };
      inline void ogLink(Klass &k) { broker = &k; };
      inline void mgLink(Klass &k) { market = &k; };
      inline void pgLink(Klass &k) { wallet = &k; };
  };
  class kLass {
    public:
      inline void link(
        Klass &EV,     Klass &DB,     Klass &UI,     Klass &QP,     Klass &OG,     Klass &MG,     Klass &PG,     Klass &QE,     Klass &GW
      ) {
                       DB.evLink(EV); UI.evLink(EV);                OG.evLink(EV); MG.evLink(EV); PG.evLink(EV); QE.evLink(EV); GW.evLink(EV);
                                                                                   MG.ogLink(OG); PG.ogLink(OG); QE.ogLink(OG);
                                                                                                  PG.mgLink(MG); QE.mgLink(MG);
                                                                                                                 QE.pgLink(PG);
      };
  };
}

#endif
