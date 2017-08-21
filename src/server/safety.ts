import Models = require("../share/models");
import Utils = require("./utils");
import moment = require('moment');
import PositionManagement = require("./position-management");

interface ITrade {
    price: number;
    quantity: number;
    time: number;
}

export class SafetyCalculator {
    private _latest: Models.TradeSafety = null;
    public get latest() { return this._latest; }
    public set latest(val: Models.TradeSafety) {
        if (!this._latest || Math.abs(val.combined - this._latest.combined) > 1e-3
          || Math.abs(val.buyPing - this._latest.buyPing) >= 1e-2
          || Math.abs(val.sellPong - this._latest.sellPong) >= 1e-2) {
            this._latest = val;
            this._evUp('Safety');
            this._uiSend(Models.Topics.TradeSafetyValue, this.latest, true);
        }
    }

    private _buys: ITrade[] = [];
    private _sells: ITrade[] = [];

    public targetPosition: PositionManagement.TargetBasePositionManager;

    constructor(
      private _fvEngine,
      private _qpRepo,
      private _positionBroker,
      private _tradesMemory,
      private _uiSnap,
      private _uiSend,
      private _evOn,
      private _evUp
    ) {
      _uiSnap(Models.Topics.TradeSafetyValue, () => [this.latest]);
      this._evOn('OrderTradeBroker', this.onTrade);

      this._evOn('QuotingParameters', this.computeQtyLimit);
      setInterval(this.computeQtyLimit, moment.duration(1, "seconds"));
    }

    private onTrade = (ut: Models.Trade) => {
        if (this.isOlderThan(ut.time)) return;

        this[ut.side === Models.Side.Ask ? '_sells' : '_buys'].push(<ITrade>{
          price: ut.price,
          quantity: ut.quantity,
          time: ut.time
        });

        this.computeQtyLimit();
    };

    private isOlderThan(time: number) {
        return Math.abs(new Date().getTime() - time) > this._qpRepo().tradeRateSeconds * 1000;
    }

    private computeQtyLimit = () => {
        const fv = this._fvEngine();
        const latestPosition = this._positionBroker();
        if (!fv || !this.targetPosition.latestTargetPosition || latestPosition === null) return;
        const params = this._qpRepo();
        let buySize: number  = (params.percentageValues && latestPosition != null)
            ? params.buySizePercentage * latestPosition.value / 100
            : params.buySize;
        let sellSize: number = (params.percentageValues && latestPosition != null)
              ? params.sellSizePercentage * latestPosition.value / 100
              : params.sellSize;
        const targetBasePosition = this.targetPosition.latestTargetPosition.tbp;
        const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
        if (params.aggressivePositionRebalancing != Models.APR.Off && params.buySizeMax) buySize = Math.max(buySize, targetBasePosition - totalBasePosition);
        if (params.aggressivePositionRebalancing != Models.APR.Off && params.sellSizeMax) sellSize = Math.max(sellSize, totalBasePosition - targetBasePosition);

        var buyPing = 0;
        var sellPong = 0;
        var buyPq = 0;
        var sellPq = 0;
        var _buyPq = 0;
        var _sellPq = 0;
        var trades = this._tradesMemory();
        var widthPong = (params.widthPercentage)
            ? params.widthPongPercentage * fv / 100
            : params.widthPong;
        if (params.pongAt == Models.PongAt.ShortPingFair || params.pongAt == Models.PongAt.ShortPingAggressive) {
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price>b.price?1:(a.price<b.price?-1:0));
          for (var ti = 0;ti<trades.length;ti++) {
            if ((!fv || (fv.price>trades[ti].price && fv-params.widthPong<trades[ti].price)) && ((params.mode !== Models.QuotingMode.Boomerang && params.mode !== Models.QuotingMode.HamelinRat && params.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Bid && buyPq<sellSize) {
              _buyPq = Math.min(sellSize - buyPq, trades[ti].quantity);
              buyPing += trades[ti].price * _buyPq;
              buyPq += _buyPq;
            }
            if (buyPq>=sellSize) break;
          }
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price<b.price?1:(a.price>b.price?-1:0));
        } else if (params.pongAt == Models.PongAt.LongPingFair || params.pongAt == Models.PongAt.LongPingAggressive)
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price>b.price?1:(a.price<b.price?-1:0));
        if (!buyPq) for (var ti = 0;ti<trades.length;ti++) {
          if ((!fv || fv>trades[ti].price) && ((params.mode !== Models.QuotingMode.Boomerang && params.mode !== Models.QuotingMode.HamelinRat && params.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Bid && buyPq<sellSize) {
            _buyPq = Math.min(sellSize - buyPq, trades[ti].quantity);
            buyPing += trades[ti].price * _buyPq;
            buyPq += _buyPq;
          }
          if (buyPq>=sellSize) break;
        }
        if (params.pongAt == Models.PongAt.ShortPingFair || params.pongAt == Models.PongAt.ShortPingAggressive) {
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price<b.price?1:(a.price>b.price?-1:0));
          for (var ti = 0;ti<trades.length;ti++) {
            if ((!fv || (fv<trades[ti].price && fv+params.widthPong>trades[ti].price)) && ((params.mode !== Models.QuotingMode.Boomerang && params.mode !== Models.QuotingMode.HamelinRat && params.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Ask && sellPq<buySize) {
              _sellPq = Math.min(buySize - sellPq, trades[ti].quantity);
              sellPong += trades[ti].price * _sellPq;
              sellPq += _sellPq;
            }
            if (sellPq>=buySize) break;
          }
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price>b.price?1:(a.price<b.price?-1:0));
        } else if (params.pongAt == Models.PongAt.LongPingFair || params.pongAt == Models.PongAt.LongPingAggressive)
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price<b.price?1:(a.price>b.price?-1:0));
        if (!sellPq) for (var ti = 0;ti<trades.length;ti++) {
          if ((!fv || fv<trades[ti].price) && ((params.mode !== Models.QuotingMode.Boomerang && params.mode !== Models.QuotingMode.HamelinRat && params.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Ask && sellPq<buySize) {
            _sellPq = Math.min(buySize - sellPq, trades[ti].quantity);
            sellPong += trades[ti].price * _sellPq;
            sellPq += _sellPq;
          }
          if (sellPq>=buySize) break;
        }

        if (buyPq) buyPing /= buyPq;
        if (sellPq) sellPong /= sellPq;

        this._buys = this._buys.filter(o => !this.isOlderThan(o.time))
          .sort(function(a,b){return a.price>b.price?-1:(a.price<b.price?1:0)});
        this._sells = this._sells.filter(o => !this.isOlderThan(o.time))
          .sort(function(a,b){return a.price>b.price?1:(a.price<b.price?-1:0)});

        // don't count good trades against safety
        while (this._buys.length && this._sells.length) {
            var sell = this._sells.slice(-1).pop();
            var buy = this._buys.slice(-1).pop();
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

        this.latest = new Models.TradeSafety(
          ((t: ITrade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / buySize)(this._buys),
          ((t: ITrade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / sellSize)(this._sells),
          ((t: ITrade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / (buySize + sellSize / 2))(this._buys.concat(this._sells)),
          buyPing,
          sellPong
        );
    };
}
