"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
    return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (_) try {
            if (f = 1, y && (t = y[op[0] & 2 ? "return" : op[0] ? "throw" : "next"]) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [0, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
exports.__esModule = true;
var Utils = require("./utils");
var _ = require("lodash");
var mongodb = require("mongodb");
var Q = require("q");
var moment = require("moment");
function loadDb(config) {
    var deferred = Q.defer();
    mongodb.MongoClient.connect(config.GetString("MongoDbUrl"), function (err, db) {
        if (err)
            deferred.reject(err);
        else
            deferred.resolve(db);
    });
    return deferred.promise;
}
exports.loadDb = loadDb;
var LoaderSaver = (function () {
    function LoaderSaver(_exchange, _pair) {
        var _this = this;
        this._exchange = _exchange;
        this._pair = _pair;
        this.loader = function (x, setDBFlag) {
            if (typeof x.time !== "undefined")
                x.time = moment.isMoment(x.time) ? x.time : moment(x.time);
            if (typeof x.exchange === "undefined")
                x.exchange = _this._exchange;
            if (typeof x.pair === "undefined")
                x.pair = _this._pair;
            if (setDBFlag === true)
                x.loadedFromDB = true;
        };
        this.saver = function (x) {
            if (typeof x.time !== "undefined")
                x.time = (moment.isMoment(x.time) ? x.time : moment(x.time)).toDate();
            if (typeof x.exchange === "undefined")
                x.exchange = _this._exchange;
            if (typeof x.pair === "undefined")
                x.pair = _this._pair;
            if (typeof x.loadedFromDB !== "undefined")
                delete x.loadedFromDB;
        };
    }
    return LoaderSaver;
}());
exports.LoaderSaver = LoaderSaver;
var RepositoryPersister = (function () {
    function RepositoryPersister(db, _defaultParameter, _dbName, _exchange, _pair, _loader, _saver) {
        var _this = this;
        this.db = db;
        this._defaultParameter = _defaultParameter;
        this._dbName = _dbName;
        this._exchange = _exchange;
        this._pair = _pair;
        this._loader = _loader;
        this._saver = _saver;
        this._log = Utils.log("tribeca:exchangebroker:repopersister");
        this.loadDBSize = function () { return __awaiter(_this, void 0, void 0, function () {
            var _this = this;
            var deferred;
            return __generator(this, function (_a) {
                deferred = Q.defer();
                this.db.then(function (db) {
                    db.stats(function (err, arr) {
                        if (err) {
                            deferred.reject(err);
                        }
                        else if (arr == null) {
                            deferred.resolve(_this._defaultParameter);
                        }
                        else {
                            var v = _.defaults(arr.dataSize, _this._defaultParameter);
                            deferred.resolve(v);
                        }
                    });
                }).done();
                return [2 /*return*/, deferred.promise];
            });
        }); };
        this.loadLatest = function () { return __awaiter(_this, void 0, void 0, function () {
            var coll, selector, docs, v;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0: return [4 /*yield*/, this.collection];
                    case 1:
                        coll = _a.sent();
                        selector = { exchange: this._exchange, pair: this._pair };
                        return [4 /*yield*/, coll.find(selector)
                                .limit(1)
                                .project({ _id: 0 })
                                .sort({ $natural: -1 })
                                .toArray()];
                    case 2:
                        docs = _a.sent();
                        if (docs.length === 0)
                            return [2 /*return*/, this._defaultParameter];
                        v = _.defaults(docs[0], this._defaultParameter);
                        this._loader(v);
                        return [2 /*return*/, v];
                }
            });
        }); };
        this.repersist = function (report, trade) { };
        this.persist = function (report) {
            _this._saver(report);
            _this.collection.then(function (coll) {
                if (_this._dbName != 'trades')
                    coll.deleteMany({ _id: { $exists: true } }, function (err) {
                        if (err)
                            _this._log.error(err, "Unable to deleteMany", _this._dbName, report);
                    });
                coll.insertOne(report, function (err) {
                    if (err)
                        _this._log.error(err, "Unable to insert", _this._dbName, report);
                    else
                        _this._log.info("Persisted", report);
                });
            }).done();
        };
        if (this._dbName != 'dataSize')
            this.collection = db.then(function (db) { return db.collection(_this._dbName); });
    }
    return RepositoryPersister;
}());
exports.RepositoryPersister = RepositoryPersister;
var Persister = (function () {
    function Persister(time, collection, _dbName, _exchange, _pair, _setDBFlag, _loader, _saver) {
        var _this = this;
        this.collection = collection;
        this._dbName = _dbName;
        this._exchange = _exchange;
        this._pair = _pair;
        this._setDBFlag = _setDBFlag;
        this._loader = _loader;
        this._saver = _saver;
        this._log = Utils.log("persister");
        this.loadAll = function (limit, query) {
            var selector = { exchange: _this._exchange, pair: _this._pair };
            if (_this._dbName == "trades")
                delete query.time;
            _.assign(selector, query);
            return _this.loadInternal(selector, limit);
        };
        this.loadInternal = function (selector, limit) { return __awaiter(_this, void 0, void 0, function () {
            var _this = this;
            var query, count, loaded;
            return __generator(this, function (_a) {
                switch (_a.label) {
                    case 0:
                        query = this.collection.find(selector, { _id: 0 });
                        if (!(limit !== null)) return [3 /*break*/, 2];
                        return [4 /*yield*/, this.collection.count(selector)];
                    case 1:
                        count = _a.sent();
                        query = query.limit(limit);
                        if (count !== 0)
                            query = query.skip(Math.max(count - limit, 0));
                        _a.label = 2;
                    case 2: return [4 /*yield*/, query.toArray()];
                    case 3:
                        loaded = _a.sent();
                        _.forEach(loaded, function (p) { return _this._loader(p, _this._setDBFlag); });
                        return [2 /*return*/, loaded];
                }
            });
        }); };
        this._persistQueue = [];
        this.persist = function (report) {
            _this._persistQueue.push(report);
        };
        this.repersist = function (report, trade) {
            _this._saver(report);
            if (trade.Kqty < 0)
                _this.collection.deleteOne({ tradeId: trade.tradeId }, function (err) {
                    if (err)
                        _this._log.error(err, "Unable to deleteOne", _this._dbName, report);
                });
            else
                _this.collection.updateOne({ tradeId: trade.tradeId }, { $set: { time: (moment.isMoment(trade.time) ? trade.time.format('Y-MM-DD HH:mm:ss') : moment(trade.time).format('Y-MM-DD HH:mm:ss')), quantity: trade.quantity, value: trade.value, Ktime: (moment.isMoment(trade.Ktime) ? trade.Ktime.format('Y-MM-DD HH:mm:ss') : moment(trade.Ktime).format('Y-MM-DD HH:mm:ss')), Kqty: trade.Kqty, Kprice: trade.Kprice, Kvalue: trade.Kvalue, Kdiff: trade.Kdiff } }, function (err) {
                    if (err)
                        _this._log.error(err, "Unable to repersist", _this._dbName, report);
                });
        };
        this._log = Utils.log("persister:" + _dbName);
        time.setInterval(function () {
            if (_this._persistQueue.length === 0)
                return;
            _this._persistQueue.forEach(_this._saver);
            if (_this._dbName != 'trades')
                collection.deleteMany({ time: { $exists: true } }, function (err) {
                    if (err)
                        _this._log.error(err, "Unable to deleteMany", _this._dbName);
                });
            // collection.insertOne(report, err => {
            // if (err)
            // this._log.error(err, "Unable to insert", this._dbName, report);
            // });
            collection.insertMany(_this._persistQueue, function (err, r) {
                if (r.result.ok) {
                    _this._persistQueue.length = 0;
                }
                else if (err)
                    _this._log.error(err, "Unable to insert", _this._dbName, _this._persistQueue);
            });
        }, moment.duration(10, "seconds"));
    }
    return Persister;
}());
exports.Persister = Persister;
