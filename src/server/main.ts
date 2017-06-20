require('events').EventEmitter.prototype._maxListeners = 30;
var packageConfig = require("./../../package.json");
import path = require("path");
import express = require('express');
import fs = require("fs");
import http = require("http");
import https = require('https');
import socket_io = require('socket.io');
import marked = require('marked');

import NullGw = require("./gateways/nullgw");
import Coinbase = require("./gateways/coinbase");
import OkCoin = require("./gateways/okcoin");
import Bitfinex = require("./gateways/bitfinex");
import Poloniex = require("./gateways/poloniex");
import Korbit = require("./gateways/korbit");
import HitBtc = require("./gateways/hitbtc");
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

const config = new Config.ConfigProvider();

for (const param in defaultQuotingParameters)
  if (config.GetDefaultString(param) !== null)
    defaultQuotingParameters[param] = config.GetDefaultString(param);

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

const app = express();

const io = socket_io(((() => {
  try {
    return https.createServer({
      key: fs.readFileSync('./dist/sslcert/server.key', 'utf8'),
      cert: fs.readFileSync('./dist/sslcert/server.crt', 'utf8')
    }, app);
  } catch (e) {
    return http.createServer(app);
  }
})()).listen(
  parseFloat(config.GetString("WebClientListenPort")),
  () => console.info(new Date().toISOString().slice(11, -1), 'main', 'Listening to admins on port', parseFloat(config.GetString("WebClientListenPort")))
));

if (config.GetString("WebClientUsername") !== "NULL" && config.GetString("WebClientPassword") !== "NULL") {
  console.info(new Date().toISOString().slice(11, -1), 'main', 'Requiring authentication to web client');
  app.use(require('basic-auth-connect')((u, p) => u === config.GetString("WebClientUsername") && p === config.GetString("WebClientPassword")));
}

app.use(compression());
app.use(express.static(path.join(__dirname, "..", "pub")));

app.get("/view/*", (req: express.Request, res: express.Response) => {
  try {
    res.send(marked(fs.readFileSync('./'+req.path.replace('/view/','').replace('/','').replace('..',''), 'utf8')));
  } catch (e) {
    res.send('Document Not Found, but today is a beautiful day.');
  }
});

const timeProvider: Utils.ITimeProvider = new Utils.RealTimeProvider();

const pair = ((raw: string): Models.CurrencyPair => {
  const split = raw.split("/");
  if (split.length !== 2) throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR");
  return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
})(config.GetString("TradedPair"));

const exchange = ((ex: string): Models.Exchange => {
  switch (ex) {
    case "coinbase": return Models.Exchange.Coinbase;
    case "okcoin": return Models.Exchange.OkCoin;
    case "bitfinex": return Models.Exchange.Bitfinex;
    case "poloniex": return Models.Exchange.Poloniex;
    case "korbit": return Models.Exchange.Korbit;
    case "hitbtc": return Models.Exchange.HitBtc;
    case "null": return Models.Exchange.Null;
    default: throw new Error("unknown configuration env variable EXCHANGE " + ex);
  }
})(config.GetString("EXCHANGE").toLowerCase());


