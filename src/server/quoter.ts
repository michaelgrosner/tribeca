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
    switch (side) {
      case Models.Side.Ask:
        this._askQuoter.updateQuote(q);
      case Models.Side.Bid:
        this._bidQuoter.updateQuote(q);
    }
  };

  public cancelQuote = (s: Models.Timestamped<Models.Side>) => {
    switch (s.data) {
      case Models.Side.Ask:
        this._askQuoter.cancelQuote(s.time);
      case Models.Side.Bid:
        this._bidQuoter.cancelQuote(s.time);
    }
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

    public updateQuote = (q: Models.Timestamped<Models.Quote>) => {
      if (this._broker.connectStatus !== Models.ConnectivityStatus.Connected)
        return;

      let dupePrice = 0;
      let dupeSide = 0;
      this._orderBroker.orderCache.allOrders.forEach(x => {
        if (this._side === x.side) {
          dupeSide++;
          if (q.data.price === x.price) dupePrice++;
        }
      });

      if (this._qlParamRepo.latest.mode !== Models.QuotingMode.AK47) {
        if (!dupeSide) this.start(q);
        else if (!dupePrice) this.modify(q);
        return;
      }

      if (dupeSide >= this._qlParamRepo.latest.bullets && !dupePrice) {
        this.modify(q);
        return;
      }

      this.start(q);
    };

    private cancelWorstQuotes = (price: number, time: Date) => {
      this._orderBroker.orderCache.allOrders.forEach(x => {
        if (this._side === x.side && ((this._side === Models.Side.Bid && price < x.price) || price > x.price))
          this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, time));
      });
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
        let dupePrice = 0;
        let dupePriceAK47 = 0;
        let dupeSide = 0;
        let bestPrice = null;
        this._orderBroker.orderCache.allOrders.forEach(x => {
          if (this._side === x.side) {
            dupeSide++;
            if (price === x.price) dupePrice++;
            if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47
              && ((price + (this._qlParamRepo.latest.range - 1e-2)) >= x.price
                && (price - (this._qlParamRepo.latest.range - 1e-2)) <= x.price)) dupePriceAK47++;
            if (bestPrice === null || ((this._side === Models.Side.Bid && bestPrice.price > x.price) || bestPrice.price < x.price))
              bestPrice = x;
          }
        });
        if (dupePrice || dupePriceAK47) {
          if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) {
            if (dupeSide<this._qlParamRepo.latest.bullets) {
              let incPrice: number = (this._qlParamRepo.latest.range * (this._side === Models.Side.Bid ? -1 : 1 ));
              let oldPrice:number = 0;
              let len:number  = 0;
              this._orderBroker.orderCache.allOrders.forEach(x => {
                if (this._side === x.side) {
                  if (oldPrice>0 && (this._side === Models.Side.Bid?x.price<price:x.price>price)) {
                    price = oldPrice + incPrice;
                    if (Math.abs(x.price - oldPrice)>incPrice) return false;
                  }
                  oldPrice = x.price;
                  ++len;
                }
              });
              if (len==dupeSide && bestPrice) price = bestPrice.price + incPrice;
              dupePriceAK47 = 0;
              this._orderBroker.orderCache.allOrders.forEach(x => {
                if (this._side === x.side && this._qlParamRepo.latest.mode === Models.QuotingMode.AK47
                  && ((price + (this._qlParamRepo.latest.range - 1e-2)) >= x.price
                    && (price - (this._qlParamRepo.latest.range - 1e-2)) <= x.price)) dupePriceAK47++;
              });
              if (dupePriceAK47) return;
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
      let worst = null;
      this._orderBroker.orderCache.allOrders.forEach(x => {
        if (this._side === x.side && (worst === null || ((this._side === Models.Side.Bid && worst.price < x.price) || worst.price > x.price)))
          worst = x;
      });
      if (worst)
        this._orderBroker.cancelOrder(new Models.OrderCancel(worst.orderId, this._exchange, t));
    };

    private stop = (t: Date) => {
      this._orderBroker.orderCache.allOrders.forEach(x => {
        if (this._side === x.side)
          this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, t));
      });
    };
}
