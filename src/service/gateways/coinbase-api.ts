///<reference path="../utils.ts"/>
///<reference path="../../common/models.ts"/>

var util = require('util');
var crypto = require('crypto');

import Utils = require("../utils");
import _ = require('lodash');
import request = require('request');
import Models = require("../../common/models");
import moment = require("moment");
import log from "../logging";

var HttpsAgent = require('agentkeepalive').HttpsAgent;
var EventEmitter = require('events').EventEmitter;
import WebSocket = require('ws');

var coinbaseLog = log("tribeca:gateway:coinbase-api");

var keepaliveAgent = new HttpsAgent();

export var PublicClient = function(apiURI?: string) {
    var self = this;
    console.log("starting coinbase public client, apiURI = ", apiURI);
    self.apiURI = apiURI || 'https://api.exchange.coinbase.com';
};

PublicClient.prototype = new function() {
    var prototype = this;
    prototype.addHeaders = function(obj, additional) {
        obj.headers = obj.headers || {};
        return _.assign(obj.headers, {
            'User-Agent': 'coinbase-node-client',
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        }, additional);
    };

    prototype.makeRelativeURI = function(parts) {
        return '/' + parts.join('/');
    };

    prototype.makeAbsoluteURI = function(relativeURI) {
        var self = this;
        return self.apiURI + relativeURI;
    };

    prototype.makeRequestCallback = function(callback) {
        return function(err, response, data) {
            if (typeof data === "string") {
                data = JSON.parse(data);
            }
            callback(err, response, data);
        };
    };

    prototype.request = function(method, uriParts, opts, callback) {
        var self = this;
        opts = opts || {};
        if (!callback && (opts instanceof Function)) {
            callback = opts;
            opts = {};
        }
        _.assign(opts, {
            'method': method.toUpperCase(),
            'uri': self.makeAbsoluteURI(self.makeRelativeURI(uriParts)),
            'json': true,
        });
        self.addHeaders(opts);
        opts.agent = keepaliveAgent;
        request(opts, self.makeRequestCallback(callback));
    };

    _.forEach(['get', 'post', 'put', 'delete'], function(method) {
        prototype[method] = _.partial(prototype.request, method);
    });

    prototype.getProducts = function(callback) {
        var self = this;
        return prototype.get.call(self, ['products'], callback);
    };

    prototype.getProductOrderBook = function(productID, level, callback) {
        var self = this;
        if (!callback && (level instanceof Function)) {
            callback = level;
            level = null;
        }
        var opts = level && { 'qs': { 'level': level } };
        return prototype.get.call(
            self, ['products', productID, 'book'], opts, callback);
    };

    prototype.getProductTicker = function(productID, callback) {
        var self = this;
        return prototype.get.call(self, ['products', productID, 'ticker'], callback);
    };

    prototype.getProductTrades = function(productID, callback) {
        var self = this;
        return prototype.get.call(self, ['products', productID, 'trades'], callback);
    };

    prototype.getProductHistoricRates = function(productID, callback) {
        var self = this;
        return prototype.get.call(self, ['products', productID, 'candles'], callback);
    };

    prototype.getProduct24HrStats = function(productID, callback) {
        var self = this;
        return prototype.get.call(self, ['products', productID, 'stats'], callback);
    };

    prototype.getCurrencies = function(callback) {
        var self = this;
        return prototype.get.call(self, ['currencies'], callback);
    };

    prototype.getTime = function(callback) {
        var self = this;
        return prototype.get.call(self, ['time'], callback);
    };
};

export var AuthenticatedClient = function(key: string, b64secret: string, passphrase: string, apiURI?: string) {
    var self = this;
    PublicClient.call(self, apiURI);
    self.key = key;
    self.b64secret = b64secret;
    self.passphrase = passphrase;
};

