/// <reference path="hitbtc.ts" />
/// <reference path="atlasats.ts" />
/// <reference path="ui.ts" />
/// <reference path="broker.ts" />
/// <reference path="agent.ts" />


var gateways : Array<CombinedGateway> = [new AtlasAts.AtlasAts(), new HitBtc.HitBtc()];
var brokers = gateways.map(g => new ExchangeBroker(g.md, g.base, g.oe));
var ui = new UI(brokers);
var orderAgg = new OrderBrokerAggregator(brokers, ui);
var agent = new Agent(orderAgg.brokers(), ui);

var exitHandler = e => {
    log("Hudson:main")("unhandled exception caught, terminating %o\n%o", e, e.stack);
    brokers.forEach(b => b.cancelOpenOrders());
};
process.on("uncaughtException", exitHandler);
