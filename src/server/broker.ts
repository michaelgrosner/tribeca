import Models = require("../share/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import FairValue = require("./fair-value");
import moment = require("moment");

class OrderStateCache {
  public allOrders = new Map<string, Models.OrderStatusReport>();
  public exchIdsToClientIds = new Map<string, string>();
}

export class OrderBroker {
    cancelOpenOrders() {
        if (this._oeGateway.supportsCancelAllOpenOrders()) {
            return this._oeGateway.cancelAllOpenOrders();
        }

        for (let e of this.orderCache.allOrders.values()) {
            if (e.orderStatus == Models.OrderStatus.New || e.orderStatus == Models.OrderStatus.Working)
              this.cancelOrder(new Models.OrderCancel(e.orderId));
        }
    }

    private cleanClosedOrders() {
      var lateCleans : {[id: string] : boolean} = {};
      for(var i = 0;i<this.tradesMemory.length;i++) {
        if (this.tradesMemory[i].Kqty+0.0001 >= this.tradesMemory[i].quantity) {
          lateCleans[this.tradesMemory[i].tradeId] = true;
        }
      }

      for (var k in lateCleans) {
        if (!(k in lateCleans)) continue;
        for(var i = 0;i<this.tradesMemory.length;i++) {
          if (k == this.tradesMemory[i].tradeId) {
            this.tradesMemory[i].Kqty = -1;
            this._uiSend(Models.Topics.Trades, this.tradesMemory[i]);
            this._dbInsert(Models.Topics.Trades, undefined, false, this.tradesMemory[i].tradeId);
            this.tradesMemory.splice(i, 1);
            break;
          }
        }
      }
    }

    private cleanTrade(tradeId: string) {
      var lateCleans : {[id: string] : boolean} = {};
      for(var i = 0;i<this.tradesMemory.length;i++) {
        if (this.tradesMemory[i].tradeId == tradeId) {
          lateCleans[this.tradesMemory[i].tradeId] = true;
        }
      }

      for (var k in lateCleans) {
        if (!(k in lateCleans)) continue;
        for(var i = 0;i<this.tradesMemory.length;i++) {
          if (k == this.tradesMemory[i].tradeId) {
            this.tradesMemory[i].Kqty = -1;
            this._uiSend(Models.Topics.Trades, this.tradesMemory[i]);
            this._dbInsert(Models.Topics.Trades, undefined, false, this.tradesMemory[i].tradeId);
            this.tradesMemory.splice(i, 1);
            break;
          }
        }
      }
    }

    private cleanOrders() {
      var lateCleans : {[id: string] : boolean} = {};
      for(var i = 0;i<this.tradesMemory.length;i++) {
        lateCleans[this.tradesMemory[i].tradeId] = true;
      }

      for (var k in lateCleans) {
        if (!(k in lateCleans)) continue;
        for(var i = 0;i<this.tradesMemory.length;i++) {
          if (k == this.tradesMemory[i].tradeId) {
            this.tradesMemory[i].Kqty = -1;
            this._uiSend(Models.Topics.Trades, this.tradesMemory[i]);
            this._dbInsert(Models.Topics.Trades, undefined, false, this.tradesMemory[i].tradeId);
            this.tradesMemory.splice(i, 1);
            break;
          }
        }
      }
    }

    private _cancelsWaitingForExchangeOrderId: {[clId : string]: Models.OrderCancel} = {};

    tradesMemory : Models.Trade[] = [];

    sendOrder = (order : Models.SubmitNewOrder) => {
        const orderId = this._oeGateway.generateClientOrderId();

        const rpt : Models.OrderStatusUpdate = {
            pair: this._pair,
            orderId: orderId,
            side: order.side,
            quantity: order.quantity,
            leavesQuantity: order.quantity,
            type: order.type,
            isPong: order.isPong,
            price: Utils.roundSide(order.price, this._minTick, order.side),
            timeInForce: order.timeInForce,
            orderStatus: Models.OrderStatus.New,
            preferPostOnly: order.preferPostOnly,
            exchange: this._exchange,
            rejectMessage: order.msg,
            source: order.source
        };

        this._oeGateway.sendOrder(this.updateOrderState(rpt));
    };

    cancelOrder = (cancel: Models.OrderCancel) => {
        const rpt = this.orderCache.allOrders.get(cancel.origOrderId);
        if (!rpt) {
          this.updateOrderState(<Models.OrderStatusUpdate>{
              orderId: cancel.origOrderId,
              orderStatus: Models.OrderStatus.Cancelled,
              leavesQuantity: 0
          });
          return;
        }

        if (!this._oeGateway.cancelsByClientOrderId) {
            // race condition! i cannot cancel an order before I get the exchangeId (oid); register it for deletion on the ack
            if (typeof rpt.exchangeId === "undefined") {
                this._cancelsWaitingForExchangeOrderId[rpt.orderId] = cancel;
                // console.info('broker', 'Registered', rpt.orderId, 'for late deletion');
                return;
            }
        }

        this._oeGateway.cancelOrder(this.updateOrderState(<Models.OrderStatusUpdate>{
            orderId: cancel.origOrderId,
            orderStatus: Models.OrderStatus.Working
        }));
    };

    private _reTrade = (reTrades: Models.Trade[], trade: Models.Trade) => {
      let gowhile = true;
      while (gowhile && trade.quantity>0 && reTrades!=null && reTrades.length) {
        var reTrade = reTrades.shift();
        gowhile = false;
        for(var i = 0;i<this.tradesMemory.length;i++) {
          if (this.tradesMemory[i].tradeId==reTrade.tradeId) {
            gowhile = true;
            var Kqty = Math.min(trade.quantity, this.tradesMemory[i].quantity - this.tradesMemory[i].Kqty);
            this.tradesMemory[i].Ktime = trade.time;
            this.tradesMemory[i].Kprice = ((Kqty*trade.price) + (this.tradesMemory[i].Kqty*this.tradesMemory[i].Kprice)) / (this.tradesMemory[i].Kqty+Kqty);
            this.tradesMemory[i].Kqty += Kqty;
            this.tradesMemory[i].Kvalue = Math.abs(this.tradesMemory[i].Kprice*this.tradesMemory[i].Kqty);
            trade.quantity -= Kqty;
            trade.value = Math.abs(trade.price*trade.quantity);
            if (this.tradesMemory[i].quantity<=this.tradesMemory[i].Kqty)
              this.tradesMemory[i].Kdiff = Math.abs((this.tradesMemory[i].quantity*this.tradesMemory[i].price)-(this.tradesMemory[i].Kqty*this.tradesMemory[i].Kprice));
            this.tradesMemory[i].loadedFromDB = false;
            this._uiSend(Models.Topics.Trades, this.tradesMemory[i]);
            this._dbInsert(Models.Topics.Trades, this.tradesMemory[i], false, this.tradesMemory[i].tradeId);
            break;
          }
        }
      }
      if (trade.quantity>0) {
        var exists = false;
        for(var i = 0;i<this.tradesMemory.length;i++) {
          if (this.tradesMemory[i].price==trade.price && this.tradesMemory[i].side==trade.side && this.tradesMemory[i].quantity>this.tradesMemory[i].Kqty) {
            exists = true;
            this.tradesMemory[i].time = trade.time;
            this.tradesMemory[i].quantity += trade.quantity;
            this.tradesMemory[i].value += trade.value;
            this.tradesMemory[i].loadedFromDB = false;
            this._uiSend(Models.Topics.Trades, this.tradesMemory[i]);
            this._dbInsert(Models.Topics.Trades, this.tradesMemory[i], false, this.tradesMemory[i].tradeId);
            break;
          }
        }
        if (!exists) {
          this._uiSend(Models.Topics.Trades, trade);
          this._dbInsert(Models.Topics.Trades, trade, false, trade.tradeId);
          this.tradesMemory.push(trade);
        }
      }
    };

    private updateOrderState = (osr: Models.OrderStatusUpdate): Models.OrderStatusReport => {
        let orig: Models.OrderStatusUpdate;
        if (osr.orderStatus === Models.OrderStatus.New) {
            orig = osr;
        } else {
            orig = this.orderCache.allOrders.get(osr.orderId);
            if (typeof orig === "undefined" && typeof osr.exchangeId !== "undefined" && osr.exchangeId) {
                const secondChance = this.orderCache.exchIdsToClientIds.get(osr.exchangeId);
                if (typeof secondChance !== "undefined") {
                    osr.orderId = secondChance;
                    orig = this.orderCache.allOrders.get(secondChance);
                }
            }
            if (typeof orig === "undefined") return;
        }

        const getOrFallback = <T>(n: T, o: T) => typeof n !== "undefined" ? n : o;

        const quantity = getOrFallback(osr.quantity, orig.quantity);

        let cumQuantity : number = undefined;
        if (typeof osr.cumQuantity !== "undefined") {
            cumQuantity = getOrFallback(osr.cumQuantity, orig.cumQuantity);
        }
        else {
            cumQuantity = getOrFallback(orig.cumQuantity, 0) + getOrFallback(osr.lastQuantity, 0);
        }

        const o: Models.OrderStatusReport = {
          pair: getOrFallback(osr.pair, orig.pair),
          side: getOrFallback(osr.side, orig.side),
          quantity: quantity,
          type: getOrFallback(osr.type, orig.type),
          price: getOrFallback(osr.price, orig.price),
          timeInForce: getOrFallback(osr.timeInForce, orig.timeInForce),
          orderId: getOrFallback(osr.orderId, orig.orderId),
          exchangeId: getOrFallback(osr.exchangeId, orig.exchangeId),
          orderStatus: getOrFallback(osr.orderStatus, orig.orderStatus),
          rejectMessage: osr.rejectMessage,
          time: getOrFallback(osr.time, new Date().getTime()),
          lastQuantity: osr.lastQuantity,
          lastPrice: osr.lastPrice,
          isPong: getOrFallback(osr.isPong, orig.isPong),
          leavesQuantity: getOrFallback(osr.leavesQuantity, orig.leavesQuantity),
          cumQuantity: cumQuantity,
          averagePrice: cumQuantity > 0 ? osr.averagePrice || orig.averagePrice : undefined,
          liquidity: getOrFallback(osr.liquidity, orig.liquidity),
          exchange: getOrFallback(osr.exchange, orig.exchange),
          computationalLatency: getOrFallback(osr.computationalLatency, 0) + getOrFallback(orig.computationalLatency, 0),
          preferPostOnly: getOrFallback(osr.preferPostOnly, orig.preferPostOnly),
          source: getOrFallback(osr.source, orig.source)
        };

        this.updateOrderStatusInMemory(o);

        // cancel any open orders waiting for oid
        if (!this._oeGateway.cancelsByClientOrderId) {
          if (typeof o.exchangeId !== "undefined" && o.orderId in this._cancelsWaitingForExchangeOrderId) {
            // console.info('broker', 'Deleting', o.exchangeId, 'late, oid:', o.orderId);
            const cancel = this._cancelsWaitingForExchangeOrderId[o.orderId];
            delete this._cancelsWaitingForExchangeOrderId[o.orderId];
            this.cancelOrder(cancel);
            if (o.orderStatus===Models.OrderStatus.Working) return;
          }
        }

        this._evUp('OrderUpdateBroker', o);
        this._uiSend(Models.Topics.OrderStatusReports, o, true);

        if (osr.lastQuantity > 0) {
            let value = Math.abs(o.lastPrice * o.lastQuantity);

            const liq = o.liquidity;
            let feeCharged = null;
            if (typeof liq !== "undefined") {
                // negative fee is a rebate, positive fee is a fee
                feeCharged = (liq === Models.Liquidity.Make ? this._makeFee : this._takeFee);
                const sign = (o.side === Models.Side.Bid ? 1 : -1);
                value = value * (1 + sign * feeCharged);
            }

            const params = this._qpRepo();
            const trade = new Models.Trade(new Date().getTime().toString(), o.time, o.exchange, o.pair,
                o.lastPrice, o.lastQuantity, o.side, value, o.liquidity, null, 0, 0, 0, 0, feeCharged, false);
            this._evUp('OrderTradeBroker', trade);
            if (params.mode === Models.QuotingMode.Boomerang || params.mode === Models.QuotingMode.HamelinRat || params.mode === Models.QuotingMode.AK47) {
              var widthPong = (params.widthPercentage)
                  ? params.widthPongPercentage * trade.price / 100
                  : params.widthPong;
              this._reTrade(this.tradesMemory.filter((x: Models.Trade) => (
                (trade.side==Models.Side.Bid?(x.price > (trade.price + widthPong)):(x.price < (trade.price - widthPong)))
                && (x.side == (trade.side==Models.Side.Bid?Models.Side.Ask:Models.Side.Bid))
                && ((x.quantity - x.Kqty) > 0)
              )).sort((a: Models.Trade, b: Models.Trade) => (
                (params.pongAt == Models.PongAt.LongPingFair || params.pongAt == Models.PongAt.LongPingAggressive)
                  ? (
                  trade.side==Models.Side.Bid
                    ? (a.price<b.price?1:(a.price>b.price?-1:0))
                    : (a.price>b.price?1:(a.price<b.price?-1:0))
                ) : (
                  trade.side==Models.Side.Bid
                    ? (a.price>b.price?1:(a.price<b.price?-1:0))
                    : (a.price<b.price?1:(a.price>b.price?-1:0))
                )
              )), trade);
            } else {
              this._uiSend(Models.Topics.Trades, trade);
              this._dbInsert(Models.Topics.Trades, trade, false, trade.tradeId);
              this.tradesMemory.push(trade);
            }

            this._uiSend(Models.Topics.TradesChart, new Models.TradeChart(o.lastPrice, o.side, o.lastQuantity, Math.round(value * 100) / 100, o.isPong));

            if (params.cleanPongsAuto != 0) {
              const cleanTime = o.time - (Math.abs(params.cleanPongsAuto) * 864e5);
              var cleanTrades = this.tradesMemory.filter((x: Models.Trade) => (params.cleanPongsAuto < 0 ? true : x.Kqty >= x.quantity) && x.time < cleanTime);
              var goWhile = true;
              while (goWhile && cleanTrades.length) {
                var cleanTrade = cleanTrades.shift();
                goWhile = false;
                for(let i = this.tradesMemory.length;i--;) {
                  if (this.tradesMemory[i].tradeId==cleanTrade.tradeId) {
                    goWhile = true;
                    this.tradesMemory[i].Kqty = -1;
                    this._uiSend(Models.Topics.Trades, this.tradesMemory[i]);
                    this._dbInsert(Models.Topics.Trades, undefined, false, this.tradesMemory[i].tradeId);
                    this.tradesMemory.splice(i, 1);
                  }
                }
              }
            }
        }

        return o;
    };

    private updateOrderStatusInMemory = (osr : Models.OrderStatusReport) => {
        if (osr.orderStatus != Models.OrderStatus.Cancelled && osr.orderStatus != Models.OrderStatus.Complete) {
          if (typeof osr.exchangeId !== "undefined" && osr.exchangeId)
            this.orderCache.exchIdsToClientIds.set(osr.exchangeId, osr.orderId);
          this.orderCache.allOrders.set(osr.orderId, osr);
        } else {
          this.orderCache.exchIdsToClientIds.delete(osr.exchangeId);
          this.orderCache.allOrders.delete(osr.orderId);
        }
    };

    public orderCache: OrderStateCache;

    constructor(
      private _qpRepo,
      private _pair,
      private _makeFee,
      private _takeFee,
      private _minTick,
      private _exchange,
      private _oeGateway,
      private _dbInsert,
      private _uiSnap,
      private _uiHand,
      private _uiSend,
      private _evOn,
      private _evUp,
      initTrades : Models.Trade[]
    ) {
        this.tradesMemory = initTrades;
        this.orderCache = new OrderStateCache();

        _uiSnap(Models.Topics.Trades, () => this.tradesMemory.map(t => Object.assign(t, { loadedFromDB: true})).slice(-1000));
        _uiSnap(Models.Topics.OrderStatusReports, () => {
          let orderCache = [];
          this.orderCache.allOrders.forEach(x => {
            if (x.orderStatus === Models.OrderStatus.Working)
              orderCache.push(x);
          });
          return orderCache;
        });

        _uiHand(Models.Topics.SubmitNewOrder, (o : Models.OrderRequestFromUI) => {
            try {
              this.sendOrder(new Models.SubmitNewOrder(
                o.side == 'Ask' ? Models.Side.Ask : Models.Side.Bid,
                o.quantity,
                Models.OrderType[o.orderType],
                o.price,
                Models.TimeInForce[o.timeInForce],
                false,
                false,
                Models.OrderSource.OrderTicket
              ));
            }
            catch (e) {
              console.error('broker', e, 'unhandled exception while submitting order', o);
            }
        });

        _uiHand(Models.Topics.CancelOrder, o => this.cancelOrder(new Models.OrderCancel(o.orderId)));
        _uiHand(Models.Topics.CancelAllOrders, () => this.cancelOpenOrders());
        _uiHand(Models.Topics.CleanAllClosedOrders, () => this.cleanClosedOrders());
        _uiHand(Models.Topics.CleanAllOrders, () => this.cleanOrders());
        _uiHand(Models.Topics.CleanTrade, t => this.cleanTrade(t.tradeId));

        this._evOn('OrderUpdateGateway', this.updateOrderState);
    }
}

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
        this._allOrders.forEach(x => {
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
