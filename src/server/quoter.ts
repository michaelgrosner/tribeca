import Models = require("../share/models");
import Utils = require("./utils");
import Broker = require("./broker");
import QuotingParameters = require("./quoting-parameters");
import moment = require("moment");

class QuoteOrder {
    constructor(public quote: Models.Quote, public orderId: string) { }
}

export class Quoter {
    private _bidQuoter: ExchangeQuoter;
    private _askQuoter: ExchangeQuoter;

    constructor(private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        broker: Broker.OrderBroker,
        exchBroker: Broker.ExchangeBroker) {
        this._bidQuoter = new ExchangeQuoter(broker, exchBroker, Models.Side.Bid, this._qlParamRepo);
        this._askQuoter = new ExchangeQuoter(broker, exchBroker, Models.Side.Ask, this._qlParamRepo);
    }

    public updateQuote = (q: Models.Timestamped<Models.Quote>, side: Models.Side): Models.QuoteSent => {
        switch (side) {
            case Models.Side.Ask:
                return this._askQuoter.updateQuote(q);
            case Models.Side.Bid:
                return this._bidQuoter.updateQuote(q);
        }
    };

    public cancelQuote = (s: Models.Timestamped<Models.Side>): Models.QuoteSent => {
        switch (s.data) {
            case Models.Side.Ask:
                return this._askQuoter.cancelQuote(s.time);
            case Models.Side.Bid:
                return this._bidQuoter.cancelQuote(s.time);
        }
    };

    public quotesActive = (s: Models.Side): QuoteOrder[] => {
        switch (s) {
            case Models.Side.Ask:
                return this._askQuoter.activeQuote;
            case Models.Side.Bid:
                return this._bidQuoter.activeQuote;
        }
    };
}

export class ExchangeQuoter {
    public activeQuote: QuoteOrder[] = [];
    private _exchange: Models.Exchange;

    constructor(private _broker: Broker.OrderBroker,
        private _exchBroker: Broker.ExchangeBroker,
        private _side: Models.Side,
        private _qlParamRepo: QuotingParameters.QuotingParametersRepository) {
        this._exchange = _exchBroker.exchange();
        this._broker.OrderUpdate.on(this.handleOrderUpdate);
    }

    private handleOrderUpdate = (o: Models.OrderStatusReport) => {
        switch (o.orderStatus) {
            case Models.OrderStatus.Cancelled:
            case Models.OrderStatus.Complete:
            case Models.OrderStatus.Rejected:
                this.activeQuote = this.activeQuote.filter(q => q.orderId !== o.orderId);
        }
    };

    public updateQuote = (q: Models.Timestamped<Models.Quote>): Models.QuoteSent => {
      if (this._exchBroker.connectStatus !== Models.ConnectivityStatus.Connected)
        return Models.QuoteSent.UnableToSend;

      let dupe = this.activeQuote.filter(o => q.data.price === o.quote.price).length;

      if (this._qlParamRepo.latest.mode !== Models.QuotingMode.AK47)
        return this.activeQuote.length
          ? (dupe ? Models.QuoteSent.UnsentDuplicate : this.modify(q))
          : this.start(q);

      if (this.activeQuote.length >= this._qlParamRepo.latest.bullets)
        return dupe ? Models.QuoteSent.UnsentDuplicate : this.modify(q);

      return this.start(q);
    };

    public cancelHigherQuotes = (price: number, time: Date) => {
      this.activeQuote.filter(o =>
        this._side === Models.Side.Bid
          ? price < o.quote.price
          : price > o.quote.price
      ).map(o => {
        var cxl = new Models.OrderCancel(o.orderId, this._exchange, time);
        this._broker.cancelOrder(cxl);
        this.activeQuote = this.activeQuote.filter(q => q.orderId !== cxl.origOrderId);
      });
    };

    public cancelQuote = (t: Date): Models.QuoteSent => {
        if (this._exchBroker.connectStatus !== Models.ConnectivityStatus.Connected)
            return Models.QuoteSent.UnableToSend;

        return this.stop(t);
    };

    private modify = (q: Models.Timestamped<Models.Quote>): Models.QuoteSent => {
        if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47)
          this.stopLowest(q.time);
        else this.stop(q.time);
        this.start(q);
        return Models.QuoteSent.Modify;
    };

    private start = (q: Models.Timestamped<Models.Quote>): Models.QuoteSent => {
        let price: number = q.data.price;
        if (this.activeQuote.filter(o =>
          (price + (this._qlParamRepo.latest.range - 1e-2)) >= o.quote.price
          && (price - (this._qlParamRepo.latest.range - 1e-2)) <= o.quote.price
        ).length) {
          if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) {
            if (this.activeQuote.length<this._qlParamRepo.latest.bullets) {
              let incPrice: number = (this._qlParamRepo.latest.range * (this._side === Models.Side.Bid ? -1 : 1 ));
              let oldPrice:number = 0;
              let len:number  = 0;
              this.activeQuote.forEach(activeQuote => {
                if (oldPrice>0 && (this._side === Models.Side.Bid?activeQuote.quote.price<price:activeQuote.quote.price>price)) {
                  price = oldPrice + incPrice;
                  if (Math.abs(activeQuote.quote.price - oldPrice)>incPrice) return false;
                }
                oldPrice = activeQuote.quote.price;
                ++len;
              });
              if (len==this.activeQuote.length) price = this.activeQuote.slice(-1).pop().quote.price + incPrice;
              if (this.activeQuote.filter(o =>
                (price + (this._qlParamRepo.latest.range - 1e-2)) >= o.quote.price
                && (price - (this._qlParamRepo.latest.range - 1e-2)) <= o.quote.price
              ).length)
                return Models.QuoteSent.UnsentDuplicate;
              this.cancelHigherQuotes(q.data.price, q.time);
              price = Utils.roundNearest(price, this._exchBroker.minTickIncrement);
              q.data.price = price;
            } else
              return Models.QuoteSent.UnsentDuplicate;
          } else
            return Models.QuoteSent.UnsentDuplicate;
        }

        const quoteOrder = new QuoteOrder(q.data, this._broker.sendOrder(
          new Models.SubmitNewOrder(this._side, q.data.size, Models.OrderType.Limit,
            price, Models.TimeInForce.GTC, this._exchange, q.time, true, Models.OrderSource.Quote)
        ).sentOrderClientId);

        this.activeQuote.push(quoteOrder);
        if (this._side === Models.Side.Bid)
          this.activeQuote.sort(function(a,b){return a.quote.price<b.quote.price?1:(a.quote.price>b.quote.price?-1:0);});
        else this.activeQuote.sort(function(a,b){return a.quote.price>b.quote.price?1:(a.quote.price<b.quote.price?-1:0);});

        return Models.QuoteSent.First;
    };

    private stopLowest = (t: Date): Models.QuoteSent => {
        if (!this.activeQuote.length) {
            return Models.QuoteSent.UnsentDelete;
        }
        const cxl = new Models.OrderCancel(this.activeQuote.shift().orderId, this._exchange, t);
        this._broker.cancelOrder(cxl);
        this.activeQuote = this.activeQuote.filter(q => q.orderId !== cxl.origOrderId);

        return Models.QuoteSent.Delete;
    };

    private stop = (t: Date): Models.QuoteSent => {
        if (!this.activeQuote.length)
            return Models.QuoteSent.UnsentDelete;

        this.activeQuote.forEach((q: QuoteOrder) => this._broker.cancelOrder(new Models.OrderCancel(q.orderId, this._exchange, t)));
        this.activeQuote = [];
        return Models.QuoteSent.Delete;
    };
}
