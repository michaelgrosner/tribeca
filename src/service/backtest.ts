/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("./config");
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import io = require("socket.io-client");
import moment = require("moment");
import WebSocket = require('ws');
import _ = require('lodash');
import fs = require("fs");

var SortedArray = require("collections/sorted-array");
var uuid = require('node-uuid');

enum TimedType {
    Interval,
    Timeout
}

class Timed {
    constructor(
        public action : () => void, 
        public time : Moment, 
        public type : TimedType,
        public interval : Duration) {}
}

export class BacktestTimeProvider implements Utils.IBacktestingTimeProvider {
    constructor(private _internalTime : Moment) { }
    
    utcNow = () => this._internalTime;
    
    private _immediates = new Array<() => void>();
    setImmediate = (action: () => void) => this._immediates.push(action);
    
    private _timeouts = new SortedArray(null, null, (a : Timed, b : Timed) => a.time.diff(b.time));
    setTimeout = (action: () => void, time: Duration) => {
        this.setAction(action, time, TimedType.Timeout);
    };
    
    setInterval = (action: () => void, time: Duration) => {
        this.setAction(action, time, TimedType.Interval);
    };
    
    private setAction  = (action: () => void, time: Duration, type : TimedType) => {
        this._timeouts.push(new Timed(action, this._internalTime.clone().add(time), type, time));
    };
    
    scrollTimeTo = (time : Moment) => {
        while (this._immediates.length > 0) {
            this._immediates.pop()();
        }
        
        while (this._timeouts.length > 0 && this._timeouts.min().time.diff(time) > 0) {
            var evt : Timed = this._timeouts.shift();
            this._internalTime = evt.time;
            evt.action();
            if (evt.type === TimedType.Interval) {
                this.setAction(evt.action, evt.interval, evt.type);
            }
        }
        
        this._internalTime = time;
    };
}

export class RecordingMarketDataBroker implements Interfaces.IMarketDataBroker {
    MarketData = new Utils.Evt<Models.Market>();
    public get currentBook() : Models.Market { return this._decorated.currentBook; }
    
    constructor(
            private _decorated: Interfaces.IMarketDataBroker,
            private _output: fs.WriteStream) {
        _decorated.MarketData.on(this.onMarketData);
    }
    
    private onMarketData = (mkt: Models.Market) => {
        this.MarketData.trigger(mkt);
        this._output.write(JSON.stringify(["MD", mkt]));
        this._output.write("\n");
    };
}

export class RecordingMarketTradeBroker implements Interfaces.IMarketTradeBroker {
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._decorated.marketTrades; }
    
    constructor(
            private _decorated: Interfaces.IMarketTradeBroker,
            private _output: fs.WriteStream) {
        _decorated.MarketTrade.on(this.onTrade);
    }
    
    private onTrade = (t: Models.MarketTrade) => {
        this.MarketTrade.trigger(t);
        
        if (Math.abs(t.time.diff(moment.utc())) < 1000) {
            this._output.write(JSON.stringify(["T", t]));
            this._output.write("\n");
        }
    };
}