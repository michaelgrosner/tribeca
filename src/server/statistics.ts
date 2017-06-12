import Models = require("../share/models");
import Utils = require("./utils");
import MarketFiltration = require("./market-filtration");
import FairValue = require("./fair-value");
import QuotingParameters = require("./quoting-parameters");
import Persister = require("./persister");
import moment = require("moment");
var bindings = require('bindings')('ctubio.node');

function computeEwma(newValue: number, previous: number, periods: number): number {
    if (previous !== null) {
        const alpha = 2 / (periods + 1);
        return alpha * newValue + (1 - alpha) * previous;
    }

    return newValue;
}

export class EwmaStatisticCalculator {
    constructor(
      private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
      private _periodsAttribute: string,
      initRfv: Models.RegularFairValue[]
    ) {
      if (initRfv !== null)
        this.initialize(initRfv.map((r: Models.RegularFairValue) => r.value));
    }

    public latest: number = null;

    initialize(seedData: number[]) {
        for (var i = 0; i < seedData.length; i++)
            this.addNewValue(seedData[i]);
    }

    addNewValue(value: number): number {
        this.latest = computeEwma(value, this.latest, this._qlParamRepo.latest[this._periodsAttribute]);
        return this.latest;
    }
}

export class ObservableEWMACalculator {
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

export class ObservableSTDEVCalculator {
    private _lastBids: number[] = [];
    private _lastAsks: number[] = [];
    private _lastFV: number[] = [];
    private _lastTops: number[] = [];

    private _latest: Models.IStdev = null;
    public get latest() { return this._latest; }

    constructor(
      private _timeProvider: Utils.ITimeProvider,
      private _fv: FairValue.FairValueEngine,
      private _filteredMarkets: MarketFiltration.MarketFiltration,
      private _minTick: number,
      private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
      private persister: Persister.IPersist<Models.MarketStats>,
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

        this._latest = <Models.IStdev>bindings.computeStdevs(
          new Float64Array(this._lastFV),
          new Float64Array(this._lastTops),
          new Float64Array(this._lastBids),
          new Float64Array(this._lastAsks),
          this._qlParamRepo.latest.quotingStdevProtectionFactor,
          this._minTick
        );
    };

    private onTick = () => {
        const fv = this._fv.latestFairValue;
        const filteredMkt = this._filteredMarkets.latestFilteredMarket;
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

        this.persister.persist(new Models.MarketStats(
          fv.price,
          filteredMkt.bids[0].price,
          filteredMkt.asks[0].price,
          new Date()
        ));
        this.persister.clean(new Date(new Date().getTime() - 1000 * this._qlParamRepo.latest.quotingStdevProtectionPeriods));
    };
}