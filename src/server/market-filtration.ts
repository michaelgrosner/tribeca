import Models = require("../share/models");
import Utils = require("./utils");
import Broker = require("./broker");
import Quoter = require("./quoter");

export class MarketFiltration {
  private _latest: Models.Market = null;
  public FilteredMarketChanged = new Utils.Evt<Models.Market>();

  public get latestFilteredMarket() { return this._latest; }
  public set latestFilteredMarket(val: Models.Market) {
    this._latest = val;
    this.FilteredMarketChanged.trigger();
  }

  constructor(
      private _minTick: number,
      private _orderBroker: Broker.OrderBroker,
      private _marketBroker: Broker.MarketDataBroker
  ) {
    _marketBroker.MarketData.on(this.filterFullMarket);
  }

  private filterFullMarket = () => {
    var mkt = this._marketBroker.currentBook;

    if (mkt == null || !mkt.bids.length || !mkt.asks.length) {
      this.latestFilteredMarket = null;
      return;
    }

    var ask = this.filterMarket(mkt.asks, Models.Side.Ask);
    var bid = this.filterMarket(mkt.bids, Models.Side.Bid);

    if (!bid.length || !ask.length) {
      this.latestFilteredMarket = null;
      return;
    }

    this.latestFilteredMarket = new Models.Market(bid, ask, mkt.time);
  };

  private filterMarket = (mkts: Models.MarketSide[], s: Models.Side): Models.MarketSide[]=> {
    let copiedMkts = [];
    for (var i = 0; i < mkts.length; i++) {
        copiedMkts.push(new Models.MarketSide(mkts[i].price, mkts[i].size))
    }

    this._orderBroker.orderCache.allOrders.forEach(x => {
      if (x.side !== s) return;
      for (var i = 0; i < copiedMkts.length; i++) {
        if (Math.abs(x.price - copiedMkts[i].price) < this._minTick)
          copiedMkts[i].size -= x.quantity;
      }
    });

    return copiedMkts.filter(x => x.size > 1e-3);
  };
}
