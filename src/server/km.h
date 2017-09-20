#ifndef K_KM_H_
#define K_KM_H_

namespace K {
  static const char alphanum[] = "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  static const vector<string> mCurrency = {
    "BTC", "LTC", "EUR", "GBP", "CNY", "CAD", "ETH", "ETC", "BFX", "RRT", "ZEC", "BCN", "DASH", "DOGE",
    "DSH", "EMC", "FCN", "LSK", "NXT", "QCN", "SDB", "SCB", "STEEM", "XDN", "XEM", "XMR", "ARDR", "WAVES",
    "BTU", "MAID", "AMP", "XRP", "KRW", "IOT", "BCY", "BELA", "BLK", "BTCD", "BTM", "BTS", "BURST", "CLAM",
    "DCR", "DGB", "EMC2", "EXP", "FCT", "FLDC", "FLO", "GAME", "GNO", "GNT", "GRC", "HUC", "LBC", "NAUT",
    "NAV", "NEOS", "NMC", "NOTE", "NXC", "OMNI", "PASC", "PINK", "POT", "PPC", "RADS", "REP", "RIC", "SBD",
    "SC", "SJCX", "STR", "STRAT", "SYS", "VIA", "VRC", "VTC", "XBC", "XCP", "XPM", "XVC", "USD", "USDT",
    "EOS", "SAN", "OMG", "PAY", "BCC", "BCH" };
  enum class mExchange: unsigned int { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Korbit, Poloniex };
  enum class mGatewayType: unsigned int { MarketData, OrderEntry };
  enum class mTimeInForce: unsigned int { IOC, FOK, GTC };
  enum class mConnectivity: unsigned int { Disconnected, Connected };
  enum class mOrderType: unsigned int { Limit, Market };
  enum class mSide: unsigned int { Bid, Ask, Unknown };
  enum class mORS: unsigned int { New, Working, Complete, Cancelled };
  enum class mPingAt: unsigned int { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings };
  enum class mPongAt: unsigned int { ShortPingFair, LongPingFair, ShortPingAggressive, LongPingAggressive };
  enum class mQuotingMode: unsigned int { Top, Mid, Join, InverseJoin, InverseTop, PingPong, Boomerang, AK47, HamelinRat, Depth };
  enum class mQuoteState: unsigned int { Live, Disconnected, DisabledQuotes, MissingData, UnknownHeld, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed };
  enum class mFairValueModel: unsigned int { BBO, wBBO };
  enum class mAutoPositionMode: unsigned int { Manual, EWMA_LS, EWMA_LMS };
  enum class mAPR: unsigned int { Off, Size, SizeWidth };
  enum class mSOP: unsigned int { Off, x2trades, x3trades, x2Size, x3Size, x2tradesSize, x3tradesSize };
  enum class mSTDEV: unsigned int { Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff };
  enum class uiBIT: unsigned char { MSG = '-', SNAP = '=' };
  enum class uiTXT: unsigned char {
    FairValue = 'a', Quote = 'b', ActiveSubscription = 'c', ActiveState = 'd', MarketData = 'e',
    QuotingParametersChange = 'f', SafetySettings = 'g', Product = 'h', OrderStatusReports = 'i',
    ProductAdvertisement = 'j', ApplicationState = 'k', Notepad = 'l', ToggleConfigs = 'm',
    Position = 'n', ExchangeConnectivity = 'o', SubmitNewOrder = 'p', CancelOrder = 'q',
    MarketTrade = 'r', Trades = 's', ExternalValuation = 't', QuoteStatus = 'u',
    TargetBasePosition = 'v', TradeSafetyValue = 'w', CancelAllOrders = 'x',
    CleanAllClosedOrders = 'y', CleanAllOrders = 'z', CleanTrade = 'A', TradesChart = 'B',
    WalletChart = 'C', EWMAChart = 'D'
  };
  static char RBLACK[] = "\033[0;30m", RRED[]    = "\033[0;31m", RGREEN[] = "\033[0;32m", RYELLOW[] = "\033[0;33m",
              RBLUE[]  = "\033[0;34m", RPURPLE[] = "\033[0;35m", RCYAN[]  = "\033[0;36m", RWHITE[]  = "\033[0;37m",
              BBLACK[] = "\033[1;30m", BRED[]    = "\033[1;31m", BGREEN[] = "\033[1;32m", BYELLOW[] = "\033[1;33m",
              BBLUE[]  = "\033[1;34m", BPURPLE[] = "\033[1;35m", BCYAN[]  = "\033[1;36m", BWHITE[]  = "\033[1;37m";
  class Gw {
    public:
      static Gw *E(mExchange e);
      mExchange exchange = mExchange::Null;
      int    base    = 0,  quote   = 0;
      double makeFee = 0,  minTick = 0,
             takeFee = 0,  minSize = 0;
      string name    = "", symbol  = "",
             apikey  = "", secret  = "",
             user    = "", pass    = "",
             ws      = "", wS      = "",
             http    = "";
      virtual mExchange config() = 0;
      virtual void pos() = 0,
                   book() = 0,
                   send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) = 0,
                   cancel(string oI, string oE, mSide oS, unsigned long oT) = 0,
                   cancelAll() = 0;
      virtual string clientId() = 0;
      bool cancelByClientId = 0,
           supportCancelAll = 0;
  };
  struct mPair {
    int base;
    int quote;
    mPair();
    mPair(int base, int quote);
  };
  struct mWallet {
    double amount;
    double held;
    int currency;
    mWallet();
    mWallet(double amount, double held, int currency);
  };
  struct mProfit {
    double baseValue;
    double quoteValue;
    unsigned long time;
    mProfit(double baseValue, double quoteValue, unsigned long time);
  };
  struct mSafety {
    double buy;
    double sell;
    double combined;
    double buyPing;
    double sellPong;
    mSafety();
    mSafety(double buy, double sell, double combined, double buyPing, double sellPong);
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
    double baseAmount;
    double quoteAmount;
    double baseHeldAmount;
    double quoteHeldAmount;
    double value;
    double quoteValue;
    double profitBase;
    double profitQuote;
    mPair pair;
    mExchange exchange;
    mPosition();
    mPosition(double baseAmount, double quoteAmount, double baseHeldAmount, double quoteHeldAmount, double value, double quoteValue, double profitBase, double profitQuote, mPair pair, mExchange exchange);
  };
  static void to_json(json& j, const mPosition& k) {
    j = {
      {"baseAmount", k.baseAmount},
      {"quoteAmount", k.quoteAmount},
      {"baseHeldAmount", k.baseHeldAmount},
      {"quoteHeldAmount", k.quoteHeldAmount},
      {"value", k.value},
      {"quoteValue", k.quoteValue},
      {"profitBase", k.profitBase},
      {"profitQuote", k.profitQuote},
      {"pair", {{"base", k.pair.base}, {"quote", k.pair.quote}}},
      {"exchange", (int)k.exchange}
    };
  };
  struct mTrade {
    double price;
    double size;
    mSide make_side;
    mTrade(double price, double size, mSide make_side);
  };
  struct mTradeHydrated {
    string tradeId;
    unsigned long time;
    mExchange exchange;
    mPair pair;
    double price;
    double quantity;
    mSide side;
    double value;
    unsigned long Ktime;
    double Kqty;
    double Kprice;
    double Kvalue;
    double Kdiff;
    double feeCharged;
    bool loadedFromDB;
    mTradeHydrated();
    mTradeHydrated(string tradeId, unsigned long time, mExchange exchange, mPair pair, double price, double quantity, mSide side, double value, unsigned long Ktime, double Kqty, double Kprice, double Kvalue, double Kdiff, double feeCharged, bool loadedFromDB);
  };
  static void to_json(json& j, const mTradeHydrated& k) {
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
  struct mTradeDehydrated {
    double price;
    double quantity;
    unsigned long time;
    mTradeDehydrated();
    mTradeDehydrated(double price, double quantity, unsigned long time);
  };
  struct mTradeDry {
    mExchange exchange;
    int base;
    int quote;
    double price;
    double size;
    unsigned long time;
    mSide make_side;
    mTradeDry(mExchange exchange, int base, int quote, double price, double size, double time, mSide make_side);
  };
  static void to_json(json& j, const mTradeDry& k) {
    j = {
      {"exchange", (int)k.exchange},
      {"pair", {{"base", k.base}, {"quote", k.quote}}},
      {"price", k.price},
      {"size", k.size},
      {"time", k.time},
      {"make_size", (int)k.make_side}
    };
  };
  struct mOrder {
    string orderId;
    string exchangeId;
    mExchange exchange;
    mPair pair;
    mSide side;
    double quantity;
    mOrderType type;
    bool isPong;
    double price;
    mTimeInForce timeInForce;
    mORS orderStatus;
    bool preferPostOnly;
    double lastQuantity;
    unsigned long time;
    unsigned long computationalLatency;
    mOrder();
    mOrder(string orderId, mORS orderStatus);
    mOrder(string orderId, string exchangeId, mORS orderStatus, double price, double quantity, double lastQuantity);
    mOrder(string orderId, mExchange exchange, mPair pair, mSide side, double quantity, mOrderType type, bool isPong, double price, mTimeInForce timeInForce, mORS orderStatus, bool preferPostOnly);
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
    double price;
    double size;
    mLevel();
    mLevel(double price, double size);
  };
  struct mLevels {
    vector<mLevel> bids;
    vector<mLevel> asks;
    mLevels();
    mLevels(vector<mLevel> bids, vector<mLevel> asks);
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
    mLevel bid;
    mLevel ask;
    bool isBidPong;
    bool isAskPong;
    mQuote();
    mQuote(mLevel bid, mLevel ask);
    mQuote(mLevel bid, mLevel ask, bool isBidPong, bool isAskPong);
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
    mQuoteState bidStatus;
    mQuoteState askStatus;
    unsigned int quotesInMemoryNew;
    unsigned int quotesInMemoryWorking;
    unsigned int quotesInMemoryDone;
    mQuoteStatus();
    mQuoteStatus(mQuoteState bidStatus, mQuoteState askStatus, unsigned int quotesInMemoryNew, unsigned int quotesInMemoryWorking, unsigned int quotesInMemoryDone);
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
}

#endif
