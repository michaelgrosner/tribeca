import _ = require("lodash");
import Q = require("q");
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
// var heapdump = require('heapdump'); // kill -USR2

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

let defaultQuotingParameters: Models.QuotingParameters = new Models.QuotingParameters(
  2,                                  /* width */
  0.02,                               /* buySize */
  0.01,                               /* sellSize */
  Models.PingAt.BothSides,            /* pingAt */
  Models.PongAt.ShortPingFair,        /* pongAt */
  Models.QuotingMode.AK47,            /* mode */
  Models.FairValueModel.BBO,          /* fvModel */
  1,                                  /* targetBasePosition */
  0.9,                                /* positionDivergence */
  true,                               /* ewmaProtection */
  Models.AutoPositionMode.EwmaBasic,  /* autoPositionMode */
  false,                              /* aggressivePositionRebalancing */
  0.9,                                /* tradesPerMinute */
  569,                                /* tradeRateSeconds */
  false,                              /* audio */
  2,                                  /* bullets */
  0.5,                                /* range */
  .095,                               /* longEwma */
  2*.095,                             /* shortEwma */
  .095,                               /* quotingEwma */
  3,                                  /* aprMultiplier */
  .1                                  /* stepOverSize */
);

let serverUrl = 'BACKTEST_SERVER_URL' in process.env
  ? process.env['BACKTEST_SERVER_URL']
  : "http://localhost:5001";

let config = new Config.ConfigProvider();

let exitingEvent : () => Q.Promise<boolean>;

const performExit = () => {
  Q.timeout(exitingEvent(), 2000).then(completed => {
    if (completed) Utils.log("main").info("All exiting event handlers have fired, exiting application.");
    else Utils.log("main").warn("Did not complete clean-up tasks successfully, still shutting down.");
    process.exit();
  }).catch(err => {
    Utils.log("main").error(err, "Error while exiting application.");
    process.exit(1);
  });
};

process.on("uncaughtException", err => {
  Utils.log("main").error(err, "Unhandled exception!");
  performExit();
});

process.on("unhandledRejection", (reason, p) => {
  Utils.log("main").error(reason, "Unhandled promise rejection!", p);
  performExit();
});

process.on("exit", (code) => {
  Utils.log("main").info("Exiting with code", code);
});

process.on("SIGINT", () => {
  Utils.log("main").info("Handling SIGINT");
  performExit();
});

let defaultActive: Models.SerializedQuotesActive = new Models.SerializedQuotesActive(
  config.GetString("TRIBECA_MODE").indexOf('auto')>-1,
  moment.utc()
);

let pair = ((raw: string): Models.CurrencyPair => {
  let split = raw.split("/");
  if (split.length !== 2) throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR");
  return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
})(config.GetString("TradedPair"));

var backTestSimulationSetup = (
  inputData: Array<Models.Market | Models.MarketTrade>,
  parameters: Backtest.BacktestParameters
) => {
    var timeProvider : Utils.ITimeProvider = new Backtest.BacktestTimeProvider(_.first(inputData).time, _.last(inputData).time);
    var exchange = Models.Exchange.Null;
    var gw = new Backtest.BacktestGateway(inputData, parameters.startingBasePosition, parameters.startingQuotePosition, <Backtest.BacktestTimeProvider>timeProvider);

    var getExchange = (orderCache: Broker.OrderStateCache): Interfaces.CombinedGateway => new Backtest.BacktestExchange(gw);

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
        getExchange: getExchange,
        getReceiver: getReceiver,
        getPersister: getPersister,
        getRepository: getRepository,
        getPublisher: getPublisher
    };
};

