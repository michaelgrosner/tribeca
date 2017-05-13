import './promises';
require('events').EventEmitter.prototype._maxListeners = 30;
import path = require("path");
import express = require('express');
import util = require('util');
import moment = require("moment");
import fs = require("fs");
import request = require('request');
import http = require("http");
import https = require('https');
import socket_io = require('socket.io');
import marked = require('marked');

import HitBtc = require("./gateways/hitbtc");
import Coinbase = require("./gateways/coinbase");
import NullGw = require("./gateways/nullgw");
import OkCoin = require("./gateways/okcoin");
import Bitfinex = require("./gateways/bitfinex");
import Utils = require("./utils");
import Config = require("./config");
import Broker = require("./broker");
import Monitor = require("./monitor");
import QuoteSender = require("./quote-sender");
import MarketTrades = require("./markettrades");
import Publish = require("./publish");
import Models = require("../share/models");
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
import Promises = require("./promises");
import log from "./logging";

let defaultQuotingParameters: Models.QuotingParameters = <Models.QuotingParameters>{
  widthPing:                      2,
  widthPong:                      2,
  bestWidth:                      true,
  buySize:                        0.02,
  buySizePercentage:              7,
  buySizeMax:                     false,
  sellSize:                       0.01,
  sellSizePercentage:             7,
  sellSizeMax:                    false,
  pingAt:                         Models.PingAt.BothSides,
  pongAt:                         Models.PongAt.ShortPingFair,
  mode:                           Models.QuotingMode.AK47,
  fvModel:                        Models.FairValueModel.BBO,
  targetBasePosition:             1,
  targetBasePositionPercentage:   50,
  positionDivergence:             0.9,
  positionDivergencePercentage:   21,
  percentageValues:               false,
  autoPositionMode:               Models.AutoPositionMode.EwmaBasic,
  aggressivePositionRebalancing:  Models.APR.Off,
  superTrades:                    Models.SOP.Off,
  tradesPerMinute:                0.9,
  tradeRateSeconds:               69,
  ewmaProtection:                 true,
  audio:                          false,
  bullets:                        2,
  range:                          0.5,
  longEwma:                       0.095,
  shortEwma:                      2*0.095,
  quotingEwma:                    0.095,
  aprMultiplier:                  2,
  sopWidthMultiplier:             2,
  cancelOrdersAuto:               false,
  stepOverSize:                   0.1,
  delayUI:                        7
};

let exitingEvent: () => Promise<number> = () => new Promise(() => 0);

const performExit = () => {
  Promises.timeout(2000, exitingEvent()).then(completed => {
    log('main').info("All exiting event handlers have fired, exiting application.");
    process.exit();
  }).catch(() => {
    log('main').warn("Did not complete clean-up tasks successfully, still shutting down.");
    process.exit(1);
  });
};

process.on("uncaughtException", err => {
  log("main").error(err, "Unhandled exception!");
  performExit();
});

process.on("unhandledRejection", (reason, p) => {
  log("main").error(reason, "Unhandled promise rejection!", p);
  performExit();
});

process.on("exit", (code) => {
  log("main").info("Exiting with code", code);
});

process.on("SIGINT", () => {
  log("main").info("Handling SIGINT");
  performExit();
});

