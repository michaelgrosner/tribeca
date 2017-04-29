"use strict";
exports.__esModule = true;
var Models = require("../share/models");
var moment = require("moment");
var events = require("events");
var bunyan = require("bunyan");
var fs = require("fs");
var _ = require("lodash");
var request = require("request");
var Q = require("q");
require('events').EventEmitter.prototype._maxListeners = 100;
exports.date = moment.utc;
function log(name) {
    // don't log while testing
    var isRunFromMocha = process.argv.length >= 2 && _.includes(process.argv[1], "mocha");
    if (isRunFromMocha) {
        return bunyan.createLogger({ name: name, stream: process.stdout, level: bunyan.FATAL });
    }
    if (!fs.existsSync('./log'))
        fs.mkdirSync('./log');
    return bunyan.createLogger({
        name: name,
        streams: [{
                level: 'info',
                stream: process.stdout // log INFO and above to stdout
            }, {
                level: 'info',
                path: './log/tribeca.log' // log ERROR and above to a file
            }
        ]
    });
}
exports.log = log;
// typesafe wrapper around EventEmitter
var Evt = (function () {
    function Evt() {
        var _this = this;
        this._event = new events.EventEmitter();
        this.on = function (handler) { return _this._event.addListener("evt", handler); };
        this.off = function (handler) { return _this._event.removeListener("evt", handler); };
        this.trigger = function (data) { return _this._event.emit("evt", data); };
        this.once = function (handler) { return _this._event.once("evt", handler); };
        this.setMaxListeners = function (max) { return _this._event.setMaxListeners(max); };
        this.removeAllListeners = function () { return _this._event.removeAllListeners(); };
    }
    return Evt;
}());
exports.Evt = Evt;
function roundSide(x, minTick, side) {
    switch (side) {
        case Models.Side.Bid: return roundDown(x, minTick);
        case Models.Side.Ask: return roundUp(x, minTick);
        default: return roundNearest(x, minTick);
    }
}
exports.roundSide = roundSide;
function roundNearest(x, minTick) {
    var up = roundUp(x, minTick);
    var down = roundDown(x, minTick);
    return (Math.abs(x - down) > Math.abs(up - x)) ? up : down;
}
exports.roundNearest = roundNearest;
function roundUp(x, minTick) {
    return Math.ceil(x / minTick) * minTick;
}
exports.roundUp = roundUp;
function roundDown(x, minTick) {
    return Math.floor(x / minTick) * minTick;
}
exports.roundDown = roundDown;
function getJSON(url, qs) {
    var d = Q.defer();
    request({ url: url, qs: qs }, function (err, resp, body) {
        if (err) {
            d.reject(err);
        }
        else {
            try {
                d.resolve(JSON.parse(body));
            }
            catch (e) {
                d.reject(e);
            }
        }
    });
    return d.promise;
}
exports.getJSON = getJSON;
var RealTimeProvider = (function () {
    function RealTimeProvider() {
        this.utcNow = function () { return moment.utc(); };
        this.setTimeout = function (action, time) { return setTimeout(action, time.asMilliseconds()); };
        this.setImmediate = function (action) { return setImmediate(action); };
        this.setInterval = function (action, time) { return setInterval(action, time.asMilliseconds()); };
    }
    return RealTimeProvider;
}());
exports.RealTimeProvider = RealTimeProvider;
