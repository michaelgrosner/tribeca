import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Broker = require("./broker");
import QuotingEngine = require("./quoting-engine");
import * as moment from "moment";

export class MarketTradeBroker {
    // TOOD: is this event needed?
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._marketTrades; }

    private _marketTrades: Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u: Models.GatewayMarketTrade) => {
        const qt = u.onStartup ? null : this._quoteEngine.latestQuote;
        const mkt = u.onStartup ? null : this._mdBroker.currentBook;

        var t = new Models.MarketTrade(this._base.exchange(), this._base.pair, u.price, u.size, u.time, qt,
            mkt === null ? null : mkt.bids[0], mkt === null ? null : mkt.asks[0], u.make_side);

        if (u.onStartup) {
            for (let existing of this._marketTrades) {
                try {
                    const dt = Math.abs(moment(existing.time).diff(moment(u.time), 'minutes'));
                    if (Math.abs(existing.size - u.size) < 1e-4 &&
                        Math.abs(existing.price - u.price) < (.5*this._base.minTickIncrement) &&
                        dt < 1)
                        return;
                } catch (error) {
                    // sigh
                    continue;
                }
            }
        }

        while (this.marketTrades.length >= 50)
            this.marketTrades.shift();
        this.marketTrades.push(t);

        this.MarketTrade.trigger(t);
        this._marketTradePublisher.publish(t);
    };

    constructor(
      private _mdGateway: Interfaces.IMarketDataGateway,
      private _marketTradePublisher: Publish.IPublish<Models.MarketTrade>,
      private _mdBroker: Broker.MarketDataBroker,
      private _quoteEngine: QuotingEngine.QuotingEngine,
      private _base: Broker.ExchangeBroker
    ) {
      _marketTradePublisher.registerSnapshot(() => this.marketTrades.slice(-69));
      this._mdGateway.MarketTrade.on(this.handleNewMarketTrade);
    }
}
