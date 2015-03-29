/// <reference path="hitbtc.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="arbagent.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />

import Utils = require("./utils");
import Config = require("./config");
import HitBtc = require("./hitbtc");
import OkCoin = require("./okcoin");
import Coinbase = require("./coinbase");
import NullGw = require("./nullgw");
import Broker = require("./broker");
import Agent = require("./arbagent");
import MarketTrades = require("./markettrades");
import Messaging = require("../common/messaging");
import Models = require("../common/models");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");
import path = require("path");
import Q = require("q");
import express = require('express');
import compression = require("compression");
import Persister = require("./persister");
import Web = require("./web");
import util = require('util');

var mainLog = Utils.log("tribeca:main");
var messagingLog = Utils.log("tribeca:messaging");

["uncaughtException", "exit", "SIGINT", "SIGTERM"].forEach(reason => {
    process.on(reason, (e?) => {
        Utils.errorLog(util.format("Terminating!", reason, e, (typeof e !== "undefined" ? e.stack : undefined)));
    });
});

var env = process.env.TRIBECA_MODE;
var config = new Config.ConfigProvider(env);

var pair = new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD);

var getExchange = () : Models.Exchange => {
    var ex = process.env.EXCHANGE.toLowerCase();
    switch (ex) {
        case "hitbtc": return Models.Exchange.HitBtc;
        case "okcoin": return Models.Exchange.OkCoin;
        case "coinbase": return Models.Exchange.Coinbase;
        case "null": return Models.Exchange.Null;
        default: throw new Error("unknown configuration env variable EXCHANGE " + ex);
    }
};

var exchange = getExchange();

var getEngineTopic = (topic : string) : string => {
    return topic;
};

var db = Persister.loadDb();
var orderPersister = new Persister.OrderStatusPersister(db);
var tradesPersister = new Persister.TradePersister(db);
var fairValuePersister = new Persister.FairValuePersister(db);
var mktTradePersister = new MarketTrades.MarketTradePersister(db);
var positionPersister = new Broker.PositionPersister(db);
var messagesPersister = new Persister.MessagesPersister(db);
var activePersister = new Persister.RepositoryPersister(db, new Models.SerializedQuotesActive(false, Utils.date()), getEngineTopic(Messaging.Topics.ActiveChange));
var safetyPersister = new Persister.RepositoryPersister(db, new Models.SafetySettings(4, 60, 5), getEngineTopic(Messaging.Topics.SafetySettings));
var paramsPersister = new Persister.RepositoryPersister(db, 
    new Models.QuotingParameters(.3, .05, Models.QuotingMode.Top, Models.FairValueModel.BBO, 3, .8, null), 
    getEngineTopic(Messaging.Topics.QuotingParametersChange));

