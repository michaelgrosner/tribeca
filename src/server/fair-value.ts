import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import moment = require("moment");

export class FairValueEngine {
    public FairValueChanged = new Utils.Evt<Models.FairValue>();

    private _latest: Models.FairValue = null;
    public get latestFairValue() { return this._latest; }
    public set latestFairValue(val: Models.FairValue) {
        if (this._latest != null && val != null
            && Math.abs(this._latest.price - val.price) < 2e-2) return;

        this._latest = val;
        this.FairValueChanged.trigger();
        this._fvPublisher.publish(this._latest);
    }

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _filtration: MarketFiltration.MarketFiltration,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        private _fvPublisher: Publish.IPublish<Models.FairValue>) {
        _qlParamRepo.NewParameters.on(() => this.recalcFairValue(_timeProvider.utcNow()));
        _filtration.FilteredMarketChanged.on(() => this.recalcFairValue(Utils.timeOrDefault(_filtration.latestFilteredMarket, _timeProvider)));
        _fvPublisher.registerSnapshot(() => this.latestFairValue === null ? [] : [this.latestFairValue]);
    }

    private static ComputeFVUnrounded(ask: Models.MarketSide, bid: Models.MarketSide, model: Models.FairValueModel) {
        switch (model) {
            case Models.FairValueModel.BBO:
                return (ask.price + bid.price) / 2;
            case Models.FairValueModel.wBBO:
                return (ask.price * ask.size + bid.price * bid.size) / (ask.size + bid.size);
        }
    }

    private static ComputeFV(ask: Models.MarketSide, bid: Models.MarketSide, model: Models.FairValueModel) {
        var unrounded = FairValueEngine.ComputeFVUnrounded(ask, bid, model);
        return Utils.roundFloat(unrounded);
    }

    private recalcFairValue = (t: moment.Moment) => {
        var mkt = this._filtration.latestFilteredMarket;

        if (mkt == null) {
            this.latestFairValue = null;
            return;
        }

        var bid = mkt.bids;
        var ask = mkt.asks;

        if (!ask.length || !bid.length) {
            this.latestFairValue = null;
            return;
        }

        var fv = new Models.FairValue(FairValueEngine.ComputeFV(ask[0], bid[0], this._qlParamRepo.latest.fvModel), t);
        this.latestFairValue = fv;
    };
}
