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
        private _details: Broker.ExchangeBroker,
        private _quoter: Quoter.Quoter,
        private _broker: Broker.MarketDataBroker) {
        _broker.MarketData.on(this.filterFullMarket);
    }

    private filterFullMarket = () => {
        var mkt = this._broker.currentBook;

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
        var rgq = this._quoter.quotesActive(s);

        var copiedMkts = [];
        for (var i = 0; i < mkts.length; i++) {
            copiedMkts.push(new Models.MarketSide(mkts[i].price, mkts[i].size))
        }

        for (var j = 0; j < rgq.length; j++) {
            var q = rgq[j].quote;

            for (var i = 0; i < copiedMkts.length; i++) {
                var m = copiedMkts[i];

                if (Math.abs(q.price - m.price) < this._details.minTickIncrement) {
                    copiedMkts[i].size = m.size - q.size;
                }
            }
        }

        return copiedMkts.filter(m => m.size > 1e-3);
    };
}
