import Models = require("../share/models");
import Utils = require("./utils");
import Broker = require("./broker");
import Publish = require("./publish");
import moment = require('moment');
import FairValue = require("./fair-value");
import Persister = require("./persister");
import QuotingParameters = require("./quoting-parameters");

interface ITrade {
    price: number;
    quantity: number;
    time: Date;
}

export class SafetyCalculator {
    NewValue = new Utils.Evt();

    private _latest: Models.TradeSafety = null;
    public get latest() { return this._latest; }
    public set latest(val: Models.TradeSafety) {
        if (!this._latest || Math.abs(val.combined - this._latest.combined) > 1e-3
          || Math.abs(val.buyPing - this._latest.buyPing) >= 1e-2
          || Math.abs(val.sellPong - this._latest.sellPong) >= 1e-2) {
            this._latest = val;
            this.NewValue.trigger(this.latest);
            this._publisher.publish(this.latest);
        }
    }

    private _buys: ITrade[] = [];
    private _sells: ITrade[] = [];
    public latestTargetPosition: Models.TargetBasePositionValue = null;

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _fvEngine: FairValue.FairValueEngine,
        private _qlParams: QuotingParameters.QuotingParametersRepository,
        private _positionBroker: Broker.PositionBroker,
        private _broker: Broker.OrderBroker,
        private _publisher: Publish.IPublish<Models.TradeSafety>) {
        _publisher.registerSnapshot(() => [this.latest]);
        _qlParams.NewParameters.on(this.computeQtyLimit);
        _broker.Trade.on(this.onTrade);

        _timeProvider.setInterval(this.computeQtyLimit, moment.duration(1, "seconds"));
    }

    private onTrade = (ut: Models.Trade) => {
        if (this.isOlderThan(ut.time, this._qlParams.latest)) return;

        this[ut.side === Models.Side.Ask ? '_sells' : '_buys'].push(<ITrade>{
          price: ut.price,
          quantity: ut.quantity,
          time: ut.time
        });

        this.computeQtyLimit();
    };

    private isOlderThan(time: Date, settings: Models.QuotingParameters) {
        return Math.abs(this._timeProvider.utcNow().valueOf() - time.valueOf()) > settings.tradeRateSeconds * 1000;
    }

    private computeQtyLimit = () => {
        var fv = this._fvEngine.latestFairValue;
        if (!fv) return;
        const settings = this._qlParams.latest;
        const latestPosition = this._positionBroker.latestReport;
        let buySize: number  = (settings.percentageValues && latestPosition != null)
            ? settings.buySizePercentage * latestPosition.value / 100
            : settings.buySize;
        let sellSize: number = (settings.percentageValues && latestPosition != null)
              ? settings.sellSizePercentage * latestPosition.value / 100
              : settings.sellSize;
        const tbp = this.latestTargetPosition;
        if (tbp !== null) {
          const targetBasePosition = tbp.data;
          const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
          if (settings.aggressivePositionRebalancing != Models.APR.Off && settings.buySizeMax) buySize = Math.max(buySize, targetBasePosition - totalBasePosition);
          if (settings.aggressivePositionRebalancing != Models.APR.Off && settings.sellSizeMax) sellSize = Math.max(sellSize, totalBasePosition - targetBasePosition);
        }
        var buyPing = 0;
        var sellPong = 0;
        var buyPq = 0;
        var sellPq = 0;
        var _buyPq = 0;
        var _sellPq = 0;
        var trades = this._broker._trades;
        var widthPong = (settings.widthPercentage)
            ? settings.widthPongPercentage * fv.price / 100
            : settings.widthPong;
        if (settings.pongAt == Models.PongAt.ShortPingFair || settings.pongAt == Models.PongAt.ShortPingAggressive) {
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price>b.price?1:(a.price<b.price?-1:0));
          for (var ti = 0;ti<trades.length;ti++) {
            if ((!fv.price || (fv.price>trades[ti].price && fv.price-settings.widthPong<trades[ti].price)) && ((settings.mode !== Models.QuotingMode.Boomerang && settings.mode !== Models.QuotingMode.HamelinRat && settings.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Bid && buyPq<sellSize) {
              _buyPq = Math.min(sellSize - buyPq, trades[ti].quantity);
              buyPing += trades[ti].price * _buyPq;
              buyPq += _buyPq;
            }
            if (buyPq>=sellSize) break;
          }
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price<b.price?1:(a.price>b.price?-1:0));
        } else if (settings.pongAt == Models.PongAt.LongPingFair || settings.pongAt == Models.PongAt.LongPingAggressive)
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price>b.price?1:(a.price<b.price?-1:0));
        if (!buyPq) for (var ti = 0;ti<trades.length;ti++) {
          if ((!fv.price || fv.price>trades[ti].price) && ((settings.mode !== Models.QuotingMode.Boomerang && settings.mode !== Models.QuotingMode.HamelinRat && settings.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Bid && buyPq<sellSize) {
            _buyPq = Math.min(sellSize - buyPq, trades[ti].quantity);
            buyPing += trades[ti].price * _buyPq;
            buyPq += _buyPq;
          }
          if (buyPq>=sellSize) break;
        }
        if (settings.pongAt == Models.PongAt.ShortPingFair || settings.pongAt == Models.PongAt.ShortPingAggressive) {
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price<b.price?1:(a.price>b.price?-1:0));
          for (var ti = 0;ti<trades.length;ti++) {
            if ((!fv.price || (fv.price<trades[ti].price && fv.price+settings.widthPong>trades[ti].price)) && ((settings.mode !== Models.QuotingMode.Boomerang && settings.mode !== Models.QuotingMode.HamelinRat && settings.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Ask && sellPq<buySize) {
              _sellPq = Math.min(buySize - sellPq, trades[ti].quantity);
              sellPong += trades[ti].price * _sellPq;
              sellPq += _sellPq;
            }
            if (sellPq>=buySize) break;
          }
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price>b.price?1:(a.price<b.price?-1:0));
        } else if (settings.pongAt == Models.PongAt.LongPingFair || settings.pongAt == Models.PongAt.LongPingAggressive)
          trades.sort((a: Models.Trade, b: Models.Trade) => a.price<b.price?1:(a.price>b.price?-1:0));
        if (!sellPq) for (var ti = 0;ti<trades.length;ti++) {
          if ((!fv.price || fv.price<trades[ti].price) && ((settings.mode !== Models.QuotingMode.Boomerang && settings.mode !== Models.QuotingMode.HamelinRat && settings.mode !== Models.QuotingMode.AK47) || trades[ti].Kqty<trades[ti].quantity) && trades[ti].side == Models.Side.Ask && sellPq<buySize) {
            _sellPq = Math.min(buySize - sellPq, trades[ti].quantity);
            sellPong += trades[ti].price * _sellPq;
            sellPq += _sellPq;
          }
          if (sellPq>=buySize) break;
        }

        if (buyPq) buyPing /= buyPq;
        if (sellPq) sellPong /= sellPq;

        this._buys = this._buys.filter(o => !this.isOlderThan(o.time, settings))
          .sort(function(a,b){return a.price>b.price?-1:(a.price<b.price?1:0)});
        this._sells = this._sells.filter(o => !this.isOlderThan(o.time, settings))
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

        var computeSafety = (t: ITrade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / (buySize + sellSize / 2);
        var computeSafetyBuy = (t: ITrade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / buySize;
        var computeSafetySell = (t: ITrade[]) => t.reduce((sum, t) => sum + t.quantity, 0) / sellSize;

        this.latest = new Models.TradeSafety(
          computeSafetyBuy(this._buys),
          computeSafetySell(this._sells),
          computeSafety(this._buys.concat(this._sells)),
          buyPing,
          sellPong,
          this._timeProvider.utcNow()
        );
    };
}
