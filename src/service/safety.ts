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

interface QuotesEnabledCondition {
    canEnable : boolean;
}

export class SafetySettingsManager implements QuotesEnabledCondition {
    private _log: Utils.Logger = Utils.log("tribeca:qg");

    private _buys: Models.Trade[] = [];
    private _sells: Models.Trade[] = [];
    private _previousVal = 0.0;

    public SafetySettingsViolated = new Utils.Evt();
    public SafetyViolationCleared = new Utils.Evt();
    canEnable: boolean = true;

    constructor(private _repo: Interfaces.IRepository<Models.SafetySettings>,
                private _broker: Interfaces.ITradeBroker,
                private _qlParams: Interfaces.IRepository<Models.QuotingParameters>,
                private _messages: Interfaces.IPublishMessages) {
        _repo.NewParameters.on(_ => this.onNewParameters());
        _qlParams.NewParameters.on(_ => this.onNewParameters());
        _broker.Trade.on(this.onTrade);
    }

    private static isOlderThan(o: Models.Trade, settings: Models.SafetySettings) {
        var now = Utils.date();
        return Math.abs(now.diff(o.time)) > (1000 * settings.tradeRateSeconds);
    }

    private recalculateSafeties = () => {
        var settings = this._repo.latest;

        var val = this.computeQtyLimit(settings);

        if (val >= settings.tradesPerMinute && val > this._previousVal) {
            this._previousVal = val;
            this.SafetySettingsViolated.trigger();
            this.canEnable = false;

            var coolOffMinutes = momentjs.duration(settings.coolOffMinutes, 'minutes');
            var msg = util.format("Trd vol safety violated (%d), waiting %s.", val, coolOffMinutes.humanize());
            this._log(msg);
            this._messages.publish(msg);

            setTimeout(() => {
                this.canEnable = true;
                this.SafetyViolationCleared.trigger();
            }, coolOffMinutes.asMilliseconds());
        }
    };

    private computeQtyLimit = (settings: Models.SafetySettings) => {
        var orderTrades = (input: Models.Trade[], direction: number): Models.Trade[] => {
            return _.chain(input)
                    .filter(o => !SafetySettingsManager.isOlderThan(o, settings))
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
                buy.quantity -= sell.quantity;
                sell.quantity -= buy.quantity;

                if (buy.quantity < 1e-4) this._buys.pop();
                if (sell.quantity < 1e-4) this._sells.pop();
            }
            else {
                break;
            }
        }

        return this._buys.concat(this._sells).reduce((sum, t) => sum + t.quantity, 0) / this._qlParams.latest.size;
    };

    private onTrade = (ut: Models.Trade) => {
        var u = _.cloneDeep(ut);
        if (SafetySettingsManager.isOlderThan(u, this._repo.latest)) return;

        if (u.side === Models.Side.Ask) {
            this._sells.push(u);
        }
        else if (u.side === Models.Side.Bid) {
            this._buys.push(u);
        }

        this.recalculateSafeties();
    };

    private onNewParameters = () => {
        this.recalculateSafeties();
    };
}
