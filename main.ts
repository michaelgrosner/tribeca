/// <reference path="coinsetter.ts" />
/// <reference path="hitbtc.ts" />
/// <reference path="okcoin.ts" />

var coinsetterBroker = new MarketDataBroker();
new Coinsetter(coinsetterBroker);
//new HitBtc();

var okCoinBroker = new MarketDataBroker();
new OkCoin(okCoinBroker);