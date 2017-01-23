/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
///<reference path="interfaces.ts"/>

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import QuotingParameters = require("./quoting-parameters");
import _ = require('lodash');
import moment = require("moment");

class QuoteOrder {
    constructor(public quote: Models.Quote, public orderId: string) { }
}

// aggregator for quoting
export class Quoter {
    private _bidQuoter: ExchangeQuoter;
    private _askQuoter: ExchangeQuoter;

    constructor(private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
        broker: Interfaces.IOrderBroker,
        exchBroker: Interfaces.IBroker) {
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

    public quotesSent = (s: Models.Side) => {
        switch (s) {
            case Models.Side.Ask:
                return this._askQuoter.quotesSent;
            case Models.Side.Bid:
                return this._bidQuoter.quotesSent;
        }
    };
}

// wraps a single broker to make orders behave like quotes
export class ExchangeQuoter {
    public quotesSent: QuoteOrder[] = [];

    private _activeQuote: QuoteOrder[] = [];
    private _exchange: Models.Exchange;

    constructor(private _broker: Interfaces.IOrderBroker,
        private _exchBroker: Interfaces.IBroker,
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
                this._activeQuote = this._activeQuote.filter(q => q.orderId !== o.orderId);
                this.quotesSent = this.quotesSent.filter(q => q.orderId !== o.orderId);
        }
    };

    public updateQuote = (q: Models.Timestamped<Models.Quote>): Models.QuoteSent => {
      if (this._exchBroker.connectStatus !== Models.ConnectivityStatus.Connected)
        return Models.QuoteSent.UnableToSend;

      let dupe = this._activeQuote.filter(o => q.data.price === o.quote.price).length;

      if (this._qlParamRepo.latest.mode !== Models.QuotingMode.AK47)
        return this._activeQuote.length
          ? (dupe ? Models.QuoteSent.UnsentDuplicate : this.modify(q))
          : this.start(q);

      if (this.quotesSent.length >= this._qlParamRepo.latest.bullets)
        return dupe ? Models.QuoteSent.UnsentDuplicate : this.modify(q);

      return this.start(q);
    };

    public cancelHigherQuotes = (price: number, time: moment.Moment) => {
      this._activeQuote.filter(o =>
        this._side === Models.Side.Bid
          ? price < o.quote.price
          : price > o.quote.price
      ).map(o => {
        var cxl = new Models.OrderCancel(o.orderId, this._exchange, time);
        this._broker.cancelOrder(cxl);
        this._activeQuote = this._activeQuote.filter(q => q.orderId !== cxl.origOrderId);
      });
    };

    public cancelQuote = (t: moment.Moment): Models.QuoteSent => {
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
        if (this._activeQuote.filter(o =>
          (price + (this._qlParamRepo.latest.range - 1e-2)) >= o.quote.price
          && (price - (this._qlParamRepo.latest.range - 1e-2)) <= o.quote.price
        ).length) {
          if (this._qlParamRepo.latest.mode === Models.QuotingMode.AK47) {
            if (this.quotesSent.length<this._qlParamRepo.latest.bullets) {
              price = Utils.roundFloat(
                _.last(this._activeQuote).quote.price
                + (this._qlParamRepo.latest.range * (this._side === Models.Side.Bid ? -1 : 1 ))
              );
              if (this.quotesSent.filter(o => price === o.quote.price).length)
                return Models.QuoteSent.UnsentDuplicate;
              this.cancelHigherQuotes(q.data.price, q.time);
              q.data.price = price;
            } else
              return Models.QuoteSent.UnsentDuplicate;
          } else
            return Models.QuoteSent.UnsentDuplicate;
        }

        var quoteOrder = new QuoteOrder(q.data, this._broker.sendOrder(
          new Models.SubmitNewOrder(this._side, q.data.size, Models.OrderType.Limit,
            price, Models.TimeInForce.GTC, this._exchange, q.time, true)
        ).sentOrderClientId);

        this.quotesSent.push(quoteOrder);
        this._activeQuote.push(quoteOrder);
        if (this._side === Models.Side.Bid)
          this._activeQuote.sort(function(a,b){return a.quote.price<b.quote.price?1:(a.quote.price>b.quote.price?-1:0);});
        else this._activeQuote.sort(function(a,b){return a.quote.price>b.quote.price?1:(a.quote.price<b.quote.price?-1:0);});

        return Models.QuoteSent.First;
    };

    private stopLowest = (t: moment.Moment): Models.QuoteSent => {
        if (!this._activeQuote.length) {
            return Models.QuoteSent.UnsentDelete;
        }
        var cxl = new Models.OrderCancel(this._activeQuote.shift().orderId, this._exchange, t);
        this._broker.cancelOrder(cxl);
        this._activeQuote = this._activeQuote.filter(q => q.orderId !== cxl.origOrderId);

        return Models.QuoteSent.Delete;
    };

    private stop = (t: moment.Moment): Models.QuoteSent => {
        if (!this._activeQuote.length)
            return Models.QuoteSent.UnsentDelete;

        _.map(this._activeQuote, (q: QuoteOrder) => this._broker.cancelOrder(new Models.OrderCancel(q.orderId, this._exchange, t)));
        this._activeQuote = [];
        return Models.QuoteSent.Delete;
    };
}
