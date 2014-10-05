/// <reference path="coinsetter.ts" />
/// <reference path="hitbtc.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="models.ts" />

var gateways : Array<IGateway> = [new Coinsetter(), new HitBtc(), new OkCoin()];
var brokers : Array<IBroker> = gateways.map(g => {
    return new ExchangeBroker(g);
});
var agent : Agent = new Agent(brokers);