/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import moment = require('moment');
import events = require("events");
require('events').EventEmitter.prototype._maxListeners = 100;

export var date = moment.utc;

export function fastDiff(x: moment.Moment, y: moment.Moment) : number {
    return x.valueOf() - y.valueOf();
}

export function timeOrDefault(x: Models.ITimestamped, timeProvider : ITimeProvider): moment.Moment {
    if (x === null)
        return timeProvider.utcNow();

    if (typeof x !== "undefined" && typeof x.time !== "undefined")
        return x.time;

    return timeProvider.utcNow();
}

import util = require("util");
import winston = require("winston");
winston.add(winston.transports.DailyRotateFile, <any>{
    handleExceptions: false,
    exitOnError: false,
    filename: 'tribeca.log',
    timestamp: false,
    json: false
}
    );

export var errorLog = winston.error;

export var log = (name: string) => {
    return (...msg: any[]) => {
        var head = util.format.bind(this, Models.toUtcFormattedTime(date()) + "\t[" + name + "]\t" + msg.shift());
        winston.info(head.apply(this, msg));
    };
};

export interface Logger { (...arg: any[]): void; }

// typesafe wrapper around EventEmitter
export class Evt<T> {
    private _event = new events.EventEmitter();

    public on = (handler: (data?: T) => void) => this._event.addListener("evt", handler);

    public trigger = (data?: T) => this._event.emit("evt", data);
    
    public once = (handler: (data?: T) => void) => this._event.once("evt", handler);
    
    public setMaxListeners = (max: number) => this._event.setMaxListeners(max);
    
    public removeAllListeners = () => this._event.removeAllListeners();
}

export function roundFloat(x: number) {
    return Math.round(x * 100) / 100;
}

export interface ITimeProvider {
    utcNow() : moment.Moment;
    setTimeout(action: () => void, time: moment.Duration);
    setImmediate(action: () => void);
    setInterval(action: () => void, time: moment.Duration);
}

export interface IBacktestingTimeProvider extends ITimeProvider {
    scrollTimeTo(time : moment.Moment);
}

export class RealTimeProvider implements ITimeProvider {
    constructor() { }
    
    utcNow = () => moment.utc();
    
    setTimeout = (action: () => void, time: moment.Duration) => setTimeout(action, time.asMilliseconds());
    
    setImmediate = (action: () => void) => setImmediate(action);
    
    setInterval = (action: () => void, time: moment.Duration) => setInterval(action, time.asMilliseconds());
}