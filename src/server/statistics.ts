import Models = require("../share/models");
import Utils = require("./utils");
import moment = require("moment");

function computeEwma(newValue: number, previous: number, periods: number): number {
  if (previous !== null) {
      const alpha = 2 / (periods + 1);
      return alpha * newValue + (1 - alpha) * previous;
  }

  return newValue;
}

export class EWMAProtectionCalculator {
  constructor(
    private _fvEngine,
    private _qpRepo,
    private _evUp
  ) {
    setInterval(this.onTick, moment.duration(1, "minutes"));
  }

  private onTick = () => {
    var fv = this._fvEngine();
    if (!fv) {
      console.warn(new Date().toISOString().slice(11, -1), 'ewma', 'Unable to compute value');
      return;
    }

    this.setLatest(computeEwma(fv, this._latest, this._qpRepo().quotingEwmaProtectionPeridos));
  };

  private _latest: number = null;
  public get latest() { return this._latest; }
  private setLatest = (v: number) => {
    this._latest = v;
    this._evUp('EWMAProtectionCalculator');
  };
}

export class STDEVProtectionCalculator {
    private _lastBids: number[] = [];
    private _lastAsks: number[] = [];
    private _lastFV: number[] = [];
    private _lastTops: number[] = [];

    private _latest: Models.IStdev = null;
    public get latest() { return this._latest; }

    constructor(
      private _fvEngine,
      private _mgFilter,
      private _qpRepo,
      private _dbInsert,
      private _computeStdevs,
      initMkt: Models.MarketStats[]
    ) {
      if (initMkt !== null) {
        this.initialize(
          initMkt.map((r: Models.MarketStats) => r.fv),
          initMkt.map((r: Models.MarketStats) => r.bid),
          initMkt.map((r: Models.MarketStats) => r.ask)
        );
      }
      setInterval(this.onTick, moment.duration(1, "seconds"));
    }

    private initialize(rfv: number[], mktBids: number[], mktAsks: number[]) {
      const params = this._qpRepo();
      for (let i = 0; i<mktBids.length||i<mktAsks.length;i++)
        if (mktBids[i] && mktAsks[i]) this._lastTops.push(mktBids[i], mktAsks[i]);
      this._lastFV = rfv.slice(-params.quotingStdevProtectionPeriods);
      this._lastTops = this._lastTops.slice(-params.quotingStdevProtectionPeriods * 2);
      this._lastBids = mktBids.slice(-params.quotingStdevProtectionPeriods);
      this._lastAsks = mktAsks.slice(-params.quotingStdevProtectionPeriods);

      this.onSave();
    };

    private onSave = () => {
      if (this._lastFV.length < 2 || this._lastTops.length < 2 || this._lastBids.length < 2 || this._lastAsks.length < 2) return;

      this._latest = <Models.IStdev>this._computeStdevs(
        new Float64Array(this._lastFV),
        new Float64Array(this._lastTops),
        new Float64Array(this._lastBids),
        new Float64Array(this._lastAsks),
        this._qpRepo().quotingStdevProtectionFactor
      );
    };

    private onTick = () => {
        const filteredMkt = this._mgFilter();
        const fv = this._fvEngine();
        if (!fv || filteredMkt == null || !filteredMkt.bids.length || !filteredMkt.asks.length) {
            console.warn(new Date().toISOString().slice(11, -1), 'stdev', 'Unable to compute value');
            return;
        }
        const params = this._qpRepo();
        this._lastFV.push(fv);
        this._lastTops.push(filteredMkt.bids[0].price, filteredMkt.asks[0].price);
        this._lastBids.push(filteredMkt.bids[0].price);
        this._lastAsks.push(filteredMkt.asks[0].price);
        this._lastFV = this._lastFV.slice(-params.quotingStdevProtectionPeriods);
        this._lastTops = this._lastTops.slice(-params.quotingStdevProtectionPeriods * 2);
        this._lastBids = this._lastBids.slice(-params.quotingStdevProtectionPeriods);
        this._lastAsks = this._lastAsks.slice(-params.quotingStdevProtectionPeriods);

        this.onSave();

        this._dbInsert(Models.Topics.MarketData, new Models.MarketStats(fv, filteredMkt.bids[0].price, filteredMkt.asks[0].price, new Date().getTime()), false, undefined, new Date().getTime() - 1000 * params.quotingStdevProtectionPeriods);
    };
}
