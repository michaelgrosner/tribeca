import Models = require("../share/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import MarketFiltration = require("./market-filtration");
import FairValue = require("./fair-value");
import QuotingParameters = require("./quoting-parameters");
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

export class ObservableSTDEVCalculator implements Interfaces.ISilentCalculator {
    private _log = log("stdev");
    private _interval = null;
    private _lastBids: number[] = [];
    private _lastAsks: number[] = [];
    private _lastFV: number[] = [];

    private _latest: Models.IStdev = null;
    public get latest() { return this._latest; }

    constructor(
      private _timeProvider: Utils.ITimeProvider,
      private _fv: FairValue.FairValueEngine,
      private _filteredMarkets: MarketFiltration.MarketFiltration,
      private _minTick: number,
      private _qlParamRepo: QuotingParameters.QuotingParametersRepository
    ) {
        _qlParamRepo.NewParameters.on(this.onNewParameters);
        this.onNewParameters();
    }

    private onNewParameters = () => {
      if (this._interval) clearInterval(this._interval);
      if (this._qlParamRepo.latest.stdevProtection !== Models.STDEV.Off && this._qlParamRepo.latest.widthStdevPeriods)
        this._interval = this._timeProvider.setInterval(this.onTick, moment.duration(1, "seconds"));
    };

    private onTick = () => {
        if (this._qlParamRepo.latest.stdevProtection === Models.STDEV.Off || !this._qlParamRepo.latest.widthStdevPeriods) return;

        const fv = this._fv.latestFairValue;
        const filteredMkt = this._filteredMarkets.latestFilteredMarket;
        if (fv === null || filteredMkt == null || !filteredMkt.bids.length || !filteredMkt.asks.length) {
            this._log.info("Unable to compute STDEV value");
            return;
        }

        this._lastFV.push(fv.price);
        this._lastBids.push(filteredMkt.bids[0].price);
        this._lastAsks.push(filteredMkt.asks[0].price);
        this._lastFV = this._lastFV.slice(-this._qlParamRepo.latest.widthStdevPeriods);
        this._lastBids = this._lastBids.slice(-this._qlParamRepo.latest.widthStdevPeriods);
        this._lastAsks = this._lastAsks.slice(-this._qlParamRepo.latest.widthStdevPeriods);

        if (this._lastFV.length < 2 ||this._lastBids.length < 2 || this._lastAsks.length < 2) return;

        this._latest = <Models.IStdev>{
          fv: computeStdev(this._lastFV),
          bid: computeStdev(this._lastBids),
          ask: computeStdev(this._lastAsks)
        };
    };
}