"use strict";
var __extends = (this && this.__extends) || (function () {
    var extendStatics = Object.setPrototypeOf ||
        ({ __proto__: [] } instanceof Array && function (d, b) { d.__proto__ = b; }) ||
        function (d, b) { for (var p in b) if (b.hasOwnProperty(p)) d[p] = b[p]; };
    return function (d, b) {
        extendStatics(d, b);
        function __() { this.constructor = d; }
        d.prototype = b === null ? Object.create(b) : (__.prototype = b.prototype, new __());
    };
})();
exports.__esModule = true;
var Models = require("../share/models");
var Utils = require("./utils");
var Interfaces = require("./interfaces");
var moment = require("moment");
var _ = require("lodash");
var fs = require("fs");
var Q = require("q");
var shortId = require("shortid");
var uuid = require('uuid');
var TimedType;
(function (TimedType) {
    TimedType[TimedType["Interval"] = 0] = "Interval";
    TimedType[TimedType["Timeout"] = 1] = "Timeout";
})(TimedType || (TimedType = {}));
var Timed = (function () {
    function Timed(action, time, type, interval) {
        this.action = action;
        this.time = time;
        this.type = type;
        this.interval = interval;
    }
    return Timed;
}());
var BacktestTimeProvider = (function () {
    function BacktestTimeProvider(_internalTime, _endTime) {
        var _this = this;
        this._internalTime = _internalTime;
        this._endTime = _endTime;
        this.utcNow = function () { return _this._internalTime; };
        this._immediates = new Array();
        this.setImmediate = function (action) { return _this._immediates.push(action); };
        this._timeouts = [];
        this.setTimeout = function (action, time) {
            _this.setAction(action, time, TimedType.Timeout);
        };
        this.setInterval = function (action, time) {
            _this.setAction(action, time, TimedType.Interval);
        };
        this.setAction = function (action, time, type) {
            var dueTime = _this._internalTime.clone().add(time);
            if (dueTime.valueOf() - _this.utcNow().valueOf() < 0) {
                return;
            }
            _this._timeouts.push(new Timed(action, dueTime, type, time));
            _this._timeouts.sort(function (a, b) { return a.time.valueOf() - b.time.valueOf(); });
        };
        this.scrollTimeTo = function (time) {
            if (time.valueOf() - _this.utcNow().valueOf() < 0) {
                throw new Error("Cannot reverse time!");
            }
            while (_this._immediates.length > 0) {
                _this._immediates.pop()();
            }
            while (_this._timeouts.length > 0 && _.first(_this._timeouts).time.valueOf() - time.valueOf() < 0) {
                var evt = _this._timeouts.shift();
                _this._internalTime = evt.time;
                evt.action();
                if (evt.type === TimedType.Interval) {
                    _this.setAction(evt.action, evt.interval, evt.type);
                }
            }
            _this._internalTime = time;
        };
    }
    return BacktestTimeProvider;
}());
exports.BacktestTimeProvider = BacktestTimeProvider;
var BacktestGateway = (function () {
    function BacktestGateway(_inputData, _baseAmount, _quoteAmount, timeProvider) {
        var _this = this;
        this._inputData = _inputData;
        this._baseAmount = _baseAmount;
        this._quoteAmount = _quoteAmount;
        this.timeProvider = timeProvider;
        this.ConnectChanged = new Utils.Evt();
        this.MarketData = new Utils.Evt();
        this.MarketTrade = new Utils.Evt();
        this.OrderUpdate = new Utils.Evt();
        this.supportsCancelAllOpenOrders = function () { return false; };
        this.cancelAllOpenOrders = function () { return Q(0); };
        this.generateClientOrderId = function () {
            return "BACKTEST-" + shortId.generate();
        };
        this.cancelsByClientOrderId = true;
        this._openBidOrders = {};
        this._openAskOrders = {};
        this.sendOrder = function (order) {
            _this.timeProvider.setTimeout(function () {
                if (order.side === Models.Side.Bid) {
                    _this._openBidOrders[order.orderId] = order;
                    _this._quoteHeld += order.price * order.quantity;
                    _this._quoteAmount -= order.price * order.quantity;
                }
                else {
                    _this._openAskOrders[order.orderId] = order;
                    _this._baseHeld += order.quantity;
                    _this._baseAmount -= order.quantity;
                }
                _this.OrderUpdate.trigger({ orderId: order.orderId, orderStatus: Models.OrderStatus.Working });
            }, moment.duration(3));
        };
        this.cancelOrder = function (cancel) {
            _this.timeProvider.setTimeout(function () {
                if (cancel.side === Models.Side.Bid) {
                    var existing = _this._openBidOrders[cancel.orderId];
                    if (typeof existing === "undefined") {
                        _this.OrderUpdate.trigger({ orderId: cancel.orderId, orderStatus: Models.OrderStatus.Rejected });
                        return;
                    }
                    _this._quoteHeld -= existing.price * existing.quantity;
                    _this._quoteAmount += existing.price * existing.quantity;
                    delete _this._openBidOrders[cancel.orderId];
                }
                else {
                    var existing = _this._openAskOrders[cancel.orderId];
                    if (typeof existing === "undefined") {
                        _this.OrderUpdate.trigger({ orderId: cancel.orderId, orderStatus: Models.OrderStatus.Rejected });
                        return;
                    }
                    _this._baseHeld -= existing.quantity;
                    _this._baseAmount += existing.quantity;
                    delete _this._openAskOrders[cancel.orderId];
                }
                _this.OrderUpdate.trigger({ orderId: cancel.orderId, orderStatus: Models.OrderStatus.Cancelled });
            }, moment.duration(3));
        };
        this.replaceOrder = function (replace) {
            _this.cancelOrder(replace);
            _this.sendOrder(replace);
        };
        this.onMarketData = function (market) {
            _this._openAskOrders = _this.tryToMatch(_.values(_this._openAskOrders), market.bids, Models.Side.Ask);
            _this._openBidOrders = _this.tryToMatch(_.values(_this._openBidOrders), market.asks, Models.Side.Bid);
            _this.MarketData.trigger(market);
        };
        this.tryToMatch = function (orders, marketSides, side) {
            if (orders.length === 0 || marketSides.length === 0)
                return _.keyBy(orders, function (k) { return k.orderId; });
            var cmp = side === Models.Side.Ask ? function (m, o) { return o < m; } : function (m, o) { return o > m; };
            _.forEach(orders, function (order) {
                _.forEach(marketSides, function (mkt) {
                    if ((cmp(mkt.price, order.price) || order.type === Models.OrderType.Market) && order.quantity > 0) {
                        var px = order.price;
                        if (order.type === Models.OrderType.Market)
                            px = mkt.price;
                        var update = { orderId: order.orderId, lastPrice: px };
                        if (mkt.size >= order.quantity) {
                            update.orderStatus = Models.OrderStatus.Complete;
                            update.lastQuantity = order.quantity;
                        }
                        else {
                            update.partiallyFilled = true;
                            update.orderStatus = Models.OrderStatus.Working;
                            update.lastQuantity = mkt.size;
                        }
                        _this.OrderUpdate.trigger(update);
                        if (side === Models.Side.Bid) {
                            _this._baseAmount += update.lastQuantity;
                            _this._quoteHeld -= (update.lastQuantity * px);
                        }
                        else {
                            _this._baseHeld -= update.lastQuantity;
                            _this._quoteAmount += (update.lastQuantity * px);
                        }
                        order.quantity = order.quantity - update.lastQuantity;
                    }
                    ;
                });
            });
            var liveOrders = _.filter(orders, function (o) { return o.quantity > 0; });
            if (liveOrders.length > 5)
                console.warn("more than 5 outstanding " + Models.Side[side] + " orders open");
            return _.keyBy(liveOrders, function (k) { return k.orderId; });
        };
        this.onMarketTrade = function (trade) {
            _this._openAskOrders = _this.tryToMatch(_.values(_this._openAskOrders), [trade], Models.Side.Ask);
            _this._openBidOrders = _this.tryToMatch(_.values(_this._openBidOrders), [trade], Models.Side.Bid);
            _this.MarketTrade.trigger(new Models.GatewayMarketTrade(trade.price, trade.size, trade.time, false, trade.make_side));
        };
        this.PositionUpdate = new Utils.Evt();
        this.recomputePosition = function () {
            _this.PositionUpdate.trigger(new Models.CurrencyPosition(_this._baseAmount, _this._baseHeld, Models.Currency.BTC));
            _this.PositionUpdate.trigger(new Models.CurrencyPosition(_this._quoteAmount, _this._quoteHeld, Models.Currency.USD));
        };
        this._baseHeld = 0;
        this._quoteHeld = 0;
        this.run = function () {
            _this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
            var hasProcessedMktData = false;
            _this.timeProvider.setInterval(function () { return _this.recomputePosition(); }, moment.duration(15, "seconds"));
            _this._inputData.forEach(function (i) {
                _this.timeProvider.scrollTimeTo(i.time);
                if (typeof i["make_side"] !== "undefined") {
                    _this.onMarketTrade(i);
                }
                else if (typeof i["bids"] !== "undefined" || typeof i["asks"] !== "undefined") {
                    _this.onMarketData(i);
                    if (!hasProcessedMktData) {
                        _this.recomputePosition();
                        hasProcessedMktData = true;
                    }
                }
            });
            _this.recomputePosition();
        };
    }
    return BacktestGateway;
}());
exports.BacktestGateway = BacktestGateway;
var BacktestGatewayDetails = (function () {
    function BacktestGatewayDetails() {
        this.minTickIncrement = 0.01;
    }
    Object.defineProperty(BacktestGatewayDetails.prototype, "hasSelfTradePrevention", {
        get: function () {
            return false;
        },
        enumerable: true,
        configurable: true
    });
    BacktestGatewayDetails.prototype.name = function () {
        return "Null";
    };
    BacktestGatewayDetails.prototype.makeFee = function () {
        return 0;
    };
    BacktestGatewayDetails.prototype.takeFee = function () {
        return 0;
    };
    BacktestGatewayDetails.prototype.exchange = function () {
        return Models.Exchange.Null;
    };
    return BacktestGatewayDetails;
}());
var BacktestParameters = (function () {
    function BacktestParameters() {
    }
    return BacktestParameters;
}());
exports.BacktestParameters = BacktestParameters;
var BacktestPersister = (function () {
    function BacktestPersister(initialData) {
        var _this = this;
        this.initialData = initialData;
        this.load = function (exchange, pair, limit) {
            return _this.loadAll(limit);
        };
        this.loadAll = function (limit) {
            if (_this.initialData) {
                if (limit) {
                    return new Promise(function (resolve, reject) { resolve(_this.initialData.slice(limit * -1)); });
                }
                else {
                    return new Promise(function (resolve, reject) { resolve(_this.initialData); });
                }
            }
            return new Promise(function (resolve, reject) { resolve([]); });
        };
        this.persist = function (report) { };
        this.repersist = function (report, trade) { };
        this.loadDBSize = function () {
            return new Promise(function (resolve, reject) { resolve(null); });
        };
        this.loadLatest = function () {
            if (_this.initialData)
                return new Promise(function (resolve, reject) { resolve(_.last(_this.initialData)); });
        };
        this.initialData = initialData || null;
    }
    return BacktestPersister;
}());
exports.BacktestPersister = BacktestPersister;
var BacktestExchange = (function (_super) {
    __extends(BacktestExchange, _super);
    function BacktestExchange(gw) {
        var _this = _super.call(this, gw, gw, gw, new BacktestGatewayDetails()) || this;
        _this.gw = gw;
        _this.run = function () { return _this.gw.run(); };
        return _this;
    }
    return BacktestExchange;
}(Interfaces.CombinedGateway));
exports.BacktestExchange = BacktestExchange;
;
// backtest server
var express = require("express");
var util = require("util");
var backtestServer = function () {
    ["uncaughtException", "exit", "SIGINT", "SIGTERM"].forEach(function (reason) {
        process.on(reason, function (e) {
            console.log(util.format("Terminating!", reason, e, (typeof e !== "undefined" ? e.stack : undefined)));
            process.exit(1);
        });
    });
    var mdFile = process.env['MD_FILE'];
    var paramFile = process.env['PARAM_FILE'];
    var savedProgressFile = process.env["PROGRESS_FILE"] || "nextParameters_saved.txt";
    var backtestResultFile = process.env["RESULT_FILE"] || 'backtestResults.txt';
    var rawParams = fs.readFileSync(paramFile, 'utf8');
    var parameters = JSON.parse(rawParams);
    if (fs.existsSync(savedProgressFile)) {
        var l = parseInt(fs.readFileSync(savedProgressFile, 'utf8'));
        parameters = parameters.slice(l * -1);
    }
    else if (fs.existsSync(backtestResultFile)) {
        fs.unlinkSync(backtestResultFile);
    }
    console.log("loaded input data...");
    var app = express();
    app.use(require('body-parser').json({ limit: '200mb' }));
    app.use(require("compression")());
    var server = app.listen(5001, function () {
        var host = server.address().address;
        var port = server.address().port;
        console.log('Backtest server listening at http://%s:%s', host, port);
    });
    app.get("/inputData", function (req, res) {
        console.log("Starting inputData download for", req.ip);
        res.sendFile(mdFile, function (err) {
            if (err)
                console.error("Error while transmitting input data to", req.ip);
            else
                console.log("Ending inputData download for", req.ip);
        });
    });
    app.get("/nextParameters", function (req, res) {
        if (_.some(parameters)) {
            var id = parameters.length;
            var served = parameters.shift();
            if (typeof served["id"] === "undefined")
                served.id = id.toString();
            console.log("Serving parameters id =", served.id, " to", req.ip);
            res.json(served);
            fs.writeFileSync(savedProgressFile, parameters.length, { encoding: 'utf8' });
            if (!_.some(parameters)) {
                console.log("Done serving parameters");
            }
        }
        else {
            res.json("done");
            if (fs.existsSync(savedProgressFile))
                fs.unlinkSync(savedProgressFile);
        }
    });
    app.post("/result", function (req, res) {
        var params = req.body;
        console.log("Accept backtest results, volume =", params[2].volume.toFixed(2), "val =", params[1].value.toFixed(2), "qVal =", params[1].quoteValue.toFixed(2));
        fs.appendFileSync(backtestResultFile, JSON.stringify(params) + "\n");
    });
};
if (process.argv[1].indexOf("backtest.js") > 1) {
    backtestServer();
}
