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
      && Math.abs(this._latest.price - val.price) < this._details.minTickIncrement) return;

    this._latest = val;
    this.FairValueChanged.trigger();
    this._fvPublisher.publish(this._latest);
  }

  constructor(
    private _details: Broker.ExchangeBroker,
    private _timeProvider: Utils.ITimeProvider,
    private _filtration: MarketFiltration.MarketFiltration,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
    private _fvPublisher: Publish.IPublish<Models.FairValue>
  ) {
    _qlParamRepo.NewParameters.on(() => this.recalcFairValue(_filtration.latestFilteredMarket));
    _filtration.FilteredMarketChanged.on(() => this.recalcFairValue(_filtration.latestFilteredMarket));
    _fvPublisher.registerSnapshot(() => this.latestFairValue ? [this.latestFairValue] : []);
  }

  private recalcFairValue = (mkt: Models.Market) => {
    this.latestFairValue = (mkt && mkt.asks.length && mkt.bids.length)
      ? new Models.FairValue(Utils.roundNearest(
          this._qlParamRepo.latest.fvModel == Models.FairValueModel.BBO
            ? (mkt.asks[0].price + mkt.bids[0].price) / 2
            : (mkt.asks[0].price * mkt.asks[0].size + mkt.bids[0].price * mkt.bids[0].size) / (mkt.asks[0].size + mkt.bids[0].size),
          this._details.minTickIncrement
        ), this._timeProvider.utcNow())
      : null;
  };
}
