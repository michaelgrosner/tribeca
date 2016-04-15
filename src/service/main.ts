/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="broker.ts"/>
/// <reference path="active-state.ts"/>
/// <reference path="backtest.ts"/>
/// <reference path="config.ts"/>
/// <reference path="fair-value.ts"/>
/// <reference path="interfaces.ts"/>
/// <reference path="market-filtration.ts"/>
/// <reference path="markettrades.ts"/>
/// <reference path="messages.ts"/>
/// <reference path="quote-sender.ts"/>
/// <reference path="quoter.ts"/>
/// <reference path="quoting-engine.ts"/>
/// <reference path="quoting-parameters.ts"/>
/// <reference path="safety.ts"/>
/// <reference path="statistics.ts"/>
/// <reference path="utils.ts"/>
/// <reference path="web.ts"/>

import _ = require("lodash");
import Q = require("q");
import path = require("path");
import express = require('express');
import util = require('util');
import moment = require("moment");
import fs = require("fs");
import bunyan = require("bunyan");
import request = require('request');
import http = require("http");
import socket_io = require('socket.io')

import HitBtc = require("./gateways/hitbtc");
import Coinbase = require("./gateways/coinbase");
import NullGw = require("./gateways/nullgw");
import OkCoin = require("./gateways/okcoin");
import Bitfinex = require("./gateways/bitfinex");

import Utils = require("./utils");
import Config = require("./config");
import Broker = require("./broker");
import QuoteSender = require("./quote-sender");
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
import QuotingEngine = require("./quoting-engine");
import Messages = require("./messages");

import QuotingStyleRegistry = require("./quoting-styles/style-registry");
import MidMarket = require("./quoting-styles/mid-market");
import TopJoin = require("./quoting-styles/top-join");

var serverUrl = 'BACKTEST_SERVER_URL' in process.env ? process.env['BACKTEST_SERVER_URL'] : "http://localhost:5001";

var config = new Config.ConfigProvider();

let exitingEvent : () => Q.Promise<boolean>;

const performExit = () => {
    Q.timeout(exitingEvent(), 2000).then(completed => {
        if (completed)
            mainLog.info("All exiting event handlers have fired, exiting application.");
        else
            mainLog.warn("Did not complete clean-up tasks successfully, still shutting down.");
        process.exit();
    }).catch(err => {
        mainLog.error(err, "Error while exiting application.");
        process.exit(1);
    });
};

process.on("uncaughtException", err => {
    mainLog.error(err, "Unhandled exception!");
    performExit();
});

process.on("unhandledRejection", (reason, p) => {
    mainLog.error(reason, "Unhandled promise rejection!", p);
    performExit();
});

process.on("exit", (code) => {
    mainLog.info("Exiting with code", code);
});

process.on("SIGINT", () => {
    mainLog.info("Handling SIGINT");
    performExit();
});

var mainLog = Utils.log("tribeca:main");
var messagingLog = Utils.log("tribeca:messaging");

function ParseCurrencyPair(raw: string) : Models.CurrencyPair {
    var split = raw.split("/");
    if (split.length !== 2) 
        throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/USD");
    
    return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
}
var pair = ParseCurrencyPair(config.GetString("TradedPair"));

var defaultActive : Models.SerializedQuotesActive = new Models.SerializedQuotesActive(false, moment.unix(1));
var defaultQuotingParameters : Models.QuotingParameters = new Models.QuotingParameters(.3, .05, Models.QuotingMode.Top, 
    Models.FairValueModel.BBO, 3, .8, false, Models.AutoPositionMode.Off, false, 2.5, 300, .095, 2*.095, .095, 3, .1);

