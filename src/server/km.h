#ifndef K_KM_H_
#define K_KM_H_

namespace K {
  enum class mExchange: unsigned int { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Kraken, OkEx, BitfinexMargin, Korbit, Poloniex };
  enum class mConnectivity: unsigned int { Disconnected, Connected };
  enum class mStatus: unsigned int { New, Working, Complete, Cancelled };
  enum class mSide: unsigned int { Bid, Ask, Both };
  enum class mTimeInForce: unsigned int { IOC, FOK, GTC };
  enum class mOrderType: unsigned int { Limit, Market };
  enum class mPingAt: unsigned int { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings };
  enum class mPongAt: unsigned int { ShortPingFair, LongPingFair, ShortPingAggressive, LongPingAggressive };
  enum class mQuotingMode: unsigned int { Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth };
  enum class mQuotingSafety: unsigned int { Off, PingPong, Boomerang, AK47 };
  enum class mQuoteState: unsigned int { Live, Disconnected, DisabledQuotes, MissingData, UnknownHeld, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed };
  enum class mFairValueModel: unsigned int { BBO, wBBO };
  enum class mAutoPositionMode: unsigned int { Manual, EWMA_LS, EWMA_LMS, EWMA_4 };
  enum class mPDivMode: unsigned int { Manual, Linear, Sine, SQRT, Switch};
  enum class mAPR: unsigned int { Off, Size, SizeWidth };
  enum class mSOP: unsigned int { Off, Trades, Size, TradesSize };
  enum class mSTDEV: unsigned int { Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff };
  enum class mPortal: unsigned char { Hello = '=', Kiss = '-' };
  enum class mMatter: unsigned char {
    FairValue            = 'a', Quote                = 'b', ActiveSubscription = 'c', Connectivity       = 'd', MarketData       = 'e',
    QuotingParameters    = 'f', SafetySettings       = 'g', Product            = 'h', OrderStatusReports = 'i',
    ProductAdvertisement = 'j', ApplicationState     = 'k', Notepad            = 'l', ToggleSettings     = 'm',
    Position             = 'n',                             SubmitNewOrder     = 'p', CancelOrder        = 'q', MarketTrade      = 'r',
    Trades               = 's',ExternalValuation     = 't', QuoteStatus        = 'u', TargetBasePosition = 'v', TradeSafetyValue = 'w',
    CancelAllOrders      = 'x', CleanAllClosedTrades = 'y', CleanAllTrades     = 'z', CleanTrade         = 'A',
    TradesChart          = 'B', WalletChart          = 'C', EWMAChart          = 'D', MarketDataLongTerm = 'G'
  };
  static          bool operator! (mConnectivity k_)                   { return !(unsigned int)k_; };
  static mConnectivity operator* (mConnectivity _k, mConnectivity k_) { return (mConnectivity)((unsigned int)_k * (unsigned int)k_); };
  struct mQuotingParams {
    double            widthPing                       = 2.0;
    double            widthPingPercentage             = 0.25;
    double            widthPong                       = 2.0;
    double            widthPongPercentage             = 0.25;
    bool              widthPercentage                 = false;
    bool              bestWidth                       = true;
    double            buySize                         = 0.02;
    unsigned int      buySizePercentage               = 7;
    bool              buySizeMax                      = false;
    double            sellSize                        = 0.01;
    unsigned int      sellSizePercentage              = 7;
    bool              sellSizeMax                     = false;
    mPingAt           pingAt                          = mPingAt::BothSides;
    mPongAt           pongAt                          = mPongAt::ShortPingFair;
    mQuotingMode      mode                            = mQuotingMode::Top;
    mQuotingSafety    safety                          = mQuotingSafety::Boomerang;
    unsigned int      bullets                         = 2;
    double            range                           = 0.5;
    double            rangePercentage                 = 5.0;
    mFairValueModel   fvModel                         = mFairValueModel::BBO;
    double            targetBasePosition              = 1.0;
    unsigned int      targetBasePositionPercentage    = 50;
    double            positionDivergence              = 0.9;
    double            positionDivergenceMin           = 0.4;
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
    unsigned int      veryLongEwmaPeriods             = 400;
    unsigned int      longEwmaPeriods                 = 200;
    unsigned int      mediumEwmaPeriods               = 100;
    unsigned int      shortEwmaPeriods                = 50;
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
    void tidy() {
      if (mode == mQuotingMode::Depth) widthPercentage = false;
    };
    void flag() {
      _matchPings = safety == mQuotingSafety::Boomerang or safety == mQuotingSafety::AK47;
    };
    void diff(mQuotingParams prev) {
      _diffVLEP = prev.veryLongEwmaPeriods != veryLongEwmaPeriods;
      _diffLEP = prev.longEwmaPeriods != longEwmaPeriods;
      _diffMEP = prev.mediumEwmaPeriods != mediumEwmaPeriods;
      _diffSEP = prev.shortEwmaPeriods != shortEwmaPeriods;
    };
    bool diffOnce(bool *k) {
      bool ret = *k;
      if (*k) *k = false;
      return ret;
    };
  };
  static void to_json(json& j, const mQuotingParams& k) {
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
      {            "veryLongEwmaPeriods", k.veryLongEwmaPeriods            },
      {                "longEwmaPeriods", k.longEwmaPeriods                },
      {              "mediumEwmaPeriods", k.mediumEwmaPeriods              },
      {               "shortEwmaPeriods", k.shortEwmaPeriods               },
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
  static void from_json(const json& j, mQuotingParams& k) {
    k.widthPing                       = fmax(1e-8,            j.value("widthPing", k.widthPing));
    k.widthPingPercentage             = fmin(1e+2, fmax(1e-1, j.value("widthPingPercentage", k.widthPingPercentage)));
    k.widthPong                       = fmax(1e-8,            j.value("widthPong", k.widthPong));
    k.widthPongPercentage             = fmin(1e+2, fmax(1e-1, j.value("widthPongPercentage", k.widthPongPercentage)));
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
    k.rangePercentage                 = fmin(1e+2, fmax(1e-1, j.value("rangePercentage", k.rangePercentage)));
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
    k.veryLongEwmaPeriods             = fmax(1,               j.value("veryLongEwmaPeriods", k.veryLongEwmaPeriods));
    k.longEwmaPeriods                 = fmax(1,               j.value("longEwmaPeriods", k.longEwmaPeriods));
    k.mediumEwmaPeriods               = fmax(1,               j.value("mediumEwmaPeriods", k.mediumEwmaPeriods));
    k.shortEwmaPeriods                = fmax(1,               j.value("shortEwmaPeriods", k.shortEwmaPeriods));
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
    string base,
           quote;
    mPair():
      base(""), quote("")
    {};
    mPair(string b, string q):
      base(b), quote(q)
    {};
  };
  static void to_json(json& j, const mPair& k) {
    j = {
      { "base", k.base },
      {"quote", k.quote}
    };
  };
  static void from_json(const json& j, mPair& k) {
    k.base  = j.value("base", "");
    k.quote = j.value("quote", "");
  };
  struct mWallet {
    double amount,
           held;
    string currency;
    mWallet():
      amount(0), held(0), currency("")
    {};
    mWallet(double a, double h, string c):
      amount(a), held(h), currency(c)
    {};
  };
  static void to_json(json& j, const mWallet& k) {
    j = {
      {  "amount", k.amount  },
      {    "held", k.held    },
      {"currency", k.currency}
    };
  };
  struct mProfit {
                double baseValue,
                       quoteValue;
    unsigned long long time;
    mProfit():
      baseValue(0), quoteValue(0), time(0)
    {};
    mProfit(double b, double q, unsigned long long t):
      baseValue(b), quoteValue(q), time(t)
    {};
  };
  static void to_json(json& j, const mProfit& k) {
    j = {
      { "baseValue", k.baseValue },
      {"quoteValue", k.quoteValue},
      {      "time", k.time      }
    };
  };
  static void from_json(const json& j, mProfit& k) {
    k.baseValue  = j.value("baseValue", 0.0);
    k.quoteValue = j.value("quoteValue", 0.0);
    k.time       = j.value("time", (unsigned long long)0);
  };
  struct mSafety {
    double buy,
           sell,
           combined,
           buyPing,
           sellPing;
    mSafety():
      buy(0), sell(0), combined(0), buyPing(-1), sellPing(-1)
    {};
    mSafety(double b, double s, double c, double bP, double sP):
      buy(b), sell(s), combined(c), buyPing(bP), sellPing(sP)
    {};
  };
  static void to_json(json& j, const mSafety& k) {
    j = {
      {     "buy", k.buy              },
      {    "sell", k.sell             },
      {"combined", k.combined         },
      { "buyPing", fmax(0, k.buyPing) },
      {"sellPing", fmax(0, k.sellPing)}
    };
  };
  struct mPosition {
       double baseAmount,
              quoteAmount,
              baseHeldAmount,
              quoteHeldAmount,
              baseValue,
              quoteValue,
              profitBase,
              profitQuote;
        mPair pair;
    mPosition():
      baseAmount(0), quoteAmount(0), baseHeldAmount(0), quoteHeldAmount(0), baseValue(0), quoteValue(0), profitBase(0), profitQuote(0), pair(mPair())
    {};
    mPosition(double bA, double qA, double bH, double qH, double bV, double qV, double bP, double qP, mPair p):
      baseAmount(bA), quoteAmount(qA), baseHeldAmount(bH), quoteHeldAmount(qH), baseValue(bV), quoteValue(qV), profitBase(bP), profitQuote(qP), pair(p)
    {};
    bool empty() {
      return !baseValue;
    };
  };
  static void to_json(json& j, const mPosition& k) {
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
                double price,
                       quantity,
                       value,
                       Kqty,
                       Kvalue,
                       Kprice,
                       Kdiff,
                       feeCharged;
    unsigned long long time,
                       Ktime;
                  bool loadedFromDB;

    mTrade():
      tradeId(""), pair(mPair()), price(0), quantity(0), side((mSide)0), time(0), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(double p, double q, unsigned long long t):
      tradeId(""), pair(mPair()), price(p), quantity(q), side((mSide)0), time(t), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(double p, double q, mSide s):
      tradeId(""), pair(mPair()), price(p), quantity(q), side(s), time(0), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(string i, mPair P, double p, double q, mSide S, unsigned long long t, double v, unsigned long long Kt, double Kq, double Kp, double Kv, double Kd, double f, bool l):
      tradeId(i), pair(P), price(p), quantity(q), side(S), time(t), value(v), Ktime(Kt), Kqty(Kq), Kprice(Kp), Kvalue(Kv), Kdiff(Kd), feeCharged(f), loadedFromDB(l)
    {};
  };
  static void to_json(json& j, const mTrade& k) {
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
  static void from_json(const json& j, mTrade& k) {
    k.tradeId      = j.value("tradeId", "");
    k.pair         = j.value("pair", json::object());
    k.price        = j.value("price", 0.0);
    k.quantity     = j.value("quantity", 0.0);
    k.side         = j.value("side", (mSide)0);
    k.time         = j.value("time", (unsigned long long)0);
    k.value        = j.value("value", 0.0);
    k.Ktime        = j.value("Ktime", (unsigned long long)0);
    k.Kqty         = j.value("Kqty", 0.0);
    k.Kprice       = j.value("Kprice", 0.0);
    k.Kvalue       = j.value("Kvalue", 0.0);
    k.Kdiff        = j.value("Kdiff", 0.0);
    k.feeCharged   = j.value("feeCharged", 0.0);
    k.loadedFromDB = j.value("loadedFromDB", false);
  };
  struct mOrder {
                string orderId,
                       exchangeId;
                 mPair pair;
                mSide side;
                double price,
                       quantity,
                       tradeQuantity;
            mOrderType type;
          mTimeInForce timeInForce;
               mStatus orderStatus;
                  bool isPong,
                       preferPostOnly;
    unsigned long long time,
                       waitingCancel,
                       latency;
    mOrder():
      orderId(""), exchangeId(""), pair(mPair()), side((mSide)0), quantity(0), type((mOrderType)0), isPong(false), price(0), timeInForce((mTimeInForce)0), orderStatus((mStatus)0), preferPostOnly(false), tradeQuantity(0), time(0), waitingCancel(0), latency(0)
    {};
    mOrder(string o, mStatus s):
      orderId(o), exchangeId(""), pair(mPair()), side((mSide)0), quantity(0), type((mOrderType)0), isPong(false), price(0), timeInForce((mTimeInForce)0), orderStatus(s), preferPostOnly(false), tradeQuantity(0), time(0), waitingCancel(0), latency(0)
    {};
    mOrder(string o, string e, mStatus s, double p, double q, double Q):
      orderId(o), exchangeId(e), pair(mPair()), side((mSide)0), quantity(q), type((mOrderType)0), isPong(false), price(p), timeInForce((mTimeInForce)0), orderStatus(s), preferPostOnly(false), tradeQuantity(Q), time(0), waitingCancel(0), latency(0)
    {};
    mOrder(string o, mPair P, mSide S, double q, mOrderType t, bool i, double p, mTimeInForce F, mStatus s, bool O):
      orderId(o), exchangeId(""), pair(P), side(S), quantity(q), type(t), isPong(i), price(p), timeInForce(F), orderStatus(s), preferPostOnly(O), tradeQuantity(0), time(0), waitingCancel(0), latency(0)
    {};
    string quantity2str() {
      stringstream ss;
      ss << setprecision(8) << fixed << quantity;
      return ss.str();
    };
    string tradeQuantity2str() {
      stringstream ss;
      ss << setprecision(8) << fixed << tradeQuantity;
      return ss.str();
    };
    string price2str() {
      stringstream ss;
      ss << setprecision(8) << fixed << price;
      return ss.str();
    };
  };
  static void to_json(json& j, const mOrder& k) {
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
      { "tradeQuantity", k.tradeQuantity },
      {          "time", k.time          },
      {       "latency", k.latency       }
    };
  };
  struct mLevel {
    double price,
           size;
    mLevel():
      price(0), size(0)
    {};
    mLevel(double p, double s):
      price(p), size(s)
    {};
  };
  static void to_json(json& j, const mLevel& k) {
    j = {
      {"price", k.price},
      { "size", k.size }
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
    bool empty() {
      return bids.empty() or asks.empty();
    };
  };
  static void to_json(json& j, const mLevels& k) {
    vector<mLevel> bids,
                   asks;
    for (const mLevel &it : k.bids)
      if (bids.size() < 15) bids.push_back(it); else break;
    for (const mLevel &it : k.asks)
      if (asks.size() < 15) asks.push_back(it); else break;
    j = {
      {"bids", bids},
      {"asks", asks}
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
  static void to_json(json& j, const mQuote& k) {
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
  static void to_json(json& j, const mQuoteStatus& k) {
    j = {
      {            "bidStatus", k.bidStatus            },
      {            "askStatus", k.askStatus            },
      {    "quotesInMemoryNew", k.quotesInMemoryNew    },
      {"quotesInMemoryWorking", k.quotesInMemoryWorking},
      {   "quotesInMemoryDone", k.quotesInMemoryDone   }
    };
  };
  static char RBLACK[] = "\033[0;30m", RRED[]    = "\033[0;31m", RGREEN[] = "\033[0;32m", RYELLOW[] = "\033[0;33m",
              RBLUE[]  = "\033[0;34m", RPURPLE[] = "\033[0;35m", RCYAN[]  = "\033[0;36m", RWHITE[]  = "\033[0;37m",
              BBLACK[] = "\033[1;30m", BRED[]    = "\033[1;31m", BGREEN[] = "\033[1;32m", BYELLOW[] = "\033[1;33m",
              BBLUE[]  = "\033[1;34m", BPURPLE[] = "\033[1;35m", BCYAN[]  = "\033[1;36m", BWHITE[]  = "\033[1;37m";
  static WINDOW *wBorder = nullptr,
                *wLog    = nullptr;
  static ostringstream             THIS_WAS_A_TRIUMPH;
  static vector<function<void()>*> gwEndings;
  class Gw {
    public:
      static Gw *config(string, string, string, int, string, string, string, string, string, string, int, int);
      string (*randId)() = 0;
      function<void(mOrder)>        evDataOrder;
      function<void(mTrade)>        evDataTrade;
      function<void(mWallet)>       evDataWallet;
      function<void(mLevels)>       evDataLevels;
      function<void(mConnectivity)> evConnectOrder,
                                    evConnectMarket;
      uWS::Hub                *hub     = nullptr;
      uWS::Group<uWS::CLIENT> *gwGroup = nullptr;
      mExchange exchange = (mExchange)0;
         int version = 0, maxLevel = 0,
             debug   = 0;
      double makeFee = 0,  minTick = 0,
             takeFee = 0,  minSize = 0;
      string base    = "", quote   = "",
             name    = "", symbol  = "",
             apikey  = "", secret  = "",
             user    = "", pass    = "",
             ws      = "", http    = "";
      virtual   void wallet() = 0,
                     levels() = 0,
                     send(string, string, string, mSide, string, string, mOrderType, mTimeInForce, bool, unsigned long long) = 0,
                     cancel(string, string, mSide, unsigned long long) = 0,
                     cancelAll() = 0,
                     close() = 0;
      virtual string A() = 0;
  };
  class Klass {
    protected:
      Gw             *gw = nullptr;
      mQuotingParams *qp = nullptr;
      Klass          *config = nullptr,
                     *events = nullptr,
                     *memory = nullptr,
                     *client = nullptr,
                     *broker = nullptr,
                     *market = nullptr,
                     *wallet = nullptr,
                     *engine = nullptr;
      virtual void load(int argc, char** argv) {};
      virtual void load() {};
      virtual void waitTime() {};
      virtual void waitData() {};
      virtual void waitUser() {};
      virtual void run() {};
    public:
      void main(int argc, char** argv) {
        load(argc, argv);
        run();
      };
      void wait() {
        load();
        waitTime();
        waitData();
        waitUser();
        run();
      };
      void gwLink(Gw *k) { gw = k; };
      void qpLink(mQuotingParams *k) { qp = k; };
      void cfLink(Klass &k) { config = &k; };
      void evLink(Klass &k) { events = &k; };
      void dbLink(Klass &k) { memory = &k; };
      void uiLink(Klass &k) { client = &k; };
      void ogLink(Klass &k) { broker = &k; };
      void mgLink(Klass &k) { market = &k; };
      void pgLink(Klass &k) { wallet = &k; };
      void qeLink(Klass &k) { engine = &k; };
  };
  class kLass: public Klass {
    private:
      mQuotingParams p;
    public:
      void link(Klass &EV, Klass &DB, Klass &UI, Klass &QP, Klass &OG, Klass &MG, Klass &PG, Klass &QE, Klass &GW) {
        Klass &CF = *this;
        EV.gwLink(gw);                UI.gwLink(gw);                OG.gwLink(gw); MG.gwLink(gw); PG.gwLink(gw); QE.gwLink(gw); GW.gwLink(gw);
        EV.cfLink(CF); DB.cfLink(CF); UI.cfLink(CF);                OG.cfLink(CF); MG.cfLink(CF); PG.cfLink(CF); QE.cfLink(CF); GW.cfLink(CF);
                       DB.evLink(EV); UI.evLink(EV); QP.evLink(EV); OG.evLink(EV); MG.evLink(EV); PG.evLink(EV); QE.evLink(EV); GW.evLink(EV);
                                      UI.dbLink(DB); QP.dbLink(DB); OG.dbLink(DB); MG.dbLink(DB); PG.dbLink(DB);
                                                     QP.uiLink(UI); OG.uiLink(UI); MG.uiLink(UI); PG.uiLink(UI); QE.uiLink(UI); GW.uiLink(UI);
                                                     QP.qpLink(&p); OG.qpLink(&p); MG.qpLink(&p); PG.qpLink(&p); QE.qpLink(&p); GW.qpLink(&p);
                                                                                   MG.ogLink(OG); PG.ogLink(OG); QE.ogLink(OG);
                                                                                                  PG.mgLink(MG); QE.mgLink(MG);
                                                                                                                 QE.pgLink(PG);
                                                                                                                                GW.qeLink(QE);
      };
  };
}

#endif
