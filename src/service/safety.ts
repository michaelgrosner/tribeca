/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="broker.ts" />
/// <reference path="../common/messaging.ts" />
///<reference path="persister.ts"/>

import util = require("util");
import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Broker = require("./broker");
import Messaging = require("../common/messaging");
import moment = require('moment');
import _ = require("lodash");
import Persister = require("./persister");

export class SafetyCalculator {
    NewValue = new Utils.Evt();

    private _latest: Models.TradeSafety = null;
    public get latest() { return this._latest; }
    public set latest(val: Models.TradeSafety) {
        if (!this._latest || Math.abs(val.combined - this._latest.combined) > 1e-3) {
            this._latest = val;
            this.NewValue.trigger(this.latest);

            this._persister.persist(this.latest);
            this._publisher.publish(this.latest);
        }
    }

    private _buys: Models.Trade[] = [];
    private _sells: Models.Trade[] = [];
    private _buysS: Models.Trade[] = [];
    private _sellsS: Models.Trade[] = [];

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _repo: Interfaces.IRepository<Models.QuotingParameters>,
        private _broker: Interfaces.ITradeBroker,
        private _qlParams: Interfaces.IRepository<Models.QuotingParameters>,
        private _publisher: Messaging.IPublish<Models.TradeSafety>,
        private _persister: Persister.IPersist<Models.TradeSafety>) {
        _publisher.registerSnapshot(() => [this.latest]);
        _repo.NewParameters.on(_ => this.computeQtyLimit());
        _qlParams.NewParameters.on(_ => this.computeQtyLimit());
        _broker.Trade.on(this.onTrade);

        _timeProvider.setInterval(this.computeQtyLimit, moment.duration(1, "seconds"));
    }

    private onTrade = (ut: Models.Trade) => {
        var u = _.cloneDeep(ut);

        var older = this.isOlderThan(u, this._repo.latest);
        if (u.side === Models.Side.Ask) {
            if (older) this._sells.push(u);
            this._sellsS.unshift(u);
        }
        else if (u.side === Models.Side.Bid) {
            if (older) this._buys.push(u);
            this._buysS.unshift(u);
        }

        this.computeQtyLimit();
    };

    private isOlderThan(o: Models.Trade, settings: Models.QuotingParameters) {
        var now = this._timeProvider.utcNow();
        return Math.abs(Utils.fastDiff(now, o.time)) > (1000 * settings.tradeRateSeconds);
    }

    private computeQtyLimit = () => {
        var settings = this._repo.latest;

        var buyS = 0;
        var sellS = 0;
        var buySq = 0;
        var sellSq = 0;
        for (var bSi = 0; bSi < this._buysS.length; bSi++) {
          if (buySq>settings.width) break;
          buyS = this._buysS[bSi].price / this._buysS[bSi].quantity;
          buySq += this._buysS[bSi].quantity;
        }
        if (bSi) {
          buyS /= bSi;
          this._buysS.slice(0,bSi);
        }

        for (var sSi = 0; sSi < this._sellsS.length; sSi++) {
          if (sellSq>settings.width) break;
          sellS = this._sellsS[sSi].price / this._sellsS[sSi].quantity;
          sellSq += this._sellsS[sSi].quantity;
        }
        if (sSi) {
          sellS /= sSi;
          this._sellsS.slice(0,sSi);
        }

        var orderTrades = (input: Models.Trade[], direction: number): Models.Trade[]=> {
            return _.chain(input)
                .filter(o => !this.isOlderThan(o, settings))
                .sortBy((t: Models.Trade) => direction * t.price)
                .value();
        };

        this._buys = orderTrades(this._buys, -1);
        this._sells = orderTrades(this._sells, 1);

        // don't count good trades against safety
        while (_.size(this._buys) > 0 && _.size(this._sells) > 0) {
            var sell = _.last(this._sells);
            var buy = _.last(this._buys);
            if (sell.price >= buy.price) {

                var sellQty = sell.quantity;
                var buyQty = buy.quantity;

                buy.quantity -= sellQty;
                sell.quantity -= buyQty;

                if (buy.quantity < 1e-4) this._buys.pop();
                if (sell.quantity < 1e-4) this._sells.pop();
            }
            else {
                break;
            }
        }

        var computeSafety = (t: Models.Trade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / this._qlParams.latest.size;

        this.latest = new Models.TradeSafety(computeSafety(this._buys) || 0, computeSafety(this._sells) || 0,
            buyS, sellS, computeSafety(this._buys.concat(this._sells)) || 0, this._timeProvider.utcNow());
    };
}