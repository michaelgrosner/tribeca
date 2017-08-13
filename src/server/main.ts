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

import Broker = require("./broker");
import QuoteSender = require("./quote-sender");
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

const orderBroker = new Broker.OrderBroker(
  bindings.qpRepo,
  bindings.cfmCurrencyPair(),
  bindings.gwMakeFee(),
  bindings.gwTakeFee(),
  bindings.gwMinTick(),
  bindings.gwExchange(),
  {sendOrder:bindings.gwSend,cancelOrder:bindings.gwCancel,cancelsByClientOrderId:bindings.gwCancelByClientId(),generateClientOrderId:bindings.gwClientId,supportsCancelAllOpenOrders:bindings.gwSupportCancelAll,cancelAllOpenOrders:bindings.gwCancelAll},
  bindings.dbInsert,
  bindings.uiSnap,
  bindings.uiHand,
  bindings.uiSend,
  bindings.evOn,
  bindings.evUp,
  bindings.dbLoad(Models.Topics.Trades)
);

const initRfv = bindings.dbLoad(Models.Topics.EWMAChart);

const fvEngine = new FairValue.FairValueEngine(
  new MarketFiltration.MarketFiltration(
    bindings.gwMinTick(),
    orderBroker.orderCache.allOrders,
    bindings.evOn,
    bindings.evUp
  ),
  bindings.gwMinTick(),
  bindings.qpRepo,
  bindings.uiSnap,
  bindings.uiSend,
  bindings.evOn,
  bindings.evUp,
  initRfv
);

const positionBroker = new Broker.PositionBroker(
  bindings.qpRepo,
  bindings.cfmCurrencyPair(),
  bindings.gwExchange(),
  orderBroker.orderCache.allOrders,
  fvEngine,
  bindings.uiSnap,
  bindings.uiSend,
  bindings.evOn,
  bindings.evUp
);

const quotingEngine = new QuotingEngine.QuotingEngine(
  fvEngine,
  bindings.qpRepo,
  positionBroker,
  bindings.gwMinTick(),
  bindings.gwMinSize(),
  new Statistics.EWMAProtectionCalculator(
    fvEngine,
    bindings.qpRepo,
    bindings.evUp
  ),
  new Statistics.STDEVProtectionCalculator(
    fvEngine,
    bindings.qpRepo,
    bindings.dbInsert,
    bindings.computeStdevs,
    bindings.dbLoad(Models.Topics.MarketData)
  ),
  new PositionManagement.TargetBasePositionManager(
    bindings.gwMinTick(),
    bindings.dbInsert,
    fvEngine,
    new Statistics.EWMATargetPositionCalculator(bindings.qpRepo, initRfv),
    bindings.qpRepo,
    positionBroker,
    bindings.uiSnap,
    bindings.uiSend,
    bindings.evOn,
    bindings.evUp,
    bindings.dbLoad(Models.Topics.TargetBasePosition)
  ),
  new Safety.SafetyCalculator(
    fvEngine,
    bindings.qpRepo,
    positionBroker,
    orderBroker.tradesMemory,
    bindings.uiSnap,
    bindings.uiSend,
    bindings.evOn,
    bindings.evUp
  ),
  bindings.evOn,
  bindings.evUp
);

new QuoteSender.QuoteSender(
  quotingEngine,
  orderBroker.orderCache.allOrders,
  orderBroker.cancelOrder,
  orderBroker.sendOrder,
  bindings.gwMinTick(),
  bindings.qpRepo,
  bindings.uiSnap,
  bindings.uiSend,
  bindings.evOn
);

happyEnding = () => {
  orderBroker.cancelOpenOrders();
  console.info(new Date().toISOString().slice(11, -1), 'main', 'Attempting to cancel all open orders, please wait..');
};

let highTime = process.hrtime();
setInterval(() => {
  const diff = process.hrtime(highTime);
  const n = ((diff[0] * 1e9 + diff[1]) / 1e6) - 500;
  if (n > 242) console.info(new Date().toISOString().slice(11, -1), 'main', 'Event loop delay', (Math.floor(n/100)*100) + 'ms');
  highTime = process.hrtime();
}, 500).unref();
