import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Broker = require("./broker");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");

export class FairValueEngine {
  public FairValueChanged = new Utils.Evt<Models.FairValue>();

  private _latest: Models.FairValue = null;
  public get latestFairValue() { return this._latest; }
  public set latestFairValue(val: Models.FairValue) {
    if (this._latest != null && val != null
      && Math.abs(this._latest.price - val.price) < this._minTick) return;

    this._latest = val;
    this.FairValueChanged.trigger();
    this._publisher.publish(Models.Topics.FairValue, this._latest, true);
  }

  constructor(
    public filtration: MarketFiltration.MarketFiltration,
    private _minTick: number,
    private _timeProvider: Utils.ITimeProvider,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
    private _publisher: Publish.Publisher,
    initRfv: Models.RegularFairValue[]
  ) {
    if (initRfv !== null)
      this.latestFairValue = new Models.FairValue(initRfv[0].value, initRfv[0].time);

    filtration.FilteredMarketChanged.on(() => this.recalcFairValue(filtration.latestFilteredMarket));
    _qlParamRepo.NewParameters.on(() => this.recalcFairValue(filtration.latestFilteredMarket));
    _publisher.registerSnapshot(Models.Topics.FairValue, () => this.latestFairValue ? [this.latestFairValue] : []);
  }

  private recalcFairValue = (mkt: Models.Market) => {
    this.latestFairValue = (mkt && mkt.asks.length && mkt.bids.length)
      ? new Models.FairValue(Utils.roundNearest(
          this._qlParamRepo.latest.fvModel == Models.FairValueModel.BBO
            ? (mkt.asks[0].price + mkt.bids[0].price) / 2
            : (mkt.asks[0].price * mkt.asks[0].size + mkt.bids[0].price * mkt.bids[0].size) / (mkt.asks[0].size + mkt.bids[0].size),
          this._minTick
        ), this._timeProvider.utcNow())
      : null;
  };
}
