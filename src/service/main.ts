/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />

import _ = require("lodash");
import Q = require("q");
import path = require("path");
import express = require('express');
import util = require('util');
import moment = require("moment");
import fs = require("fs");

import HitBtc = require("./gateways/hitbtc");
import OkCoin = require("./gateways/okcoin");
import Coinbase = require("./gateways/coinbase");
import NullGw = require("./gateways/nullgw");

import Utils = require("./utils");
import Config = require("./config");
import Broker = require("./broker");
import Agent = require("./arbagent");
import MarketTrades = require("./markettrades");
import Messaging = require("../common/messaging");
import Models = require("../common/models");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");
import compression = require("compression");
import Persister = require("./persister");
import Active = require("./active-state");
import FairValue = require("./fair-value");
import Web = require("./web");
import QuotingParameters = require("./quoting-parameters");
import MarketFiltration = require("./market-filtration");
import PositionManagement = require("./position-management");
import Statistics = require("./statistics");
import Backtest = require("./backtest");

["uncaughtException", "exit", "SIGINT", "SIGTERM"].forEach(reason => {
    process.on(reason, (e?) => {
        Utils.errorLog(util.format("Terminating!", reason, e, (typeof e !== "undefined" ? e.stack : undefined)));
    });
});

var mainLog = Utils.log("tribeca:main");
var messagingLog = Utils.log("tribeca:messaging");

var pair = new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD);
var orderCache = new Broker.OrderStateCache();
var config = new Config.ConfigProvider();

