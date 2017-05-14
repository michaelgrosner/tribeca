import Models = require("../share/models");
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
    constructor(private _alpha: number, initRfv: Models.RegularFairValue[]) {
      if (initRfv !== null)
        this.initialize(initRfv.map((r: Models.RegularFairValue) => r.value));
    }

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

export function computeStdev(arrFV: number[]): number {
    var n = arrFV.length;
    var sum = 0;
    arrFV.forEach((fv) => sum += fv);
    var mean = sum / n;
    var variance = 0.0;
    var v1 = 0.0;
    var v2 = 0.0;
    if (n != 1) {
        for (var i = 0; i<n; i++) {
            v1 = v1 + (arrFV[i] - mean) * (arrFV[i] - mean);
            v2 = v2 + (arrFV[i] - mean);
        }
        v2 = v2 * v2 / n;
        variance = (v1 - v2) / (n-1);
        if (variance < 0) variance = 0;
    }
    return Math.round(Math.sqrt(variance) * 100) / 100;
}

export class EmptyEWMACalculator implements Interfaces.ICalculator {
    constructor() { }
    latest: number = null;
    Updated = new Utils.Evt<any>();
}

export class ObservableEWMACalculator implements Interfaces.ICalculator {
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
        }
    };

    Updated = new Utils.Evt<any>();
}

export class ObservableSTDEVCalculator implements Interfaces.ICalculator {
    private _log = log("stdev");

    constructor(private _timeProvider: Utils.ITimeProvider, private _fv: FairValue.FairValueEngine, minutes?: number) {
        _timeProvider.setInterval(this.onTick, moment.duration(minutes, "minutes"));
        this.onTick();
    }

    private fvs: number[] = [];

    private onTick = () => {
        var fv = this._fv.latestFairValue;

        if (fv === null) {
            this._log.info("Unable to compute STDEV value");
            return;
        }

        this.fvs.push(fv.price);
        this.fvs = this.fvs.slice(0, 20);

        if (this.fvs.length < 2) return;

        var value = computeStdev(this.fvs);

        this.setLatest(value);
    };

    private _latest: number = null;
    public get latest() { return this._latest; }
    private setLatest = (v: number) => {
        if (Math.abs(v - this._latest) > 1e-3) {
            this._latest = v;
            this.Updated.trigger();
        }
    };

    Updated = new Utils.Evt<any>();
}
