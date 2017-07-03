const packageConfig = require("./../../package.json");
console.log('a1');

import http = require("http");

const noop = () => {};

const abortConnection = (socket, code, name) => {
    socket.end('HTTP/1.1 ' + code + ' ' + name + '\r\n\r\n');
}

const emitConnection = (ws) => {
    // this.emit('connection', ws);
}

const onServerMessage = (message, webSocket) => {
    webSocket.internalOnMessage(message);
}

const bindings = ((K) => { try {
  return require('./lib/'+K.join('.'));
} catch (e) {
  if (process.version.substring(1).split('.').map((n) => parseInt(n))[0] < 6)
    throw new Error('K requires Node.js v6.0.0 or greater.');
  else throw new Error(e+'K is obsolete (maybe because Node.js was upgraded), please run "npm install" again to recompile also K.');
}})([packageConfig.name[0], process.platform, process.versions.modules]);

bindings.setNoop(noop);

var _upgradeReq = null;

const clientGroup = bindings.client.group.create();

bindings.client.group.onConnection(clientGroup, (external) => {
    const webSocket = bindings.getUserData(external);
    webSocket.external = external;
    webSocket.internalOnOpen();
});

bindings.client.group.onMessage(clientGroup, (message, webSocket) => {
    webSocket.internalOnMessage(message);
});

bindings.client.group.onDisconnection(clientGroup, (external, code, message, webSocket) => {
    webSocket.external = null;

    process.nextTick(() => {
        webSocket.internalOnClose(code, message);
    });

    bindings.clearUserData(external);
});

bindings.client.group.onPing(clientGroup, (message, webSocket) => {
    webSocket.onping(message);
});

bindings.client.group.onPong(clientGroup, (message, webSocket) => {
    webSocket.onpong(message);
});

bindings.client.group.onError(clientGroup, (webSocket) => {
    process.nextTick(() => {
        webSocket.internalOnError({
            message: 'uWs client connection error',
            stack: 'uWs client connection error'
        });
    });
});

class WebSocket {
    public external;
    public onping;
    public onpong;
    public internalOnOpen;
    public internalOnClose;
    public internalOnError;
    public internalOnMessage;
    public OPCODE_PING;
    public OPCODE_BINARY;
    public OPCODE_TEXT;
    public PERMESSAGE_DEFLATE = 1;
    constructor(external) {
        this.internalOnMessage = noop;
        this.internalOnClose = noop;
        this.onping = noop;
        this.onpong = noop;
    }

    get upgradeReq() {
        return _upgradeReq;
    }

    set onmessage(f) {
        if (f) {
            this.internalOnMessage = (message) => {
                f({data: message});
            };
        } else {
            this.internalOnMessage = noop;
        }
    }

    set onopen(f) {
        if (f) {
            this.internalOnOpen = f;
        } else {
            this.internalOnOpen = noop;
        }
    }

    set onclose(f) {
        if (f) {
            this.internalOnClose = (code, message) => {
                f({code: code, reason: message});
            };
        } else {
            this.internalOnClose = noop;
        }
    }

    set onerror(f) {
        if (f && this instanceof WebSocketClient) {
            this.internalOnError = f;
        } else {
            this.internalOnError = noop;
        }
    }

    emit(eventName, arg1, arg2) {
        if (eventName === 'message') {
            this.internalOnMessage(arg1);
        } else if (eventName === 'close') {
            this.internalOnClose(arg1, arg2);
        } else if (eventName === 'ping') {
            this.onping(arg1);
        } else if (eventName === 'pong') {
            this.onpong(arg1);
        }
        return this;
    }

