#ifndef K_KM_H_
#define K_KM_H_

namespace K {
  enum class mExchange: unsigned int { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Kraken, Korbit, Poloniex };
  enum class mGatewayType: unsigned int { MarketData, OrderEntry };
  enum class mTimeInForce: unsigned int { IOC, FOK, GTC };
  enum class mConnectivity: unsigned int { Disconnected, Connected };
  enum class mOrderType: unsigned int { Limit, Market };
  enum class mSide: unsigned int { Bid, Ask, Unknown };
  enum class mORS: unsigned int { New, Working, Complete, Cancelled };
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
  enum class uiBIT: unsigned char { Hello = '=', Kiss = '-' };
  enum class uiTXT: unsigned char {
    FairValue = 'a', Quote = 'b', ActiveSubscription = 'c', ActiveState = 'd', MarketData = 'e',
    QuotingParameters = 'f', SafetySettings = 'g', Product = 'h', OrderStatusReports = 'i',
    ProductAdvertisement = 'j', ApplicationState = 'k', Notepad = 'l', ToggleSettings = 'm',
    Position = 'n', ExchangeConnectivity = 'o', SubmitNewOrder = 'p', CancelOrder = 'q',
    MarketTrade = 'r', Trades = 's', ExternalValuation = 't', QuoteStatus = 'u',
    TargetBasePosition = 'v', TradeSafetyValue = 'w', CancelAllOrders = 'x',
    CleanAllClosedTrades = 'y', CleanAllTrades = 'z', CleanTrade = 'A', TradesChart = 'B',
    WalletChart = 'C', EWMAChart = 'D'
  };
  struct mQuotingParams {
    double            widthPing                       = 2.0;
    double            widthPingPercentage             = 0.25;
    double            widthPong                       = 2.0;
    double            widthPongPercentage             = 0.25;
    bool              widthPercentage                 = false;
    bool              bestWidth                       = true;
    double            buySize                         = 0.02;
    int               buySizePercentage               = 7;
    bool              buySizeMax                      = false;
    double            sellSize                        = 0.01;
    int               sellSizePercentage              = 7;
    bool              sellSizeMax                     = false;
    mPingAt           pingAt                          = mPingAt::BothSides;
    mPongAt           pongAt                          = mPongAt::ShortPingFair;
    mQuotingMode      mode                            = mQuotingMode::Top;
    mQuotingSafety    safety                          = mQuotingSafety::Boomerang;
    int               bullets                         = 2;
    double            range                           = 0.5;
    double            rangePercentage                 = 1.0;
    mFairValueModel   fvModel                         = mFairValueModel::BBO;
    double            targetBasePosition              = 1.0;
    int               targetBasePositionPercentage    = 50;
    double            positionDivergence              = 0.9;
    double            positionDivergenceMin           = 0.4;
    int               positionDivergencePercentage    = 21;
    int               positionDivergencePercentageMin = 10;
    mPDivMode         positionDivergenceMode          = mPDivMode::Manual;
    bool              percentageValues                = false;
    mAutoPositionMode autoPositionMode                = mAutoPositionMode::EWMA_LS;
    mAPR              aggressivePositionRebalancing   = mAPR::Off;
    mSOP              superTrades                     = mSOP::Off;
    double            tradesPerMinute                 = 0.9;
    int               tradeRateSeconds                = 3;
    bool              quotingEwmaProtection           = true;
    int               quotingEwmaProtectionPeriods    = 200;
    mSTDEV            quotingStdevProtection          = mSTDEV::Off;
    bool              quotingStdevBollingerBands      = false;
    double            quotingStdevProtectionFactor    = 1.0;
    int               quotingStdevProtectionPeriods   = 1200;
    double            ewmaSensiblityPercentage        = 0.5;
    int               veryLongEwmaPeriods             = 400;
    int               longEwmaPeriods                 = 200;
    int               mediumEwmaPeriods               = 100;
    int               shortEwmaPeriods                = 50;
    double            aprMultiplier                   = 2;
    double            sopWidthMultiplier              = 2;
    double            sopSizeMultiplier               = 2;
    double            sopTradesMultiplier             = 2;
    bool              cancelOrdersAuto                = false;
    double            cleanPongsAuto                  = 0.0;
    double            profitHourInterval              = 0.5;
    bool              audio                           = false;
    int               delayUI                         = 7;
    bool              _matchPings                     = true;
  };
  static void to_json(json& j, const mQuotingParams& k) {
    j = {
      {"widthPing", k.widthPing},
      {"widthPingPercentage", k.widthPingPercentage},
      {"widthPong", k.widthPong},
      {"widthPongPercentage", k.widthPongPercentage},
      {"widthPercentage", k.widthPercentage},
      {"bestWidth", k.bestWidth},
      {"buySize", k.buySize},
      {"buySizePercentage", k.buySizePercentage},
      {"buySizeMax", k.buySizeMax},
      {"sellSize", k.sellSize},
      {"sellSizePercentage", k.sellSizePercentage},
      {"sellSizeMax", k.sellSizeMax},
      {"pingAt", (int)k.pingAt},
      {"pongAt", (int)k.pongAt},
      {"mode", (int)k.mode},
      {"safety", (int)k.safety},
      {"bullets", k.bullets},
      {"range", k.range},
      {"rangePercentage", k.rangePercentage},
      {"fvModel", (int)k.fvModel},
      {"targetBasePosition", k.targetBasePosition},
      {"targetBasePositionPercentage", k.targetBasePositionPercentage},
      {"positionDivergence", k.positionDivergence},
      {"positionDivergencePercentage", k.positionDivergencePercentage},
      {"positionDivergenceMin", k.positionDivergenceMin},
      {"positionDivergencePercentageMin", k.positionDivergencePercentageMin},
      {"positionDivergenceMode", (int)k.positionDivergenceMode},
      {"percentageValues", k.percentageValues},
      {"autoPositionMode", (int)k.autoPositionMode},
      {"aggressivePositionRebalancing", (int)k.aggressivePositionRebalancing},
      {"superTrades", (int)k.superTrades},
      {"tradesPerMinute", k.tradesPerMinute},
      {"tradeRateSeconds", k.tradeRateSeconds},
      {"quotingEwmaProtection", k.quotingEwmaProtection},
      {"quotingEwmaProtectionPeriods", k.quotingEwmaProtectionPeriods},
      {"quotingStdevProtection", (int)k.quotingStdevProtection},
      {"quotingStdevBollingerBands", k.quotingStdevBollingerBands},
      {"quotingStdevProtectionFactor", k.quotingStdevProtectionFactor},
      {"quotingStdevProtectionPeriods", k.quotingStdevProtectionPeriods},
      {"ewmaSensiblityPercentage", k.ewmaSensiblityPercentage},
      {"veryLongEwmaPeriods", k.veryLongEwmaPeriods},
      {"longEwmaPeriods", k.longEwmaPeriods},
      {"mediumEwmaPeriods", k.mediumEwmaPeriods},
      {"shortEwmaPeriods", k.shortEwmaPeriods},
      {"aprMultiplier", k.aprMultiplier},
      {"sopWidthMultiplier", k.sopWidthMultiplier},
      {"sopSizeMultiplier", k.sopSizeMultiplier},
      {"sopTradesMultiplier", k.sopTradesMultiplier},
      {"cancelOrdersAuto", k.cancelOrdersAuto},
      {"cleanPongsAuto", k.cleanPongsAuto},
      {"profitHourInterval", k.profitHourInterval},
      {"audio", k.audio},
      {"delayUI", k.delayUI},
      {"_matchPings", k._matchPings}
    };
  };
  static void from_json(const json& j, mQuotingParams& k) {
    if (j.end() != j.find("widthPing")) k.widthPing = j.at("widthPing").get<double>();
    if (j.end() != j.find("widthPingPercentage")) k.widthPingPercentage = j.at("widthPingPercentage").get<double>();
    if (j.end() != j.find("widthPong")) k.widthPong = j.at("widthPong").get<double>();
    if (j.end() != j.find("widthPongPercentage")) k.widthPongPercentage = j.at("widthPongPercentage").get<double>();
    if (j.end() != j.find("widthPercentage")) k.widthPercentage = j.at("widthPercentage").get<bool>();
    if (j.end() != j.find("bestWidth")) k.bestWidth = j.at("bestWidth").get<bool>();
    if (j.end() != j.find("buySize")) k.buySize = j.at("buySize").get<double>();
    if (j.end() != j.find("buySizePercentage")) k.buySizePercentage = j.at("buySizePercentage").get<int>();
    if (j.end() != j.find("buySizeMax")) k.buySizeMax = j.at("buySizeMax").get<bool>();
    if (j.end() != j.find("sellSize")) k.sellSize = j.at("sellSize").get<double>();
    if (j.end() != j.find("sellSizePercentage")) k.sellSizePercentage = j.at("sellSizePercentage").get<int>();
    if (j.end() != j.find("sellSizeMax")) k.sellSizeMax = j.at("sellSizeMax").get<bool>();
    if (j.end() != j.find("pingAt")) k.pingAt = (mPingAt)j.at("pingAt").get<int>();
    if (j.end() != j.find("pongAt")) k.pongAt = (mPongAt)j.at("pongAt").get<int>();
    if (j.end() != j.find("mode")) k.mode = (mQuotingMode)j.at("mode").get<int>();
    if (j.end() != j.find("safety")) k.safety = (mQuotingSafety)j.at("safety").get<int>();
    if (j.end() != j.find("bullets")) k.bullets = j.at("bullets").get<int>();
    if (j.end() != j.find("range")) k.range = j.at("range").get<double>();
    if (j.end() != j.find("rangePercentage")) k.rangePercentage = j.at("rangePercentage").get<double>();
    if (j.end() != j.find("fvModel")) k.fvModel = (mFairValueModel)j.at("fvModel").get<int>();
    if (j.end() != j.find("targetBasePosition")) k.targetBasePosition = j.at("targetBasePosition").get<double>();
    if (j.end() != j.find("targetBasePositionPercentage")) k.targetBasePositionPercentage = j.at("targetBasePositionPercentage").get<int>();
    if (j.end() != j.find("positionDivergenceMin")) k.positionDivergenceMin = j.at("positionDivergenceMin").get<double>();
    if (j.end() != j.find("positionDivergencePercentageMin")) k.positionDivergencePercentageMin = j.at("positionDivergencePercentageMin").get<int>();
    if (j.end() != j.find("positionDivergenceMode")) k.positionDivergenceMode = (mPDivMode)j.at("positionDivergenceMode").get<int>();
    if (j.end() != j.find("positionDivergence")) k.positionDivergence = j.at("positionDivergence").get<double>();
    if (j.end() != j.find("positionDivergencePercentage")) k.positionDivergencePercentage = j.at("positionDivergencePercentage").get<int>();
    if (j.end() != j.find("percentageValues")) k.percentageValues = j.at("percentageValues").get<bool>();
    if (j.end() != j.find("autoPositionMode")) k.autoPositionMode = (mAutoPositionMode)j.at("autoPositionMode").get<int>();
    if (j.end() != j.find("aggressivePositionRebalancing")) k.aggressivePositionRebalancing = (mAPR)j.at("aggressivePositionRebalancing").get<int>();
    if (j.end() != j.find("superTrades")) k.superTrades = (mSOP)j.at("superTrades").get<int>();
    if (j.end() != j.find("tradesPerMinute")) k.tradesPerMinute = j.at("tradesPerMinute").get<double>();
    if (j.end() != j.find("tradeRateSeconds")) k.tradeRateSeconds = j.at("tradeRateSeconds").get<int>();
    if (j.end() != j.find("quotingEwmaProtection")) k.quotingEwmaProtection = j.at("quotingEwmaProtection").get<bool>();
    if (j.end() != j.find("quotingEwmaProtectionPeriods")) k.quotingEwmaProtectionPeriods = j.at("quotingEwmaProtectionPeriods").get<int>();
    if (j.end() != j.find("quotingStdevProtection")) k.quotingStdevProtection = (mSTDEV)j.at("quotingStdevProtection").get<int>();
    if (j.end() != j.find("quotingStdevBollingerBands")) k.quotingStdevBollingerBands = j.at("quotingStdevBollingerBands").get<bool>();
    if (j.end() != j.find("quotingStdevProtectionFactor")) k.quotingStdevProtectionFactor = j.at("quotingStdevProtectionFactor").get<double>();
    if (j.end() != j.find("quotingStdevProtectionPeriods")) k.quotingStdevProtectionPeriods = j.at("quotingStdevProtectionPeriods").get<int>();
    if (j.end() != j.find("ewmaSensiblityPercentage")) k.ewmaSensiblityPercentage = j.at("ewmaSensiblityPercentage").get<double>();
    if (j.end() != j.find("veryLongEwmaPeriods")) k.veryLongEwmaPeriods = j.at("veryLongEwmaPeriods").get<int>();
    if (j.end() != j.find("longEwmaPeriods")) k.longEwmaPeriods = j.at("longEwmaPeriods").get<int>();
    if (j.end() != j.find("mediumEwmaPeriods")) k.mediumEwmaPeriods = j.at("mediumEwmaPeriods").get<int>();
    if (j.end() != j.find("shortEwmaPeriods")) k.shortEwmaPeriods = j.at("shortEwmaPeriods").get<int>();
    if (j.end() != j.find("aprMultiplier")) k.aprMultiplier = j.at("aprMultiplier").get<double>();
    if (j.end() != j.find("sopWidthMultiplier")) k.sopWidthMultiplier = j.at("sopWidthMultiplier").get<double>();
    if (j.end() != j.find("sopSizeMultiplier")) k.sopSizeMultiplier = j.at("sopSizeMultiplier").get<double>();
    if (j.end() != j.find("sopTradesMultiplier")) k.sopTradesMultiplier = j.at("sopTradesMultiplier").get<double>();
    if (j.end() != j.find("cancelOrdersAuto")) k.cancelOrdersAuto = j.at("cancelOrdersAuto").get<bool>();
    if (j.end() != j.find("cleanPongsAuto")) k.cleanPongsAuto = j.at("cleanPongsAuto").get<double>();
    if (j.end() != j.find("profitHourInterval")) k.profitHourInterval = j.at("profitHourInterval").get<double>();
    if (j.end() != j.find("audio")) k.audio = j.at("audio").get<bool>();
    if (j.end() != j.find("delayUI")) k.delayUI = j.at("delayUI").get<int>();
    if (k.mode == mQuotingMode::Depth) k.widthPercentage = false;
    k._matchPings = k.safety == mQuotingSafety::Boomerang or k.safety == mQuotingSafety::AK47;
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
      {"amount", k.amount},
      {"held", k.held},
      {"currency", k.currency}
    };
  };
  struct mProfit {
           double baseValue,
                  quoteValue;
    unsigned long time;
    mProfit():
      baseValue(0), quoteValue(0), time(0)
    {};
    mProfit(double b, double q, unsigned long t):
      baseValue(b), quoteValue(q), time(t)
    {};
  };
  static void to_json(json& j, const mProfit& k) {
    j = {
      {"baseValue", k.baseValue},
      {"quoteValue", k.quoteValue},
      {"time", k.time}
    };
  };
  static void from_json(const json& j, mProfit& k) {
    if (j.end() != j.find("baseValue")) k.baseValue = j.at("baseValue").get<double>();
    if (j.end() != j.find("quoteValue")) k.quoteValue = j.at("quoteValue").get<double>();
    if (j.end() != j.find("time")) k.time = j.at("time").get<unsigned long>();
  };
  struct mSafety {
    double buy,
           sell,
           combined,
           buyPing,
           sellPong;
    mSafety():
      buy(0), sell(0), combined(0), buyPing(-1), sellPong(-1)
    {};
    mSafety(double b, double s, double c, double bP, double sP):
      buy(b), sell(s), combined(c), buyPing(bP), sellPong(sP)
    {};
  };
  static void to_json(json& j, const mSafety& k) {
    j = {
      {"buy", k.buy},
      {"sell", k.sell},
      {"combined", k.combined},
      {"buyPing", k.buyPing},
      {"sellPong", k.sellPong}
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
    mExchange exchange;
    mPosition():
      baseAmount(0), quoteAmount(0), baseHeldAmount(0), quoteHeldAmount(0), baseValue(0), quoteValue(0), profitBase(0), profitQuote(0), pair(mPair()), exchange((mExchange)0)
    {};
    mPosition(double bA, double qA, double bH, double qH, double bV, double qV, double bP, double qP, mPair p, mExchange e):
      baseAmount(bA), quoteAmount(qA), baseHeldAmount(bH), quoteHeldAmount(qH), baseValue(bV), quoteValue(qV), profitBase(bP), profitQuote(qP), pair(p), exchange(e)
    {};
  };
  static void to_json(json& j, const mPosition& k) {
    j = {
      {"baseAmount", k.baseAmount},
      {"quoteAmount", k.quoteAmount},
      {"baseHeldAmount", k.baseHeldAmount},
      {"quoteHeldAmount", k.quoteHeldAmount},
      {"baseValue", k.baseValue},
      {"quoteValue", k.quoteValue},
      {"profitBase", k.profitBase},
      {"profitQuote", k.profitQuote},
      {"pair", {{"base", k.pair.base}, {"quote", k.pair.quote}}},
      {"exchange", (int)k.exchange}
    };
  };
  struct mTrade {
           string tradeId;
        mExchange exchange;
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
    unsigned long time,
                  Ktime;
             bool loadedFromDB;

    mTrade():
      tradeId(""), exchange((mExchange)0), pair(mPair()), price(0), quantity(0), side((mSide)0), time(0), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(double p, double q, unsigned long t):
      tradeId(""), exchange((mExchange)0), pair(mPair()), price(p), quantity(q), side((mSide)0), time(t), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(double p, double q, mSide s):
      tradeId(""), exchange((mExchange)0), pair(mPair()), price(p), quantity(q), side(s), time(0), value(0), Ktime(0), Kqty(0), Kprice(0), Kvalue(0), Kdiff(0), feeCharged(0), loadedFromDB(false)
    {};
    mTrade(string i, mExchange e, mPair P, double p, double q, mSide S, unsigned long t, double v, unsigned long Kt, double Kq, double Kp, double Kv, double Kd, double f, bool l):
      tradeId(i), exchange(e), pair(P), price(p), quantity(q), side(S), time(t), value(v), Ktime(Kt), Kqty(Kq), Kprice(Kp), Kvalue(Kv), Kdiff(Kd), feeCharged(f), loadedFromDB(l)
    {};
  };
  static void to_json(json& j, const mTrade& k) {
    j = {
      {"tradeId", k.tradeId},
      {"time", k.time},
      {"exchange", (int)k.exchange},
      {"pair", {{"base", k.pair.base}, {"quote", k.pair.quote}}},
      {"price", k.price},
      {"quantity", k.quantity},
      {"side", (int)k.side},
      {"value", k.value},
      {"Ktime", k.Ktime},
      {"Kqty", k.Kqty},
      {"Kprice", k.Kprice},
      {"Kvalue", k.Kvalue},
      {"Kdiff", k.Kdiff},
      {"feeCharged", k.feeCharged},
      {"loadedFromDB", k.loadedFromDB},
    };
  };
  static void from_json(const json& j, mTrade& k) {
    if (j.end() != j.find("tradeId")) k.tradeId = j.at("tradeId").get<string>();
    if (j.end() != j.find("exchange")) k.exchange = (mExchange)j.at("exchange").get<int>();
    if (j.end() != j.find("pair")) k.pair = mPair(j["/pair/base"_json_pointer].get<string>(), j["/pair/quote"_json_pointer].get<string>());
    if (j.end() != j.find("price")) k.price = j.at("price").get<double>();
    if (j.end() != j.find("quantity")) k.quantity = j.at("quantity").get<double>();
    if (j.end() != j.find("side")) k.side = (mSide)j.at("side").get<int>();
    if (j.end() != j.find("time")) k.time = j.at("time").get<unsigned long>();
    if (j.end() != j.find("value")) k.value = j.at("value").get<double>();
    if (j.end() != j.find("Ktime")) k.Ktime = j.at("Ktime").get<unsigned long>();
    if (j.end() != j.find("Kqty")) k.Kqty = j.at("Kqty").get<double>();
    if (j.end() != j.find("Kprice")) k.Kprice = j.at("Kprice").get<double>();
    if (j.end() != j.find("Kvalue")) k.Kvalue = j.at("Kvalue").get<double>();
    if (j.end() != j.find("Kdiff")) k.Kdiff = j.at("Kdiff").get<double>();
    if (j.end() != j.find("feeCharged")) k.feeCharged = j.at("feeCharged").get<double>();
    if (j.end() != j.find("loadedFromDB")) k.loadedFromDB = j.at("loadedFromDB").get<bool>();
  };
  struct mOrder {
           string orderId,
                  exchangeId;
        mExchange exchange;
            mPair pair;
            mSide side;
           double price,
                  quantity,
                  lastQuantity;
       mOrderType type;
     mTimeInForce timeInForce;
             mORS orderStatus;
             bool isPong,
                  preferPostOnly;
    unsigned long time,
                  computationalLatency;
    mOrder():
      orderId(""), exchangeId(""), exchange((mExchange)0), pair(mPair()), side((mSide)0), quantity(0), type((mOrderType)0), isPong(false), price(0), timeInForce((mTimeInForce)0), orderStatus((mORS)0), preferPostOnly(false), lastQuantity(0), time(0), computationalLatency(0)
    {};
    mOrder(string o, mORS s):
      orderId(o), exchangeId(""), exchange((mExchange)0), pair(mPair()), side((mSide)0), quantity(0), type((mOrderType)0), isPong(false), price(0), timeInForce((mTimeInForce)0), orderStatus(s), preferPostOnly(false), lastQuantity(0), time(0), computationalLatency(0)
    {};
    mOrder(string o, string e, mORS s, double p, double q, double Q):
      orderId(o), exchangeId(e), exchange((mExchange)0), pair(mPair()), side((mSide)0), quantity(q), type((mOrderType)0), isPong(false), price(p), timeInForce((mTimeInForce)0), orderStatus(s), preferPostOnly(false), lastQuantity(Q), time(0), computationalLatency(0)
    {};
    mOrder(string o, mExchange e, mPair P, mSide S, double q, mOrderType t, bool i, double p, mTimeInForce F, mORS s, bool O):
      orderId(o), exchangeId(""), exchange(e), pair(P), side(S), quantity(q), type(t), isPong(i), price(p), timeInForce(F), orderStatus(s), preferPostOnly(O), lastQuantity(0), time(0), computationalLatency(0)
    {};
    string quantity2str() {
      stringstream ss;
      ss << setprecision(8) << fixed << quantity;
      return ss.str();
    };
    string lastQuantity2str() {
      stringstream ss;
      ss << setprecision(8) << fixed << lastQuantity;
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
      {"orderId", k.orderId},
      {"exchangeId", k.exchangeId},
      {"exchange", (int)k.exchange},
      {"pair", {{"base", k.pair.base}, {"quote", k.pair.quote}}},
      {"side", (int)k.side},
      {"quantity", k.quantity},
      {"type", (int)k.type},
      {"isPong", k.isPong},
      {"price", k.price},
      {"timeInForce", (int)k.timeInForce},
      {"orderStatus", (int)k.orderStatus},
      {"preferPostOnly", k.preferPostOnly},
      {"lastQuantity", k.lastQuantity},
      {"time", k.time},
      {"computationalLatency", k.computationalLatency}
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
  struct mLevels {
    vector<mLevel> bids,
                   asks;
    mLevels():
      bids({}), asks({})
    {};
    mLevels(vector<mLevel> b, vector<mLevel> a):
      bids(b), asks(a)
    {};
  };
  static void to_json(json& j, const mLevels& k) {
    json b, a;
    for (vector<mLevel>::const_iterator it = k.bids.begin(); it != k.bids.end(); ++it)
      b.push_back({{"price", it->price}, {"size", it->size}});
    for (vector<mLevel>::const_iterator it = k.asks.begin(); it != k.asks.end(); ++it)
      a.push_back({{"price", it->price}, {"size", it->size}});
    j = {{"bids", b}, {"asks", a}};
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
      {"bid", {
        {"price", k.bid.price},
        {"size", k.bid.size}
      }},
      {"ask", {
        {"price", k.ask.price},
        {"size", k.ask.size}
      }}
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
      {"bidStatus", (int)k.bidStatus},
      {"askStatus", (int)k.askStatus},
      {"quotesInMemoryNew", k.quotesInMemoryNew},
      {"quotesInMemoryWorking", k.quotesInMemoryWorking},
      {"quotesInMemoryDone", k.quotesInMemoryDone}
    };
  };
  static const char kB64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "abcdefghijklmnopqrstuvwxyz"
                                     "0123456789+/";
  static char RBLACK[] = "\033[0;30m", RRED[]    = "\033[0;31m", RGREEN[] = "\033[0;32m", RYELLOW[] = "\033[0;33m",
              RBLUE[]  = "\033[0;34m", RPURPLE[] = "\033[0;35m", RCYAN[]  = "\033[0;36m", RWHITE[]  = "\033[0;37m",
              BBLACK[] = "\033[1;30m", BRED[]    = "\033[1;31m", BGREEN[] = "\033[1;32m", BYELLOW[] = "\033[1;33m",
              BBLUE[]  = "\033[1;34m", BPURPLE[] = "\033[1;35m", BCYAN[]  = "\033[1;36m", BWHITE[]  = "\033[1;37m";
  static WINDOW *wBorder = nullptr,
                *wLog = nullptr;
  static mutex wMutex;
  static vector<function<void()>*> gwEndings;
  class Gw {
    public:
      static Gw *E(mExchange e);
      string (*randId)() = 0;
      function<void(mOrder)>        evDataOrder;
      function<void(mTrade)>        evDataTrade;
      function<void(mWallet)>       evDataWallet;
      function<void(mLevels)>       evDataLevels;
      function<void(mConnectivity)> evConnectOrder,
                                    evConnectMarket;
      uWS::Hub                *hub     = nullptr;
      uWS::Group<uWS::CLIENT> *gwGroup = nullptr;
      mExchange exchange = mExchange::Null;
          int version = 0;
       double makeFee = 0,  minTick = 0,
              takeFee = 0,  minSize = 0;
       string base    = "", quote   = "",
              name    = "", symbol  = "",
              apikey  = "", secret  = "",
              user    = "", pass    = "",
              ws      = "", http    = "";
      virtual string A() = 0;
      virtual   void wallet() = 0,
                     levels() = 0,
                     send(string oI, mSide oS, string oP, string oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) = 0,
                     cancel(string oI, string oE, mSide oS, unsigned long oT) = 0,
                     cancelAll() = 0,
                     close() = 0;
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
      void cfLink(Klass *k) { config = k; };
      void evLink(Klass *k) { events = k; };
      void dbLink(Klass *k) { memory = k; };
      void uiLink(Klass *k) { client = k; };
      void ogLink(Klass *k) { broker = k; };
      void mgLink(Klass *k) { market = k; };
      void pgLink(Klass *k) { wallet = k; };
      void qeLink(Klass *k) { engine = k; };
  };
  class kLass: public Klass {
    private:
      mQuotingParams p;
    public:
      void link(Klass *EV, Klass *DB, Klass *UI, Klass *QP, Klass *OG, Klass *MG, Klass *PG, Klass *QE, Klass *GW) {
        Klass *CF = (Klass*)this;
        EV->gwLink(gw);                 UI->gwLink(gw);                 OG->gwLink(gw); MG->gwLink(gw); PG->gwLink(gw); QE->gwLink(gw); GW->gwLink(gw);
        EV->cfLink(CF); DB->cfLink(CF); UI->cfLink(CF);                 OG->cfLink(CF); MG->cfLink(CF); PG->cfLink(CF); QE->cfLink(CF); GW->cfLink(CF);
                                        UI->evLink(EV); QP->evLink(EV); OG->evLink(EV); MG->evLink(EV); PG->evLink(EV); QE->evLink(EV); GW->evLink(EV);
                                        UI->dbLink(DB); QP->dbLink(DB); OG->dbLink(DB); MG->dbLink(DB); PG->dbLink(DB);
                                                        QP->uiLink(UI); OG->uiLink(UI); MG->uiLink(UI); PG->uiLink(UI); QE->uiLink(UI); GW->uiLink(UI);
                                                        QP->qpLink(&p); OG->qpLink(&p); MG->qpLink(&p); PG->qpLink(&p); QE->qpLink(&p); GW->qpLink(&p);
                                                                                        MG->ogLink(OG); PG->ogLink(OG); QE->ogLink(OG);
                                                                                                        PG->mgLink(MG); QE->mgLink(MG);
                                                                                                                        QE->pgLink(PG);
                                                                                                                                        GW->qeLink(QE);
      };
  };
}

#endif
