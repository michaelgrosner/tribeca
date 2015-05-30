/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import momentjs = require('moment');
export var date = momentjs.utc;

export function timeOrDefault(x: Models.ITimestamped): Moment {
    if (x === null)
        return date();

    if (typeof x !== "undefined" && typeof x.time !== "undefined")
        return x.time;

    return date();
}

import util = require("util");
import winston = require("winston");
winston.add(winston.transports.DailyRotateFile, {
    handleExceptions: true,
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

export class Evt<T> {
    constructor(private handlers: { (data?: T): void; }[] = []) {
        handlers.forEach(this.on);
    }

    public on(handler: (data?: T) => void) {
        this.handlers.push(handler);
    }

    public off(handler: (data?: T) => void) {
        this.handlers = this.handlers.filter(h => h !== handler);
    }

    public trigger(data?: T) {
        for (var i = 0; i < this.handlers.length; i++) {
            this.handlers[i](data);
        }
    }
}

export function roundFloat(x: number) {
    return Math.round(x * 100) / 100;
}

export interface ITimeProvider {
    utcNow() : Moment;
    setTimeout(action: () => void, time: Duration);
    setImmediate(action: () => void);
    setInterval(action: () => void, time: Duration);
}

export class RealTimeProvider implements ITimeProvider {
    constructor() { }
    
    utcNow = () => momentjs.utc();
    
    setTimeout = (action: () => void, time: Duration) => setTimeout(action, time.asMilliseconds());
    
    setImmediate = (action: () => void) => setImmediate(action);
    
    setInterval = (action: () => void, time: Duration) => setInterval(action, time.asMilliseconds());
}