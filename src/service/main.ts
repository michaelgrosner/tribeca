/// <reference path="hitbtc.ts" />
/// <reference path="atlasats.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="ui.ts" />
/// <reference path="arbagent.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />

import Config = require("./config");
import HitBtc = require("./hitbtc");
import OkCoin = require("./okcoin");
import BtcChina = require("./btcchina");
import Broker = require("./broker");
import Agent = require("./arbagent");
import UI = require("./ui");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Aggregators = require("./aggregators");

var env = process.env.TRIBECA_MODE;
var config = new Config.ConfigProvider(env);
var gateways : Array<Interfaces.CombinedGateway> = [new HitBtc.HitBtc(config)];
var persister = new Broker.OrderStatusPersister();
var brokers = gateways.map(g => new Broker.ExchangeBroker(g.md, g.base, g.oe, g.pg, persister));
var posAgg = new Aggregators.PositionAggregator(brokers);
var orderAgg = new Aggregators.OrderBrokerAggregator(brokers);
var mdAgg = new Aggregators.MarketDataAggregator(brokers);
var agent = new Agent.ArbAgent(brokers, mdAgg, orderAgg, config);
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
