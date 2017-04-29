/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="quoter.ts"/>
/// <reference path="safety.ts"/>
/// <reference path="persister.ts"/>
/// <reference path="statistics.ts"/>
/// <reference path="active-state.ts"/>
/// <reference path="market-filtration.ts"/>
/// <reference path="quoting-parameters.ts"/>

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");
import util = require("util");
import _ = require("lodash");
import Persister = require("./persister");
import Statistics = require("./statistics");
import Active = require("./active-state");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import moment = require("moment");

export class FairValueEngine {
    public FairValueChanged = new Utils.Evt<Models.FairValue>();

    private _latest: Models.FairValue = null;
    public get latestFairValue() { return this._latest; }
    public set latestFairValue(val: Models.FairValue) {
        if (this._latest != null
            && val != null
            && Math.abs(this._latest.price - val.price) < this._details.minTickIncrement) return;

        this._latest = val;
        this.FairValueChanged.trigger();
        this._fvPublisher.publish(this._latest);

        if (this._latest !== null)
            this._fvPersister.persist(this._latest);
    }

    constructor(
        private _details: Interfaces.IBroker,
        private _timeProvider: Utils.ITimeProvider,
        private _filtration: MarketFiltration.MarketFiltration,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository, // should not co-mingle these settings
        private _fvPublisher: Messaging.IPublish<Models.FairValue>,
        private _fvPersister: Persister.IPersist<Models.FairValue>) {
        _qlParamRepo.NewParameters.on(() => this.recalcFairValue(_timeProvider.utcNow()));
        _filtration.FilteredMarketChanged.on(() => this.recalcFairValue(Utils.timeOrDefault(_filtration.latestFilteredMarket, _timeProvider)));
        _fvPublisher.registerSnapshot(() => this.latestFairValue === null ? [] : [this.latestFairValue]);
    }

    private static ComputeFVUnrounded(ask: Models.MarketSide, bid: Models.MarketSide, model: Models.FairValueModel) {
        switch (model) {
            case Models.FairValueModel.BBO:
                return (ask.price + bid.price) / 2.0;
            case Models.FairValueModel.wBBO:
                return (ask.price * ask.size + bid.price * bid.size) / (ask.size + bid.size);
        }
    }

    private ComputeFV(ask: Models.MarketSide, bid: Models.MarketSide, model: Models.FairValueModel) {
        var unrounded = FairValueEngine.ComputeFVUnrounded(ask, bid, model);
        return Utils.roundNearest(unrounded, this._details.minTickIncrement);
    }

    private recalcFairValue = (t: Date) => {
        var mkt = this._filtration.latestFilteredMarket;

        if (mkt == null) {
            this.latestFairValue = null;
            return;
        }

        var bid = mkt.bids;
        var ask = mkt.asks;

        if (ask.length < 1 || bid.length < 1) {
            this.latestFairValue = null;
            return;
        }

        var fv = new Models.FairValue(this.ComputeFV(ask[0], bid[0], this._qlParamRepo.latest.fvModel), t);
        this.latestFairValue = fv;
    };
}