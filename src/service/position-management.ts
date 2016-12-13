/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="statistics.ts"/>
/// <reference path="persister.ts"/>
/// <reference path="fair-value.ts"/>
/// <reference path="interfaces.ts"/>
/// <reference path="quoting-parameters.ts"/>

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Statistics = require("./statistics");
import util = require("util");
import _ = require("lodash");
import Persister = require("./persister");
import mongodb = require('mongodb');
import FairValue = require("./fair-value");
import moment = require("moment");
import Interfaces = require("./interfaces");
import QuotingParameters = require("./quoting-parameters");

export class PositionManager {
    private _log = Utils.log("rfv");

    public NewTargetPosition = new Utils.Evt();

    private _latest: number = null;
    public get latestTargetPosition(): number {
        return this._latest;
    }

    private _timer: RegularTimer;
    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _persister: Persister.IPersist<Models.RegularFairValue>,
        private _fvAgent: FairValue.FairValueEngine,
        private _data: Models.RegularFairValue[],
        private _shortEwma: Statistics.IComputeStatistics,
        private _longEwma: Statistics.IComputeStatistics) {
        var lastTime = (this._data !== null && _.some(_data)) ? _.last(this._data).time : null;
        this._timer = new RegularTimer(_timeProvider, this.updateEwmaValues, moment.duration(1, 'hours'), lastTime);
    }

    private updateEwmaValues = () => {
        var fv = this._fvAgent.latestFairValue;
        if (fv === null)
            return;

        var rfv = new Models.RegularFairValue(this._timeProvider.utcNow(), fv.price);

        var newShort = this._shortEwma.addNewValue(fv.price);
        var newLong = this._longEwma.addNewValue(fv.price);

        var newTargetPosition = (newShort - newLong) / 2.0;

        if (newTargetPosition > 1) newTargetPosition = 1;
        if (newTargetPosition < -1) newTargetPosition = -1;

        if (Math.abs(newTargetPosition - this._latest) > 1e-2) {
            this._latest = newTargetPosition;
            this.NewTargetPosition.trigger();
        }

        this._log.info("recalculated regular fair value, short:", Utils.roundFloat(newShort), "long:",
            Utils.roundFloat(newLong), "target:", Utils.roundFloat(this._latest), "currentFv:", Utils.roundFloat(fv.price));

        this._data.push(rfv);
        this._persister.persist(rfv);
    };
}

export class TargetBasePositionManager {
    private _log = Utils.log("positionmanager");

    public NewTargetPosition = new Utils.Evt();

    private _latest: Models.TargetBasePositionValue = null;
    public get latestTargetPosition(): Models.TargetBasePositionValue {
        return this._latest;
    }

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _positionManager: PositionManager,
        private _params: QuotingParameters.QuotingParametersRepository,
        private _positionBroker: Interfaces.IPositionBroker,
        private _wrapped: Messaging.IPublish<Models.TargetBasePositionValue>,
        private _persister: Persister.IPersist<Models.TargetBasePositionValue>) {
        _wrapped.registerSnapshot(() => [this._latest]);
        _positionBroker.NewReport.on(r => this.recomputeTargetPosition());
        _params.NewParameters.on(() => this.recomputeTargetPosition());
        _positionManager.NewTargetPosition.on(() => this.recomputeTargetPosition());
    }

    private recomputeTargetPosition = () => {
        var latestPosition = this._positionBroker.latestReport;
        var params = this._params.latest;

        if (params === null || latestPosition === null)
            return;

        var targetBasePosition: number = params.targetBasePosition;
        if (params.autoPositionMode === Models.AutoPositionMode.EwmaBasic) {
            targetBasePosition = ((1 + this._positionManager.latestTargetPosition) / 2.0) * latestPosition.value;
        }

        if (this._latest === null || Math.abs(this._latest.data - targetBasePosition) > 0.05) {
            this._latest = new Models.TargetBasePositionValue(targetBasePosition, this._timeProvider.utcNow());
            this.NewTargetPosition.trigger();

            this._wrapped.publish(this.latestTargetPosition);
            this._persister.persist(this.latestTargetPosition);

            this._log.info("recalculated target base position:", Utils.roundFloat(this.latestTargetPosition.data));
        }
    };
}

// performs an action every duration apart, even across new instances
export class RegularTimer {
    constructor(
        private _timeProvider : Utils.ITimeProvider,
        private _action: () => void,
        private _diffTime: moment.Duration,
        lastTime: moment.Moment) {
        if (!moment.isMoment(lastTime)) {
            this.startTicking();
        }
        else {
            var timeout = lastTime.add(_diffTime).diff(_timeProvider.utcNow());

            if (timeout > 0) {
                _timeProvider.setTimeout(this.startTicking, moment.duration(timeout));
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
        this._timeProvider.setInterval(this.tick, moment.duration(this._diffTime.asMilliseconds()));
    };
}
