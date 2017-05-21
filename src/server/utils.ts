import Models = require("../share/models");
import moment = require('moment');

// typesafe event raiser
type EvtCallback<T> = (data?: T) => void;
export class Evt<T> {
    private _singleCallback : EvtCallback<T> = null;
    private _multiCallback = new Array<EvtCallback<T>>();

    public on = (handler: EvtCallback<T>) => {
        if (this._singleCallback) {
            this._multiCallback = [this._singleCallback, handler];
            this._singleCallback = null;
        } else if (this._multiCallback.length > 0)
          this._multiCallback.push(handler);
        else this._singleCallback = handler;
    };

    public off = (handler: EvtCallback<T>) => {
        if (this._multiCallback.length > 0)
          for(let i = this._multiCallback.length; i--;)
            if (this._multiCallback[i] === handler)
              this._multiCallback.splice(i, 1);
        if (this._singleCallback === handler)
            this._singleCallback = null;
    };

    public trigger = (data?: T) => {
        if (this._singleCallback !== null)
            this._singleCallback(data);
        else {
            for(let i = this._multiCallback.length; i--;)
                this._multiCallback[i](data);
        }
    };
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