(async (): Promise<void> => {
  const persister = new Persister.Repository(config, exchange, pair);
  persister.loadCollection('trades');
  persister.loadCollection('rfv');
  persister.loadCollection('mkt');
  persister.loadCollection(Models.Topics.QuotingParametersChange, defaultQuotingParameters);
  persister.loadCollection('dataSize', 0);

  const [initParams, initTrades, initRfv, initMkt] = await Promise.all([
    persister.loadLatest(Models.Topics.QuotingParametersChange),
    persister.loadAll('trades', 10000),
    persister.loadAll('rfv', 10000),
    persister.loadAll('mkt', 10000)
  ]);

  const gateway = await ((): Promise<Interfaces.CombinedGateway> => {
    switch (exchange) {
      case Models.Exchange.Coinbase: return Coinbase.createCoinbase(config, pair);
      case Models.Exchange.OkCoin: return OkCoin.createOkCoin(config, pair);
      case Models.Exchange.Bitfinex: return Bitfinex.createBitfinex(config, pair);
      case Models.Exchange.Poloniex: return Poloniex.createPoloniex(config, pair);
      case Models.Exchange.Korbit: return Korbit.createKorbit(config, pair);
      case Models.Exchange.HitBtc: return HitBtc.createHitBtc(config, pair);
      case Models.Exchange.Null: return NullGw.createNullGateway(config, pair);
      default: throw new Error("no gateway provided for exchange " + exchange);
    }
  })();

  new Publish.Publisher(Models.Topics.ProductAdvertisement, io)
    .registerSnapshot(() => [new Models.ProductAdvertisement(
      exchange,
      pair,
      config.GetString("BotIdentifier").replace('auto',''),
      config.GetString("MatryoshkaUrl"),
      packageConfig.homepage,
      gateway.base.minTickIncrement
    )]);

  const paramsRepo = new QuotingParameters.QuotingParametersRepository(
    persister,
    new Publish.Publisher(Models.Topics.QuotingParametersChange, io),
    new Publish.Receiver(Models.Topics.QuotingParametersChange, io),
    initParams
  );

  const monitor = new Monitor.ApplicationState(
    timeProvider,
    paramsRepo,
    new Publish.Publisher(Models.Topics.ApplicationState, io),
    new Publish.Publisher(Models.Topics.Notepad, io),
    new Publish.Receiver(Models.Topics.Notepad, io),
    new Publish.Publisher(Models.Topics.ToggleConfigs, io),
    new Publish.Receiver(Models.Topics.ToggleConfigs, io),
    persister,
    io
  );

  const broker = new Broker.ExchangeBroker(
    pair,
    gateway.md,
    gateway.base,
    gateway.oe,
    new Publish.Publisher(Models.Topics.ExchangeConnectivity, io)
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
    timeProvider,
    paramsRepo,
    broker,
    gateway.oe,
    persister,
    new Publish.Publisher(Models.Topics.OrderStatusReports, io, monitor),
    new Publish.Publisher(Models.Topics.Trades, io),
    new Publish.Publisher(Models.Topics.TradesChart, io),
    new Publish.Receiver(Models.Topics.SubmitNewOrder, io),
    new Publish.Receiver(Models.Topics.CancelOrder, io),
    new Publish.Receiver(Models.Topics.CancelAllOrders, io),
    new Publish.Receiver(Models.Topics.CleanAllClosedOrders, io),
    new Publish.Receiver(Models.Topics.CleanAllOrders, io),
    new Publish.Receiver(Models.Topics.CleanTrade, io),
    initTrades
  );

  const marketDataBroker = new Broker.MarketDataBroker(
    gateway.md,
    new Publish.Publisher(Models.Topics.MarketData, io, monitor)
  );

  const quoter = new Quoter.Quoter(paramsRepo, orderBroker, broker);
  const filtration = new MarketFiltration.MarketFiltration(broker, quoter, marketDataBroker);
  const fvEngine = new FairValue.FairValueEngine(
    broker,
    timeProvider,
    filtration,
    paramsRepo,
    new Publish.Publisher(Models.Topics.FairValue, io, monitor)
  );

  const positionBroker = new Broker.PositionBroker(
    timeProvider,
    paramsRepo,
    broker,
    orderBroker,
    quoter,
    fvEngine,
    gateway.pg,
    new Publish.Publisher(Models.Topics.Position, io, monitor)
  );

  const quotingEngine = new QuotingEngine.QuotingEngine(
    timeProvider,
    filtration,
    fvEngine,
    paramsRepo,
    orderBroker,
    positionBroker,
    broker,
    new Statistics.ObservableEWMACalculator(
      timeProvider,
      fvEngine,
      paramsRepo
    ),
    new Statistics.ObservableSTDEVCalculator(
      timeProvider,
      fvEngine,
      filtration,
      broker.minTickIncrement,
      paramsRepo,
      persister,
      initMkt
    ),
    new PositionManagement.TargetBasePositionManager(
      timeProvider,
      new PositionManagement.PositionManager(
        broker,
        timeProvider,
        paramsRepo,
        persister,
        fvEngine,
        new Statistics.EwmaStatisticCalculator(paramsRepo, initRfv),
        new Publish.Publisher(Models.Topics.EWMAChart, io, monitor)
      ),
      paramsRepo,
      positionBroker,
      new Publish.Publisher(Models.Topics.TargetBasePosition, io, monitor)
    ),
    new Safety.SafetyCalculator(
      timeProvider,
      fvEngine,
      paramsRepo,
      positionBroker,
      orderBroker,
      new Publish.Publisher(Models.Topics.TradeSafetyValue, io, monitor)
    )
  );

  new QuoteSender.QuoteSender(
    timeProvider,
    paramsRepo,
    quotingEngine,
    new Publish.Publisher(Models.Topics.QuoteStatus, io, monitor),
    quoter,
    broker,
    new Active.ActiveRepository(
      config.GetString("BotIdentifier").indexOf('auto')>-1,
      broker,
      new Publish.Publisher(Models.Topics.ActiveChange, io),
      new Publish.Receiver(Models.Topics.ActiveChange, io)
    )
  );

  new MarketTrades.MarketTradeBroker(
    gateway.md,
    new Publish.Publisher(Models.Topics.MarketTrade, io),
    marketDataBroker,
    quotingEngine,
    broker
  );

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
})();
