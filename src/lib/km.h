#ifndef K_KM_H_
#define K_KM_H_

namespace K {
  enum class mExchange: unsigned int { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Korbit, Poloniex };
  vector<string> mCurrency = {
    "BTC", "LTC", "EUR", "GBP", "CNY", "CAD", "ETH", "ETC", "BFX", "RRT", "ZEC", "BCN", "DASH", "DOGE",
    "DSH", "EMC", "FCN", "LSK", "NXT", "QCN", "SDB", "SCB", "STEEM", "XDN", "XEM", "XMR", "ARDR", "WAVES",
    "BTU", "MAID", "AMP", "XRP", "KRW", "IOT", "BCY", "BELA", "BLK", "BTCD", "BTM", "BTS", "BURST", "CLAM",
    "DCR", "DGB", "EMC2", "EXP", "FCT", "FLDC", "FLO", "GAME", "GNO", "GNT", "GRC", "HUC", "LBC", "NAUT",
    "NAV", "NEOS", "NMC", "NOTE", "NXC", "OMNI", "PASC", "PINK", "POT", "PPC", "RADS", "REP", "RIC", "SBD",
    "SC", "SJCX", "STR", "STRAT", "SYS", "VIA", "VRC", "VTC", "XBC", "XCP", "XPM", "XVC", "USD", "USDT",
    "EOS", "SAN", "OMG", "PAY" };
  enum class mGatewayType: unsigned int { MarketData, OrderEntry, Position };
  enum class mConnectivityStatus: unsigned int { Connected, Disconnected };
  enum class mSide: unsigned int { Bid, Ask, Unknown };
  enum class mORS: unsigned int { New, Working, Complete, Cancelled };
  enum class mPingAt: unsigned int { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings };
  enum class mPongAt: unsigned int { ShortPingFair, LongPingFair, ShortPingAggressive, LongPingAggressive };
  enum class mQuotingMode: unsigned int { Top, Mid, Join, InverseJoin, InverseTop, PingPong, Boomerang, AK47, HamelinRat, Depth };
  enum class mFairValueModel: unsigned int { BBO, wBBO };
  enum class mAutoPositionMode: unsigned int { Manual, EWMA_LS, EWMA_LMS };
  enum class mAPR: unsigned int { Off, Size, SizeWidth };
  enum class mSOP: unsigned int { Off, x2trades, x3trades, x2Size, x3Size, x2tradesSize, x3tradesSize };
  enum class mSTDEV: unsigned int{ Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff };
  enum class uiBIT: unsigned char { MSG = '-', SNAP = '=' };
  enum class uiTXT: unsigned char {
    FairValue = 'a',
    Quote = 'b',
    ActiveSubscription = 'c',
    ActiveState = 'd',
    MarketData = 'e',
    QuotingParametersChange = 'f',
    SafetySettings = 'g',
    Product = 'h',
    OrderStatusReports = 'i',
    ProductAdvertisement = 'j',
    ApplicationState = 'k',
    Notepad = 'l',
    ToggleConfigs = 'm',
    Position = 'n',
    ExchangeConnectivity = 'o',
    SubmitNewOrder = 'p',
    CancelOrder = 'q',
    MarketTrade = 'r',
    Trades = 's',
    ExternalValuation = 't',
    QuoteStatus = 'u',
    TargetBasePosition = 'v',
    TradeSafetyValue = 'w',
    CancelAllOrders = 'x',
    CleanAllClosedOrders = 'y',
    CleanAllOrders = 'z',
    CleanTrade = 'A',
    TradesChart = 'B',
    WalletChart = 'C',
    EWMAChart = 'D'
  };
  struct mGWbt {
    double price;
    double size;
    mSide make_side;
    mGWbt(double p_, double s_, mSide m_):
      price(p_), size(s_), make_side(m_) {}
  };
  struct mGWbl {
    double price;
    double size;
    mGWbl(double p_, double s_):
      price(p_), size(s_) {}
  };
  struct mGWbls {
    vector<mGWbl> bids;
    vector<mGWbl> asks;
    mGWbls(vector<mGWbl> b_, vector<mGWbl> a_):
      bids(b_), asks(a_) {}
  };
}

#endif
