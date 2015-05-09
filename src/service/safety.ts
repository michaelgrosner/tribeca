/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="broker.ts" />
/// <reference path="../common/messaging.ts" />

import util = require("util");
import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Broker = require("./broker");
import Messaging = require("../common/messaging");
import momentjs = require('moment');
import _ = require("lodash");
import Persister = require("./persister");

export class SafetySettingsRepository extends Interfaces.Repository<Models.SafetySettings> {
    constructor(pub: Messaging.IPublish<Models.SafetySettings>,
        rec: Messaging.IReceive<Models.SafetySettings>,
        initParam: Models.SafetySettings) {
        super("ssr",
            (s: Models.SafetySettings) => s.tradesPerMinute > 0 && s.coolOffMinutes > 0,
            (a: Models.SafetySettings, b: Models.SafetySettings) => !_.isEqual(a, b),
            initParam, rec, pub
            );
    }
}

export class SafetyCalculator {
    private _log: Utils.Logger = Utils.log("tribeca:sc");

    NewValue = new Utils.Evt();

    private _latest: Models.TradeSafety = null;
    public get latest() { return this._latest; }
    public set latest(val: Models.TradeSafety) {
        if (!this._latest || Math.abs(val.combined - this._latest.combined) > 1e-3) {
            this._latest = val;
            this.NewValue.trigger(this.latest);

            this._persister.persist(this.latest);
            this._publisher.publish(this.latest);

            this._log("New safety value: %j", this.latest);
        }
    }

    private _buys: Models.Trade[] = [];
    private _sells: Models.Trade[] = [];

    constructor(private _repo: Interfaces.IRepository<Models.SafetySettings>,
        private _broker: Interfaces.ITradeBroker,
        private _qlParams: Interfaces.IRepository<Models.QuotingParameters>,
        private _publisher: Messaging.IPublish<Models.TradeSafety>,
        private _persister: Persister.IPersist<Models.TradeSafety>) {
        _publisher.registerSnapshot(() => [this.latest]);
        _repo.NewParameters.on(_ => this.computeQtyLimit());
        _qlParams.NewParameters.on(_ => this.computeQtyLimit());
        _broker.Trade.on(this.onTrade);
        
        setInterval(this.computeQtyLimit, 1000);
    }

    private onTrade = (ut: Models.Trade) => {
        var u = _.cloneDeep(ut);
        if (SafetyCalculator.isOlderThan(u, this._repo.latest)) return;

        if (u.side === Models.Side.Ask) {
            this._sells.push(u);
        }
        else if (u.side === Models.Side.Bid) {
            this._buys.push(u);
        }

        this.computeQtyLimit();
    };

    private static isOlderThan(o: Models.Trade, settings: Models.SafetySettings) {
        var now = Utils.date();
        return Math.abs(now.diff(o.time)) > (1000 * settings.tradeRateSeconds);
    }

    private computeQtyLimit = () => {
        var settings = this._repo.latest;

        var orderTrades = (input: Models.Trade[], direction: number): Models.Trade[]=> {
            return _.chain(input)
                .filter(o => !SafetyCalculator.isOlderThan(o, settings))
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

        this.latest = new Models.TradeSafety(computeSafety(this._buys), computeSafety(this._sells),
            computeSafety(this._buys.concat(this._sells)), Utils.date());
    };
}

export interface ISafetyManager {
    SafetySettingsViolated: Utils.Evt<any>;
    SafetyViolationCleared: Utils.Evt<any>;
    canEnable: boolean;
}

export class SafetySettingsManager implements ISafetyManager {
    private _log: Utils.Logger = Utils.log("tribeca:qg");

    private _previousVal = 0.0;

    public SafetySettingsViolated = new Utils.Evt<any>();
    public SafetyViolationCleared = new Utils.Evt<any>();
    canEnable: boolean = true;

    constructor(private _repo: Interfaces.IRepository<Models.SafetySettings>,
        private _safetyCalculator: SafetyCalculator,
        private _messages: Interfaces.IPublishMessages) {
        _safetyCalculator.NewValue.on(() => this.recalculateSafeties());
        _repo.NewParameters.on(() => this.recalculateSafeties());
    }

    private recalculateSafeties = () => {
        var val = this._safetyCalculator.latest === null ? 0 : this._safetyCalculator.latest.combined;
        var settings = this._repo.latest;

        if (val >= settings.tradesPerMinute && val > this._previousVal && this.canEnable) {
            this._previousVal = val;
            this.canEnable = false;
            this.SafetySettingsViolated.trigger();

            var coolOffMinutes = momentjs.duration(settings.coolOffMinutes, 'minutes');
            var msg = util.format("Trd vol safety violated (%d), waiting %s.", Utils.roundFloat(val), coolOffMinutes.humanize());
            this._log(msg);
            this._messages.publish(msg);

            setTimeout(() => {
                this._previousVal = 0.0;
                this.canEnable = true;
                this.SafetyViolationCleared.trigger();
            }, coolOffMinutes.asMilliseconds());
        }
    };
}
