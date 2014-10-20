/// <reference path="hitbtc.ts" />
/// <reference path="atlasats.ts" />
/// <reference path="ui.ts" />
/// <reference path="broker.ts" />
/// <reference path="agent.ts" />


var gateways : Array<CombinedGateway> = [new AtlasAts.AtlasAts(), new HitBtc.HitBtc()];
var brokers = gateways.map(g => new ExchangeBroker(g.md, g.base, g.oe));
var agent = new Agent(brokers);
var ui = new UI(brokers, agent);
var orderAgg = new OrderBrokerAggregator(brokers, ui);

var exitHandler = e => {
    if (!(typeof e === 'undefined') && e.hasOwnProperty('stack'))
        log("Hudson:main")("Terminating", e.stack);
    else
        log("Hudson:main")("Terminating [no stack]");
    brokers.forEach(b => b.cancelOpenOrders());
    process.exit();
};
process.on("uncaughtException", exitHandler);
process.on("SIGINT", exitHandler);