    on(eventName, f) {
        if (eventName === 'message') {
            if (this.internalOnMessage !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.internalOnMessage = f;
        } else if (eventName === 'close') {
            if (this.internalOnClose !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.internalOnClose = f;
        } else if (eventName === 'ping') {
            if (this.onping !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.onping = f;
        } else if (eventName === 'pong') {
            if (this.onpong !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.onpong = f;
        } else if (eventName === 'open') {
            if (this.internalOnOpen !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.internalOnOpen = f;
        } else if (eventName === 'error' && this instanceof WebSocketClient) {
            if (this.internalOnError !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.internalOnError = f;
        }
        return this;
    }

    once(eventName, f) {
        if (eventName === 'message') {
            if (this.internalOnMessage !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.internalOnMessage = (message) => {
                this.internalOnMessage = noop;
                f(message);
            };
        } else if (eventName === 'close') {
            if (this.internalOnClose !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.internalOnClose = (code, message) => {
                this.internalOnClose = noop;
                f(code, message);
            };
        } else if (eventName === 'ping') {
            if (this.onping !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.onping = () => {
                this.onping = noop;
                f();
            };
        } else if (eventName === 'pong') {
            if (this.onpong !== noop) {
                throw Error('Registering more than one listener to a WebSocket is not supported.');
            }
            this.onpong = () => {
                this.onpong = noop;
                f();
            };
        }
        return this;
    }

    removeAllListeners(eventName) {
        if (!eventName || eventName === 'message') {
            this.internalOnMessage = noop;
        }
        if (!eventName || eventName === 'close') {
            this.internalOnClose = noop;
        }
        if (!eventName || eventName === 'ping') {
            this.onping = noop;
        }
        if (!eventName || eventName === 'pong') {
            this.onpong = noop;
        }
        return this;
    }

    removeListener(eventName, cb) {
        if (eventName === 'message' && this.internalOnMessage === cb) {
            this.internalOnMessage = noop;
        } else if (eventName === 'close' && this.internalOnClose === cb) {
            this.internalOnClose = noop;
        } else if (eventName === 'ping' && this.onping === cb) {
            this.onping = noop;
        } else if (eventName === 'pong' && this.onpong === cb) {
            this.onpong = noop;
        }
        return this;
    }

    get OPEN() {
        return (<any>WebSocketClient).OPEN;
    }

    get CLOSED() {
        return (<any>WebSocketClient).CLOSED;
    }

    get readyState() {
        return this.external ? (<any>WebSocketClient).OPEN : (<any>WebSocketClient).CLOSED;
    }

    get _socket() {
        const address = this.external ? bindings.getAddress(this.external) : new Array(3);
        return {
            remotePort: address[0],
            remoteAddress: address[1],
            remoteFamily: address[2]
        };
    }

    // from here down, functions are not common between client and server

    ping(message, options, dontFailWhenClosed) {
        if (this.external) {
            bindings.server.send(this.external, message, (<any>WebSocketClient).OPCODE_PING);
        }
    }

    terminate() {
        if (this.external) {
            bindings.server.terminate(this.external);
            this.external = null;
        }
    }

    send(message, options, cb) {
        if (this.external) {
            if (typeof options === 'function') {
                cb = options;
                options = null;
            }

            const binary = options && options.binary || typeof message !== 'string';

            bindings.server.send(this.external, message, binary ? (<any>WebSocketClient).OPCODE_BINARY : (<any>WebSocketClient).OPCODE_TEXT, cb ? (() => {
                process.nextTick(cb);
            }) : undefined);
        } else if (cb) {
            cb(new Error('not opened'));
        }
    }

    close(code, data) {
        if (this.external) {
            bindings.server.close(this.external, code, data);
            this.external = null;
        }
    }
}

class WebSocketClient extends WebSocket {
    public PERMESSAGE_DEFLATE = 1;
    public SERVER_NO_CONTEXT_TAKEOVER = 2;
    public CLIENT_NO_CONTEXT_TAKEOVER = 4;
    public OPCODE_TEXT = 1;
    public OPCODE_BINARY = 2;
    public OPCODE_PING = 9;
    public OPEN = 1;
    public CLOSED = 0;
    public Server = Server;
    public http = bindings.httpServer;
    public bindings = bindings;
    constructor(uri) {
        super(null);
        this.internalOnOpen = noop;
        this.internalOnError = noop;
        bindings.connect(clientGroup, uri, this);
    }

    ping(message, options, dontFailWhenClosed) {
        if (this.external) {
            bindings.client.send(this.external, message, (<any>WebSocketClient).OPCODE_PING);
        }
    }

    terminate() {
        if (this.external) {
            bindings.client.terminate(this.external);
            this.external = null;
        }
    }

    send(message, options, cb) {
        if (this.external) {
            if (typeof options === 'function') {
                cb = options;
                options = null;
            }

            const binary = options && options.binary || typeof message !== 'string';

            bindings.client.send(this.external, message, binary ? (<any>WebSocketClient).OPCODE_BINARY : (<any>WebSocketClient).OPCODE_TEXT, cb ? (() => {
                process.nextTick(cb);
            }) : undefined);
        } else if (cb) {
            cb(new Error('not opened'));
        }
    }

    close(code, data) {
        if (this.external) {
            bindings.client.close(this.external, code, data);
            this.external = null;
        }
    }
}

class Server {
    public serverGroup;
    public _upgradeCallback;
    public _upgradeListener;
    public httpServer;
    public _noDelay;
    public _lastUpgradeListener;
    public _passedHttpServer;
    constructor(options) {
        if (!options) {
            throw new TypeError('missing options');
        }

        if (options.port === undefined && !options.server && !options.noServer) {
            throw new TypeError('invalid options');
        }

        var bindingsOptions = (<any>WebSocketClient).PERMESSAGE_DEFLATE;

        if (options.perMessageDeflate !== undefined) {
            if (options.perMessageDeflate === false) {
                bindingsOptions = 0;
            }
        }

        this.serverGroup = bindings.server.group.create(bindingsOptions, options.maxPayload === undefined ? 1048576 : options.maxPayload);

        // can these be made private?
        this._upgradeCallback = noop;
        this._upgradeListener = null;
        this._noDelay = options.noDelay === undefined ? true : options.noDelay;
        this._lastUpgradeListener = true;
        this._passedHttpServer = options.server;

        if (!options.noServer) {
            this.httpServer = options.server ? options.server : http.createServer((request, response) => {
                // todo: default HTTP response
                response.end();
            });

            if (options.path && (!options.path.length || options.path[0] !== '/')) {
                options.path = '/' + options.path;
            }

            this.httpServer.on('upgrade', this._upgradeListener = ((request, socket, head) => {
                if (!options.path || options.path == request.url.split('?')[0].split('#')[0]) {
                    if (options.verifyClient) {
                        const info = {
                            origin: request.headers.origin,
                            secure: request.connection.authorized !== undefined || request.connection.encrypted !== undefined,
                            req: request
                        };

                        if (options.verifyClient.length === 2) {
                            options.verifyClient(info, (result, code, name) => {
                                if (result) {
                                    this.handleUpgrade(request, socket, head, emitConnection);
                                } else {
                                    abortConnection(socket, code, name);
                                }
                            });
                        } else {
                            if (options.verifyClient(info)) {
                                this.handleUpgrade(request, socket, head, emitConnection);
                            } else {
                                abortConnection(socket, 400, 'Client verification failed');
                            }
                        }
                    } else {
                        this.handleUpgrade(request, socket, head, emitConnection);
                    }
                } else {
                    if (this._lastUpgradeListener) {
                        abortConnection(socket, 400, 'URL not supported');
                    }
                }
            }));

            this.httpServer.on('newListener', (eventName, listener) => {
                if (eventName === 'upgrade') {
                    this._lastUpgradeListener = false;
                }
            });
        }

        bindings.server.group.onDisconnection(this.serverGroup, (external, code, message, webSocket) => {
            webSocket.external = null;

            process.nextTick(() => {
                webSocket.internalOnClose(code, message);
            });

            bindings.clearUserData(external);
        });

        bindings.server.group.onMessage(this.serverGroup, onServerMessage);

        bindings.server.group.onPing(this.serverGroup, (message, webSocket) => {
            webSocket.onping(message);
        });

        bindings.server.group.onPong(this.serverGroup, (message, webSocket) => {
            webSocket.onpong(message);
        });

        bindings.server.group.onConnection(this.serverGroup, (external) => {
            const webSocket = new WebSocket(external);

            bindings.setUserData(external, webSocket);
            this._upgradeCallback(webSocket);
            _upgradeReq = null;
        });

        if (options.port !== undefined) {
            if (options.host) {
                this.httpServer.listen(options.port, options.host, () => {
                    // this.emit('listening');
                    // callback && callback();
                });
            } else {
                this.httpServer.listen(options.port, () => {
                    // this.emit('listening');
                    // callback && callback();
                });
            }
        }
    }

    handleUpgrade(request, socket, upgradeHead, callback) {
        if (socket._isNative) {
            if (this.serverGroup) {
                _upgradeReq = request;
                this._upgradeCallback = callback ? callback : noop;
                bindings.upgrade(this.serverGroup, socket.external, request.headers['sec-websocket-key'], request.headers['sec-websocket-extensions'], request.headers['sec-websocket-protocol']);
            }
        } else {
            const secKey = request.headers['sec-websocket-key'];
            const socketHandle = socket.ssl ? socket._parent._handle : socket._handle;
            const sslState = socket.ssl ? socket.ssl._external : null;
            if (socketHandle && secKey && secKey.length == 24) {
                socket.setNoDelay(this._noDelay);
                const ticket = bindings.transfer(socketHandle.fd === -1 ? socketHandle : socketHandle.fd, sslState);
                socket.on('close', (error) => {
                    if (this.serverGroup) {
                        _upgradeReq = request;
                        this._upgradeCallback = callback ? callback : noop;
                        bindings.upgrade(this.serverGroup, ticket, secKey, request.headers['sec-websocket-extensions'], request.headers['sec-websocket-protocol']);
                    }
                });
            }
            socket.destroy();
        }
    }

    broadcast(message, options) {
        if (this.serverGroup) {
            bindings.server.group.broadcast(this.serverGroup, message, options && options.binary || false);
        }
    }

    startAutoPing(interval, userMessage) {
        if (this.serverGroup) {
            bindings.server.group.startAutoPing(this.serverGroup, interval, userMessage);
        }
    }

    close() {
        if (this._upgradeListener && this.httpServer) {
            this.httpServer.removeListener('upgrade', this._upgradeListener);

            if (!this._passedHttpServer) {
                this.httpServer.close();
            }
        }

        if (this.serverGroup) {
            bindings.server.group.close(this.serverGroup);
            this.serverGroup = null;
        }
    }

    get clients() {
        if (this.serverGroup) {
            return {
                length: bindings.server.group.getSize(this.serverGroup),
                forEach: ((cb) => {bindings.server.group.forEach(this.serverGroup, cb)})
            };
        }
    }
}








console.log('a2');

require('events').EventEmitter.prototype._maxListeners = 30;
import path = require("path");
import express = require('express');
import request = require('request');
import fs = require("fs");
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
console.log('a3');
// const socket = new bindings.UI(9334);
var wss = new Server({ port: 3001 });


// wss.on('connection', (ws) => {
    // ws.send('something');
// });
console.log('a4');
console.log('a5');
// var app = socket.getExpressApp(express);

// socket.createServer(app).listen(3000);

console.log('fre');
// const app = express();

// const io = socket_io(((() => { try {
  // return https.createServer({
    // key: fs.readFileSync('./dist/sslcert/server.key', 'utf8'),
    // cert: fs.readFileSync('./dist/sslcert/server.crt', 'utf8')
  // }, app);
// } catch (e) {
  // return http.createServer(app);
// }})()).listen(
  // parseFloat(config.GetString("WebClientListenPort")),
  // () => console.info(new Date().toISOString().slice(11, -1), 'main', 'Listening to admins on port', parseFloat(config.GetString("WebClientListenPort")))
// ));

console.log('fre');
// if (config.GetString("WebClientUsername") !== "NULL" && config.GetString("WebClientPassword") !== "NULL") {
  // console.info(new Date().toISOString().slice(11, -1), 'main', 'Requiring authentication to web client');
  // app.use(require('basic-auth-connect')((u, p) => u === config.GetString("WebClientUsername") && p === config.GetString("WebClientPassword")));
// }

// app.use(compression());
// app.use(express.static(path.join(__dirname, "..", "pub")));

// app.get("/view/*", (req: express.Request, res: express.Response) => {
  // try {
    // res.send(marked(fs.readFileSync('./'+req.path.replace('/view/','').replace('/','').replace('..',''), 'utf8')));
  // } catch (e) {
    // res.send('Document Not Found, but today is a beautiful day.');
  // }
// });

// const pair = ((raw: string): Models.CurrencyPair => {
  // const split = raw.split("/");
  // if (split.length !== 2) throw new Error("Invalid currency pair! Must be in the format of BASE/QUOTE, eg BTC/EUR");
  // return new Models.CurrencyPair(Models.Currency[split[0]], Models.Currency[split[1]]);
// })(config.GetString("TradedPair"));

// const exchange = ((ex: string): Models.Exchange => {
  // switch (ex) {
    // case "coinbase": return Models.Exchange.Coinbase;
    // case "okcoin": return Models.Exchange.OkCoin;
    // case "bitfinex": return Models.Exchange.Bitfinex;
    // case "poloniex": return Models.Exchange.Poloniex;
    // case "korbit": return Models.Exchange.Korbit;
    // case "hitbtc": return Models.Exchange.HitBtc;
    // case "null": return Models.Exchange.Null;
    // default: throw new Error("Invalid configuration value EXCHANGE: " + ex);
  // }
// })(config.GetString("EXCHANGE").toLowerCase());

// for (const param in defaultQuotingParameters)
  // if (config.GetDefaultString(param) !== null)
    // defaultQuotingParameters[param] = config.GetDefaultString(param);

// const sqlite = new bindings.SQLite(exchange, pair.base, pair.quote);

// const initParams = Object.assign(defaultQuotingParameters, sqlite.load(Models.Topics.QuotingParametersChange)[0] || {});
// const initTrades = sqlite.load(Models.Topics.Trades).map(x => Object.assign(x, {time: new Date(x.time)}));
// const initRfv = sqlite.load(Models.Topics.FairValue).map(x => Object.assign(x, {time: new Date(x.time)}));
// const initMkt = sqlite.load(Models.Topics.MarketData).map(x => Object.assign(x, {time: new Date(x.time)}));
// const initTBP = sqlite.load(Models.Topics.TargetBasePosition).map(x => Object.assign(x, {time: new Date(x.time)}))[0];

// const receiver = new Publish.Receiver(io);
// const publisher = new Publish.Publisher(io);

// (async (): Promise<void> => {
  // const gateway = await ((): Promise<Interfaces.CombinedGateway> => {
    // switch (exchange) {
      // case Models.Exchange.Coinbase: return Coinbase.createCoinbase(config, pair);
      // case Models.Exchange.OkCoin: return OkCoin.createOkCoin(config, pair);
      // case Models.Exchange.Bitfinex: return Bitfinex.createBitfinex(config, pair);
      // case Models.Exchange.Poloniex: return Poloniex.createPoloniex(config, pair);
      // case Models.Exchange.Korbit: return Korbit.createKorbit(config, pair);
      // case Models.Exchange.HitBtc: return HitBtc.createHitBtc(config, pair);
      // case Models.Exchange.Null: return NullGw.createNullGateway(config, pair);
      // default: throw new Error("no gateway provided for exchange " + exchange);
    // }
  // })();

  // publisher
    // .registerSnapshot(Models.Topics.ProductAdvertisement, () => [new Models.ProductAdvertisement(
      // exchange,
      // pair,
      // config.GetString("BotIdentifier").replace('auto',''),
      // config.GetString("MatryoshkaUrl"),
      // packageConfig.homepage,
      // gateway.base.minTickIncrement
    // )]);

  // const paramsRepo = new QuotingParameters.QuotingParametersRepository(
    // sqlite,
    // publisher,
    // receiver,
    // initParams
  // );

  // publisher.monitor = new Monitor.ApplicationState(
    // '/data/db/K.'+exchange+'.'+pair.base+'.'+pair.quote+'.db',
    // paramsRepo,
    // publisher,
    // receiver,
    // io
  // );

  // const broker = new Broker.ExchangeBroker(
    // pair,
    // gateway.md,
    // gateway.base,
    // gateway.oe,
    // publisher,
    // receiver,
    // config.GetString("BotIdentifier").indexOf('auto')>-1
  // );

  // const orderBroker = new Broker.OrderBroker(
    // timeProvider,
    // paramsRepo,
    // broker,
    // gateway.oe,
    // sqlite,
    // publisher,
    // receiver,
    // initTrades
  // );

  // const marketBroker = new Broker.MarketDataBroker(
    // gateway.md,
    // publisher
  // );

  // const fvEngine = new FairValue.FairValueEngine(
    // new MarketFiltration.MarketFiltration(
      // broker.minTickIncrement,
      // orderBroker,
      // marketBroker
    // ),
    // broker.minTickIncrement,
    // timeProvider,
    // paramsRepo,
    // publisher,
    // initRfv
  // );

  // const positionBroker = new Broker.PositionBroker(
    // timeProvider,
    // paramsRepo,
    // broker,
    // orderBroker,
    // fvEngine,
    // gateway.pg,
    // publisher
  // );

  // const quotingEngine = new QuotingEngine.QuotingEngine(
    // timeProvider,
    // fvEngine,
    // paramsRepo,
    // orderBroker,
    // positionBroker,
    // broker.minTickIncrement,
    // broker.minSize,
    // new Statistics.EWMAProtectionCalculator(timeProvider, fvEngine, paramsRepo),
    // new Statistics.STDEVProtectionCalculator(
      // timeProvider,
      // fvEngine,
      // paramsRepo,
      // sqlite,
      // bindings.computeStdevs,
      // initMkt
    // ),
    // new PositionManagement.TargetBasePositionManager(
      // timeProvider,
      // broker.minTickIncrement,
      // sqlite,
      // fvEngine,
      // new Statistics.EWMATargetPositionCalculator(paramsRepo, initRfv),
      // paramsRepo,
      // positionBroker,
      // publisher,
      // initTBP
    // ),
    // new Safety.SafetyCalculator(
      // timeProvider,
      // fvEngine,
      // paramsRepo,
      // positionBroker,
      // orderBroker,
      // publisher
    // )
  // );

  // new QuoteSender.QuoteSender(
    // timeProvider,
    // quotingEngine,
    // broker,
    // orderBroker,
    // paramsRepo,
    // publisher
  // );

  // new MarketTrades.MarketTradeBroker(
    // gateway.md,
    // publisher,
    // marketBroker,
    // quotingEngine,
    // broker
  // );

  // happyEnding = () => {
    // orderBroker.cancelOpenOrders();
    // console.info(new Date().toISOString().slice(11, -1), 'main', 'Attempting to cancel all open orders, please wait..');
  // };

  // let highTime = process.hrtime();
  // setInterval(() => {
    // const diff = process.hrtime(highTime);
    // const n = ((diff[0] * 1e9 + diff[1]) / 1e6) - 500;
    // if (n > 121) console.info(new Date().toISOString().slice(11, -1), 'main', 'Event loop delay', Utils.roundNearest(n, 100) + 'ms');
    // highTime = process.hrtime();
  // }, 500).unref();
// })();
