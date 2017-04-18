import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Statistics = require("./statistics");
import _ = require("lodash");
import Persister = require("./persister");
import FairValue = require("./fair-value");
import moment = require("moment");
import Interfaces = require("./interfaces");
import QuotingParameters = require("./quoting-parameters");

export class PositionManager {
    private _log = Utils.log("rfv");

    private newQuote: number = null;
    private newShort: number = null;
    private newLong: number = null;
    private fairValue: number = null;
    public set quoteEwma(quoteEwma: number) {
      var fv = this._fvAgent.latestFairValue;
      if (fv === null || quoteEwma === null) return;
      this.fairValue = fv.price;
      this.newQuote = quoteEwma;
      this._ewmaPublisher.publish(new Models.EWMAChart(
        Utils.roundFloat(quoteEwma),
        this.newShort?Utils.roundFloat(this.newShort):null,
        this.newLong?Utils.roundFloat(this.newLong):null,
        Utils.roundFloat(fv.price),
        this._timeProvider.utcNow()
      ));
    }

    public NewTargetPosition = new Utils.Evt();

    private _latest: number = null;
    public get latestTargetPosition(): number {
        return this._latest;
    }

    private _data: Models.RegularFairValue[] = [];

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _fvAgent: FairValue.FairValueEngine,
        private _shortEwma: Statistics.IComputeStatistics,
        private _longEwma: Statistics.IComputeStatistics,
        private _ewmaPublisher : Publish.IPublish<Models.EWMAChart>) {
        _ewmaPublisher.registerSnapshot(() => [this.fairValue?new Models.EWMAChart(
          this.newQuote?Utils.roundFloat(this.newQuote):null,
          this.newShort?Utils.roundFloat(this.newShort):null,
          this.newLong?Utils.roundFloat(this.newLong):null,
          this.fairValue?Utils.roundFloat(this.fairValue):null,
          this._timeProvider.utcNow()
        ):null]);
        this._timeProvider.setInterval(this.updateEwmaValues, moment.duration(10, 'minutes'));
        this._timeProvider.setTimeout(this.updateEwmaValues, moment.duration(1, 'minutes'));
    }

    private updateEwmaValues = () => {
        var fv = this._fvAgent.latestFairValue;
        if (fv === null)
            return;
        this.fairValue = fv.price;

        var rfv = new Models.RegularFairValue(this._timeProvider.utcNow(), fv.price);

        this.newShort = this._shortEwma.addNewValue(fv.price);
        this.newLong = this._longEwma.addNewValue(fv.price);
        var newTargetPosition = ((this.newShort * 100 / this.newLong) - 100) * 5;

        if (newTargetPosition > 1) newTargetPosition = 1;
        if (newTargetPosition < -1) newTargetPosition = -1;

        if (Math.abs(newTargetPosition - this._latest) > 1e-2) {
            this._latest = newTargetPosition;
            this.NewTargetPosition.trigger();
        }

        this._ewmaPublisher.publish(new Models.EWMAChart(
          this.newQuote?Utils.roundFloat(this.newQuote):null,
          Utils.roundFloat(this.newShort),
          Utils.roundFloat(this.newLong),
          Utils.roundFloat(fv.price),
          this._timeProvider.utcNow()
        ));

        this._log.info("recalculated regular fair value, short:", Utils.roundFloat(this.newShort), "long:",
            Utils.roundFloat(this.newLong), "target:", Utils.roundFloat(this._latest), "currentFv:", Utils.roundFloat(fv.price));

        this._data.push(rfv);
        this._data = _.takeRight(this._data, 7);
    };
}

export class TargetBasePositionManager {
    private _log = Utils.log("positionmanager");

    public NewTargetPosition = new Utils.Evt();

    public sideAPR: string[] = [];

    private _latest: Models.TargetBasePositionValue = null;
    public get latestTargetPosition(): Models.TargetBasePositionValue {
        return this._latest;
    }

    public set quoteEWMA(quoteEwma: number) {
      this._positionManager.quoteEwma = quoteEwma;
    }

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _positionManager: PositionManager,
        private _params: QuotingParameters.QuotingParametersRepository,
        private _positionBroker: Interfaces.IPositionBroker,
        private _wrapped: Publish.IPublish<Models.TargetBasePositionValue>) {
        _wrapped.registerSnapshot(() => [this._latest]);
        _positionBroker.NewReport.on(r => this.recomputeTargetPosition());
        _params.NewParameters.on(() => setTimeout(() => this.recomputeTargetPosition(), 121));
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

        if (this._latest === null || Math.abs(this._latest.data - targetBasePosition) > 1e-2 || !_.isEqual(this.sideAPR, this._latest.sideAPR)) {
            this._latest = new Models.TargetBasePositionValue(
              targetBasePosition,
              this.sideAPR,
              this._timeProvider.utcNow()
            );
            this.NewTargetPosition.trigger();
            this._wrapped.publish(this.latestTargetPosition);
            this._log.info("recalculated target base position:", Utils.roundFloat(this.latestTargetPosition.data));
        }
    };
}
