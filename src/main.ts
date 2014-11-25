/// <reference path="hitbtc.ts" />
/// <reference path="atlasats.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="ui.ts" />
/// <reference path="agent.ts" />
/// <reference path="models.ts" />
/// <reference path="config.ts" />

import Config = require("./config");
import HitBtc = require("./hitbtc");
import OkCoin = require("./okcoin");
import Broker = require("./broker");
import Agent = require("./agent");
import UI = require("./ui");
import Models = require("./models");
import Utils = require("./utils");

var env = process.env.TRIBECA_MODE;
var config = new Config.ConfigProvider(env);
var gateways : Array<Models.CombinedGateway> = [new HitBtc.HitBtc(config), new OkCoin.OkCoin(config)];
var persister = new Broker.OrderStatusPersister();
var brokers = gateways.map(g => new Broker.ExchangeBroker(g.md, g.base, g.oe, g.pg, persister));
var posAgg = new Agent.PositionAggregator(brokers);
var orderAgg = new Agent.OrderBrokerAggregator(brokers);
var mdAgg = new Agent.MarketDataAggregator(brokers);
var agent = new Agent.Agent(brokers, mdAgg, orderAgg, config);
var ui = new UI.UI(env, brokers, agent, orderAgg, mdAgg, posAgg);

var exitHandler = e => {
    if (!(typeof e === 'undefined') && e.hasOwnProperty('stack'))
        Utils.log("tribeca:main")("Terminating", e, e.stack);
    else
        Utils.log("tribeca:main")("Terminating [no stack]");
    brokers.forEach(b => b.cancelOpenOrders());
    process.exit();
};
process.on("uncaughtException", exitHandler);
process.on("SIGINT", exitHandler);
