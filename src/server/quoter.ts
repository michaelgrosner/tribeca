import Models = require("../share/models");
import Utils = require("./utils");
import Broker = require("./broker");
import QuotingParameters = require("./quoting-parameters");
import moment = require("moment");

export class Quoter {
  private _bidQuoter: ExchangeQuoter;
  private _askQuoter: ExchangeQuoter;

  constructor(
    broker: Broker.ExchangeBroker,
    orderBroker: Broker.OrderBroker,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository
  ) {
    this._bidQuoter = new ExchangeQuoter(broker, orderBroker, Models.Side.Bid, this._qlParamRepo);
    this._askQuoter = new ExchangeQuoter(broker, orderBroker, Models.Side.Ask, this._qlParamRepo);
  }

  public updateQuote = (q: Models.Timestamped<Models.Quote>, side: Models.Side) => {
    this[side === Models.Side.Ask ? '_askQuoter' : '_bidQuoter'].updateQuote(q);
  };

  public cancelQuote = (s: Models.Timestamped<Models.Side>) => {
    this[s.data === Models.Side.Ask ? '_askQuoter' : '_bidQuoter'].cancelQuote(s.time);
  };
}

export class ExchangeQuoter {
  private _exchange: Models.Exchange;

  constructor(
    private _broker: Broker.ExchangeBroker,
    private _orderBroker: Broker.OrderBroker,
    private _side: Models.Side,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository
  ) {
    this._exchange = _broker.exchange();
  }

  private orderCacheSide = (sort?: boolean) => {
    let orderSide = Array.from(this._orderBroker.orderCache.allOrders.values()).filter(x => this._side === x.side);
    if (sort) {
      if (this._side === Models.Side.Bid)
        orderSide.sort(function(a,b){return a.price<b.price?1:(a.price>b.price?-1:0);});
      else orderSide.sort(function(a,b){return a.price>b.price?1:(a.price<b.price?-1:0);});
    }
    return orderSide;
  };

  public updateQuote = (q: Models.Timestamped<Models.Quote>) => {
    if (this._broker.connectStatus !== Models.ConnectivityStatus.Connected)
      return;

    const orderSide = this.orderCacheSide();
    const dupe = orderSide.filter(x => q.data.price.toFixed(8) === x.price.toFixed(8)).length;

    if (this._qlParamRepo.latest.mode !== Models.QuotingMode.AK47) {
      if (orderSide.length) {
        if (!dupe) this.modify(q);
      } else this.start(q);
      return;
    }

    if (!dupe && orderSide.length >= this._qlParamRepo.latest.bullets) {
      this.modify(q);
      return;
    }

    this.start(q);
  };

  private cancelWorstQuotes = (price: number, time: Date) => {
    this.orderCacheSide().filter(x =>
      this._side === Models.Side.Bid
        ? price < x.price
        : price > x.price
    ).forEach(x =>
      this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, time))
    );
  };

  public cancelQuote = (t: Date) => {
    if (this._broker.connectStatus !== Models.ConnectivityStatus.Connected) return;
    this.stop(t);
  };

  private modify = (q: Models.Timestamped<Models.Quote>) => {
    if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) this.stopWorst(q.time);
    else this.stop(q.time);
    this.start(q);
  };

  private start = (q: Models.Timestamped<Models.Quote>) => {
    let price: number = q.data.price;
    let orderSide = this.orderCacheSide(true);
    if (orderSide.filter(x => price.toFixed(8) == x.price.toFixed(8)
      || (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47 &&
           ((price + (this._qlParamRepo.latest.range - 1e-2)) >= x.price
           && (price - (this._qlParamRepo.latest.range - 1e-2)) <= x.price))
    ).length) {
      if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) {
        if (orderSide.length<this._qlParamRepo.latest.bullets) {
          let incPrice: number = (this._qlParamRepo.latest.range * (this._side === Models.Side.Bid ? -1 : 1 ));
          let oldPrice:number = 0;
          let len:number  = 0;
          orderSide.forEach(x => {
            if (oldPrice>0 && (this._side === Models.Side.Bid?x.price<price:x.price>price)) {
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
          this.cancelWorstQuotes(q.data.price, q.time);
          price = Utils.roundNearest(price, this._broker.minTickIncrement);
          q.data.price = price;
        } else return;
      } else return;
    }

    this._orderBroker.sendOrder(new Models.SubmitNewOrder(
      this._side,
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

  private stopWorst = (t: Date) => {
    const orderSide = this.orderCacheSide(true);
    if (orderSide.length)
      this._orderBroker.cancelOrder(new Models.OrderCancel(orderSide.shift().orderId, this._exchange, t));
  };

  private stop = (t: Date) => {
    this.orderCacheSide().forEach(x =>
      this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, t))
    );
  };
}
