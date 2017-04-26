import Models = require("../share/models");
import moment = require('moment');
import events = require("events");
import bunyan = require("bunyan");
import fs = require("fs");
import _ = require("lodash");
import * as request from "request";
import * as Q from "q";

require('events').EventEmitter.prototype._maxListeners = 100;

export var date = moment.utc;

export function log(name: string): bunyan {
    // don't log while testing
    const isRunFromMocha = process.argv.length >= 2 && _.includes(process.argv[1], "mocha");
    if (isRunFromMocha) {
        return bunyan.createLogger({name: name, stream: process.stdout, level: bunyan.FATAL});
    }

    if (!fs.existsSync('./log')) fs.mkdirSync('./log');

    return bunyan.createLogger({
        name: name,
        streams: [{
            level: 'info',
            stream: process.stdout            // log INFO and above to stdout
        }, {
            level: 'info',
            path: './log/tribeca.log'  // log ERROR and above to a file
        }
    ]});
}

// typesafe wrapper around EventEmitter
export class Evt<T> {
    private _event = new events.EventEmitter();

    public on = (handler: (data?: T) => void) => this._event.addListener("evt", handler);
    
    public off = (handler: (data?: T) => void) => this._event.removeListener("evt", handler);

    public trigger = (data?: T) => this._event.emit("evt", data);

    public once = (handler: (data?: T) => void) => this._event.once("evt", handler);

    public setMaxListeners = (max: number) => this._event.setMaxListeners(max);

    public removeAllListeners = () => this._event.removeAllListeners();
}

export function roundSide(x: number, minTick: number, side: Models.Side) {
    switch (side) {
        case Models.Side.Bid: return roundDown(x, minTick);
        case Models.Side.Ask: return roundUp(x, minTick);
        default: return roundNearest(x, minTick);
    }
}

export function roundNearest(x: number, minTick: number) {
    const up = roundUp(x, minTick);
    const down = roundDown(x, minTick);
    return (Math.abs(x - down) > Math.abs(up - x)) ? up : down;
}

export function roundUp(x: number, minTick: number) {
    return Math.ceil(x/minTick)*minTick;
}

export function roundDown(x: number, minTick: number) {
    return Math.floor(x/minTick)*minTick;
}

export interface ITimeProvider {
    utcNow() : moment.Moment;
    setTimeout(action: () => void, time: moment.Duration);
    setImmediate(action: () => void);
    setInterval(action: () => void, time: moment.Duration);
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
