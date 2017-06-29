import Models = require("../share/models");
import Utils = require("./utils");
import MarketFiltration = require("./market-filtration");
import FairValue = require("./fair-value");
import QuotingParameters = require("./quoting-parameters");
import moment = require("moment");

function computeEwma(newValue: number, previous: number, periods: number): number {
    if (previous !== null) {
        const alpha = 2 / (periods + 1);
        return alpha * newValue + (1 - alpha) * previous;
    }

    return newValue;
}

export class EWMATargetPositionCalculator {
    constructor(
      private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
      initRfv: Models.RegularFairValue[]
    ) {
      if (initRfv !== null)
        this.initialize(initRfv.map((r: Models.RegularFairValue) => r.value));
    }

    private latestShort: number = null;
    private latestMedium: number = null;
    private latestLong: number = null;

    initialize(seedData: number[]) {
        for (var i = 0; i < seedData.length; i++) {
          this.addNewShortValue(seedData[i]);
          this.addNewMediumValue(seedData[i]);
          this.addNewLongValue(seedData[i]);
        }
    }

    addNewShortValue(value: number): number {
        this.latestShort = computeEwma(value, this.latestShort, this._qlParamRepo.latest.shortEwmaPeridos);
        return this.latestShort;
    }
    addNewMediumValue(value: number): number {
        this.latestMedium = computeEwma(value, this.latestMedium, this._qlParamRepo.latest.mediumEwmaPeridos);
        return this.latestMedium;
    }
    addNewLongValue(value: number): number {
        this.latestLong = computeEwma(value, this.latestLong, this._qlParamRepo.latest.longEwmaPeridos);
        return this.latestLong;
    }
}

export class EWMAProtectionCalculator {
    constructor(
      private _timeProvider: Utils.ITimeProvider,
      private _fv: FairValue.FairValueEngine,
      private _qlParamRepo: QuotingParameters.QuotingParametersRepository
    ) {
        _timeProvider.setInterval(this.onTick, moment.duration(1, "minutes"));
    }

    private onTick = () => {
        var fv = this._fv.latestFairValue;

        if (fv === null) {
            console.warn(new Date().toISOString().slice(11, -1), 'ewma', 'Unable to compute value');
            return;
        }

        var value = computeEwma(fv.price, this._latest, this._qlParamRepo.latest.quotingEwmaProtectionPeridos);

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

export class STDEVProtectionCalculator {
    private _lastBids: number[] = [];
    private _lastAsks: number[] = [];
    private _lastFV: number[] = [];
    private _lastTops: number[] = [];

    private _latest: Models.IStdev = null;
    public get latest() { return this._latest; }

    constructor(
      private _timeProvider: Utils.ITimeProvider,
      private _fv: FairValue.FairValueEngine,
      private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
      private _sqlite,
      private _computeStdevs,
      initMkt: Models.MarketStats[]
    ) {
        _timeProvider.setInterval(this.onTick, moment.duration(1, "seconds"));
        if (initMkt !== null) {
          this.initialize(
            initMkt.map((r: Models.MarketStats) => r.fv),
            initMkt.map((r: Models.MarketStats) => r.bid),
            initMkt.map((r: Models.MarketStats) => r.ask)
          );
        }
    }

    private initialize(rfv: number[], mktBids: number[], mktAsks: number[]) {
        for (let i = 0; i<mktBids.length||i<mktAsks.length;i++)
          if (mktBids[i] && mktAsks[i]) this._lastTops.push(mktBids[i], mktAsks[i]);
        this._lastFV = rfv.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods);
        this._lastTops = this._lastTops.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods * 2);
        this._lastBids = mktBids.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods);
        this._lastAsks = mktAsks.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods);

        this.onSave();
    };

    private onSave = () => {
        if (this._lastFV.length < 2 || this._lastTops.length < 2 || this._lastBids.length < 2 || this._lastAsks.length < 2) return;

        this._latest = <Models.IStdev>this._computeStdevs(
          new Float64Array(this._lastFV),
          new Float64Array(this._lastTops),
          new Float64Array(this._lastBids),
          new Float64Array(this._lastAsks),
          this._qlParamRepo.latest.quotingStdevProtectionFactor
        );
    };

    private onTick = () => {
        const fv = this._fv.latestFairValue;
        const filteredMkt = this._fv.filtration.latestFilteredMarket;
        if (fv === null || filteredMkt == null || !filteredMkt.bids.length || !filteredMkt.asks.length) {
            console.warn(new Date().toISOString().slice(11, -1), 'stdev', 'Unable to compute value');
            return;
        }

        this._lastFV.push(fv.price);
        this._lastTops.push(filteredMkt.bids[0].price, filteredMkt.asks[0].price);
        this._lastBids.push(filteredMkt.bids[0].price);
        this._lastAsks.push(filteredMkt.asks[0].price);
        this._lastFV = this._lastFV.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods);
        this._lastTops = this._lastTops.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods * 2);
        this._lastBids = this._lastBids.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods);
        this._lastAsks = this._lastAsks.slice(-this._qlParamRepo.latest.quotingStdevProtectionPeriods);

        this.onSave();

        this._sqlite.insert(Models.Topics.MarketData, new Models.MarketStats(
          fv.price,
          filteredMkt.bids[0].price,
          filteredMkt.asks[0].price,
          new Date()
        ), false, undefined, new Date().getTime() - 1000 * this._qlParamRepo.latest.quotingStdevProtectionPeriods);
    };
}