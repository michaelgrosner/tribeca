/// <reference path="coinsetter.ts" />
/// <reference path="hitbtc.ts" />
/// <reference path="okcoin.ts" />

var coinsetterBroker = new ExchangeBroker();
new Coinsetter(coinsetterBroker);

var hitbtcBroker = new ExchangeBroker();
new HitBtc(hitbtcBroker);

var okCoinBroker = new ExchangeBroker();
new OkCoin(okCoinBroker);

class CombinedBroker {
    _brokers : Array<ExchangeBroker>;

    constructor(brokers : Array<ExchangeBroker>) {
        this._brokers = brokers;
    }
}

new CombinedBroker([coinsetterBroker, hitbtcBroker]);