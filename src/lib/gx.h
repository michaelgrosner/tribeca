#ifndef K_GX_H_
#define K_GX_H_

namespace K {
  Gw *Gw::E(mExchange e) {
    if (e == mExchange::Null) return new GwNull;
    else if (e == mExchange::OkCoin) return new GwOkCoin;
    else if (e == mExchange::Coinbase) return new GwCoinbase;
    else if (e == mExchange::Bitfinex) return new GwBitfinex;
    else if (e == mExchange::Korbit) return new GwKorbit;
    else if (e == mExchange::HitBtc) return new GwHitBtc;
    else if (e == mExchange::Poloniex) return new GwPoloniex;
    cout << FN::uiT() << "Errrror: No gateway provided for exchange \"" << CF::cfString("EXCHANGE") << "\"." << endl;
    exit(1);
  };
}

#endif
