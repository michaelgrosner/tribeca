import Models = require("../share/models");

export class MarketFiltration {
  private _latest: Models.Market = null;

  public get latestFilteredMarket() { return this._latest; }
  public set latestFilteredMarket(val: Models.Market) {
    this._latest = val;
    this._evUp('FilteredMarket');
  }

  constructor(
      private _minTick: number,
      private _allOrders,
      private _evOn,
      private _evUp
  ) {
    this._evOn('MarketDataBroker', mkt => this.filterFullMarket(mkt));
  }

  private filterFullMarket = (mkt) => {
    if (mkt == null || typeof mkt.bids == "undefined" || typeof mkt.asks == "undefined" || !mkt.bids.length || !mkt.asks.length) return;

    var ask = this.filterMarket(mkt.asks, Models.Side.Ask);
    var bid = this.filterMarket(mkt.bids, Models.Side.Bid);

    if (!bid.length || !ask.length) {
      this.latestFilteredMarket = null;
      return;
    }

    this.latestFilteredMarket = new Models.Market(bid, ask);
  };

  private filterMarket = (mkts: Models.MarketSide[], s: Models.Side): Models.MarketSide[]=> {
    let copiedMkts = [];
    for (var i = 0; i < mkts.length; i++) {
        copiedMkts.push(new Models.MarketSide(mkts[i].price, mkts[i].size))
    }

    this._allOrders().forEach(x => {
      if (x.side !== s) return;
      for (var i = 0; i < copiedMkts.length; i++) {
        if (Math.abs(x.price - copiedMkts[i].price) < this._minTick)
          copiedMkts[i].size -= x.quantity;
      }
    });

    return copiedMkts.filter(x => x.size > 1e-3);
  };
}
