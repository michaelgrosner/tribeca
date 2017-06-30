const packageConfig = require("./../../package.json");
const bindings = ((K) => { try {
  return require('./lib/'+K.join('.'));
} catch (e) {
  if (process.version.substring(1).split('.').map((n) => parseInt(n))[0] < 6)
    throw new Error('Error: K requires Node.js v6.0.0 or greater.');
  else throw new Error('Error: compilation of K is obsolete (maybe because Node.js was upgraded), please run "npm install" again to recompile also K.');
}})([packageConfig.name[0], process.platform, process.versions.modules]);

require('events').EventEmitter.prototype._maxListeners = 30;
import path = require("path");
import express = require('express');
import request = require('request');
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
import Safety = require("./safety");
import compression = require("compression");
import FairValue = require("./fair-value");
import QuotingParameters = require("./quoting-parameters");
import MarketFiltration = require("./market-filtration");
import PositionManagement = require("./position-management");
import Statistics = require("./statistics");
import QuotingEngine = require("./quoting-engine");

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
  quotingStdevBollingerBands:     false,
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

let happyEnding = () => { console.info(new Date().toISOString().slice(11, -1), 'main', 'Error', 'THE END IS NEVER '.repeat(21)+'THE END'); };

const processExit = () => {
  happyEnding();
  setTimeout(process.exit, 2000);
};

process.on("uncaughtException", err => {
  console.error(new Date().toISOString().slice(11, -1), 'main', 'Unhandled exception!', err);
  processExit();
});

process.on("unhandledRejection", (reason, p) => {
  console.error(new Date().toISOString().slice(11, -1), 'main', 'Unhandled rejection!', reason, p);
  processExit();
});

process.on("SIGINT", () => {
  process.stdout.write("\n"+new Date().toISOString().slice(11, -1)+' main Excellent decision! ');
  request({url: 'https://api.icndb.com/jokes/random?escape=javascript&limitTo=[nerdy]',json: true,timeout:3000}, (err, resp, body) => {
    if (!err && resp.statusCode === 200) process.stdout.write(body.value.joke);
    process.stdout.write("\n");
    processExit();
  });
});

process.on("exit", (code) => {
  console.info(new Date().toISOString().slice(11, -1), 'main', 'Exit code', code);
});

const timeProvider: Utils.ITimeProvider = new Utils.RealTimeProvider();

const config = new Config.ConfigProvider();

const app = express();

const io = socket_io(((() => { try {
  return https.createServer({
    key: fs.readFileSync('./dist/sslcert/server.key', 'utf8'),
    cert: fs.readFileSync('./dist/sslcert/server.crt', 'utf8')
  }, app);
} catch (e) {
  return http.createServer(app);
}})()).listen(
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
    default: throw new Error("Invalid configuration value EXCHANGE: " + ex);
  }
})(config.GetString("EXCHANGE").toLowerCase());

for (const param in defaultQuotingParameters)
  if (config.GetDefaultString(param) !== null)
    defaultQuotingParameters[param] = config.GetDefaultString(param);

const sqlite = new bindings.SQLite(exchange, pair.base, pair.quote);

const initParams = Object.assign(defaultQuotingParameters, sqlite.load(Models.Topics.QuotingParametersChange)[0] || {});
const initTrades = sqlite.load(Models.Topics.Trades).map(x => Object.assign(x, {time: new Date(x.time)}));
const initRfv = sqlite.load(Models.Topics.FairValue).map(x => Object.assign(x, {time: new Date(x.time)}));
const initMkt = sqlite.load(Models.Topics.MarketData).map(x => Object.assign(x, {time: new Date(x.time)}));
const initTBP = sqlite.load(Models.Topics.TargetBasePosition).map(x => Object.assign(x, {time: new Date(x.time)}))[0];

const receiver = new Publish.Receiver(io);
const publisher = new Publish.Publisher(io);

(async (): Promise<void> => {
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

  publisher
    .registerSnapshot(Models.Topics.ProductAdvertisement, () => [new Models.ProductAdvertisement(
      exchange,
      pair,
      config.GetString("BotIdentifier").replace('auto',''),
      config.GetString("MatryoshkaUrl"),
      packageConfig.homepage,
      gateway.base.minTickIncrement
    )]);

  const paramsRepo = new QuotingParameters.QuotingParametersRepository(
    sqlite,
    publisher,
    receiver,
    initParams
  );

  publisher.monitor = new Monitor.ApplicationState(
    '/data/db/K.'+exchange+'.'+pair.base+'.'+pair.quote+'.db',
    timeProvider,
    paramsRepo,
    publisher,
    receiver,
    io
  );

  const broker = new Broker.ExchangeBroker(
    pair,
    gateway.md,
    gateway.base,
    gateway.oe,
    publisher,
    receiver,
    config.GetString("BotIdentifier").indexOf('auto')>-1
  );

  const orderBroker = new Broker.OrderBroker(
    timeProvider,
    paramsRepo,
    broker,
    gateway.oe,
    sqlite,
    publisher,
    receiver,
    initTrades
  );

  const marketBroker = new Broker.MarketDataBroker(
    gateway.md,
    publisher
  );

  const fvEngine = new FairValue.FairValueEngine(
    new MarketFiltration.MarketFiltration(
      broker.minTickIncrement,
      orderBroker,
      marketBroker
    ),
    broker.minTickIncrement,
    timeProvider,
    paramsRepo,
    publisher,
    initRfv
  );

  const positionBroker = new Broker.PositionBroker(
    timeProvider,
    paramsRepo,
    broker,
    orderBroker,
    fvEngine,
    gateway.pg,
    publisher
  );

  const quotingEngine = new QuotingEngine.QuotingEngine(
    timeProvider,
    fvEngine,
    paramsRepo,
    orderBroker,
    positionBroker,
    broker.minTickIncrement,
    broker.minSize,
    new Statistics.EWMAProtectionCalculator(timeProvider, fvEngine, paramsRepo),
    new Statistics.STDEVProtectionCalculator(
      timeProvider,
      fvEngine,
      paramsRepo,
      sqlite,
      bindings.computeStdevs,
      initMkt
    ),
    new PositionManagement.TargetBasePositionManager(
      timeProvider,
      broker.minTickIncrement,
      sqlite,
      fvEngine,
      new Statistics.EWMATargetPositionCalculator(paramsRepo, initRfv),
      paramsRepo,
      positionBroker,
      publisher,
      initTBP
    ),
    new Safety.SafetyCalculator(
      timeProvider,
      fvEngine,
      paramsRepo,
      positionBroker,
      orderBroker,
      publisher
    )
  );

  new QuoteSender.QuoteSender(
    timeProvider,
    quotingEngine,
    broker,
    orderBroker,
    paramsRepo,
    publisher
  );

  new MarketTrades.MarketTradeBroker(
    gateway.md,
    publisher,
    marketBroker,
    quotingEngine,
    broker
  );

  happyEnding = () => {
    orderBroker.cancelOpenOrders();
    console.info(new Date().toISOString().slice(11, -1), 'main', 'Attempting to cancel all open orders, please wait..');
  };

  let highTime = process.hrtime();
  setInterval(() => {
    const diff = process.hrtime(highTime);
    const n = ((diff[0] * 1e9 + diff[1]) / 1e6) - 500;
    if (n > 121) console.info(new Date().toISOString().slice(11, -1), 'main', 'Event loop delay', Utils.roundNearest(n, 100) + 'ms');
    highTime = process.hrtime();
  }, 500).unref();
})();