var backTestSimulationSetup = (inputData : Array<Models.Market | Models.MarketTrade>, parameters : Backtest.BacktestParameters) => {
    var timeProvider : Utils.ITimeProvider = new Backtest.BacktestTimeProvider(_.first(inputData).time, _.last(inputData).time);
    var exchange = Models.Exchange.Null;
    var gw = new Backtest.BacktestGateway(inputData, parameters.startingBasePosition, parameters.startingQuotePosition, <Backtest.BacktestTimeProvider>timeProvider);
    
    var getExch = (orderCache: Broker.OrderStateCache): Interfaces.CombinedGateway => new Backtest.BacktestExchange(gw);
    
    var getPublisher = <T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T> => { 
        return new Messaging.NullPublisher<T>();
    };
    
    var getReceiver = <T>(topic: string) : Messaging.IReceive<T> => new Messaging.NullReceiver<T>();
    
    var getPersister = <T>(collectionName: string) : Persister.ILoadAll<T> => new Backtest.BacktestPersister<T>();
    
    var getRepository = <T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T> => new Backtest.BacktestPersister<T>([defValue]);
    
    var startingActive : Models.SerializedQuotesActive = new Models.SerializedQuotesActive(true, timeProvider.utcNow());
    var startingParameters : Models.QuotingParameters = parameters.quotingParameters;

    return {
        exchange: exchange,
        startingActive: startingActive,
        startingParameters: startingParameters,
        timeProvider: timeProvider,
        getExch: getExch,
        getReceiver: getReceiver,
        getPersister: getPersister,
        getRepository: getRepository,
        getPublisher: getPublisher
    };
};

var liveTradingSetup = () => {
    var timeProvider : Utils.ITimeProvider = new Utils.RealTimeProvider();
    
    var app = express();
    var http_server = http.createServer(app);
    var io = socket_io(http_server);

    var username = config.GetString("WebClientUsername");
    var password = config.GetString("WebClientPassword");
    if (username !== "NULL" && password !== "NULL") {
        mainLog.info("Requiring authentication to web client");
        var basicAuth = require('basic-auth-connect');
        app.use(basicAuth((u, p) => u === username && p === password));
    }

    app.use(compression());
    app.use(express.static(path.join(__dirname, "admin")));
    
    var webport = config.GetNumber("WebClientListenPort");
    http_server.listen(webport, () => mainLog.info('Listening to admins on *:', webport));
    
    var getExchange = (): Models.Exchange => {
        var ex = config.GetString("EXCHANGE").toLowerCase();
        switch (ex) {
            case "hitbtc": return Models.Exchange.HitBtc;
            case "coinbase": return Models.Exchange.Coinbase;
            case "okcoin": return Models.Exchange.OkCoin;
            case "null": return Models.Exchange.Null;
            case "bitfinex": return Models.Exchange.Bitfinex;
            default: throw new Error("unknown configuration env variable EXCHANGE " + ex);
        }
    };
    
    var exchange = getExchange();
    
    var getExch = (orderCache: Broker.OrderStateCache): Interfaces.CombinedGateway => {
        switch (exchange) {
            case Models.Exchange.HitBtc: return <Interfaces.CombinedGateway>(new HitBtc.HitBtc(config, pair));
            case Models.Exchange.Coinbase: return <Interfaces.CombinedGateway>(new Coinbase.Coinbase(config, orderCache, timeProvider, pair));
            case Models.Exchange.OkCoin: return <Interfaces.CombinedGateway>(new OkCoin.OkCoin(config, pair));
            case Models.Exchange.Null: return <Interfaces.CombinedGateway>(new NullGw.NullGateway());
            case Models.Exchange.Bitfinex: return <Interfaces.CombinedGateway>(new Bitfinex.Bitfinex(timeProvider, config, pair));
            default: throw new Error("no gateway provided for exchange " + exchange);
        }
    };
    
    var getPublisher = <T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T> => {
        var socketIoPublisher = new Messaging.Publisher<T>(topic, io, null, messagingLog.info.bind(messagingLog));
        if (persister)
            return new Web.StandaloneHttpPublisher<T>(socketIoPublisher, topic, app, persister);
        else
            return socketIoPublisher;
    };
    
    var getReceiver = <T>(topic: string) : Messaging.IReceive<T> => 
        new Messaging.Receiver<T>(topic, io, messagingLog.info.bind(messagingLog));
    
    var db = Persister.loadDb(config);
    
    var loaderSaver = new Persister.LoaderSaver(exchange, pair);
    var mtLoaderSaver = new MarketTrades.MarketTradesLoaderSaver(loaderSaver);
    
    var getPersister = <T>(collectionName: string) : Persister.ILoadAll<T> => {
        var ls = collectionName === "mt" ? mtLoaderSaver : loaderSaver;
        return new Persister.Persister<T>(db, collectionName, exchange, pair, ls.loader, ls.saver);
    };
        
    var getRepository = <T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T> => 
        new Persister.RepositoryPersister<T>(db, defValue, collectionName, exchange, pair, loaderSaver.loader, loaderSaver.saver);

    return {
        exchange: exchange,
        startingActive: defaultActive,
        startingParameters: defaultQuotingParameters,
        timeProvider: timeProvider,
        getExch: getExch,
        getReceiver: getReceiver,
        getPersister: getPersister,
        getRepository: getRepository,
        getPublisher: getPublisher
    };
};

