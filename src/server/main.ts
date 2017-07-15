const packageConfig = require("./../../package.json");

const noop = () => {};
const bindings = ((K) => { try {
  console.log(K.join('.'));
  return require('./lib/'+K.join('.'));
} catch (e) {
  if (process.version.substring(1).split('.').map((n) => parseInt(n))[0] < 7)
    throw new Error('K requires Node.js v7.0.0 or greater.');
  else throw new Error(e);
}})([packageConfig.name[0], process.platform, process.versions.modules]);
bindings.uiLoop(noop);

require('events').EventEmitter.prototype._maxListeners = 30;
import request = require('request');

import NullGw = require("./gateways/nullgw");
import Coinbase = require("./gateways/coinbase");
import OkCoin = require("./gateways/okcoin");
import Bitfinex = require("./gateways/bitfinex");
import Poloniex = require("./gateways/poloniex");
import Korbit = require("./gateways/korbit");
import HitBtc = require("./gateways/hitbtc");
import Utils = require("./utils");
import Broker = require("./broker");
import QuoteSender = require("./quote-sender");
import MarketTrades = require("./markettrades");
import Publish = require("./publish");
import Models = require("../share/models");
import Interfaces = require("./interfaces");
import Safety = require("./safety");
import FairValue = require("./fair-value");
import MarketFiltration = require("./market-filtration");
import PositionManagement = require("./position-management");
import Statistics = require("./statistics");
import QuotingEngine = require("./quoting-engine");

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

const pair = ((raw: string): Models.CurrencyPair => {
  const split = raw.split("/");
  if (split.length !== 2) throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR");
  return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
})(bindings.cfString("TradedPairx"));

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
})(bindings.cfString("EXCHANGE").toLowerCase());

const sqlite = new bindings.DB(exchange, pair.base, pair.quote);

const initTrades = sqlite.load(Models.Topics.Trades).map(x => Object.assign(x, {time: new Date(x.time)}));
const initRfv = sqlite.load(Models.Topics.FairValue).map(x => Object.assign(x, {time: new Date(x.time)}));
const initMkt = sqlite.load(Models.Topics.MarketData).map(x => Object.assign(x, {time: new Date(x.time)}));
const initTBP = sqlite.load(Models.Topics.TargetBasePosition).map(x => Object.assign(x, {time: new Date(x.time)}))[0];

const publisher = new Publish.Publisher(
  bindings.dbSize,
  new bindings.UI(
    bindings.cfString("WebClientListenPort"),
    bindings.cfString("WebClientUsername"),
    bindings.cfString("WebClientPassword")
  ),
  bindings.evOn,
  initParams.delayUI,
  bindings.o60,
  bindings.uiSnap,
  bindings.uiHand,
  bindings.uiSend
);

(async (): Promise<void> => {
  const gateway = await ((): Promise<Interfaces.CombinedGateway> => {
    switch (exchange) {
      case Models.Exchange.Coinbase: return Coinbase.createCoinbase(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      case Models.Exchange.OkCoin: return OkCoin.createOkCoin(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      case Models.Exchange.Bitfinex: return Bitfinex.createBitfinex(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      case Models.Exchange.Poloniex: return Poloniex.createPoloniex(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      case Models.Exchange.Korbit: return Korbit.createKorbit(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      case Models.Exchange.HitBtc: return HitBtc.createHitBtc(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      case Models.Exchange.Null: return NullGw.createNullGateway(bindings.cfString, pair, bindings.evOn, bindings.evUp);
      default: throw new Error("no gateway provided for exchange " + exchange);
    }
  })();

  publisher
    .registerSnapshot(Models.Topics.ProductAdvertisement, () => [new Models.ProductAdvertisement(
      exchange,
      pair,
      bindings.cfString("BotIdentifier").replace('auto',''),
      bindings.cfString("MatryoshkaUrl"),
      packageConfig.homepage,
      gateway.base.minTickIncrement
    )]);

  const broker = new Broker.ExchangeBroker(
    pair,
    gateway.base,
    publisher,
    bindings.evOn,
    bindings.evUp,
    bindings.cfString("BotIdentifier").indexOf('auto')>-1
  );

  const orderBroker = new Broker.OrderBroker(
    timeProvider,
    bindings.qpRepo,
    broker,
    gateway.oe,
    sqlite,
    publisher,
    bindings.evOn,
    bindings.evUp,
    initTrades
  );

  const marketBroker = new Broker.MarketDataBroker(
    publisher,
    bindings.evOn,
    bindings.evUp
  );

  const fvEngine = new FairValue.FairValueEngine(
    new MarketFiltration.MarketFiltration(
      broker.minTickIncrement,
      orderBroker,
      marketBroker,
      bindings.evOn,
      bindings.evUp
    ),
    broker.minTickIncrement,
    timeProvider,
    bindings.qpRepo,
    publisher,
    bindings.evOn,
    bindings.evUp,
    initRfv
  );

  const positionBroker = new Broker.PositionBroker(
    timeProvider,
    bindings.qpRepo,
    broker,
    orderBroker,
    fvEngine,
    publisher,
    bindings.evOn,
    bindings.evUp
  );

  const quotingEngine = new QuotingEngine.QuotingEngine(
    timeProvider,
    fvEngine,
    bindings.qpRepo,
    positionBroker,
    broker.minTickIncrement,
    broker.minSize,
    new Statistics.EWMAProtectionCalculator(
      timeProvider,
      fvEngine,
      bindings.qpRepo,
      bindings.evUp
    ),
    new Statistics.STDEVProtectionCalculator(
      timeProvider,
      fvEngine,
      bindings.qpRepo,
      sqlite,
      bindings.computeStdevs,
      initMkt
    ),
    new PositionManagement.TargetBasePositionManager(
      timeProvider,
      broker.minTickIncrement,
      sqlite,
      fvEngine,
      new Statistics.EWMATargetPositionCalculator(bindings.qpRepo, initRfv),
      bindings.qpRepo,
      positionBroker,
      publisher,
      bindings.evOn,
      bindings.evUp,
      initTBP
    ),
    new Safety.SafetyCalculator(
      timeProvider,
      fvEngine,
      bindings.qpRepo,
      positionBroker,
      orderBroker,
      publisher,
      bindings.evOn,
      bindings.evUp
    ),
    bindings.evOn,
    bindings.evUp
  );

  new QuoteSender.QuoteSender(
    timeProvider,
    quotingEngine,
    broker,
    orderBroker,
    bindings.qpRepo,
    publisher,
    bindings.evOn
  );

  new MarketTrades.MarketTradeBroker(
    publisher,
    marketBroker,
    quotingEngine,
    broker,
    bindings.evOn,
    bindings.evUp
  );

  happyEnding = () => {
    orderBroker.cancelOpenOrders();
    console.info(new Date().toISOString().slice(11, -1), 'main', 'Attempting to cancel all open orders, please wait..');
  };

  let highTime = process.hrtime();
  setInterval(() => {
    const diff = process.hrtime(highTime);
    const n = ((diff[0] * 1e9 + diff[1]) / 1e6) - 500;
    if (n > 242) console.info(new Date().toISOString().slice(11, -1), 'main', 'Event loop delay', Utils.roundNearest(n, 100) + 'ms');
    highTime = process.hrtime();
  }, 500).unref();
})();
