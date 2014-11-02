/// <reference path="hitbtc.ts" />
/// <reference path="atlasats.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="ui.ts" />
/// <reference path="broker.ts" />
/// <reference path="agent.ts" />

var env = process.env.TRIBECA_MODE;
var config = new ConfigProvider(env);
var gateways : Array<CombinedGateway> = [new AtlasAts.AtlasAts(config), new HitBtc.HitBtc(config)]; //, new OkCoin.OkCoin(config)];
var brokers = gateways.map(g => new ExchangeBroker(g.md, g.base, g.oe, g.pg));
var posAgg = new PositionAggregator(brokers);
var orderAgg = new OrderBrokerAggregator(brokers);
var mdAgg = new MarketDataAggregator(brokers);
var agent = new Agent(brokers, mdAgg, orderAgg, config);
var ui = new UI(env, brokers, agent, orderAgg, mdAgg, posAgg);

var exitHandler = e => {
    if (!(typeof e === 'undefined') && e.hasOwnProperty('stack'))
        log("tribeca:main")("Terminating", e, e.stack);
    else
        log("tribeca:main")("Terminating [no stack]");
    brokers.forEach(b => b.cancelOpenOrders());
    process.exit();
};
process.on("uncaughtException", exitHandler);
process.on("SIGINT", exitHandler);
