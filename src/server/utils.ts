import Models = require("../share/models");
import moment = require('moment');
import _ = require("lodash");
import * as request from "request";
import { EventEmitter } from 'eventemitter3';

export const date = () => new Date();

// typesafe wrapper around EventEmitter
export class Evt<T> {
    private _event = new EventEmitter();

    public on = (handler: (data?: T) => void) => this._event.addListener("evt", handler);

    public off = (handler: (data?: T) => void) => this._event.removeListener("evt", handler);

    public trigger = (data?: T) => this._event.emit("evt", data);

    public once = (handler: (data?: T) => void) => this._event.once("evt", handler);

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

export class RealTimeProvider implements ITimeProvider {
    constructor() { }

    utcNow = () => new Date();

    setTimeout = (action: () => void, time: moment.Duration) => setTimeout(action, time.asMilliseconds());

    setImmediate = (action: () => void) => setImmediate(action);

    setInterval = (action: () => void, time: moment.Duration) => setInterval(action, time.asMilliseconds());
}

export interface IBacktestingTimeProvider extends ITimeProvider {
    scrollTimeTo(time : moment.Moment);
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
