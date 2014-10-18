/// <reference path="hitbtc.ts" />
/// <reference path="atlasats.ts" />
/// <reference path="ui.ts" />
/// <reference path="broker.ts" />
/// <reference path="agent.ts" />

var _log = log("Hudson:main");

var brokers : Array<IBroker> = [];

try {
    var gateways : Array<ICombinedGateway> = [new AtlasAts.AtlasAts(), new HitBtc.HitBtc()];
    brokers = gateways.map(g => new ExchangeBroker(g,g,g));
    var ui = new UI(brokers);
    var orderAgg = new OrderBrokerAggregator(brokers, ui);
    var agent = new Agent(orderAgg.brokers(), ui);
}
catch (e) {
    _log("unhandled exception caught, terminating %o", e);
    brokers.forEach(b => b.cancelOpenOrders());
}