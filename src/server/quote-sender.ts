import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Broker = require("./broker");
import QuotingEngine = require("./quoting-engine");
import QuotingParameters = require("./quoting-parameters");

export class QuoteSender {
  private _exchange: Models.Exchange;
  private _latestStatus = new Models.TwoSidedQuoteStatus(Models.QuoteStatus.MissingData, Models.QuoteStatus.MissingData, 0, 0, 0);

  constructor(
    private _timeProvider: Utils.ITimeProvider,
    private _quotingEngine: QuotingEngine.QuotingEngine,
    private _broker: Broker.ExchangeBroker,
    private _orderBroker: Broker.OrderBroker,
    private _qpRepo: QuotingParameters.QuotingParametersRepository,
    private _publisher: Publish.Publisher,
    private _evOn
  ) {
    this._exchange = _broker.exchange();

    this._evOn('Quote', this.sendQuote);
    this._evOn('ExchangeConnect', this.sendQuote);
    _publisher.registerSnapshot(Models.Topics.QuoteStatus, () => [this._latestStatus]);
  }

  private checkCrossedQuotes = (side: Models.Side, px: number): boolean => {
    if (this._broker.hasSelfTradePrevention) return false;

    var oppSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;

    var doesQuoteCross = oppSide === Models.Side.Bid
      ? (a, b) => a.price >= b
      : (a, b) => a.price <= b;

    let qs = this._quotingEngine.latestQuote[oppSide === Models.Side.Bid ? 'bid' : 'ask'];
    if (qs && doesQuoteCross(qs.price, px)) {
      console.warn('quotesender', 'crossing quote detected! gen quote at', px, 'would crossed with', Models.Side[oppSide], 'quote at', qs);
      return true;
    }
    return false;
  };

  private sendQuote = (): void => {
    var quote = this._quotingEngine.latestQuote;

    let askStatus = this._quotingEngine.latestQuoteAskStatus;
    let bidStatus = this._quotingEngine.latestQuoteBidStatus;

    if (quote !== null && this._broker.connectStatus === Models.ConnectivityStatus.Connected) {
      if (this._broker.latestState) {
        if (quote.ask !== null)
          askStatus = !this.checkCrossedQuotes(Models.Side.Ask, quote.ask.price)
            ? Models.QuoteStatus.Live
            : Models.QuoteStatus.Crossed;

        if (quote.bid !== null)
          bidStatus = !this.checkCrossedQuotes(Models.Side.Bid, quote.bid.price)
            ? Models.QuoteStatus.Live
            : Models.QuoteStatus.Crossed;
      } else {
        askStatus = Models.QuoteStatus.DisabledQuotes;
        bidStatus = Models.QuoteStatus.DisabledQuotes;
      }

      if (askStatus === Models.QuoteStatus.Live)
        this.updateQuote(quote.ask, Models.Side.Ask);
      else this.stopAllQuotes(Models.Side.Ask);

      if (bidStatus === Models.QuoteStatus.Live)
        this.updateQuote(quote.bid, Models.Side.Bid);
      else this.stopAllQuotes(Models.Side.Bid);
    } else {
      askStatus = Models.QuoteStatus.Disconnected;
      bidStatus = Models.QuoteStatus.Disconnected;
    }

    var quotesInMemoryNew = 0;
    var quotesInMemoryWorking = 0;
    var quotesInMemoryDone = 0;
    this._orderBroker.orderCache.allOrders.forEach(x => {
      if (x.orderStatus === Models.OrderStatus.New) ++quotesInMemoryNew;
      else if (x.orderStatus === Models.OrderStatus.Working) ++quotesInMemoryWorking;
      else ++quotesInMemoryDone;
    });
    if (bidStatus === this._latestStatus.bidStatus && askStatus === this._latestStatus.askStatus && quotesInMemoryNew === this._latestStatus.quotesInMemoryNew && quotesInMemoryWorking === this._latestStatus.quotesInMemoryWorking && quotesInMemoryDone === this._latestStatus.quotesInMemoryDone) return;

    this._latestStatus = new Models.TwoSidedQuoteStatus(bidStatus, askStatus, quotesInMemoryNew, quotesInMemoryWorking, quotesInMemoryDone);
    this._publisher.publish(Models.Topics.QuoteStatus, this._latestStatus, true);
  };

  private updateQuote = (q: Models.Quote, side: Models.Side) => {
    const orderSide = this.orderCacheSide(side, false);
    const dupe = orderSide.filter(x => q.price.toFixed(8) === x.price.toFixed(8)).length;

    if (this._qpRepo.latest.mode !== Models.QuotingMode.AK47) {
      if (orderSide.length) {
        if (!dupe) this.modify(side, q);
      } else this.start(side, q);
      return;
    }

    if (!dupe && orderSide.length >= this._qpRepo.latest.bullets) {
      this.modify(side, q);
      return;
    }

    this.start(side, q);
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

  private modify = (side: Models.Side, q: Models.Quote) => {
    if (this._qpRepo.latest.mode === Models.QuotingMode.AK47) this.stopWorstQuote(side);
    else this.stopAllQuotes(side);
    this.start(side, q);
  };

  private start = (side: Models.Side, q: Models.Quote) => {
    let price: number = q.price;
    let orderSide = this.orderCacheSide(side, true);
    if (orderSide.filter(x => price.toFixed(8) == x.price.toFixed(8)
      || (this._qpRepo.latest.mode === Models.QuotingMode.AK47 &&
           ((price + (this._qpRepo.latest.range - 1e-2)) >= x.price
           && (price - (this._qpRepo.latest.range - 1e-2)) <= x.price))
    ).length) {
      if (this._qpRepo.latest.mode === Models.QuotingMode.AK47) {
        if (orderSide.length<this._qpRepo.latest.bullets) {
          let incPrice: number = (this._qpRepo.latest.range * (side === Models.Side.Bid ? -1 : 1 ));
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
            (price + (this._qpRepo.latest.range - 1e-2)) >= x.price
            && (price - (this._qpRepo.latest.range - 1e-2)) <= x.price
          ).length) return;
          this.stopWorstsQuotes(side, q.price);
          price = Utils.roundNearest(price, this._broker.minTickIncrement);
          q.price = price;
        } else return;
      } else return;
    }

    this._orderBroker.sendOrder(new Models.SubmitNewOrder(
      side,
      q.size,
      Models.OrderType.Limit,
      price,
      Models.TimeInForce.GTC,
      q.isPong,
      this._exchange,
      this._timeProvider.utcNow(),
      true,
      Models.OrderSource.Quote
    ));
  };

  private stopWorstsQuotes = (side: Models.Side, price: number) => {
    this.orderCacheSide(side, false).filter(x =>
      side === Models.Side.Bid
        ? price < x.price
        : price > x.price
    ).forEach(x =>
      this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, this._timeProvider.utcNow()))
    );
  };

  private stopWorstQuote = (side: Models.Side) => {
    const orderSide = this.orderCacheSide(side, true);
    if (orderSide.length)
      this._orderBroker.cancelOrder(new Models.OrderCancel(orderSide.shift().orderId, this._exchange, this._timeProvider.utcNow()));
  };

  private stopAllQuotes = (side: Models.Side) => {
    this.orderCacheSide(side, false).forEach(x =>
      this._orderBroker.cancelOrder(new Models.OrderCancel(x.orderId, this._exchange, this._timeProvider.utcNow()))
    );
  };
}
