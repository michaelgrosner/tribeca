import Models = require("../common/models");
import moment = require('moment');
import events = require("events");
import util = require("util");
import bunyan = require("bunyan");
import _ = require("lodash");
import * as request from "request";
import * as Q from "q";

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

export function log(name: string) : bunyan {
    // don't log while testing
    const isRunFromMocha = process.argv.length >= 2 && _.includes(process.argv[1], "mocha");
    if (isRunFromMocha) {
        return bunyan.createLogger({name: name, stream: process.stdout, level: bunyan.FATAL});
    }

    return bunyan.createLogger({
        name: name,
        streams: [{
            level: 'info',
            stream: process.stdout            // log INFO and above to stdout
        }, {
            level: 'info',
            path: './tribeca.log'  // log ERROR and above to a file
        }
    ]});
}

// typesafe wrapper around EventEmitter
export class Evt<T> {
    private _event = new events.EventEmitter();

    public on = (handler: (data?: T) => void) => this._event.addListener("evt", handler);

    public trigger = (data?: T) => this._event.emit("evt", data);
    
    public once = (handler: (data?: T) => void) => this._event.once("evt", handler);
    
    public setMaxListeners = (max: number) => this._event.setMaxListeners(max);
    
    public removeAllListeners = () => this._event.removeAllListeners();
}

export function roundFloat(x: number, minTick: number) {
    const b = 1/minTick;
    return Math.round(x * b) / b;
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

export interface IActionScheduler {
    schedule(action: () => void);
}

export class ImmediateActionScheduler implements IActionScheduler {
    constructor(private _timeProvider: ITimeProvider) {}
    
    private _shouldSchedule = true;
    public schedule = (action: () => void) => {
        if (this._shouldSchedule) {
            this._shouldSchedule = false;
            this._timeProvider.setImmediate(() => {
                action();
                this._shouldSchedule = true;
            });
        }
    };
}

export function getJSON<T>(url: string, qs?: any) : Q.Promise<T> {
    const d = Q.defer<T>();
    request({url: url, qs: qs}, (err, resp, body) => {
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