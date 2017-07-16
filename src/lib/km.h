#ifndef K_KM_H_
#define K_KM_H_

namespace K {
  Persistent<Object> qpRepo;
  enum class mExchange: unsigned int { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Korbit, Poloniex };
  enum class mCurrency: unsigned int {
    BTC, LTC, EUR, GBP, CNY, CAD, ETH, ETC, BFX, RRT, ZEC, BCN, DASH, DOGE,
    DSH, EMC, FCN, LSK, NXT, QCN, SDB, SCB, STEEM, XDN, XEM, XMR, ARDR, WAVES,
    BTU, MAID, AMP, XRP, KRW, IOT, BCY, BELA, BLK, BTCD, BTM, BTS, BURST, CLAM,
    DCR, DGB, EMC2, EXP, FCT, FLDC, FLO, GAME, GNO, GNT, GRC, HUC, LBC, NAUT,
    NAV, NEOS, NMC, NOTE, NXC, OMNI, PASC, PINK, POT, PPC, RADS, REP, RIC, SBD,
    SC, SJCX, STR, STRAT, SYS, VIA, VRC, VTC, XBC, XCP, XPM, XVC, USD, USDT,
    EOS, SAN, OMG }; vector<string> mCurrency_ = {
    "BTC", "LTC", "EUR", "GBP", "CNY", "CAD", "ETH", "ETC", "BFX", "RRT", "ZEC", "BCN", "DASH", "DOGE",
    "DSH", "EMC", "FCN", "LSK", "NXT", "QCN", "SDB", "SCB", "STEEM", "XDN", "XEM", "XMR", "ARDR", "WAVES",
    "BTU", "MAID", "AMP", "XRP", "KRW", "IOT", "BCY", "BELA", "BLK", "BTCD", "BTM", "BTS", "BURST", "CLAM",
    "DCR", "DGB", "EMC2", "EXP", "FCT", "FLDC", "FLO", "GAME", "GNO", "GNT", "GRC", "HUC", "LBC", "NAUT",
    "NAV", "NEOS", "NMC", "NOTE", "NXC", "OMNI", "PASC", "PINK", "POT", "PPC", "RADS", "REP", "RIC", "SBD",
    "SC", "SJCX", "STR", "STRAT", "SYS", "VIA", "VRC", "VTC", "XBC", "XCP", "XPM", "XVC", "USD", "USDT",
    "EOS", "SAN", "OMG" };
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
    MarketData = 'e',
    OrderStatusReports = 'i',
    Position = 'n',
    QuotingParametersChange = 'f'
  };
}

#endif