const backTestSimulationSetup = (
  inputData: (Models.Market | Models.MarketTrade)[],
  parameters: Backtest.BacktestParameters
) => {
    const timeProvider: Utils.ITimeProvider = new Backtest.BacktestTimeProvider(moment(inputData.slice(0,1).pop().time), moment(inputData.slice(-1).pop().time));

    const getExchange = async (orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway> => new Backtest.BacktestExchange(
      new Backtest.BacktestGateway(
        inputData,
        parameters.startingBasePosition,
        parameters.startingQuotePosition,
        <Backtest.BacktestTimeProvider>timeProvider
      )
    );

    const getPublisher = <T>(topic: string, monitor?: Monitor.ApplicationState, persister?: Persister.ILoadAll<T>): Publish.IPublish<T> => {
        return new Publish.NullPublisher<T>();
    };

    const getReceiver = <T>(topic: string): Publish.IReceive<T> => new Publish.NullReceiver<T>();

    const getPersister = <T>(collectionName: string): Promise<Persister.ILoadAll<T>> => new Promise((cb) => cb(new Backtest.BacktestPersister<T>()));

    const getRepository = <T>(defValue: T, collectionName: string): Persister.ILoadLatest<T> => new Backtest.BacktestPersister<T>([defValue]);

    return {
        config: null,
        pair: null,
        exchange: Models.Exchange.Null,
        startingActive: true,
        startingParameters: parameters.quotingParameters,
        timeProvider: timeProvider,
        getExchange: getExchange,
        getReceiver: getReceiver,
        getPersister: getPersister,
        getRepository: getRepository,
        getPublisher: getPublisher
    };
};

const liveTradingSetup = (config: Config.ConfigProvider) => {
    const timeProvider: Utils.ITimeProvider = new Utils.RealTimeProvider();

    const app = express();

    let web_server;
    try {
      web_server = https.createServer({
        key: fs.readFileSync('./etc/sslcert/server.key', 'utf8'),
        cert: fs.readFileSync('./etc/sslcert/server.crt', 'utf8')
      }, app);
    } catch (e) {
      web_server = http.createServer(app)
    }

    const io = socket_io(web_server);

    const username = config.GetString("WebClientUsername");
    const password = config.GetString("WebClientPassword");
    if (username !== "NULL" && password !== "NULL") {
        log("main").info("Requiring authentication to web client");
        const basicAuth = require('basic-auth-connect');
        app.use(basicAuth((u, p) => u === username && p === password));
    }

    app.use(compression());
    app.use(express.static(path.join(__dirname, "..", "pub")));

    const webport = config.GetNumber("WebClientListenPort");
    web_server.listen(webport, () => log("main").info('Listening to admins on *:', webport));

    app.get("/view/*", (req: express.Request, res: express.Response) => {
      try {
        res.send(marked(fs.readFileSync('./'+req.path.replace('/view/','').replace('/','').replace('..',''), 'utf8')));
      } catch (e) {
        res.send('Document Not Found, but today is a beautiful day.');
      }
    });

    let pair = ((raw: string): Models.CurrencyPair => {
      const split = raw.split("/");
      if (split.length !== 2) throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR");
      return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
    })(config.GetString("TradedPair"));

    const exchange = ((): Models.Exchange => {
      let ex: string = config.GetString("EXCHANGE").toLowerCase();
      switch (ex) {
        case "hitbtc": return Models.Exchange.HitBtc;
        case "coinbase": return Models.Exchange.Coinbase;
        case "okcoin": return Models.Exchange.OkCoin;
        case "null": return Models.Exchange.Null;
        case "bitfinex": return Models.Exchange.Bitfinex;
        default: throw new Error("unknown configuration env variable EXCHANGE " + ex);
      }
    })();

    const getExchange = (orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway> => {
      switch (exchange) {
        case Models.Exchange.HitBtc: return HitBtc.createHitBtc(config, pair);
        case Models.Exchange.Coinbase: return Coinbase.createCoinbase(config, orderCache, timeProvider, pair);
        case Models.Exchange.OkCoin: return OkCoin.createOkCoin(config, pair);
        case Models.Exchange.Null: return NullGw.createNullGateway(config, pair);
        case Models.Exchange.Bitfinex: return Bitfinex.createBitfinex(timeProvider, config, pair);
        default: throw new Error("no gateway provided for exchange " + exchange);
      }
    };

    const getPublisher = <T>(topic: string, monitor?: Monitor.ApplicationState, persister?: Persister.ILoadAll<T>): Publish.IPublish<T> => {
      if (monitor && !monitor.io) monitor.io = io;
      const socketIoPublisher = new Publish.Publisher<T>(topic, io, monitor, null);
      if (persister) return new Web.StandaloneHttpPublisher<T>(socketIoPublisher, topic, app, persister);
      else return socketIoPublisher;
    };

    const getReceiver = <T>(topic: string): Publish.IReceive<T> => new Publish.Receiver<T>(topic, io);

    const db = Persister.loadDb(config)

    const getPersister = async <T extends Persister.Persistable>(collectionName: string) : Promise<Persister.ILoadAll<T>> => {
        const coll = (await (await db).collection(collectionName));
        return new Persister.Persister<T>(timeProvider, coll, collectionName, exchange, pair);
    };

    const getRepository = <T extends Persister.Persistable>(defValue: T, collectionName: string): Persister.ILoadLatest<T> =>
        new Persister.RepositoryPersister<T>(db, defValue, collectionName, exchange, pair);

    return {
        config: config,
        pair: pair,
        exchange: exchange,
        startingActive: config.GetString("TRIBECA_MODE").indexOf('auto')>-1,
        startingParameters: defaultQuotingParameters,
        timeProvider: timeProvider,
        getExchange: getExchange,
        getReceiver: getReceiver,
        getPersister: getPersister,
        getRepository: getRepository,
        getPublisher: getPublisher
    };
};

interface TradingSystem {
    config: Config.ConfigProvider;
    pair: Models.CurrencyPair;
    exchange: Models.Exchange;
    startingActive: boolean;
    startingParameters: Models.QuotingParameters;
    timeProvider: Utils.ITimeProvider;
    getExchange(orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway>;
    getReceiver<T>(topic: string): Publish.IReceive<T>;
    getPersister<T extends Persister.Persistable>(collectionName: string): Promise<Persister.ILoadAll<T>>;
    getRepository<T extends Persister.Persistable>(defValue: T, collectionName: string): Persister.ILoadLatest<T>;
    getPublisher<T>(topic: string, monitor?: Monitor.ApplicationState, persister?: Persister.ILoadAll<T>): Publish.IPublish<T>;
}

var runTradingSystem = async (system: TradingSystem) : Promise<void> => {
    const tradesPersister = await system.getPersister<Models.Trade>("trades");

    const paramsPersister = system.getRepository<Models.QuotingParameters>(system.startingParameters, Models.Topics.QuotingParametersChange);

    const [initParams, initTrades] = await Promise.all([
      paramsPersister.loadLatest(),
      tradesPersister.loadAll(10000)
    ]);
    const orderCache = new Broker.OrderStateCache();
    const gateway = await system.getExchange(orderCache);

    system.getPublisher(Models.Topics.ProductAdvertisement)
      .registerSnapshot(() => [new Models.ProductAdvertisement(
        system.exchange,
        system.pair,
        system.config.GetString("TRIBECA_MODE").replace('auto',''),
        system.config.GetString("MatryoshkaUrl"),
        gateway.base.minTickIncrement
      )]);

    const paramsRepo = new QuotingParameters.QuotingParametersRepository(
      system.getPublisher(Models.Topics.QuotingParametersChange),
      system.getReceiver(Models.Topics.QuotingParametersChange),
      initParams,
      paramsPersister
    );

    const monitor = new Monitor.ApplicationState(
      system.timeProvider,
      paramsRepo,
      system.getPublisher(Models.Topics.ApplicationState),
      system.getPublisher(Models.Topics.Notepad),
      system.getReceiver(Models.Topics.Notepad),
      system.getPublisher(Models.Topics.ToggleConfigs),
      system.getReceiver(Models.Topics.ToggleConfigs),
      system.getRepository(0, 'dataSize')
    );

    const broker = new Broker.ExchangeBroker(
      system.pair,
      gateway.md,
      gateway.base,
      gateway.oe,
      system.getPublisher(Models.Topics.ExchangeConnectivity)
    );

    log("main").info({
        exchange: broker.exchange,
        pair: broker.pair.toString(),
        minTick: broker.minTickIncrement,
        makeFee: broker.makeFee,
        takeFee: broker.takeFee,
        hasSelfTradePrevention: broker.hasSelfTradePrevention,
    }, "using the following exchange details");

    const orderBroker = new Broker.OrderBroker(
      system.timeProvider,
      paramsRepo,
      broker,
      gateway.oe,
      tradesPersister,
      system.getPublisher(Models.Topics.OrderStatusReports, monitor),
      system.getPublisher(Models.Topics.Trades, null, tradesPersister),
      system.getPublisher(Models.Topics.TradesChart),
      system.getReceiver(Models.Topics.SubmitNewOrder),
      system.getReceiver(Models.Topics.CancelOrder),
      system.getReceiver(Models.Topics.CancelAllOrders),
      system.getReceiver(Models.Topics.CleanAllClosedOrders),
      system.getReceiver(Models.Topics.CleanAllOrders),
      orderCache,
      initTrades
    );

    const marketDataBroker = new Broker.MarketDataBroker(
      gateway.md,
      system.getPublisher(Models.Topics.MarketData, monitor)
    );

    const quoter = new Quoter.Quoter(paramsRepo, orderBroker, broker);
    const filtration = new MarketFiltration.MarketFiltration(broker, quoter, marketDataBroker);
    const fvEngine = new FairValue.FairValueEngine(
      broker,
      system.timeProvider,
      filtration,
      paramsRepo,
      system.getPublisher(Models.Topics.FairValue, monitor)
    );

    const positionBroker = new Broker.PositionBroker(
      system.timeProvider,
      broker,
      orderBroker,
      quoter,
      fvEngine,
      gateway.pg,
      system.getPublisher(Models.Topics.Position, monitor)
    );

    const quotingEngine = new QuotingEngine.QuotingEngine(
      system.timeProvider,
      filtration,
      fvEngine,
      paramsRepo,
      orderBroker,
      positionBroker,
      broker,
      new Statistics.ObservableEWMACalculator(
        system.timeProvider,
        fvEngine,
        initParams.quotingEwma
      ),
      new PositionManagement.TargetBasePositionManager(
        system.timeProvider,
        new PositionManagement.PositionManager(
          broker,
          system.timeProvider,
          fvEngine,
          new Statistics.EwmaStatisticCalculator(initParams.shortEwma, null),
          new Statistics.EwmaStatisticCalculator(initParams.longEwma, null),
          system.getPublisher(Models.Topics.EWMAChart, monitor)
        ),
        paramsRepo,
        positionBroker,
        system.getPublisher(Models.Topics.TargetBasePosition, monitor)
      ),
      new Safety.SafetyCalculator(
        system.timeProvider,
        fvEngine,
        paramsRepo,
        positionBroker,
        orderBroker,
        system.getPublisher(Models.Topics.TradeSafetyValue, monitor)
      )
    );

    new QuoteSender.QuoteSender(
      system.timeProvider,
      paramsRepo,
      quotingEngine,
      system.getPublisher(Models.Topics.QuoteStatus, monitor),
      quoter,
      broker,
      new Active.ActiveRepository(
        system.startingActive,
        broker,
        system.getPublisher(Models.Topics.ActiveChange),
        system.getReceiver(Models.Topics.ActiveChange)
      )
    );

    new MarketTrades.MarketTradeBroker(
      gateway.md,
      system.getPublisher(Models.Topics.MarketTrade),
      marketDataBroker,
      quotingEngine,
      broker
    );

    if (system.config.inBacktestMode) {
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

      request({url: ('BACKTEST_SERVER_URL' in process.env ? process.env['BACKTEST_SERVER_URL'] : "http://localhost:5001")+"/result",
         method: 'POST',
         json: results}, (err, resp, body) => { });
    }

    exitingEvent = () => {
      return orderBroker.cancelOpenOrders();
    };

    let start = process.hrtime();
    const interval = 500;
    setInterval(() => {
      const delta = process.hrtime(start);
      const ms = (delta[0] * 1e9 + delta[1]) / 1e6;
      const n = ms - interval;
      if (n > 121)
        log("main").info("Event loop delay " + Utils.roundNearest(n, 100) + "ms");
      start = process.hrtime();
    }, interval).unref();
};

(async (): Promise<any> => {
  const config = new Config.ConfigProvider();
  if (!config.inBacktestMode) return runTradingSystem(liveTradingSetup(config));

  console.log("enter backtest mode");

  const getFromBacktestServer = (ep: string) : Promise<any> => {
    return new Promise((resolve, reject) => {
      request.get(('BACKTEST_SERVER_URL' in process.env ? process.env['BACKTEST_SERVER_URL'] : "http://localhost:5001")+"/"+ep, (err, resp, body) => {
        if (err) reject(err);
        else resolve(body);
      });
    });
  };

  const input = await getFromBacktestServer("inputData").then(body => {
    const inp: Array<Models.Market | Models.MarketTrade> = (typeof body ==="string") ? eval(body) : body;

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
})();
