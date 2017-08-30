process.on("uncaughtException", err => { console.error(new Date().toISOString().slice(11, -1), 'main', 'Unhandled exception!', err); process.exit(); });
process.on("unhandledRejection", (reason, p) => { console.error(new Date().toISOString().slice(11, -1), 'main', 'Unhandled rejection!', reason, p); process.exit(); });

const packageConfig = require("./../../package.json");

const noop = () => {};
const bindings = ((K) => { try {
  console.log(K.join('.'));
  return require('./lib/'+K.join('.'));
} catch (e) {
  if (process.version.substring(1).split('.').map((n) => parseInt(n))[0] < 8)
    throw new Error('K requires Node.js v8.0.0 or greater.');
  else throw new Error(e);
}})([packageConfig.name[0], process.platform, process.versions.modules]);
bindings.uiLoop(noop);

let highTime = process.hrtime();
setInterval(() => {
  const diff = process.hrtime(highTime);
  const n = ((diff[0] * 1e9 + diff[1]) / 1e6) - 500;
  if (n > 242) console.info(new Date().toISOString().slice(11, -1), 'main', 'Event loop delay', (Math.floor(n/100)*100) + 'ms');
  highTime = process.hrtime();
}, 500).unref();
