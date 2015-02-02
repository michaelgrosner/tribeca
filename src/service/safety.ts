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

export class SafetySettingsRepository extends Interfaces.Repository<Models.SafetySettings> {
    constructor(pub : Messaging.IPublish<Models.SafetySettings>,
                rec : Messaging.IReceive<Models.SafetySettings>) {
        super("ssr",
            (s : Models.SafetySettings) => s.tradesPerMinute > 0,
            (a : Models.SafetySettings, b : Models.SafetySettings) => Math.abs(a.tradesPerMinute - b.tradesPerMinute) >= 0,
            new Models.SafetySettings(2), rec, pub
        );
    }
}

export class SafetySettingsManager {
    private _log : Utils.Logger = Utils.log("tribeca:qg");

    private _trades : Models.Trade[] = [];
    public SafetySettingsViolated = new Utils.Evt();

    constructor(private _repo : SafetySettingsRepository,
                private _broker : Interfaces.IOrderBroker,
                private _messages : Broker.MessagesPubisher) {
        _repo.NewParameters.on(() => this.onNewParameters);
        _broker.Trade.on(this.onOrderUpdate);
    }

    private static isOlderThanOneMinute(o : Models.Trade) {
        var now = Utils.date();
        return Math.abs(now.diff(o.time)) > 1000*60;
    }

    private recalculateSafeties = () => {
        this._trades = this._trades.filter(o => !SafetySettingsManager.isOlderThanOneMinute(o));

        if (this._trades.length >= this._repo.latest.tradesPerMinute) {
            var msg = util.format("NTrades/Sec safety setting violated! %d trades", this._trades.length);
            this._log(msg);
            this._messages.publish(msg);
            this.SafetySettingsViolated.trigger();
        }
    };

    private onOrderUpdate = (u : Models.Trade) => {
        if (SafetySettingsManager.isOlderThanOneMinute(u)) return;
        this._trades.push(u);

        this.recalculateSafeties();
    };

    private onNewParameters = () => {
        this.recalculateSafeties();
    };
}