/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="interfaces.ts"/>
/// <reference path="persister.ts"/>
/// <reference path="broker.ts"/>
/// <reference path="web.ts"/>
/// <reference path="quoting-engine.ts"/>

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import _ = require("lodash");
import P = require("./persister");
import Broker = require("./broker");
import mongodb = require('mongodb');
import Web = require("./web");
import QuotingEngine = require("./quoting-engine");

export class MarketTradesLoaderSaver {
    public loader = (x : Models.MarketTrade) => {
        this._wrapped.loader(x);
        
        if (typeof x.quote !== "undefined" && x.quote !== null)
            this._wrapped.loader(x.quote);
    };
    
    public saver = (x : Models.MarketTrade) => {
        this._wrapped.saver(x);
        
        if (typeof x.quote !== "undefined" && x.quote !== null)
            this._wrapped.saver(x.quote);
    };
    
    constructor(private _wrapped: P.LoaderSaver) {}
}

export class MarketTradeBroker implements Interfaces.IMarketTradeBroker {
    private _log = Utils.log("mt:broker");

    // TOOD: is this event needed?
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._marketTrades; }

    private _marketTrades: Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u: Models.GatewayMarketTrade) => {
        var qt = u.onStartup ? null : this._quoteEngine.latestQuote;
        var mkt = u.onStartup ? null : this._mdBroker.currentBook;

        var t = new Models.MarketTrade(this._base.exchange(), this._base.pair, u.price, u.size, u.time, qt, 
            mkt === null ? null : mkt.bids[0], mkt === null ? null : mkt.asks[0], u.make_side);

        if (u.onStartup) {
            for (var i = 0; i < this.marketTrades.length; i++) {
                var existing = this.marketTrades[i];

                try {
                    var dt = Math.abs(existing.time.diff(u.time, 'minutes'));
                    if (Math.abs(existing.size - u.size) < 1e-4 && Math.abs(existing.price - u.price) < 1e-4 && dt < 1)
                        return;
                } catch (error) {
                    // sigh
                    continue;
                }
            }
        }

        this.marketTrades.push(t);
        this.MarketTrade.trigger(t);
        this._marketTradePublisher.publish(t);
        this._persister.persist(t);
    };

    constructor(private _mdGateway: Interfaces.IMarketDataGateway,
        private _marketTradePublisher: Messaging.IPublish<Models.MarketTrade>,
        private _mdBroker: Interfaces.IMarketDataBroker,
        private _quoteEngine: QuotingEngine.QuotingEngine,
        private _base: Broker.ExchangeBroker,
        private _persister: P.IPersist<Models.MarketTrade>,
        initMkTrades: Array<Models.MarketTrade>) {
            
        initMkTrades.forEach(t => this.marketTrades.push(t));
        this._log.info("loaded %d market trades", this.marketTrades.length);

        _marketTradePublisher.registerSnapshot(() => _.takeRight(this.marketTrades, 50));
        this._mdGateway.MarketTrade.on(this.handleNewMarketTrade);
    }
}