util.inherits(AuthenticatedClient, PublicClient);
_.assign(AuthenticatedClient.prototype, new function() {
    var prototype = this;

    prototype.request = function(method, uriParts, opts, callback) {
        var self = this;
        opts = opts || {};
        method = method.toUpperCase();
        if (!callback && (opts instanceof Function)) {
            callback = opts;
            opts = {};
        }
        var relativeURI = self.makeRelativeURI(uriParts);
        _.assign(opts, {
            'method': method,
            'uri': self.makeAbsoluteURI(relativeURI),
        });
        if (opts.body && (typeof opts.body !== 'string')) {
            opts.body = JSON.stringify(opts.body);
        }
        opts.agent = keepaliveAgent;
        var timestamp = Date.now() / 1000;
        var what = timestamp + method + relativeURI + (opts.body || '');
        var key = new Buffer(self.b64secret, 'base64');
        var hmac = crypto.createHmac('sha256', key);
        var signature = hmac.update(what).digest('base64');
        self.addHeaders(opts, {
            'CB-ACCESS-KEY': self.key,
            'CB-ACCESS-SIGN': signature,
            'CB-ACCESS-TIMESTAMP': timestamp,
            'CB-ACCESS-PASSPHRASE': self.passphrase,
        });
        request(opts, self.makeRequestCallback(callback));
    };

    _.forEach(['get', 'post', 'put', 'delete'], function(method) {
        prototype[method] = _.partial(prototype.request, method);
    });

    prototype.getAccounts = function(callback) {
        var self = this;
        return prototype.get.call(self, ['accounts'], callback);
    };

    prototype.getAccount = function(accountID, callback) {
        var self = this;
        return prototype.get.call(self, ['accounts', accountID], callback);
    };

    prototype.getAccountHistory = function(accountID, callback) {
        var self = this;
        return prototype.get.call(self, ['accounts', accountID, 'ledger'], callback);
    };

    prototype.getAccountHolds = function(accountID, callback) {
        var self = this;
        return prototype.get.call(self, ['accounts', accountID, 'holds'], callback);
    };

    prototype._placeOrder = function(params, callback) {
        var self = this;
        _.forEach(['size', 'side', 'product_id'], function(param) {
            if (params[param] === undefined) {
                throw "`opts` must include param `" + param + "`";
            }
        });
        var opts = { 'body': params };
        return prototype.post.call(self, ['orders'], opts, callback);
    };

    prototype.buy = function(params, callback) {
        var self = this;
        params.side = 'buy';
        return self._placeOrder(params, callback);
    };

    prototype.sell = function(params, callback) {
        var self = this;
        params.side = 'sell';
        return self._placeOrder(params, callback);
    };

    prototype.cancelOrder = function(orderID, callback) {
        var self = this;
        return prototype.delete.call(self, ['orders', orderID], callback);
    };
    
    prototype.cancelAllOrders = function(callback) {
        var self = this;
        return prototype.delete.call(self, ['orders'], callback);
    };

    prototype.getOrders = function(callback) {
        var self = this;
        return prototype.get.call(self, ['orders'], callback);
    };

    prototype.getOrder = function(orderID, callback) {
        var self = this;
        return prototype.get.call(self, ['orders', orderID], callback);
    };

    prototype.getFills = function(callback) {
        var self = this;
        return prototype.get.call(self, ['fills'], callback);
    };

    prototype.deposit = function(params, callback) {
        var self = this;
        params.type = 'deposit';
        return self._transferFunds(params, callback);
    };

    prototype.withdraw = function(params, callback) {
        var self = this;
        params.type = 'withdraw';
        return self._transferFunds(params, callback);
    };

    prototype._transferFunds = function(params, callback) {
        var self = this;
        _.forEach(['type', 'amount', 'coinbase_account_id'], function(param) {
            if (params[param] === undefined) {
                throw "`opts` must include param `" + param + "`";
            }
        });
        var opts = { 'body': params };
        return prototype.post.call(self, ['transfers'], opts, callback);
    };

});

export var OrderBook = function(productID: string, websocketURI: string, restURI: string, timeProvider: Utils.ITimeProvider) {
    var self = this;
    EventEmitter.call(self);

    self.productID = productID || 'BTC-USD';
    self.websocketURI = websocketURI || 'wss://ws-feed.exchange.coinbase.com';
    self.restURI = restURI;
    self.state = self.STATES.closed;
    self.fail_count = 0;
    self.timeProvider = timeProvider;
    self.connect();
};

