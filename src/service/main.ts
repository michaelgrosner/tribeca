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
import Promises = require("./promises");
import QuotingParameters = require("./quoting-parameters");
import MarketFiltration = require("./market-filtration");
import PositionManagement = require("./position-management");
import Statistics = require("./statistics");
import Backtest = require("./backtest");
import QuotingEngine = require("./quoting-engine");
import Messages = require("./messages");
import log from "./logging";

import QuotingStyleRegistry = require("./quoting-styles/style-registry");
import MidMarket = require("./quoting-styles/mid-market");
import TopJoin = require("./quoting-styles/top-join");
import Depth = require("./quoting-styles/depth");

const serverUrl = 'BACKTEST_SERVER_URL' in process.env ? process.env['BACKTEST_SERVER_URL'] : "http://localhost:5001";

const config = new Config.ConfigProvider();

let exitingEvent : () => Promise<number> = () => new Promise(() => 0);

const performExit = () => {
    Promises.timeout(2000, exitingEvent()).then(completed => {
        mainLog.info("All exiting event handlers have fired, exiting application.");
        process.exit();
    }).catch(() => {
        mainLog.warn("Did not complete clean-up tasks successfully, still shutting down.");
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

const mainLog = log("tribeca:main");
const messagingLog = log("tribeca:messaging");

function ParseCurrencyPair(raw: string) : Models.CurrencyPair {
    const split = raw.split("/");
    if (split.length !== 2) 
        throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/USD");
    
    return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
}
const pair = ParseCurrencyPair(config.GetString("TradedPair"));

const defaultActive : Models.SerializedQuotesActive = new Models.SerializedQuotesActive(false, new Date(1));
const defaultQuotingParameters : Models.QuotingParameters = new Models.QuotingParameters(.3, .05, Models.QuotingMode.Top, 
    Models.FairValueModel.BBO, 3, .8, false, Models.AutoPositionMode.Off, false, 2.5, 300, .095, 2*.095, .095, 3, .1);

const backTestSimulationSetup = (inputData : Array<Models.Market | Models.MarketTrade>, parameters : Backtest.BacktestParameters) : SimulationClasses => {
    const timeProvider : Utils.ITimeProvider = new Backtest.BacktestTimeProvider(moment(_.first(inputData).time), moment(_.last(inputData).time));
    const exchange = Models.Exchange.Null;
    const gw = new Backtest.BacktestGateway(inputData, parameters.startingBasePosition, parameters.startingQuotePosition, <Backtest.BacktestTimeProvider>timeProvider);
    
    const getExch = async (orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway> => new Backtest.BacktestExchange(gw);
    
    const getPublisher = <T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T> => { 
        return new Messaging.NullPublisher<T>();
    };
    
    const getReceiver = <T>(topic: string) : Messaging.IReceive<T> => new Messaging.NullReceiver<T>();
    
    const getPersister = <T>(collectionName: string) : Promise<Persister.ILoadAll<T>> => new Promise((cb) => cb(new Backtest.BacktestPersister<T>()));
    
    const getRepository = <T>(defValue: T, collectionName: string) : Promise<Persister.ILoadLatest<T>> => new Promise(cb => cb(new Backtest.BacktestPersister<T>([defValue])));
    
    const startingActive : Models.SerializedQuotesActive = new Models.SerializedQuotesActive(true, timeProvider.utcNow());
    const startingParameters : Models.QuotingParameters = parameters.quotingParameters;

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

const liveTradingSetup = () : SimulationClasses => {
    const timeProvider : Utils.ITimeProvider = new Utils.RealTimeProvider();
    
    const app = express();
    const http_server = http.createServer(app);
    const io = socket_io(http_server);

    const username = config.GetString("WebClientUsername");
    const password = config.GetString("WebClientPassword");
    if (username !== "NULL" && password !== "NULL") {
        mainLog.info("Requiring authentication to web client");
        const basicAuth = require('basic-auth-connect');
        app.use(basicAuth((u, p) => u === username && p === password));
    }

    app.use(compression());
    app.use(express.static(path.join(__dirname, "admin")));
    
    const webport = config.GetNumber("WebClientListenPort");
    http_server.listen(webport, () => mainLog.info('Listening to admins on *:', webport));
    
    const getExchange = (): Models.Exchange => {
        const ex = config.GetString("EXCHANGE").toLowerCase();
        switch (ex) {
            case "hitbtc": return Models.Exchange.HitBtc;
            case "coinbase": return Models.Exchange.Coinbase;
            case "okcoin": return Models.Exchange.OkCoin;
            case "null": return Models.Exchange.Null;
            case "bitfinex": return Models.Exchange.Bitfinex;
            default: throw new Error("unknown configuration env variable EXCHANGE " + ex);
        }
    };
    
    const exchange = getExchange();
    
    const getExch = (orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway> => {
        switch (exchange) {
            case Models.Exchange.HitBtc: return HitBtc.createHitBtc(config, pair);
            case Models.Exchange.Coinbase: return Coinbase.createCoinbase(config, orderCache, timeProvider, pair);
            case Models.Exchange.OkCoin: return OkCoin.createOkCoin(config, pair);
            case Models.Exchange.Null: return NullGw.createNullGateway(config, pair);
            case Models.Exchange.Bitfinex: return Bitfinex.createBitfinex(timeProvider, config, pair);
            default: throw new Error("no gateway provided for exchange " + exchange);
        }
    };
    
    const getPublisher = <T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T> => {
        const socketIoPublisher = new Messaging.Publisher<T>(topic, io, null, messagingLog.info.bind(messagingLog));
        if (persister)
            return new Web.StandaloneHttpPublisher<T>(socketIoPublisher, topic, app, persister);
        else
            return socketIoPublisher;
    };
    
    const getReceiver = <T>(topic: string) : Messaging.IReceive<T> => 
        new Messaging.Receiver<T>(topic, io, messagingLog.info.bind(messagingLog));
    
    const db = Persister.loadDb(config);
    
    const getPersister = async <T extends Persister.Persistable>(collectionName: string) : Promise<Persister.ILoadAll<T>> => {
        const coll = (await (await db).collection(collectionName));
        return new Persister.Persister<T>(timeProvider, coll, collectionName, exchange, pair);
    };
        
    const getRepository = async <T extends Persister.Persistable>(defValue: T, collectionName: string) : Promise<Persister.ILoadLatest<T>> => 
        new Persister.RepositoryPersister<T>(await (await db).collection(collectionName), defValue, collectionName, exchange, pair);

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
    getExch(orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway>;
    getReceiver<T>(topic: string) : Messaging.IReceive<T>;
    getPersister<T extends Persister.Persistable>(collectionName: string) : Promise<Persister.ILoadAll<T>>;
    getRepository<T>(defValue: T, collectionName: string) : Promise<Persister.ILoadLatest<T>>;
    getPublisher<T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T>;
}

const runTradingSystem = async (classes: SimulationClasses) : Promise<void> => {
    const getPersister = classes.getPersister;
    const orderPersister = await getPersister<Models.OrderStatusReport>("osr");
    const tradesPersister = await getPersister<Models.Trade>("trades");
    const fairValuePersister = await getPersister<Models.FairValue>("fv");
    const mktTradePersister = await getPersister<Models.MarketTrade>("mt");
    const positionPersister = await getPersister<Models.PositionReport>("pos");
    const messagesPersister = await getPersister<Models.Message>("msg");
    const rfvPersister = await getPersister<Models.RegularFairValue>("rfv");
    const tbpPersister = await getPersister<Models.TargetBasePositionValue>("tbp");
    const tsvPersister = await getPersister<Models.TradeSafety>("tsv");
    const marketDataPersister = await getPersister<Models.Market>(Messaging.Topics.MarketData);
    
    const activePersister = await classes.getRepository<Models.SerializedQuotesActive>(classes.startingActive, Messaging.Topics.ActiveChange);
    const paramsPersister = await classes.getRepository<Models.QuotingParameters>(classes.startingParameters, Messaging.Topics.QuotingParametersChange);
    
    const exchange = classes.exchange;

    const shouldPublishAllOrders = !config.Has("ShowAllOrders") || config.GetBoolean("ShowAllOrders");
    const ordersFilter = shouldPublishAllOrders ? {} : {source: {$gte: Models.OrderSource.OrderTicket}};

    const [
        initOrders, initTrades, initMktTrades, initMsgs, initParams, initActive, initRfv] = await Promise.all([
        orderPersister.loadAll(10000, ordersFilter),
        tradesPersister.loadAll(10000),
        mktTradePersister.loadAll(100),
        messagesPersister.loadAll(50),
        paramsPersister.loadLatest(),
        activePersister.loadLatest(),
        rfvPersister.loadAll(50)
    ])
            
    _.defaults(initParams, defaultQuotingParameters);
    _.defaults(initActive, defaultActive);

    const orderCache = new Broker.OrderStateCache();
    const timeProvider = classes.timeProvider;
    const getPublisher = classes.getPublisher;

    const gateway = await classes.getExch(orderCache);        
    
    const advert = new Models.ProductAdvertisement(exchange, pair, config.GetString("TRIBECA_MODE"), gateway.base.minTickIncrement);
    getPublisher(Messaging.Topics.ProductAdvertisement).registerSnapshot(() => [advert]).publish(advert);
    
    const quotePublisher = getPublisher(Messaging.Topics.Quote);
    const fvPublisher = getPublisher(Messaging.Topics.FairValue, fairValuePersister);
    const marketDataPublisher = getPublisher(Messaging.Topics.MarketData, marketDataPersister);
    const orderStatusPublisher = getPublisher(Messaging.Topics.OrderStatusReports, orderPersister);
    const tradePublisher = getPublisher(Messaging.Topics.Trades, tradesPersister);
    const activePublisher = getPublisher(Messaging.Topics.ActiveChange);
    const quotingParametersPublisher = getPublisher(Messaging.Topics.QuotingParametersChange);
    const marketTradePublisher = getPublisher(Messaging.Topics.MarketTrade, mktTradePersister);
    const messagesPublisher = getPublisher(Messaging.Topics.Message, messagesPersister);
    const quoteStatusPublisher = getPublisher(Messaging.Topics.QuoteStatus);
    const targetBasePositionPublisher = getPublisher(Messaging.Topics.TargetBasePosition, tbpPersister);
    const tradeSafetyPublisher = getPublisher(Messaging.Topics.TradeSafetyValue, tsvPersister);
    const positionPublisher = getPublisher(Messaging.Topics.Position, positionPersister);
    const connectivity = getPublisher(Messaging.Topics.ExchangeConnectivity);
    
    const messages = new Messages.MessagesPubisher(timeProvider, messagesPersister, initMsgs, messagesPublisher);
    messages.publish("start up");

    const getReceiver = classes.getReceiver;
    const activeReceiver = getReceiver<boolean>(Messaging.Topics.ActiveChange);
    const quotingParametersReceiver = getReceiver<Models.QuotingParameters>(Messaging.Topics.QuotingParametersChange);
    const submitOrderReceiver = getReceiver<Models.OrderRequestFromUI>(Messaging.Topics.SubmitNewOrder);
    const cancelOrderReceiver = getReceiver<Models.OrderStatusReport>(Messaging.Topics.CancelOrder);
    const cancelAllOrdersReceiver = getReceiver(Messaging.Topics.CancelAllOrders);
            
    const broker = new Broker.ExchangeBroker(pair, gateway.md, gateway.base, gateway.oe, connectivity);
    mainLog.info({
        exchange: broker.exchange, 
        pair: broker.pair.toString(), 
        minTick: broker.minTickIncrement, 
        makeFee: broker.makeFee,
        takeFee: broker.takeFee,
        hasSelfTradePrevention: broker.hasSelfTradePrevention,
    }, "using the following exchange details");

    const orderBroker = new Broker.OrderBroker(timeProvider, broker, gateway.oe, orderPersister, tradesPersister, orderStatusPublisher,
        tradePublisher, submitOrderReceiver, cancelOrderReceiver, cancelAllOrdersReceiver, messages, orderCache, initOrders, initTrades, shouldPublishAllOrders);
    const marketDataBroker = new Broker.MarketDataBroker(timeProvider, gateway.md, marketDataPublisher, marketDataPersister, messages);
    const positionBroker = new Broker.PositionBroker(timeProvider, broker, gateway.pg, positionPublisher, positionPersister, marketDataBroker);

    const paramsRepo = new QuotingParameters.QuotingParametersRepository(quotingParametersPublisher, quotingParametersReceiver, initParams);
    paramsRepo.NewParameters.on(() => paramsPersister.persist(paramsRepo.latest));

    const safetyCalculator = new Safety.SafetyCalculator(timeProvider, paramsRepo, orderBroker, paramsRepo, tradeSafetyPublisher, tsvPersister);

    const startQuoting = (moment(timeProvider.utcNow()).diff(moment(initActive.time), 'minutes') < 3 && initActive.active);
    const active = new Active.ActiveRepository(startQuoting, broker, activePublisher, activeReceiver);

    const quoter = new Quoter.Quoter(orderBroker, broker);
    const filtration = new MarketFiltration.MarketFiltration(broker, new Utils.ImmediateActionScheduler(timeProvider), quoter, marketDataBroker);
    const fvEngine = new FairValue.FairValueEngine(broker, timeProvider, filtration, paramsRepo, fvPublisher, fairValuePersister);
    const ewma = new Statistics.ObservableEWMACalculator(timeProvider, fvEngine, initParams.quotingEwma);

    const rfvValues = _.map(initRfv, (r: Models.RegularFairValue) => r.value);
    const shortEwma = new Statistics.EwmaStatisticCalculator(initParams.shortEwma);
    shortEwma.initialize(rfvValues);
    const longEwma = new Statistics.EwmaStatisticCalculator(initParams.longEwma);
    longEwma.initialize(rfvValues);
    
    const registry = new QuotingStyleRegistry.QuotingStyleRegistry([
        new MidMarket.MidMarketQuoteStyle(),
        new TopJoin.InverseJoinQuoteStyle(),
        new TopJoin.InverseTopOfTheMarketQuoteStyle(),
        new TopJoin.JoinQuoteStyle(),
        new TopJoin.TopOfTheMarketQuoteStyle(),
        new TopJoin.PingPongQuoteStyle(),
        new Depth.DepthQuoteStyle()
    ]);

    const positionMgr = new PositionManagement.PositionManager(broker, timeProvider, rfvPersister, fvEngine, initRfv, shortEwma, longEwma);
    const tbp = new PositionManagement.TargetBasePositionManager(timeProvider, positionMgr, paramsRepo, positionBroker, targetBasePositionPublisher, tbpPersister);
    const quotingEngine = new QuotingEngine.QuotingEngine(registry, timeProvider, filtration, fvEngine, paramsRepo, quotePublisher,
        orderBroker, positionBroker, broker, ewma, tbp, safetyCalculator);
    const quoteSender = new QuoteSender.QuoteSender(timeProvider, quotingEngine, quoteStatusPublisher, quoter, active, positionBroker, fvEngine, marketDataBroker, broker);

    const marketTradeBroker = new MarketTrades.MarketTradeBroker(gateway.md, marketTradePublisher, marketDataBroker,
        quotingEngine, broker, mktTradePersister, initMktTrades);
        
    if (config.inBacktestMode) {
        const t = Utils.date();
        console.log("starting backtest");
        try {
            (<Backtest.BacktestExchange>gateway).run();
        }
        catch (err) {
            console.error("exception while running backtest!", err.message, err.stack);
            throw err;
        }
        
        const results = [paramsRepo.latest, positionBroker.latestReport, {
            trades: orderBroker._trades.map(t => [t.time.valueOf(), t.price, t.quantity, t.side]),
            volume: orderBroker._trades.reduce((p, c) => p + c.quantity, 0)
        }];
        console.log("sending back results, took: ", moment(Utils.date()).diff(t, "seconds"));
        
        request({url: serverUrl+"/result", 
                    method: 'POST', 
                    json: results}, (err, resp, body) => { });
    }
    
    exitingEvent = () => {
        const a = new Models.SerializedQuotesActive(active.savedQuotingMode, timeProvider.utcNow());
        mainLog.info("persisting active to", a.active);
        activePersister.persist(a);

        return orderBroker.cancelOpenOrders();
    };

    // event looped blocked timer
    let start = process.hrtime();
    const interval = 100;
    setInterval(() => {
        const delta = process.hrtime(start);
        const ms = (delta[0] * 1e9 + delta[1]) / 1e6;
        const n = ms - interval;
        if (n > 25)
            mainLog.info(`Event looped blocked for ${Utils.roundUp(n, .001)}ms`);
        start = process.hrtime();
    }, interval).unref();
};

const harness = async () : Promise<any> => {
    if (config.inBacktestMode) {
        console.log("enter backtest mode");
        
        const getFromBacktestServer = (ep: string) : Promise<any> => {
            return new Promise((resolve, reject) => {
                request.get(serverUrl+"/"+ep, (err, resp, body) => { 
                    if (err) reject(err);
                    else resolve(body);
                });
            });
        };

        const input = await getFromBacktestServer("inputData").then(body => {
            const inp : Array<Models.Market | Models.MarketTrade> = (typeof body ==="string") ? eval(body) : body;
            
            for (let i = 0; i < inp.length; i++) {
                const d = inp[i];
                d.time = new Date(d.time);
            }
            
            return inp;
        });

        const nextParameters = () : Promise<Backtest.BacktestParameters> => getFromBacktestServer("nextParameters").then(body => {
            const p = (typeof body ==="string") ? <string|Backtest.BacktestParameters>JSON.parse(body) : body;
            console.log("Recv'd parameters", util.inspect(p));
            return (typeof p === "string") ? null : p;
        });

        while (true) {
            const next = await nextParameters();
            if (!next) break;
            runTradingSystem(backTestSimulationSetup(input, next));
        }
    }
    else {
        return runTradingSystem(liveTradingSetup());
    }
};

harness();
