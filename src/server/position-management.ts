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
    private newWidth: Models.IStdev = null;
    private newQuote: number = null;
    private newShort: number = null;
    private newMedium: number = null;
    private newLong: number = null;
    private fairValue: number = null;
    public set quoteEwma(quoteEwma: number) {
      if (quoteEwma === null) return;
      this.newQuote = quoteEwma;
    }
    public set widthStdev(widthStdev: Models.IStdev) {
      const fv = this._fvAgent.latestFairValue;
      if (fv === null || widthStdev === null) return;
      this.fairValue = fv.price;
      this.newWidth = widthStdev;
      const minTick = this._details.minTickIncrement;
      this._ewmaPublisher.publish(new Models.EWMAChart(
        widthStdev,
        this.newQuote?Utils.roundNearest(this.newQuote, minTick):null,
        this.newShort?Utils.roundNearest(this.newShort, minTick):null,
        this.newMedium?Utils.roundNearest(this.newMedium, minTick):null,
        this.newLong?Utils.roundNearest(this.newLong, minTick):null,
        Utils.roundNearest(fv.price, minTick),
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
        private _details: Interfaces.IBroker,
        private _timeProvider: Utils.ITimeProvider,
        private _persister: Persister.IPersist<Models.RegularFairValue>,
        private _fvAgent: FairValue.FairValueEngine,
        private _shortEwma: Statistics.IComputeStatistics,
        private _mediumEwma: Statistics.IComputeStatistics,
        private _longEwma: Statistics.IComputeStatistics,
        private _ewmaPublisher : Publish.IPublish<Models.EWMAChart>
    ) {
        const minTick = this._details.minTickIncrement;
        _ewmaPublisher.registerSnapshot(() => [this.fairValue?new Models.EWMAChart(
          this.newWidth,
          this.newQuote?Utils.roundNearest(this.newQuote, minTick):null,
          this.newShort?Utils.roundNearest(this.newShort, minTick):null,
          this.newMedium?Utils.roundNearest(this.newMedium, minTick):null,
          this.newLong?Utils.roundNearest(this.newLong, minTick):null,
          this.fairValue?Utils.roundNearest(this.fairValue, minTick):null,
          this._timeProvider.utcNow()
        ):null]);
        this._timeProvider.setInterval(this.updateEwmaValues, moment.duration(1, 'minutes'));
        this._timeProvider.setTimeout(this.updateEwmaValues, moment.duration(1, 'seconds'));
    }

    private updateEwmaValues = () => {
        const fv = this._fvAgent.latestFairValue;
        if (fv === null)
            return;
        this.fairValue = fv.price;

        const rfv = new Models.RegularFairValue(this._timeProvider.utcNow(), fv.price);

        this.newShort = this._shortEwma.addNewValue(fv.price);
        this.newMedium = this._mediumEwma.addNewValue(fv.price);
        this.newLong = this._longEwma.addNewValue(fv.price);

        const minTick = this._details.minTickIncrement;
        let newTargetPosition = ((this.newShort * 100/ this.newLong) - 100) * 5;

        if (newTargetPosition > 1) newTargetPosition = 1;
        if (newTargetPosition < -1) newTargetPosition = -1;

        if (Math.abs(newTargetPosition - this._latest) > minTick) {
            this._latest = newTargetPosition;
            this.NewTargetPosition.trigger();
        }

        this._ewmaPublisher.publish(new Models.EWMAChart(
          this.newWidth,
          this.newQuote?Utils.roundNearest(this.newQuote, minTick):null,
          Utils.roundNearest(this.newShort, minTick),
          Utils.roundNearest(this.newMedium, minTick),
          Utils.roundNearest(this.newLong, minTick),
          Utils.roundNearest(fv.price, minTick),
          this._timeProvider.utcNow()
        ));

        this._persister.persist(rfv);
        this._data.push(rfv);
        this._data = this._data.slice(-7);
    };
}

export class TargetBasePositionManager {
    public NewTargetPosition = new Utils.Evt();

    public sideAPR: string[] = [];

    private _latest: Models.TargetBasePositionValue = null;
    public get latestTargetPosition(): Models.TargetBasePositionValue {
        return this._latest;
    }

    public set quoteEWMA(quoteEwma: number) {
      this._positionManager.quoteEwma = quoteEwma;
    }

    public set widthSTDEV(widthStdev: Models.IStdev) {
      this._positionManager.widthStdev = widthStdev;
    }

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _positionManager: PositionManager,
        private _params: QuotingParameters.QuotingParametersRepository,
        private _positionBroker: Interfaces.IPositionBroker,
        private _wrapped: Publish.IPublish<Models.TargetBasePositionValue>) {
        _wrapped.registerSnapshot(() => [this._latest]);
        _positionBroker.NewReport.on(r => this.recomputeTargetPosition());
        _params.NewParameters.on(() => _timeProvider.setTimeout(() => this.recomputeTargetPosition(), moment.duration(121)));
        _positionManager.NewTargetPosition.on(() => this.recomputeTargetPosition());
    }

    private recomputeTargetPosition = () => {
        const latestPosition = this._positionBroker.latestReport;
        const params = this._params.latest;

        if (params === null || latestPosition === null)
            return;

        let targetBasePosition: number = params.percentageValues
          ? params.targetBasePositionPercentage * latestPosition.value / 100
          : params.targetBasePosition;

        if (params.autoPositionMode === Models.AutoPositionMode.EWMA)
            targetBasePosition = ((1 + this._positionManager.latestTargetPosition) / 2) * latestPosition.value;

        if (this._latest === null || Math.abs(this._latest.data - targetBasePosition) > 1e-2 || !_.isEqual(this.sideAPR, this._latest.sideAPR)) {
            this._latest = new Models.TargetBasePositionValue(
              targetBasePosition,
              this.sideAPR,
              this._timeProvider.utcNow()
            );
            this.NewTargetPosition.trigger();
            this._wrapped.publish(this.latestTargetPosition);
            console.info(new Date().toISOString().slice(11, -1), 'tbp', 'recalculated', this.latestTargetPosition.data);
        }
    };
}
