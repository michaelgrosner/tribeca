import Models = require("../share/models");
import FairValue = require("./fair-value");

export class PositionBroker {
    private _lastPositions: any[] = [];
    private _report : Models.PositionReport = null;
    public get latestReport() : Models.PositionReport {
        return this._report;
    }

    private _currencies : { [currency : number] : Models.CurrencyPosition } = {};
    public getPosition(currency : Models.Currency) : Models.CurrencyPosition {
        return this._currencies[currency] || new Models.CurrencyPosition(0, 0, currency);
    }

    private onPositionUpdate = (rpt : Models.CurrencyPosition) => {
        if (rpt !== null) this._currencies[rpt.currency] = rpt;
        if (!this._currencies[this._pair.base] || !this._currencies[this._pair.quote]) return;
        var basePosition = this.getPosition(this._pair.base);
        var quotePosition = this.getPosition(this._pair.quote);
        var fv = this._fvEngine.latestFairValue;
        if (typeof basePosition === "undefined" || typeof quotePosition === "undefined" || fv === null) return;

        const baseAmount = basePosition.amount;
        const quoteAmount = quotePosition.amount;
        const baseValue = baseAmount + quoteAmount / fv.price + basePosition.heldAmount + quotePosition.heldAmount / fv.price;
        const quoteValue = baseAmount * fv.price + quoteAmount + basePosition.heldAmount * fv.price + quotePosition.heldAmount;

        const timeNow = new Date();
        const now = timeNow.getTime();
        this._lastPositions.push({ baseValue: baseValue, quoteValue: quoteValue, time: now });
        this._lastPositions = this._lastPositions.filter(x => x.time+(this._qpRepo().profitHourInterval * 36e+5)>now);
        const profitBase = ((baseValue - this._lastPositions[0].baseValue) / baseValue) * 1e+2;
        const profitQuote = ((quoteValue - this._lastPositions[0].quoteValue) / quoteValue) * 1e+2;

        const positionReport = new Models.PositionReport(baseAmount, quoteAmount, basePosition.heldAmount,
            quotePosition.heldAmount, baseValue, quoteValue, profitBase, profitQuote, this._pair, this._exchange);

        let sameValue = true;
        if (this._report !== null) {
          sameValue = Math.abs(positionReport.value - this._report.value) < 2e-6;
          if(sameValue &&
            Math.abs(positionReport.quoteValue - this._report.quoteValue) < 2e-2 &&
            Math.abs(positionReport.baseAmount - this._report.baseAmount) < 2e-6 &&
            Math.abs(positionReport.quoteAmount - this._report.quoteAmount) < 2e-2 &&
            Math.abs(positionReport.baseHeldAmount - this._report.baseHeldAmount) < 2e-6 &&
            Math.abs(positionReport.quoteHeldAmount - this._report.quoteHeldAmount) < 2e-2 &&
            Math.abs(positionReport.profitBase - this._report.profitBase) < 2e-2 &&
            Math.abs(positionReport.profitQuote - this._report.profitQuote) < 2e-2
          ) return;
        }

        this._report = positionReport;
        if (!sameValue) this._evUp('PositionBroker');
        this._uiSend(Models.Topics.Position, positionReport, true);
    };

    private handleOrderUpdate = (o: Models.OrderStatusReport) => {
        if (!this._report) return;
        var heldAmount = 0;
        var amount = o.side == Models.Side.Ask
          ? this._report.baseAmount + this._report.baseHeldAmount
          : this._report.quoteAmount + this._report.quoteHeldAmount;
        this._allOrders().forEach(x => {
          if (x.side !== o.side) return;
          let held = x.quantity * (x.side == Models.Side.Bid ? x.price : 1);
          if (amount>=held) {
            amount -= held;
            heldAmount += held;
          }
        });
        this.onPositionUpdate(new Models.CurrencyPosition(
          amount,
          heldAmount,
          o.side == Models.Side.Ask ? o.pair.base : o.pair.quote
        ));
    };

    constructor(
      private _qpRepo,
      private _pair,
      private _exchange,
      private _allOrders,
      private _fvEngine: FairValue.FairValueEngine,
      private _uiSnap,
      private _uiSend,
      private _evOn,
      private _evUp
    ) {
        this._evOn('PositionGateway', this.onPositionUpdate);
        this._evOn('OrderUpdateBroker', this.handleOrderUpdate);
        this._evOn('FairValue', () => this.onPositionUpdate(null));

        _uiSnap(Models.Topics.Position, () => (this._report === null ? [] : [this._report]));
    }
}