interface SimulationClasses {
    exchange: Models.Exchange;
    startingActive : Models.SerializedQuotesActive;
    startingParameters : Models.QuotingParameters;
    timeProvider: Utils.ITimeProvider;
    getExch(orderCache: Broker.OrderStateCache): Interfaces.CombinedGateway;
    getReceiver<T>(topic: string) : Messaging.IReceive<T>;
    getPersister<T>(collectionName: string) : Persister.ILoadAll<T>;
    getRepository<T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T>;
    getPublisher<T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T>;
}

var runTradingSystem = (classes: SimulationClasses) : Q.Promise<boolean> => {
    var getPersister = classes.getPersister;
    var orderPersister = getPersister("osr");
    var tradesPersister = getPersister("trades");
    var fairValuePersister = getPersister("fv");
    var mktTradePersister = getPersister("mt");
    var positionPersister = getPersister("pos");
    var messagesPersister = getPersister("msg");
    var rfvPersister = getPersister("rfv");
    var tbpPersister = getPersister("tbp");
    var tsvPersister = getPersister("tsv");
    var marketDataPersister = getPersister(Messaging.Topics.MarketData);
    
    var activePersister = classes.getRepository(classes.startingActive, Messaging.Topics.ActiveChange);
    var paramsPersister = classes.getRepository(classes.startingParameters, Messaging.Topics.QuotingParametersChange);
    
    var exchange = classes.exchange;
    var completedSuccessfully = Q.defer<boolean>();
    
    Q.all<any>([
        orderPersister.loadAll(25000),
        tradesPersister.loadAll(10000),
        mktTradePersister.loadAll(100),
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
            
        _.defaults(initParams, defaultQuotingParameters);
        _.defaults(initActive, defaultActive);
    
        var orderCache = new Broker.OrderStateCache();
        var timeProvider = classes.timeProvider;
        var getPublisher = classes.getPublisher;
        
        var advert = new Models.ProductAdvertisement(exchange, pair, config.GetString("TRIBECA_MODE"));
        getPublisher(Messaging.Topics.ProductAdvertisement).registerSnapshot(() => [advert]).publish(advert);
        
        var quotePublisher = getPublisher(Messaging.Topics.Quote);
        var fvPublisher = getPublisher(Messaging.Topics.FairValue, fairValuePersister);
        var marketDataPublisher = getPublisher(Messaging.Topics.MarketData, marketDataPersister);
        var orderStatusPublisher = getPublisher(Messaging.Topics.OrderStatusReports, orderPersister);
        var tradePublisher = getPublisher(Messaging.Topics.Trades, tradesPersister);
        var activePublisher = getPublisher(Messaging.Topics.ActiveChange);
        var quotingParametersPublisher = getPublisher(Messaging.Topics.QuotingParametersChange);
        var marketTradePublisher = getPublisher(Messaging.Topics.MarketTrade, mktTradePersister);
        var messagesPublisher = getPublisher(Messaging.Topics.Message, messagesPersister);
        var quoteStatusPublisher = getPublisher(Messaging.Topics.QuoteStatus);
        var targetBasePositionPublisher = getPublisher(Messaging.Topics.TargetBasePosition, tbpPersister);
        var tradeSafetyPublisher = getPublisher(Messaging.Topics.TradeSafetyValue, tsvPersister);
        var positionPublisher = getPublisher(Messaging.Topics.Position, positionPersister);
        var connectivity = getPublisher(Messaging.Topics.ExchangeConnectivity);
        
        var messages = new Messages.MessagesPubisher(timeProvider, messagesPersister, initMsgs, messagesPublisher);
        messages.publish("start up");
    
        var getReceiver = classes.getReceiver;
        var activeReceiver = getReceiver(Messaging.Topics.ActiveChange);
        var quotingParametersReceiver = getReceiver(Messaging.Topics.QuotingParametersChange);
        var submitOrderReceiver = getReceiver(Messaging.Topics.SubmitNewOrder);
        var cancelOrderReceiver = getReceiver(Messaging.Topics.CancelOrder);
        var cancelAllOrdersReceiver = getReceiver(Messaging.Topics.CancelAllOrders);
        
        var gateway = classes.getExch(orderCache);
        
        if (!_.some(gateway.base.supportedCurrencyPairs, p => p.base === pair.base && p.quote === pair.quote))
            throw new Error("Unsupported currency pair!. Please check that gateway " + gateway.base.name() + " supports the value specified in TradedPair config value");
    
        var broker = new Broker.ExchangeBroker(pair, gateway.md, gateway.base, gateway.oe, connectivity);
        var orderBroker = new Broker.OrderBroker(timeProvider, broker, gateway.oe, orderPersister, tradesPersister, orderStatusPublisher,
            tradePublisher, submitOrderReceiver, cancelOrderReceiver, cancelAllOrdersReceiver, messages, orderCache, initOrders, initTrades);
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
        var ewma = new Statistics.ObservableEWMACalculator(timeProvider, fvEngine, initParams.quotingEwma);
    
        var rfvValues = _.map(initRfv, (r: Models.RegularFairValue) => r.value);
        var shortEwma = new Statistics.EwmaStatisticCalculator(initParams.shortEwma);
        shortEwma.initialize(rfvValues);
        var longEwma = new Statistics.EwmaStatisticCalculator(initParams.longEwma);
        longEwma.initialize(rfvValues);
        
        var registry = new QuotingStyleRegistry.QuotingStyleRegistry([
            new MidMarket.MidMarketQuoteStyle(),
            new TopJoin.InverseJoinQuoteStyle(),
            new TopJoin.InverseTopOfTheMarketQuoteStyle(),
            new TopJoin.JoinQuoteStyle(),
            new TopJoin.TopOfTheMarketQuoteStyle(),
            new TopJoin.PingPongQuoteStyle(),
        ]);
    
        var positionMgr = new PositionManagement.PositionManager(timeProvider, rfvPersister, fvEngine, initRfv, shortEwma, longEwma);
        var tbp = new PositionManagement.TargetBasePositionManager(timeProvider, positionMgr, paramsRepo, positionBroker, targetBasePositionPublisher, tbpPersister);
        var quotingEngine = new QuotingEngine.QuotingEngine(registry, timeProvider, filtration, fvEngine, paramsRepo, quotePublisher,
            orderBroker, positionBroker, ewma, tbp, safetyCalculator);
        var quoteSender = new QuoteSender.QuoteSender(timeProvider, quotingEngine, quoteStatusPublisher, quoter, active, positionBroker, fvEngine, marketDataBroker, broker);
    
        var marketTradeBroker = new MarketTrades.MarketTradeBroker(gateway.md, marketTradePublisher, marketDataBroker,
            quotingEngine, broker, mktTradePersister, initMktTrades);
            
        if (config.inBacktestMode) {
            var t = Utils.date();
            console.log("starting backtest");
            try {
                (<Backtest.BacktestExchange>gateway).run();
            }
            catch (err) {
                console.error("exception while running backtest!", err.message, err.stack);
                completedSuccessfully.reject(err);
                return completedSuccessfully.promise;
            }
            
            var results = [paramsRepo.latest, positionBroker.latestReport, {
                trades: orderBroker._trades.map(t => [t.time.valueOf(), t.price, t.quantity, t.side]),
                volume: orderBroker._trades.reduce((p, c) => p + c.quantity, 0)
            }];
            console.log("sending back results, took: ", Utils.date().diff(t, "seconds"));
            
            request({url: serverUrl+"/result", 
                     method: 'POST', 
                     json: results}, (err, resp, body) => { });
                     
            completedSuccessfully.resolve(true);
            return completedSuccessfully.promise;
        }
        
        exitingEvent = () => {
            var a = new Models.SerializedQuotesActive(active.savedQuotingMode, timeProvider.utcNow());
            mainLog.info("persisting active to", a.active);
            activePersister.persist(a);

            orderBroker.cancelOpenOrders().then(n_cancelled => {
                mainLog.info("Cancelled all", n_cancelled, "open orders");
                completedSuccessfully.resolve(true);
            }).done();

            timeProvider.setTimeout(() => {
                if (completedSuccessfully.promise.isFulfilled) return;
                mainLog.error("Could not cancel all open orders!");
                completedSuccessfully.resolve(false);
            }, moment.duration(1000));
            return completedSuccessfully.promise;
        };
    
        // event looped blocked timer
        var start = process.hrtime();
        var interval = 100;
        setInterval(() => {
            var delta = process.hrtime(start);
            var ms = (delta[0] * 1e9 + delta[1]) / 1e6;
            var n = ms - interval;
            if (n > 25)
                mainLog.info("Event looped blocked for " + Utils.roundFloat(n) + "ms");
            start = process.hrtime();
        }, interval).unref();
    
    }).done();

    return completedSuccessfully.promise;
};