if (config.inBacktestMode) {
    var inputData : Array<Models.Market | Models.MarketTrade> = JSON.parse(fs.readFileSync("inputData.json", 'utf8'));
    inputData = _.sortBy(inputData, d => d.time);
    var parameters : Backtest.BacktestParameters = JSON.parse(fs.readFileSync("backtestParameters.json", 'utf8'));
    var timeProvider : Utils.ITimeProvider = new Backtest.BacktestTimeProvider(_.first(inputData).time);
    var exchange = Models.Exchange.Null;
    var gw = new Backtest.BacktestGateway(inputData, parameters.startingBasePosition, parameters.startingQuotePosition, <Backtest.BacktestTimeProvider>timeProvider);
    var gateway = new Backtest.BacktestExchange(gw);
    
    var getPublisher = <T>(topic: string, persister: Persister.ILoadAll<T> = null): Messaging.IPublish<T> => { 
        return new Messaging.NullPublisher<T>();
    }
    
    var getReceiver = <T>(topic: string) : Messaging.IReceive<T> => new Messaging.NullReceiver<T>();
    
    var getPersister = <T>(collectionName: string) : Persister.ILoadAllByExchangeAndPair<T> => new Backtest.BacktestPersister<T>();
    var getMarketTradePersister = () => getPersister<Models.MarketTrade>("mt");
    
    var getRepository = <T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T> => new Backtest.BacktestPersister<T>();
    
    var startingActive : Models.SerializedQuotesActive = new Models.SerializedQuotesActive(true, timeProvider.utcNow());
    var startingParameters : Models.QuotingParameters = parameters.quotingParameters;
}
else {
    var timeProvider : Utils.ITimeProvider = new Utils.RealTimeProvider();
    
    var app = express();
    var http = (<any>require('http')).Server(app);
    var io = require('socket.io')(http);

    var basicAuth = require('basic-auth-connect');

    var isFullUser = (username, password) => username === "mgrosner" && password === "password";
    app.use(basicAuth(isFullUser));

    app.use(<any>compression());
    app.use(express.static(path.join(__dirname, "admin")));
    http.listen(3000, () => mainLog('Listening to admins on *:3000...'));

    var advert = new Models.ProductAdvertisement(exchange, pair, Config.Environment[config.environment()]);
    new Messaging.Publisher<Models.ProductAdvertisement>(Messaging.Topics.ProductAdvertisement, io, () => [advert]).publish(advert);
    
    var getExchange = (): Models.Exchange => {
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
    
    var getExch = (): Interfaces.CombinedGateway => {
        switch (exchange) {
            case Models.Exchange.HitBtc: return <Interfaces.CombinedGateway>(new HitBtc.HitBtc(config));
            case Models.Exchange.OkCoin: return <Interfaces.CombinedGateway>(new OkCoin.OkCoin(config));
            case Models.Exchange.Coinbase: return <Interfaces.CombinedGateway>(new Coinbase.Coinbase(config, orderCache, timeProvider));
            case Models.Exchange.Null: return <Interfaces.CombinedGateway>(new NullGw.NullGateway());
            default: throw new Error("no gateway provided for exchange " + exchange);
        }
    };

    var gateway = getExch();
    
    var getPublisher = <T>(topic: string, persister: Persister.ILoadAll<T> = null): Messaging.IPublish<T> => {
        var socketIoPublisher = new Messaging.Publisher<T>(topic, io, null, Utils.log("tribeca:messaging"));
        if (persister !== null)
            return new Web.StandaloneHttpPublisher<T>(socketIoPublisher, topic, app, persister);
        else
            return socketIoPublisher;
    };
    
    var getReceiver = <T>(topic: string) : Messaging.IReceive<T> => new Messaging.Receiver<T>(topic, io, messagingLog);
    
    var db = config.inBacktestMode ? null : Persister.loadDb();
    
    var getPersister = <T>(collectionName: string) : Persister.ILoadAllByExchangeAndPair<T> => 
        new Persister.BasicPersister<T>(db, collectionName);
    var getMarketTradePersister = () : Persister.ILoadAllByExchangeAndPair<Models.MarketTrade> => new MarketTrades.MarketTradePersister(db);
        
    var getRepository = <T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T> => 
        new Persister.RepositoryPersister<T>(db, defValue, collectionName);
        
    var startingActive : Models.SerializedQuotesActive = new Models.SerializedQuotesActive(false, timeProvider.utcNow())
    var startingParameters : Models.QuotingParameters = new Models.QuotingParameters(.3, .05, Models.QuotingMode.Top, Models.FairValueModel.BBO, 3, .8, false, Models.AutoPositionMode.Off, false, 2.5, 300);
}

var orderPersister = getPersister("osr");
var tradesPersister = getPersister("trades");
var fairValuePersister = getPersister("fv");
var mktTradePersister = getMarketTradePersister();
var positionPersister = getPersister("pos");
var messagesPersister = getPersister("msg");
var rfvPersister = getPersister("rfv");
var tbpPersister = getPersister("tbp");
var tsvPersister = getPersister("tsv");
var marketDataPersister = getPersister(Messaging.Topics.MarketData);

var activePersister = getRepository(startingActive, Messaging.Topics.ActiveChange);
var paramsPersister = getRepository(startingParameters, Messaging.Topics.QuotingParametersChange);

Q.all([
    orderPersister.load(exchange, pair, 25000),
    tradesPersister.load(exchange, pair, 10000),
    mktTradePersister.load(exchange, pair, 100),
    messagesPersister.loadAll(50),
    paramsPersister.loadLatest(),
    activePersister.loadLatest(),
    rfvPersister.loadAll(50)
]).spread((initOrders: Models.OrderStatusReport[],
    initTrades: Models.Trade[],
    initMktTrades: Models.MarketTrade[],
    initMsgs: Models.Message[],
    initParams: Models.QuotingParameters,
    initActive: Models.SerializedQuotesActive,
    initRfv: Models.RegularFairValue[]) => {

    var quotePublisher = getPublisher(Messaging.Topics.Quote);
    var fvPublisher = getPublisher(Messaging.Topics.FairValue, fairValuePersister);
    var marketDataPublisher = getPublisher(Messaging.Topics.MarketData, marketDataPersister);
    var orderStatusPublisher = getPublisher(Messaging.Topics.OrderStatusReports, orderPersister);
    var tradePublisher = getPublisher(Messaging.Topics.Trades, tradesPersister);
    var safetySettingsPublisher = getPublisher(Messaging.Topics.SafetySettings);
    var activePublisher = getPublisher(Messaging.Topics.ActiveChange);
    var quotingParametersPublisher = getPublisher(Messaging.Topics.QuotingParametersChange);
    var marketTradePublisher = getPublisher(Messaging.Topics.MarketTrade, mktTradePersister);
    var messagesPublisher = getPublisher(Messaging.Topics.Message, messagesPersister);
    var quoteStatusPublisher = getPublisher(Messaging.Topics.QuoteStatus);
    var targetBasePositionPublisher = getPublisher(Messaging.Topics.TargetBasePosition, tbpPersister);
    var tradeSafetyPublisher = getPublisher(Messaging.Topics.TradeSafetyValue, tsvPersister);
    var positionPublisher = getPublisher(Messaging.Topics.Position, positionPersister);
    var connectivity = getPublisher(Messaging.Topics.ExchangeConnectivity);

    var messages = new Broker.MessagesPubisher(timeProvider, messagesPersister, initMsgs, messagesPublisher);
    messages.publish("start up");

    var safetySettingsReceiver = getReceiver(Messaging.Topics.SafetySettings);
    var activeReceiver = getReceiver(Messaging.Topics.ActiveChange);
    var quotingParametersReceiver = getReceiver(Messaging.Topics.QuotingParametersChange);
    var submitOrderReceiver = getReceiver(Messaging.Topics.SubmitNewOrder);
    var cancelOrderReceiver = getReceiver(Messaging.Topics.CancelOrder);

    var broker = new Broker.ExchangeBroker(pair, gateway.md, gateway.base, gateway.oe, gateway.pg, connectivity);
    var orderBroker = new Broker.OrderBroker(timeProvider, broker, gateway.oe, orderPersister, tradesPersister, orderStatusPublisher,
        tradePublisher, submitOrderReceiver, cancelOrderReceiver, messages, orderCache, initOrders, initTrades);
    var marketDataBroker = new Broker.MarketDataBroker(gateway.md, marketDataPublisher, marketDataPersister, messages);
    var positionBroker = new Broker.PositionBroker(timeProvider, broker, gateway.pg, positionPublisher, positionPersister, marketDataBroker);

    var paramsRepo = new QuotingParameters.QuotingParametersRepository(quotingParametersPublisher, quotingParametersReceiver, initParams);
    paramsRepo.NewParameters.on(() => paramsPersister.persist(paramsRepo.latest));

    var safetyCalculator = new Safety.SafetyCalculator(timeProvider, paramsRepo, orderBroker, paramsRepo, tradeSafetyPublisher, tsvPersister);

    var startQuoting = (timeProvider.utcNow().diff(initActive.time, 'minutes') < 3 && initActive.active);
    var active = new Active.ActiveRepository(startQuoting, broker, activePublisher, activeReceiver);

    var quoter = new Quoter.Quoter(orderBroker, broker);
    var filtration = new MarketFiltration.MarketFiltration(quoter, marketDataBroker);
    var fvEngine = new FairValue.FairValueEngine(timeProvider, filtration, paramsRepo, fvPublisher, fairValuePersister);
    var ewma = new Agent.EWMACalculator(timeProvider, fvEngine);

    var rfvValues = _.map(initRfv, (r: Models.RegularFairValue) => r.value);
    var shortEwma = new Statistics.EwmaStatisticCalculator(2 * .095);
    shortEwma.initialize(rfvValues);
    var longEwma = new Statistics.EwmaStatisticCalculator(.095);
    longEwma.initialize(rfvValues);

    var positionMgr = new PositionManagement.PositionManager(timeProvider, rfvPersister, fvEngine, initRfv, shortEwma, longEwma);
    var tbp = new PositionManagement.TargetBasePositionManager(timeProvider, positionMgr, paramsRepo, positionBroker, targetBasePositionPublisher, tbpPersister);
    var quotingEngine = new Agent.QuotingEngine(timeProvider, filtration, fvEngine, paramsRepo, quotePublisher,
        orderBroker, positionBroker, ewma, tbp, safetyCalculator);
    var quoteSender = new Agent.QuoteSender(timeProvider, quotingEngine, quoteStatusPublisher, quoter, active, positionBroker, fvEngine, marketDataBroker, broker);

    var marketTradeBroker = new MarketTrades.MarketTradeBroker(gateway.md, marketTradePublisher, marketDataBroker,
        quotingEngine, broker, mktTradePersister, initMktTrades);
        
    if (config.inBacktestMode) {
        (<Backtest.BacktestExchange>gateway).run();
        return;
    }

    ["uncaughtException", "exit", "SIGINT", "SIGTERM"].forEach(reason => {
        process.on(reason, (e?) => {

            var a = new Models.SerializedQuotesActive(active.savedQuotingMode, timeProvider.utcNow());
            mainLog("persisting active to", active.savedQuotingMode);
            activePersister.persist(a);

            orderBroker.cancelOpenOrders().then(n_cancelled => {
                Utils.errorLog(util.format("Cancelled all", n_cancelled, "open orders"), () => {
                    process.exit(0)
                });
            }).done();

            timeProvider.setTimeout(() => {
                Utils.errorLog("Could not cancel all open orders!", () => {
                    process.exit(2)
                });
            }, moment.duration(1000));
        });
    });

    // event looped blocked timer
    var start = process.hrtime();
    var interval = 100;
    setInterval(() => {
        var delta = process.hrtime(start);
        var ms = (delta[0] * 1e9 + delta[1]) / 1e6;
        var n = ms - interval;
        if (n > 25)
            mainLog("Event looped blocked for " + Utils.roundFloat(n) + "ms");
        start = process.hrtime();
    }, interval).unref();

}).done();