util.inherits(OrderBook, EventEmitter);
_.assign(OrderBook.prototype, new function() {
    var prototype = this;

    prototype.STATES = {
        'closed': 'closed',
        'open': 'open',
        'syncing': 'syncing',
        'processing': 'processing',
        'error': 'error',
    };

    prototype.clear_book = function() {
        var self = this;
        self.queue = [];
        self.book = {
            'sequence': -1,
            'bids': {},
            'asks': {},
        };
    };

    prototype.connect = function() {
        coinbaseLog.info("Starting connect");
        var self = this;
        if (self.socket) {
            self.socket.close();
        }
        self.clear_book();
        self.socket = new WebSocket(self.websocketURI);
        self.socket.on('message', self.onMessage.bind(self));
        self.socket.on('open', self.onOpen.bind(self));
        self.socket.on('close', self.onClose.bind(self));
    };

    prototype.disconnect = function() {
        var self = this;
        if (!self.socket) {
            throw "Could not disconnect (not connected)"
        }
        self.socket.close();
        self.onClose();
    };

    prototype.changeState = function(stateName) {
        var self = this;
        var newState = self.STATES[stateName];
        if (newState === undefined) {
            throw "Unrecognized state: " + stateName;
        }
        var oldState = self.state;
        self.state = newState;

        if (self.fail_count > 3)
            throw "Tried to reconnect 4 times. Giving up.";

        if (self.state === self.STATES.error || self.state === self.STATES.closed) {
            self.fail_count += 1;
            self.socket.close();
            setTimeout(() => self.connect(), 5000);
        }
        else if (self.state === self.STATES.processing) {
            self.fail_count = 0;
        }

        var sc = { 'old': oldState, 'new': newState };
        coinbaseLog.info("statechange: ", sc);
        self.emit('statechange', sc);
    };

    prototype.onOpen = function() {
        var self = this;
        self.changeState(self.STATES.open);
        self.sync();
    };

    prototype.onClose = function() {
        var self = this;
        self.changeState(self.STATES.closed);
    };

    prototype.onMessage = function(datastr: string) {
        var self = this;
        var t = self.timeProvider.utcNow();
        var data = JSON.parse(datastr);
        if (self.state !== self.STATES.processing) {
            self.queue.push(data);
        } else {
            self.processMessage(data, t);
        }
    };

    prototype.sync = function() {
        var self = this;
        self.changeState(self.STATES.syncing);
        var subscribeMessage = {
            'type': 'subscribe',
            'product_id': self.productID,
        };
        self.socket.send(JSON.stringify(subscribeMessage));
        self.loadSnapshot();
    };

    prototype.loadSnapshot = function(snapshotData) {
        var self = this;

        var load = function(data) {
            var i, bid, ask;
            var convertSnapshotArray = function(array) {
                return { 'price': array[0], 'size': array[1], 'id': array[2] }
            };

            for (i = 0; data.bids && i < data.bids.length; i++) {
                bid = convertSnapshotArray(data.bids[i]);
                self.book.bids[bid.id] = bid;
            }
            ;
            for (i = 0; data.asks && i < data.asks.length; i++) {
                ask = convertSnapshotArray(data.asks[i]);
                self.book.asks[ask.id] = ask;
            }
            ;
            self.book.sequence = data.sequence
            self.changeState(self.STATES.processing);
            _.forEach(self.queue, self.processMessage.bind(self));
            self.queue = [];
        };

        request({
            'url': self.restURI + '/products/' + self.productID + '/book?level=3',
            'headers': { 'User-Agent': 'coinbase-node-client' },
        }, function(err, response, body) {
                if (err) {
                    self.changeState(self.STATES.error);
                    coinbaseLog.error(err, "error: Failed to load snapshot");
                }
                else if (response.statusCode !== 200) {
                    self.changeState(self.STATES.error);
                    coinbaseLog.error("Failed to load snapshot", response.statusCode);
                }
                else {
                    load(JSON.parse(body));
                }
            });
    };

    prototype.processMessage = function(message, t: Date) {
        var self = this;
        if (message.sequence <= self.book.sequence) {
            self.emit('ignored', message);
            return;
        }
        if (message.sequence != self.book.sequence + 1) {
            self.changeState(self.STATES.error);
            coinbaseLog.warn("Received message out of order, expected", self.book.sequence, "but got", message.sequence);
        }
        self.book.sequence = message.sequence;

        self.emit(message.type, new Models.Timestamped(message, t));
    };
});
