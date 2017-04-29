import Models = require("../common/models");
import moment = require('moment');
import events = require("events");
import util = require("util");
import _ = require("lodash");
import * as request from "request";
import * as Q from "q";

require('events').EventEmitter.prototype._maxListeners = 100;

export const date = () => new Date();

export function fastDiff(x: Date, y: Date) : number {
    return x.getTime() - y.getTime();
}

export function timeOrDefault(x: Models.ITimestamped, timeProvider : ITimeProvider): Date {
    if (x === null)
        return timeProvider.utcNow();

    if (typeof x !== "undefined" && typeof x.time !== "undefined")
        return x.time;

    return timeProvider.utcNow();
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
    utcNow() : Date;
    setTimeout(action: () => void, time: moment.Duration);
    setImmediate(action: () => void);
    setInterval(action: () => void, time: moment.Duration);
}

export interface IBacktestingTimeProvider extends ITimeProvider {
    scrollTimeTo(time : moment.Moment);
}

export class RealTimeProvider implements ITimeProvider {
    constructor() { }
    
    utcNow = () => new Date();
    
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

export function getJSON<T>(url: string, qs?: any) : Promise<T> {
    return new Promise((resolve, reject) => {
        request({url: url, qs: qs}, (err: Error, resp, body) => {
            if (err) {
                reject(err);
            }
            else {
                try {
                    resolve(JSON.parse(body));
                }
                catch (e) {
                    reject(e);
                }
            }
        });
    });
 }