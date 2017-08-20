import Models = require("../share/models");
import Utils = require("./utils");
import Broker = require("./broker");

export class FairValueEngine {
  private _latest: Models.FairValue = null;
  public get latestFairValue() { return this._latest; }
  public set latestFairValue(val: Models.FairValue) {
    if (!val.price || (this._latest != null && val != null
      && Math.abs(this._latest.price - val.price) < this._minTick)) return;

    this._latest = val;
    this._evUp('FairValue');
    this._uiSend(Models.Topics.FairValue, this._latest, true);
  }

  constructor(
    private _mgFilter,
    private _minTick: number,
    private _qpRepo,
    private _uiSnap,
    private _uiSend,
    private _evOn,
    private _evUp,
    initRfv: Models.RegularFairValue[]
  ) {
    if (initRfv !== null && initRfv.length)
      this.latestFairValue = new Models.FairValue(initRfv[0].fairValue);

    this._evOn('FilteredMarket', this.recalcFairValue);
    this._evOn('QuotingParameters', this.recalcFairValue);
    _uiSnap(Models.Topics.FairValue, () => [this.latestFairValue]);
  }

  private recalcFairValue = () => {
    const mkt: Models.Market = this._mgFilter();
    if (mkt && mkt.asks.length && mkt.bids.length)
      this.latestFairValue = new Models.FairValue(Utils.roundNearest(
        this._qpRepo().fvModel == Models.FairValueModel.BBO
          ? (mkt.asks[0].price + mkt.bids[0].price) / 2
          : (mkt.asks[0].price * mkt.asks[0].size + mkt.bids[0].price * mkt.bids[0].size) / (mkt.asks[0].size + mkt.bids[0].size),
        this._minTick
      ));
  };
}
