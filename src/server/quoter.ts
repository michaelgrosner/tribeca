import Models = require("../share/models");
import Utils = require("./utils");
import Broker = require("./broker");
import QuotingParameters = require("./quoting-parameters");
import moment = require("moment");

export class Quoter {
  private _exchange: Models.Exchange;

  constructor(
    private _broker: Broker.ExchangeBroker,
    private _orderBroker: Broker.OrderBroker,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository
  ) {
    this._exchange = _broker.exchange();
  }

  public updateQuote = (q: Models.Timestamped<Models.Quote>, side: Models.Side) => {
    if (this._broker.connectStatus !== Models.ConnectivityStatus.Connected)
      return;

    const orderSide = this.orderCacheSide(side, false);
    const dupe = orderSide.filter(x => q.data.price.toFixed(8) === x.price.toFixed(8)).length;

    if (this._qlParamRepo.latest.mode !== Models.QuotingMode.AK47) {
      if (orderSide.length) {
        if (!dupe) this.modify(side, q);
      } else this.start(side, q);
      return;
    }

    if (!dupe && orderSide.length >= this._qlParamRepo.latest.bullets) {
      this.modify(side, q);
      return;
    }

    this.start(side, q);
  };

  public cancelQuote = (s: Models.Timestamped<Models.Side>) => {
    if (this._broker.connectStatus !== Models.ConnectivityStatus.Connected) return;
    this.stop(s.data, s.time);
  };

  private orderCacheSide = (side: Models.Side, sort: boolean) => {
    let orderSide = [];
    this._orderBroker.orderCache.allOrders.forEach(x => {
      if (side === x.side) orderSide.push(x);
    });
    if (sort) {
      if (side === Models.Side.Bid)
        orderSide.sort(function(a,b){return a.price<b.price?1:(a.price>b.price?-1:0);});
      else orderSide.sort(function(a,b){return a.price>b.price?1:(a.price<b.price?-1:0);});
    }
    return orderSide;
  };

  private cancelWorstQuotes = (side: Models.Side, price: number, time: Date) => {
    this.orderCacheSide(side, false).filter(x =>
      side === Models.Side.Bid
        ? price < x.price
        : price > x.price
    ).forEach(x =>
      this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, time))
    );
  };

  private modify = (side: Models.Side, q: Models.Timestamped<Models.Quote>) => {
    if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) this.stopWorst(side, q.time);
    else this.stop(side, q.time);
    this.start(side, q);
  };

  private start = (side: Models.Side, q: Models.Timestamped<Models.Quote>) => {
    let price: number = q.data.price;
    let orderSide = this.orderCacheSide(side, true);
    if (orderSide.filter(x => price.toFixed(8) == x.price.toFixed(8)
      || (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47 &&
           ((price + (this._qlParamRepo.latest.range - 1e-2)) >= x.price
           && (price - (this._qlParamRepo.latest.range - 1e-2)) <= x.price))
    ).length) {
      if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) {
        if (orderSide.length<this._qlParamRepo.latest.bullets) {
          let incPrice: number = (this._qlParamRepo.latest.range * (side === Models.Side.Bid ? -1 : 1 ));
          let oldPrice:number = 0;
          let len:number  = 0;
          orderSide.forEach(x => {
            if (oldPrice>0 && (side === Models.Side.Bid?x.price<price:x.price>price)) {
              price = oldPrice + incPrice;
              if (Math.abs(x.price - oldPrice)>incPrice) return false;
            }
            oldPrice = x.price;
            ++len;
          });
          if (len==orderSide.length) price = orderSide.slice(-1).pop().price + incPrice;
          if (orderSide.filter(x =>
            (price + (this._qlParamRepo.latest.range - 1e-2)) >= x.price
            && (price - (this._qlParamRepo.latest.range - 1e-2)) <= x.price
          ).length) return;
          this.cancelWorstQuotes(side, q.data.price, q.time);
          price = Utils.roundNearest(price, this._broker.minTickIncrement);
          q.data.price = price;
        } else return;
      } else return;
    }

    this._orderBroker.sendOrder(new Models.SubmitNewOrder(
      side,
      q.data.size,
      Models.OrderType.Limit,
      price,
      Models.TimeInForce.GTC,
      q.data.isPong,
      this._exchange,
      q.time,
      true,
      Models.OrderSource.Quote
    ));
  };

  private stopWorst = (side: Models.Side, t: Date) => {
    const orderSide = this.orderCacheSide(side, true);
    if (orderSide.length)
      this._orderBroker.cancelOrder(new Models.OrderCancel(orderSide.shift().orderId, this._exchange, t));
  };

  private stop = (side: Models.Side, t: Date) => {
    this.orderCacheSide(side, false).forEach(x =>
      this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, t))
    );
  };
}