var harness = () : Q.Promise<any> => {
    if (config.inBacktestMode) {
        console.log("enter backtest mode");
        
        var getFromBacktestServer = (ep: string) : Q.Promise<any> => {
            var d = Q.defer<any>();
            request.get(serverUrl+"/"+ep, (err, resp, body) => { 
                if (err) d.reject(err);
                else d.resolve(body);
            });
            return d.promise;
        };
        
        var inputDataPromise = getFromBacktestServer("inputData").then(body => {
            var inp : Array<Models.Market | Models.MarketTrade> = (typeof body ==="string") ? eval(body) : body;
            
            for (var i = 0; i < inp.length; i++) {
                var d = inp[i];
                d.time = moment(d.time);
            }
            
            return inp;
        });
        
        var nextParameters = () : Q.Promise<Backtest.BacktestParameters> => getFromBacktestServer("nextParameters").then(body => {
            var p = (typeof body ==="string") ? <string|Backtest.BacktestParameters>JSON.parse(body) : body;
            console.log("Recv'd parameters", util.inspect(p));
            return (typeof p === "string") ? null : p;
        });
        
        var promiseWhile = <T>(body : () => Q.Promise<boolean>) => {
            var done = Q.defer<any>();
        
            var loop = () => {
                body().then(possibleResult => {
                    if (!possibleResult) return done.resolve(null);
                    else Q.when(possibleResult, loop, done.reject);
                });
            };
            
            Q.nextTick(loop);
            return done.promise;
        };
        
        var runLoop = (inputMarketData : Array<Models.Market | Models.MarketTrade>) : Q.Promise<any> => {
            var singleRun = () => {
                var runWithParameters = (p : Backtest.BacktestParameters) => {
                    return p !== null ? runTradingSystem(backTestSimulationSetup(inputMarketData, p)) : false;
                };
                    
                return nextParameters().then(runWithParameters);
            };
            
            return promiseWhile(<any>singleRun);
        };
        
        return inputDataPromise.then(runLoop);
    }
    else {
        return runTradingSystem(liveTradingSetup());
    }
};

harness().done();