var liveTradingSetup = () => {
    var timeProvider : Utils.ITimeProvider = new Utils.RealTimeProvider();

    var app = express();

    var web_server;
    try {
      web_server = https.createServer({
        key: fs.readFileSync('./etc/sslcert/server.key', 'utf8'),
        cert: fs.readFileSync('./etc/sslcert/server.crt', 'utf8')
      }, app);
    } catch (e) {
      web_server = http.createServer(app)
    }

    var io = socket_io(web_server);

    var username = config.GetString("WebClientUsername");
    var password = config.GetString("WebClientPassword");
    if (username !== "NULL" && password !== "NULL") {
        Utils.log("main").info("Requiring authentication to web client");
        var basicAuth = require('basic-auth-connect');
        app.use(basicAuth((u, p) => u === username && p === password));
    }

    app.use(compression());
    app.use(express.static(path.join(__dirname, "admin")));

    var webport = config.GetNumber("WebClientListenPort");
    web_server.listen(webport, () => Utils.log("main").info('Listening to admins on *:', webport));

    app.get("/view/*", (req: express.Request, res: express.Response) => {
      try {
        res.send(marked(fs.readFileSync('./'+req.path.replace('/view/','').replace('/','').replace('..',''), 'utf8')));
      } catch (e) {
        res.send('Document Not Found, but today is a beautiful day.');
      }
    });

    var exchange = ((): Models.Exchange => {
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

    var getExchange = (orderCache: Broker.OrderStateCache): Interfaces.CombinedGateway => {
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
        var socketIoPublisher = new Messaging.Publisher<T>(topic, io, null);
        if (persister)
            return new Web.StandaloneHttpPublisher<T>(socketIoPublisher, topic, app, persister);
        else
            return socketIoPublisher;
    };

    var getReceiver = <T>(topic: string) : Messaging.IReceive<T> =>
        new Messaging.Receiver<T>(topic, io);

    var db = Persister.loadDb(config);

    var loaderSaver = new Persister.LoaderSaver(exchange, pair);

    var getPersister = <T>(collectionName: string) : Persister.ILoadAll<T> => {
        var ls = collectionName === "mt" ? new MarketTrades.MarketTradesLoaderSaver(loaderSaver) : loaderSaver;
        var setDBFlag = (collectionName === "trades");
        return new Persister.Persister<T>(db, collectionName, exchange, pair, setDBFlag, ls.loader, ls.saver);
    };

    var getRepository = <T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T> =>
        new Persister.RepositoryPersister<T>(db, defValue, collectionName, exchange, pair, loaderSaver.loader, loaderSaver.saver);

    return {
        exchange: exchange,
        startingActive: defaultActive,
        startingParameters: defaultQuotingParameters,
        timeProvider: timeProvider,
        getExchange: getExchange,
        getReceiver: getReceiver,
        getPersister: getPersister,
        getRepository: getRepository,
        getPublisher: getPublisher
    };
};

interface SystemClasses {
    exchange: Models.Exchange;
    startingActive : Models.SerializedQuotesActive;
    startingParameters : Models.QuotingParameters;
    timeProvider: Utils.ITimeProvider;
    getExchange(orderCache: Broker.OrderStateCache): Interfaces.CombinedGateway;
    getReceiver<T>(topic: string) : Messaging.IReceive<T>;
    getPersister<T>(collectionName: string) : Persister.ILoadAll<T>;
    getRepository<T>(defValue: T, collectionName: string) : Persister.ILoadLatest<T>;
    getPublisher<T>(topic: string, persister?: Persister.ILoadAll<T>): Messaging.IPublish<T>;
}

var runTradingSystem = (classes: SystemClasses) : Q.Promise<boolean> => {
    var getPersister = classes.getPersister;
    var orderPersister = getPersister("osr");
    var tradesPersister = getPersister("trades");
    var fairValuePersister = getPersister("fv");
    var mktTradePersister = getPersister("mt");
    var positionPersister = getPersister("pos");
    var rfvPersister = getPersister("rfv");
    var tbpPersister = getPersister("tbp");
    var tsvPersister = getPersister("tsv");
    var marketDataPersister = getPersister(Messaging.Topics.MarketData);

    var paramsPersister = classes.getRepository(classes.startingParameters, Messaging.Topics.QuotingParametersChange);

    var completedSuccessfully = Q.defer<boolean>();

    Q.all<any>([
        orderPersister.loadAll(25000),
        tradesPersister.loadAll(10000),
        mktTradePersister.loadAll(100),
        paramsPersister.loadLatest(),
        defaultActive,
        rfvPersister.loadAll(50)
    ]).spread((initOrders: Models.OrderStatusReport[],
        initTrades: Models.Trade[],
        initMktTrades: Models.MarketTrade[],
        initParams: Models.QuotingParameters,
        initActive: Models.SerializedQuotesActive,
        initRfv: Models.RegularFairValue[]) => {

        _.defaults(initParams, defaultQuotingParameters);

        var orderCache = new Broker.OrderStateCache();
        var getPublisher = classes.getPublisher;

        var advert = new Models.ProductAdvertisement(
          classes.exchange,
          pair,
          config.GetString("TRIBECA_MODE").replace('auto','')
        );
        getPublisher(Messaging.Topics.ProductAdvertisement)
          .registerSnapshot(() => [advert]).publish(advert);

        var getReceiver = classes.getReceiver;

        new Monitor.ApplicationState(
          classes.timeProvider,
          getPublisher(Messaging.Topics.ApplicationState),
          getPublisher(Messaging.Topics.Notepad),
          getReceiver(Messaging.Topics.Notepad),
          getPublisher(Messaging.Topics.ToggleConfigs),
          getReceiver(Messaging.Topics.ToggleConfigs)
        );

        var gateway = classes.getExchange(orderCache);

        if (!_.some(gateway.base.supportedCurrencyPairs, p => p.base === pair.base && p.quote === pair.quote))
            throw new Error("Unsupported currency pair! Please open issue in github or check that gateway " + gateway.base.name() + " really supports the specified currencies defined in TradedPair configuration option.");

        var paramsRepo = new QuotingParameters.QuotingParametersRepository(
          getPublisher(Messaging.Topics.QuotingParametersChange),
          getReceiver(Messaging.Topics.QuotingParametersChange),
          initParams,
          paramsPersister
        );

        var broker = new Broker.ExchangeBroker(
          pair,
          gateway.md,
          gateway.base,
          gateway.oe,
          getPublisher(Messaging.Topics.ExchangeConnectivity)
        );

        var orderBroker = new Broker.OrderBroker(
          classes.timeProvider,
          paramsRepo,
          broker,
          gateway.oe,
          orderPersister,
          tradesPersister,
          getPublisher(Messaging.Topics.OrderStatusReports, orderPersister),
          getPublisher(Messaging.Topics.Trades, tradesPersister),
          getReceiver(Messaging.Topics.SubmitNewOrder),
          getReceiver(Messaging.Topics.CancelOrder),
          getReceiver(Messaging.Topics.CancelAllOrders),
          getReceiver(Messaging.Topics.CleanAllClosedOrders),
          getReceiver(Messaging.Topics.CleanAllOrders),
          orderCache,
          initOrders,
          initTrades
        );

        var marketDataBroker = new Broker.MarketDataBroker(
          gateway.md,
          getPublisher(Messaging.Topics.MarketData, marketDataPersister),
          marketDataPersister
        );

        var positionBroker = new Broker.PositionBroker(
          classes.timeProvider,
          broker,
          orderBroker,
          gateway.pg,
          getPublisher(Messaging.Topics.Position, positionPersister),
          positionPersister,
          marketDataBroker
        );

        var active = new Active.ActiveRepository(
          initActive.active,
          broker,
          getPublisher(Messaging.Topics.ActiveChange),
          getReceiver(Messaging.Topics.ActiveChange)
        );

        var quoter = new Quoter.Quoter(paramsRepo, orderBroker, broker);
        var filtration = new MarketFiltration.MarketFiltration(quoter, marketDataBroker);
        var fvEngine = new FairValue.FairValueEngine(
          classes.timeProvider,
          filtration,
          paramsRepo,
          getPublisher(Messaging.Topics.FairValue, fairValuePersister),
          fairValuePersister
        );

        var safetyCalculator = new Safety.SafetyCalculator(
          classes.timeProvider,
          fvEngine,
          paramsRepo,
          orderBroker,
          paramsRepo,
          getPublisher(Messaging.Topics.TradeSafetyValue, tsvPersister),
          tsvPersister
        );

        var ewma = new Statistics.ObservableEWMACalculator(
          classes.timeProvider,
          fvEngine,
          initParams.quotingEwma
        );

        var rfvValues = _.map(initRfv, (r: Models.RegularFairValue) => r.value);
        var shortEwma = new Statistics.EwmaStatisticCalculator(initParams.shortEwma);
        shortEwma.initialize(rfvValues);
        var longEwma = new Statistics.EwmaStatisticCalculator(initParams.longEwma);
        longEwma.initialize(rfvValues);

        var positionMgr = new PositionManagement.PositionManager(
          classes.timeProvider,
          rfvPersister,
          fvEngine,
          initRfv,
          shortEwma,
          longEwma
        );

        var tbp = new PositionManagement.TargetBasePositionManager(
          classes.timeProvider,
          positionMgr,
          paramsRepo,
          positionBroker,
          getPublisher(Messaging.Topics.TargetBasePosition, tbpPersister),
          tbpPersister
        );

        var quotingEngine = new QuotingEngine.QuotingEngine(
          classes.timeProvider,
          filtration,
          fvEngine,
          paramsRepo,
          orderBroker,
          positionBroker,
          ewma,
          tbp,
          safetyCalculator
        );

        new QuoteSender.QuoteSender(
          classes.timeProvider,
          paramsRepo,
          quotingEngine,
          getPublisher(Messaging.Topics.QuoteStatus),
          quoter,
          active,
          positionBroker,
          fvEngine,
          marketDataBroker,
          broker
        );

        new MarketTrades.MarketTradeBroker(
          gateway.md,
          getPublisher(Messaging.Topics.MarketTrade, mktTradePersister),
          marketDataBroker,
          quotingEngine,
          broker,
          mktTradePersister,
          initMktTrades
        );

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
            orderBroker.cancelOpenOrders().then(n_cancelled => {
                Utils.log("main").info("Cancelled all", n_cancelled, "open orders");
                completedSuccessfully.resolve(true);
            }).done();

            classes.timeProvider.setTimeout(() => {
                if (completedSuccessfully.promise.isFulfilled) return;
                Utils.log("main").error("Could not cancel all open orders!");
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
            if (n > 121)
                Utils.log("main").info("Event loop delay " + Utils.roundFloat(n) + "ms");
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
      var inp: Array<Models.Market | Models.MarketTrade> = (typeof body ==="string") ? eval(body) : body;

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
  else return runTradingSystem(liveTradingSetup());
};

harness().done();
