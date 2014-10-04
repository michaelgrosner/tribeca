/// <reference path="coinsetter.ts" />
/// <reference path="hitbtc.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="models.ts" />

var gateways : Array<IGateway> = [new Coinsetter(), new HitBtc(), new OkCoin()];
var brokers  = gateways.map(g => {
    return new ExchangeBroker(g);
});