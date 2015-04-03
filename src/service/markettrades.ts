/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import momentjs = require('moment');
import Interfaces = require("./interfaces");
import Agent = require("./arbagent");
import _ = require("lodash");
import P = require("./persister");
import Broker = require("./broker");
import mongodb = require('mongodb');
import Web = require("./web");

export class MarketTradePersister extends P.Persister<Models.ExchangePairMessage<Models.MarketTrade>> {
    constructor(db : Q.Promise<mongodb.Db>) {
        super(db, "mt", d => P.timeLoader(d.data), d => P.timeSaver(d.data));
    }
}

export class MarketTradeBroker implements Interfaces.IMarketTradeBroker {
    _log : Utils.Logger = Utils.log("tribeca:mtbroker");

    // TOOD: is this event needed?
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._marketTrades; }

    private _marketTrades : Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u : Models.GatewayMarketTrade) => {
        var qt = u.onStartup ? null : this._quoteEngine.latestQuote;
        var mkt = u.onStartup ? null : this._mdBroker.currentBook;

        var t = new Models.MarketTrade(u.price, u.size, u.time, qt, mkt === null ? null : mkt.bids[0], mkt === null ? null : mkt.asks[0], u.make_side);

        if (u.onStartup) {
            for (var i = 0; i < this.marketTrades.length; i++) {
                var existing = this.marketTrades[i];

                var dt = Math.abs(existing.time.diff(u.time, 'minutes'));
                if (Math.abs(existing.size - u.size) < 1e-4 && Math.abs(existing.price - u.price) < 1e-4 && dt < 1)
                    return;
            }
        }

        this.marketTrades.push(t);
        this.MarketTrade.trigger(t);
        this._marketTradePublisher.publish(t);
        this._persister.persist(new Models.ExchangePairMessage(this._base.exchange(), this._base.pair, t));
    };

    constructor(private _mdGateway : Interfaces.IMarketDataGateway,
                private _marketTradePublisher : Messaging.IPublish<Models.MarketTrade>,
                private _mdBroker : Interfaces.IMarketDataBroker,
                private _quoteEngine : Agent.QuotingEngine,
                private _base : Broker.ExchangeBroker,
                private _persister : MarketTradePersister,
                initMkTrades : Models.ExchangePairMessage<Models.MarketTrade>[],
                webPub : Web.StandaloneHttpPublisher<Models.MarketTrade>) {
        webPub.registerSnapshot(n => _persister.loadAll(n).then(mts => _.map(mts, x => x.data)));

        initMkTrades.forEach(t => this.marketTrades.push(t.data));
        this._log("loaded %d market trades", this.marketTrades.length);

        _marketTradePublisher.registerSnapshot(() => _.last(this.marketTrades, 50));
        this._mdGateway.MarketTrade.on(this.handleNewMarketTrade);
    }
}