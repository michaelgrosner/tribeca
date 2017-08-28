import Models = require("../share/models");
import Utils = require("./utils");
import moment = require("moment");

export class QuoteSender {
  private _latestStatus = new Models.TwoSidedQuoteStatus(Models.QuoteStatus.MissingData, Models.QuoteStatus.MissingData, 0, 0, 0);
  private _lastStart: number = new Date().getTime();
  private _timeoutStart: number = 0;
  private _brokerStatus = Models.ConnectivityStatus.Disconnected;
  private _brokerState: boolean = false;
  constructor(
    private _latestQuote,
    private _latestQuoteBidStatus,
    private _latestQuoteAskStatus,
    private _allOrders,
    private _allOrdersDelete,
    private _cancelOrder,
    private _sendOrder,
    private _minTick,
    private _qpRepo,
    private _uiSnap,
    private _uiSend,
    private _evOn
  ) {
    this._evOn('Quote', this.sendQuote);
    this._evOn('ExchangeConnect', (statusState) => {
      this._brokerStatus = statusState.status;
      this._brokerState = statusState.state;
      this.sendQuote();
    });
    _uiSnap(Models.Topics.QuoteStatus, () => [this._latestStatus]);
  }

  private checkCrossedQuotes = (side: Models.Side, px: number): boolean => {
    var oppSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;

    var doesQuoteCross = oppSide === Models.Side.Bid
      ? (a, b) => a.price >= b
      : (a, b) => a.price <= b;

    let qs = this._latestQuote()[oppSide === Models.Side.Bid ? 'bid' : 'ask'];
    if (qs && doesQuoteCross(qs.price, px)) {
      console.warn('quotesender', 'crossing quote detected! gen quote at', px, 'would crossed with', Models.Side[oppSide], 'quote at', qs);
      return true;
    }
    return false;
  };

  private sendQuote = (): void => {
    var quote = this._latestQuote();

    let askStatus = this._latestQuoteAskStatus();
    let bidStatus = this._latestQuoteBidStatus();

    if (quote !== null && this._brokerStatus === Models.ConnectivityStatus.Connected) {
      if (this._brokerState) {
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
    var oIx = [];
    var oIt = new Date().getTime();
    this._allOrders().forEach(x => {
      if (x.orderStatus === Models.OrderStatus.New) {
        if (!x.exchangeId && oIt-10000>x.time)
          oIx.push(x.orderId)
        else ++quotesInMemoryNew;
      }
      else if (x.orderStatus === Models.OrderStatus.Working) ++quotesInMemoryWorking;
      else ++quotesInMemoryDone;
    });
    while(oIx.length) this._allOrdersDelete(oIx.pop());

    if (bidStatus === this._latestStatus.bidStatus && askStatus === this._latestStatus.askStatus && quotesInMemoryNew === this._latestStatus.quotesInMemoryNew && quotesInMemoryWorking === this._latestStatus.quotesInMemoryWorking && quotesInMemoryDone === this._latestStatus.quotesInMemoryDone) return;

    this._latestStatus = new Models.TwoSidedQuoteStatus(bidStatus, askStatus, quotesInMemoryNew, quotesInMemoryWorking, quotesInMemoryDone);
    this._uiSend(Models.Topics.QuoteStatus, this._latestStatus, true);
  };

  private updateQuote = (q: Models.Quote, side: Models.Side) => {
    const orderSide = this.orderCacheSide(side, false);
    const dupe = orderSide.filter(x => q.price.toFixed(8) === x.price.toFixed(8)).length;

    if (this._qpRepo().mode !== Models.QuotingMode.AK47) {
      if (orderSide.length) {
        if (!dupe) this.modify(side, q);
      } else this.start(side, q);
      return;
    }

    if (!dupe && orderSide.length >= this._qpRepo().bullets) {
      this.modify(side, q);
      return;
    }

    this.start(side, q);
  };

  private orderCacheSide = (side: Models.Side, sort: boolean) => {
    let orderSide = [];
    this._allOrders().forEach(x => {
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
    if (this._qpRepo().mode === Models.QuotingMode.AK47) this.stopWorstQuote(side);
    else this.stopAllQuotes(side);
    this.start(side, q);
  };

  private start = (side: Models.Side, q: Models.Quote) => {
    const params = this._qpRepo();
    if (params.delayAPI > 0) {
      if (this._timeoutStart) clearTimeout(this._timeoutStart);
      var nextStart = this._lastStart + (60000/params.delayAPI);
      var diffStart = nextStart - new Date().getTime();
      if (diffStart>0) {
        this._timeoutStart = setTimeout(() => this.start(side, q), moment.duration(diffStart/1000, 'seconds'));
        return;
      }
      this._lastStart = new Date().getTime();
    }
    let price: number = q.price;
    let orderSide = this.orderCacheSide(side, true);
    if (orderSide.filter(x => price.toFixed(8) == x.price.toFixed(8)
      || (params.mode === Models.QuotingMode.AK47 &&
         ((price + (params.range - 1e-2)) >= x.price
         && (price - (params.range - 1e-2)) <= x.price))
    ).length) {
      if (params.mode === Models.QuotingMode.AK47) {
        if (orderSide.length<params.bullets) {
          let incPrice: number = (params.range * (side === Models.Side.Bid ? -1 : 1 ));
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
            (price + (params.range - 1e-2)) >= x.price
            && (price - (params.range - 1e-2)) <= x.price
          ).length) return;
          this.stopWorstsQuotes(side, q.price);
          price = Utils.roundNearest(price, this._minTick);
          q.price = price;
        } else return;
      } else return;
    }

    this._sendOrder(side, price, q.size, Models.OrderType.Limit, Models.TimeInForce.GTC, q.isPong, true);
  };

  private stopWorstsQuotes = (side: Models.Side, price: number) => {
    this.orderCacheSide(side, false).filter(x =>
      side === Models.Side.Bid
        ? price < x.price
        : price > x.price
    ).forEach(x =>
      this._cancelOrder(x.orderId)
    );
  };

  private stopWorstQuote = (side: Models.Side) => {
    const orderSide = this.orderCacheSide(side, true);
    if (orderSide.length)
      this._cancelOrder(orderSide.shift().orderId);
  };

  private stopAllQuotes = (side: Models.Side) => {
    this.orderCacheSide(side, false).forEach(x =>
      this._cancelOrder(x.orderId)
    );
  };
}
