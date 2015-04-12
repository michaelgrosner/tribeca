/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Statistics = require("./statistics");
import Safety = require("./safety");
import util = require("util");
import _ = require("lodash");
import Persister = require("./persister");
import Agent = require("./arbagent");
import mongodb = require('mongodb');
import FairValue = require("./fair-value");

class RegularFairValue {
    constructor(public time: Moment, public value: number) {}
}

class RegularFairValuePersister extends Persister.Persister<RegularFairValue> {
    constructor(db : Q.Promise<mongodb.Db>) {
        super(db, "rfv", Persister.timeLoader, Persister.timeSaver);
    }
}

export class PositionManager {
    private _log: Utils.Logger = Utils.log("tribeca:rfv");

    public NewTargetPosition = new Utils.Evt();

    private _latest : number = null;
    public get latestTargetPosition() : number {
        return this._latest;
    }

    private _timer : RegularTimer;
    constructor(private _persister: Persister.IPersist<RegularFairValue>,
                private _fvAgent: FairValue.FairValueEngine,
                private _data: RegularFairValue[],
                private _shortEwma : Statistics.IComputeStatistics,
                private _longEwma : Statistics.IComputeStatistics) {
        var lastTime = (this._data !== null || _.any(_data)) ? _.last(this._data).time : null;
        this._timer = new RegularTimer(this.updateEwmaValues, moment.duration(10, 'minutes'), lastTime);
    }

    private updateEwmaValues = () => {
        var fv = this._fvAgent.latestFairValue;
        if (fv === null)
            return;

        var rfv = new RegularFairValue(Utils.date(), fv.price);

        var newShort = this._shortEwma.addNewValue(fv.price);
        var newLong = this._longEwma.addNewValue(fv.price);

        var newTargetPosition = newShort - newLong;

        if (Math.abs(newTargetPosition - this._latest) > 1e-2) {
            this._latest = newTargetPosition;
            this.NewTargetPosition.trigger();
        }

        this._log("recalculated regular fair value, short:", newShort, "long:", newLong, "currentFv:", fv.price);

        this._data.push(rfv);
        this._persister.persist(rfv);
    };
}

// performs an action every duration apart, even across new instances
export class RegularTimer {
    constructor(private _action : () => void,
                private _diffTime : Duration,
                lastTime : Moment = null) {
        if (lastTime === null) {
            this.startTicking();
        }
        else {
            var timeout = lastTime.add(_diffTime).diff(Utils.date());

            if (timeout > 0) {
                setTimeout(this.startTicking, timeout)
            }
            else {
                this.startTicking();
            }
        }
    }

    private tick = () => {
        this._action();
    };

    private startTicking = () => {
        this.tick();
        setInterval(this.tick, this._diffTime.asMilliseconds());
    };
}
