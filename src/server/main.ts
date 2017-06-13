import './promises';
require('events').EventEmitter.prototype._maxListeners = 30;
var packageConfig = require("./../../package.json");
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
import Korbit = require("./gateways/korbit");
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
import QuotingParameters = require("./quoting-parameters");
import MarketFiltration = require("./market-filtration");
import PositionManagement = require("./position-management");
import Statistics = require("./statistics");
import Backtest = require("./backtest");
import QuotingEngine = require("./quoting-engine");
import Promises = require("./promises");

let defaultQuotingParameters: Models.QuotingParameters = <Models.QuotingParameters>{
  widthPing:                      2,
  widthPingPercentage:            0.25,
  widthPong:                      2,
  widthPongPercentage:            0.25,
  widthPercentage:                false,
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
  bullets:                        2,
  range:                          0.5,
  fvModel:                        Models.FairValueModel.BBO,
  targetBasePosition:             1,
  targetBasePositionPercentage:   50,
  positionDivergence:             0.9,
  positionDivergencePercentage:   21,
  percentageValues:               false,
  autoPositionMode:               Models.AutoPositionMode.EWMA_LS,
  aggressivePositionRebalancing:  Models.APR.Off,
  superTrades:                    Models.SOP.Off,
  tradesPerMinute:                0.9,
  tradeRateSeconds:               69,
  quotingEwmaProtection:          true,
  quotingEwmaProtectionPeridos:   200,
  quotingStdevProtection:         Models.STDEV.Off,
  quotingStdevProtectionFactor:   1,
  quotingStdevProtectionPeriods:  1200,
  ewmaSensiblityPercentage:       0.5,
  longEwmaPeridos:                200,
  mediumEwmaPeridos:              100,
  shortEwmaPeridos:               50,
  aprMultiplier:                  2,
  sopWidthMultiplier:             2,
  cancelOrdersAuto:               false,
  cleanPongsAuto:                 0,
  profitHourInterval:             0.5,
  audio:                          false,
  delayUI:                        7
};

let exitingEvent: () => Promise<number> = () => new Promise(() => 0);

const performExit = () => {
  Promises.timeout(2000, exitingEvent()).then(completed => {
    console.info(new Date().toISOString().slice(11, -1), 'main', 'All exiting event handlers have fired, exiting application.');
    process.exit();
  }).catch(() => {
    console.warn(new Date().toISOString().slice(11, -1), 'main', 'Did not complete clean-up tasks successfully, still shutting down.');
    process.exit(1);
  });
};

process.on("uncaughtException", err => {
  console.error(new Date().toISOString().slice(11, -1), 'main', 'Unhandled exception!', err);
  performExit();
});

process.on("unhandledRejection", (reason, p) => {
  console.error(new Date().toISOString().slice(11, -1), 'main', 'Unhandled promise rejection!', reason, p);
  performExit();
});

process.on("exit", (code) => {
  console.info(new Date().toISOString().slice(11, -1), 'main', 'Exiting with code', code);
});

