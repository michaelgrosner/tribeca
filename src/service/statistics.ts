///<reference path="utils.ts"/>
///<reference path="interfaces.ts"/>
///<reference path="fair-value.ts"/>

import Utils = require("./utils");
import Interfaces = require("./interfaces");
import FairValue = require("./fair-value");
import moment = require("moment");
import log from "./logging";

export interface IComputeStatistics {
    latest: number;
    initialize(seedData: number[]): void;
    addNewValue(value: number): number;
}

export class EwmaStatisticCalculator implements IComputeStatistics {
    constructor(private _alpha: number) { }

    public latest: number = null;

    initialize(seedData: number[]) {
        for (var i = 0; i < seedData.length; i++)
            this.addNewValue(seedData[i]);
    }

    addNewValue(value: number): number {
        this.latest = computeEwma(value, this.latest, this._alpha);
        return this.latest;
    }
}

export function computeEwma(newValue: number, previous: number, alpha: number): number {
    if (previous !== null) {
        return alpha * newValue + (1 - alpha) * previous;
    }

    return newValue;
}

export class EmptyEWMACalculator implements Interfaces.IEwmaCalculator {
    constructor() { }
    latest: number = null;
    Updated = new Utils.Evt<any>();
}

export class ObservableEWMACalculator implements Interfaces.IEwmaCalculator {
    private _log = log("ewma");

    constructor(private _timeProvider: Utils.ITimeProvider, private _fv: FairValue.FairValueEngine, private _alpha?: number) {
        this._alpha = _alpha || .095;
        _timeProvider.setInterval(this.onTick, moment.duration(1, "minutes"));
        this.onTick();
    }

    private onTick = () => {
        var fv = this._fv.latestFairValue;

        if (fv === null) {
            this._log.info("Unable to compute EMWA value");
            return;
        }

        var value = computeEwma(fv.price, this._latest, this._alpha);

        this.setLatest(value);
    };

    private _latest: number = null;
    public get latest() { return this._latest; }
    private setLatest = (v: number) => {
        if (Math.abs(v - this._latest) > 1e-3) {
            this._latest = v;
            this.Updated.trigger();

            this._log.info("New EMWA value", this._latest);
        }
    };

    Updated = new Utils.Evt<any>();
}