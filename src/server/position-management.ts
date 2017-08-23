import Models = require("../share/models");
import Utils = require("./utils");
import moment = require("moment");

export class TargetBasePositionManager {
  public sideAPR: string;

  private newWidth: Models.IStdev = null;
  private newQuote: number = null;
  private newShort: number = null;
  private newMedium: number = null;
  private newLong: number = null;
  private fairValue: number = null;
  public set quoteEwma(quoteEwma: number) {
    this.newQuote = quoteEwma;
  }
  public set widthStdev(widthStdev: Models.IStdev) {
    this.newWidth = widthStdev;
  }

  private _newTargetPosition: number = 0;
  private _lastPosition: number = null;

  private _latest: Models.TargetBasePositionValue = null;
  public get latestTargetPosition(): Models.TargetBasePositionValue {
    return this._latest;
  }

  constructor(
    private _minTick: number,
    private _dbInsert,
    private _fvEngine,
    private _mgEwmaShort,
    private _mgEwmaMedium,
    private _mgEwmaLong,
    private _mgTBP,
    private _qpRepo,
    private _positionBroker,
    private _uiSnap,
    private _uiSend,
    private _evOn,
    private _evUp,
    initTBP: Models.TargetBasePositionValue[]
  ) {
    if (initTBP.length && typeof initTBP[0].tbp != "undefined") {
      this._latest = initTBP[0];
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Loaded from DB:', this._latest.tbp);
    }

    _uiSnap(Models.Topics.TargetBasePosition, () => [this._latest]);
    _uiSnap(Models.Topics.EWMAChart, () => [this.fairValue?new Models.EWMAChart(
      this.newWidth,
      this.newQuote?Utils.roundNearest(this.newQuote, this._minTick):null,
      this.newShort?Utils.roundNearest(this.newShort, this._minTick):null,
      this.newMedium?Utils.roundNearest(this.newMedium, this._minTick):null,
      this.newLong?Utils.roundNearest(this.newLong, this._minTick):null,
      this.fairValue?Utils.roundNearest(this.fairValue, this._minTick):null
    ):null]);
    this._evOn('PositionBroker', this.recomputeTargetPosition);
    this._evOn('QuotingParameters', () => setTimeout(() => this.recomputeTargetPosition(), moment.duration(121)));
    setInterval(this.updateEwmaValues, moment.duration(1, 'minutes'));
  }

  private recomputeTargetPosition = () => {
    const params = this._qpRepo();
    const latestReport = this._positionBroker();
    if (params === null || latestReport === null) {
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Unable to compute tbp [ qp | pos ] = [', !!params, '|', !!this._positionBroker.latestReport, ']');
      return;
    }
    const targetBasePosition: number = (params.autoPositionMode === Models.AutoPositionMode.Manual)
      ? (params.percentageValues
        ? params.targetBasePositionPercentage * latestReport.value / 100
        : params.targetBasePosition)
      : ((1 + this._newTargetPosition) / 2) * latestReport.value;

    if (this._latest === null || Math.abs(this._latest.tbp - targetBasePosition) > 1e-4 || this.sideAPR !== this._latest.sideAPR) {
      this._latest = new Models.TargetBasePositionValue(targetBasePosition, this.sideAPR);
      this._evUp('TargetPosition');
      this._uiSend(Models.Topics.TargetBasePosition, this._latest, true);
      this._dbInsert(Models.Topics.TargetBasePosition, this._latest);
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'recalculated', this._latest.tbp);
    }
  };

  private updateEwmaValues = () => {
    const fv = this._fvEngine();
    if (!fv) {
      console.info(new Date().toISOString().slice(11, -1), 'tbp', 'Unable to update ewma');
      return;
    }
    this.fairValue = fv;

    this.newShort = this._mgEwmaShort(this.fairValue);
    this.newMedium = this._mgEwmaMedium(this.fairValue);
    this.newLong = this._mgEwmaLong(this.fairValue);
    this._newTargetPosition = this._mgTBP(this.fairValue, this.newLong, this.newMedium, this.newShort);
    // console.info(new Date().toISOString().slice(11, -1), 'tbp', 'recalculated ewma [ FV | L | M | S ] = [',this.fairValue,'|',this.newLong,'|',this.newMedium,'|',this.newShort,']');
    this.recomputeTargetPosition();

    this._uiSend(Models.Topics.EWMAChart, new Models.EWMAChart(
      this.newWidth,
      this.newQuote?Utils.roundNearest(this.newQuote, this._minTick):null,
      Utils.roundNearest(this.newShort, this._minTick),
      Utils.roundNearest(this.newMedium, this._minTick),
      Utils.roundNearest(this.newLong, this._minTick),
      Utils.roundNearest(this.fairValue, this._minTick)
    ), true);

    this._dbInsert(Models.Topics.EWMAChart, new Models.RegularFairValue(this.fairValue, this.newLong, this.newMedium, this.newShort));
  };
}