Q.all([
    orderPersister.load(exchange, pair, 25000),
    tradesPersister.load(exchange, pair, 10000),
    mktTradePersister.load(exchange, pair, 100),
    messagesPersister.loadAll(50),
    safetyPersister.loadLatest(),
    paramsPersister.loadLatest(),
    activePersister.loadLatest()
]).spread((initOrders : Models.OrderStatusReport[], 
           initTrades : Models.Trade[], 
           initMktTrades : Models.ExchangePairMessage<Models.MarketTrade>[],
           initMsgs : Models.Message[],
           initSafety : Models.SafetySettings, 
           initParams : Models.QuotingParameters,
           initActive : Models.SerializedQuotesActive) => {

    var app = express();
    var http = (<any>require('http')).Server(app);
    var io = require('socket.io')(http);

    app.use(<any>compression());
    app.use(express.static(path.join(__dirname, "admin")));
    http.listen(3000, () => mainLog('Listening to admins on *:3000...'));

    var advert = new Models.ProductAdvertisement(exchange, pair, Config.Environment[config.environment()]);
    new Messaging.Publisher<Models.ProductAdvertisement>(Messaging.Topics.ProductAdvertisement, io, () => [advert]).publish(advert);

    var getEnginePublisher = <T>(topic : string) : Messaging.IPublish<T> => {
        var wrappedTopic = getEngineTopic(topic);
        var socketIoPublisher = new Messaging.Publisher<T>(wrappedTopic, io, null, Utils.log("tribeca:messaging"));
        return new Web.HttpPublisher<T>(topic, socketIoPublisher, app);
    };

    var quotePublisher = getEnginePublisher<Models.TwoSidedQuote>(Messaging.Topics.Quote);
    var fvPublisher = getEnginePublisher(Messaging.Topics.FairValue);
    var marketDataPublisher = getEnginePublisher(Messaging.Topics.MarketData);
    var orderStatusPublisher = getEnginePublisher(Messaging.Topics.OrderStatusReports);
    var tradePublisher = getEnginePublisher(Messaging.Topics.Trades);
    var safetySettingsPublisher = getEnginePublisher(Messaging.Topics.SafetySettings);
    var activePublisher = getEnginePublisher(Messaging.Topics.ActiveChange);
    var quotingParametersPublisher = getEnginePublisher(Messaging.Topics.QuotingParametersChange);
    var marketTradePublisher = getEnginePublisher(Messaging.Topics.MarketTrade);
    var messagesPublisher = getEnginePublisher(Messaging.Topics.Message);
    var externalValuationPublisher = getEnginePublisher(Messaging.Topics.ExternalValuation);
    var quoteStatusPublisher = getEnginePublisher(Messaging.Topics.QuoteStatus);

    var messages = new Broker.MessagesPubisher(messagesPersister, initMsgs, messagesPublisher);
    messages.publish("start up");

    var getHttpPublisher = <T>(topic : string) => {
        return new Web.StandaloneHttpPublisher<T>(topic, app);
    };

    var tradeHttpPublisher = getHttpPublisher<Models.Trade>("trades");
    var latencyHttpPublisher = getHttpPublisher<number>("latency");
    var positionHttpPublisher = getHttpPublisher<Models.PositionReport>("position");
    var osrHttpPublisher = getHttpPublisher<Models.OrderStatusReport>("new_orders");
    var fvHttpPublisher = getHttpPublisher<Models.FairValue>("fair_value");

    var getExchangePublisher = <T>(topic : string) => {
        return new Messaging.Publisher<T>(topic, io, null, messagingLog);
    };

    var positionPublisher = getExchangePublisher(Messaging.Topics.Position);
    var connectivity = getExchangePublisher(Messaging.Topics.ExchangeConnectivity);

    var getReceiver = <T>(topic : string) => {
        return new Messaging.Receiver<T>(getEngineTopic(topic), io, messagingLog);
    };

    var safetySettingsReceiver = getReceiver(Messaging.Topics.SafetySettings);
    var activeReceiver = getReceiver(Messaging.Topics.ActiveChange);
    var quotingParametersReceiver = getReceiver(Messaging.Topics.QuotingParametersChange);
    var submitOrderReceiver = new Messaging.Receiver(Messaging.Topics.SubmitNewOrder, io, messagingLog);
    var cancelOrderReceiver = new Messaging.Receiver(Messaging.Topics.CancelOrder, io, messagingLog);

    var orderCache = new Broker.OrderStateCache();

    var getExch = () : Interfaces.CombinedGateway => {
        switch (exchange) {
            case Models.Exchange.HitBtc: return <Interfaces.CombinedGateway>(new HitBtc.HitBtc(config));
            case Models.Exchange.OkCoin: return <Interfaces.CombinedGateway>(new OkCoin.OkCoin(config));
            case Models.Exchange.Coinbase: return <Interfaces.CombinedGateway>(new Coinbase.Coinbase(config, orderCache));
            case Models.Exchange.Null: return <Interfaces.CombinedGateway>(new NullGw.NullGateway());
            default: throw new Error("no gateway provided for exchange " + exchange);
        }
    };

    var gateway = getExch();

    var broker = new Broker.ExchangeBroker(pair, gateway.md, gateway.base, gateway.oe, gateway.pg, connectivity);
    var orderBroker = new Broker.OrderBroker(broker, gateway.oe, orderPersister, tradesPersister, orderStatusPublisher,
        tradePublisher, submitOrderReceiver, cancelOrderReceiver, messages, tradeHttpPublisher, latencyHttpPublisher, osrHttpPublisher,
        orderCache, initOrders, initTrades);
    var marketDataBroker = new Broker.MarketDataBroker(gateway.md, marketDataPublisher, messages);
    var positionBroker = new Broker.PositionBroker(broker, gateway.pg, positionPublisher, positionPersister, marketDataBroker, positionHttpPublisher);

    var paramsRepo = new Agent.QuotingParametersRepository(quotingParametersPublisher, quotingParametersReceiver, initParams);
    paramsRepo.NewParameters.on(() => paramsPersister.persist(paramsRepo.latest));

    var safetyRepo = new Safety.SafetySettingsRepository(safetySettingsPublisher, safetySettingsReceiver, initSafety);
    safetyRepo.NewParameters.on(() => safetyPersister.persist(safetyRepo.latest));
    var safeties = new Safety.SafetySettingsManager(safetyRepo, orderBroker, paramsRepo, messages);

    var startQuoting = (Utils.date().diff(initActive.time, 'minutes') < 3 && initActive.active);
    var active = new Agent.ActiveRepository(startQuoting, safeties, broker, activePublisher, activeReceiver);

    var quoter = new Quoter.Quoter(orderBroker, broker);
    var filtration = new Agent.MarketFiltration(quoter, marketDataBroker);
    var fvEngine = new Agent.FairValueEngine(filtration, paramsRepo, fvPublisher, fvHttpPublisher, fairValuePersister);
    var emptyEwma = new Agent.EWMACalculator(fvEngine);
    var quotingEngine = new Agent.QuotingEngine(pair, filtration, fvEngine, paramsRepo, safetyRepo, quotePublisher, marketDataBroker, 
        orderBroker, positionBroker, emptyEwma);
    var quoteSender = new Agent.QuoteSender(quotingEngine, quoteStatusPublisher, quoter, pair, active, positionBroker, fvEngine, marketDataBroker);

    var marketTradeBroker = new MarketTrades.MarketTradeBroker(gateway.md, marketTradePublisher, marketDataBroker,
        quotingEngine, broker, mktTradePersister, initMktTrades);

    ["uncaughtException", "exit", "SIGINT", "SIGTERM"].forEach(reason => {
        process.on(reason, (e?) => {

            var a = new Models.SerializedQuotesActive(active.latest, Utils.date());
            mainLog("setting active to", a);
            activePersister.persist(a);

            orderBroker.cancelOpenOrders().then(n_cancelled => {
                Utils.errorLog(util.format("Cancelled all", n_cancelled, "open orders"), () => {
                    process.exit(0)
                });
            }).done();

            setTimeout(() => {
                Utils.errorLog("Could not cancel all open orders!", () => {
                    process.exit(2)
                });
            }, 2000);
        });
    });

}).done();