process.on("SIGINT", () => {
  console.info(new Date().toISOString().slice(11, -1), 'main', 'Handling SIGINT');
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

    const getPublisher = <T>(topic: string, monitor?: Monitor.ApplicationState): Publish.IPublish<T> => {
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

const liveTradingSetup = () => {
    const timeProvider: Utils.ITimeProvider = new Utils.RealTimeProvider();

    const app = express();

    let web_server;
    try {
      web_server = https.createServer({
        key: fs.readFileSync('./dist/sslcert/server.key', 'utf8'),
        cert: fs.readFileSync('./dist/sslcert/server.crt', 'utf8')
      }, app);
    } catch (e) {
      web_server = http.createServer(app)
    }

    const io = socket_io(web_server);

    const config = new Config.ConfigProvider();

    const username = config.GetString("WebClientUsername");
    const password = config.GetString("WebClientPassword");
    if (username !== "NULL" && password !== "NULL") {
        console.info(new Date().toISOString().slice(11, -1), 'main', 'Requiring authentication to web client');
        const basicAuth = require('basic-auth-connect');
        app.use(basicAuth((u, p) => u === username && p === password));
    }

    app.use(compression());
    app.use(express.static(path.join(__dirname, "..", "pub")));

    const webport = parseFloat(config.GetString("WebClientListenPort"));
    web_server.listen(webport, () => console.info(new Date().toISOString().slice(11, -1), 'main', 'Listening to admins on *:', webport));

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
        case "bitfinex": return Models.Exchange.Bitfinex;
        case "korbit": return Models.Exchange.Korbit;
        case "null": return Models.Exchange.Null;
        default: throw new Error("unknown configuration env variable EXCHANGE " + ex);
      }
    })();

    const getExchange = (orderCache: Broker.OrderStateCache): Promise<Interfaces.CombinedGateway> => {
      switch (exchange) {
        case Models.Exchange.HitBtc: return HitBtc.createHitBtc(config, pair);
        case Models.Exchange.Coinbase: return Coinbase.createCoinbase(config, orderCache, timeProvider, pair);
        case Models.Exchange.OkCoin: return OkCoin.createOkCoin(config, pair);
        case Models.Exchange.Bitfinex: return Bitfinex.createBitfinex(timeProvider, config, pair);
        case Models.Exchange.Korbit: return Korbit.createKorbit(config, pair);
        case Models.Exchange.Null: return NullGw.createNullGateway(config, pair);
        default: throw new Error("no gateway provided for exchange " + exchange);
      }
    };

    const getPublisher = <T>(topic: string, monitor?: Monitor.ApplicationState): Publish.IPublish<T> => {
      if (monitor && !monitor.io) monitor.io = io;
      return new Publish.Publisher<T>(topic, io, monitor, null);
    };

    const getReceiver = <T>(topic: string): Publish.IReceive<T> => new Publish.Receiver<T>(topic, io);

    const db = Persister.loadDb(config)

    const getPersister = async <T extends Persister.Persistable>(collectionName: string) : Promise<Persister.ILoadAll<T>> => {
        const coll = (await (await db).collection(collectionName));
        return new Persister.Persister<T>(timeProvider, coll, collectionName, exchange, pair);
    };

    const getRepository = <T extends Persister.Persistable>(defValue: T, collectionName: string): Persister.ILoadLatest<T> =>
        new Persister.RepositoryPersister<T>(db, defValue, collectionName, exchange, pair);

    for (const param in defaultQuotingParameters)
      if (config.GetDefaultString(param) !== null)
        defaultQuotingParameters[param] = config.GetDefaultString(param);

    return {
        config: config,
        pair: pair,
        exchange: exchange,
        startingActive: config.GetString("BotIdentifier").indexOf('auto')>-1,
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
    getPublisher<T>(topic: string, monitor?: Monitor.ApplicationState): Publish.IPublish<T>;
}

var runTradingSystem = async (system: TradingSystem) : Promise<void> => {
    const tradesPersister = await system.getPersister<Models.Trade>("trades");
    const rfvPersister = await system.getPersister<Models.RegularFairValue>("rfv");
    const marketDataPersister = await system.getPersister<Models.MarketStats>("mkt");

    const paramsPersister = system.getRepository<Models.QuotingParameters>(system.startingParameters, Models.Topics.QuotingParametersChange);

    const [initParams, initTrades, initRfv, initMkt] = await Promise.all([
      paramsPersister.loadLatest(),
      tradesPersister.loadAll(10000),
      rfvPersister.loadAll(10000),
      marketDataPersister.loadAll(10000)
    ]);
    const orderCache = new Broker.OrderStateCache();
    const gateway = await system.getExchange(orderCache);

    system.getPublisher(Models.Topics.ProductAdvertisement)
      .registerSnapshot(() => [new Models.ProductAdvertisement(
        system.exchange,
        system.pair,
        system.config.GetString("BotIdentifier").replace('auto',''),
        system.config.GetString("MatryoshkaUrl"),
        packageConfig.homepage,
        gateway.base.minTickIncrement
      )]);

    const paramsRepo = new QuotingParameters.QuotingParametersRepository(
      paramsPersister,
      system.getPublisher(Models.Topics.QuotingParametersChange),
      system.getReceiver(Models.Topics.QuotingParametersChange),
      initParams
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

    console.info(new Date().toISOString().slice(11, -1), 'main', 'Exchange details' ,{
        exchange: Models.Exchange[broker.exchange()],
        pair: broker.pair.toString(),
        minTick: broker.minTickIncrement,
        minSize: broker.minSize,
        makeFee: broker.makeFee(),
        takeFee: broker.takeFee(),
        hasSelfTradePrevention: broker.hasSelfTradePrevention,
    });

    const orderBroker = new Broker.OrderBroker(
      system.timeProvider,
      paramsRepo,
      broker,
      gateway.oe,
      tradesPersister,
      system.getPublisher(Models.Topics.OrderStatusReports, monitor),
      system.getPublisher(Models.Topics.Trades),
      system.getPublisher(Models.Topics.TradesChart),
      system.getReceiver(Models.Topics.SubmitNewOrder),
      system.getReceiver(Models.Topics.CancelOrder),
      system.getReceiver(Models.Topics.CancelAllOrders),
      system.getReceiver(Models.Topics.CleanAllClosedOrders),
      system.getReceiver(Models.Topics.CleanAllOrders),
      system.getReceiver(Models.Topics.CleanTrade),
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
      paramsRepo,
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
        paramsRepo
      ),
      new Statistics.ObservableSTDEVCalculator(
        system.timeProvider,
        fvEngine,
        filtration,
        broker.minTickIncrement,
        paramsRepo,
        marketDataPersister,
        initMkt
      ),
      new PositionManagement.TargetBasePositionManager(
        system.timeProvider,
        new PositionManagement.PositionManager(
          broker,
          system.timeProvider,
          paramsRepo,
          rfvPersister,
          fvEngine,
          new Statistics.EwmaStatisticCalculator(paramsRepo, 'shortEwmaPeridos', initRfv),
          new Statistics.EwmaStatisticCalculator(paramsRepo, 'mediumEwmaPeridos', initRfv),
          new Statistics.EwmaStatisticCalculator(paramsRepo, 'longEwmaPeridos', initRfv),
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

    if (process.env["BACKTEST_MODE"]) {
      const t = new Date();
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
      console.log("sending back results, took: ", moment(new Date()).diff(t, "seconds"));

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
        console.info(new Date().toISOString().slice(11, -1), 'main', 'Event loop delay', Utils.roundNearest(n, 100) + 'ms');
      start = process.hrtime();
    }, interval).unref();
};

(async (): Promise<any> => {
  if (!process.env["BACKTEST_MODE"])
    return runTradingSystem(liveTradingSetup());

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
