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
  enum class mGatewayType: unsigned int { MarketData, OrderEntry, Position };
  enum class mTimeInForce: unsigned int { IOC, FOK, GTC };
  enum class mConnectivity: unsigned int { Connected, Disconnected };
  enum class mLiquidity: unsigned int { Make, Take };
  enum class mOrderType: unsigned int { Limit, Market };
  enum class mSide: unsigned int { Bid, Ask, Unknown };
  enum class mORS: unsigned int { New, Working, Complete, Cancelled };
  enum class mPingAt: unsigned int { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings };
  enum class mPongAt: unsigned int { ShortPingFair, LongPingFair, ShortPingAggressive, LongPingAggressive };
  enum class mQuotingMode: unsigned int { Top, Mid, Join, InverseJoin, InverseTop, PingPong, Boomerang, AK47, HamelinRat, Depth };
  enum class mQuoteStatus: unsigned int { Live, Disconnected, DisabledQuotes, MissingData, UnknownHeld, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed };
  enum class mFairValueModel: unsigned int { BBO, wBBO };
  enum class mAutoPositionMode: unsigned int { Manual, EWMA_LS, EWMA_LMS };
  enum class mAPR: unsigned int { Off, Size, SizeWidth };
  enum class mSOP: unsigned int { Off, x2trades, x3trades, x2Size, x3Size, x2tradesSize, x3tradesSize };
  enum class mSTDEV: unsigned int{ Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff };
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
  class Gw {
    public:
      static Gw *E(mExchange e);
      mExchange exchange = mExchange::Null;
      double makeFee = 0;
      double takeFee = 0;
      double minTick = 0;
      double minSize = 0;
      string symbol = "";
      string target = "";
      string apikey = "";
      string secret = "";
      string user = "";
      string pass = "";
      string http = "";
      string ws = "";
      string wS = "";
      int quote = 0;
      int base = 0;
      virtual mExchange config() = 0;
      virtual void pos() = 0;
      virtual void book() = 0;
      virtual void send(string oI, mSide oS, double oP, double oQ, mOrderType oLM, mTimeInForce oTIF, bool oPO, unsigned long oT) = 0;
      virtual void cancel(string oI, string oE, mSide oS, unsigned long oT) = 0;
      virtual void cancelAll() = 0;
      virtual string clientId() = 0;
      bool cancelByClientId = 0;
      bool supportCancelAll = 0;
  };
  struct mGWp {
    double amount;
    double held;
    int currency;
    mGWp(double amount, double held, int currency);
  };
  struct mGWbt {
    double price;
    double size;
    mSide make_side;
    mGWbt(double price, double size, mSide make_side);
  };
  struct mGWbl {
    double price;
    double size;
    mGWbl(double price, double size);
  };
  struct mGWbls {
    vector<mGWbl> bids;
    vector<mGWbl> asks;
    mGWbls(vector<mGWbl> bids, vector<mGWbl> asks);
  };
  struct mGWos {
    string oI;
    string oE;
    mORS oS;
    mGWos(string oI, string oE, mORS oS);
  };
  struct mGWol {
    string oI;
    mORS oS;
    double oP;
    double oQ;
    mLiquidity oL;
    mGWol(string oI, mORS oS, double oP, double oQ, mLiquidity oL);
  };
  struct mGWoS {
    string oI;
    string oE;
    mORS os;
    double oP;
    double oQ;
    mSide oS;
    mGWoS(string oI, string oE, mORS os, double oP, double oQ, mSide oS);
  };
  struct mGWoa {
    string oI;
    string oE;
    mORS oS;
    double oP;
    double oQ;
    double oLQ;
    mGWoa(string oI, string oE, mORS oS, double oP, double oQ, double oLQ);
  };
  struct mGWmt {
    mExchange exchange;
    int base;
    int quote;
    double price;
    double size;
    unsigned long time;
    mSide make_side;
    mGWmt(mExchange exchange, int base, int quote, double price, double size, double time, mSide make_side);
  };
}

#endif
