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

export class MarketTradeBroker implements Interfaces.IMarketTradeBroker {
    // TOOD: is this event needed?
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._marketTrades; }

    private _marketTrades : Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u : Models.GatewayMarketTrade) => {
        var qt = u.onStartup ? null : this._quoteGenerator.latestQuote;
        var mkt = u.onStartup ? null : this._mdBroker.currentBook;

        var t = new Models.MarketTrade(u.price, u.size, u.time, qt, mkt);
        this.marketTrades.push(t);
        this.MarketTrade.trigger(t);
        this._marketTradePublisher.publish(t);
    };

    constructor(private _mdGateway : Interfaces.IMarketDataGateway,
                private _marketTradePublisher : Messaging.IPublish<Models.MarketTrade>,
                private _mdBroker : Interfaces.IMarketDataBroker,
                private _quoteGenerator : Agent.QuoteGenerator) {
        _marketTradePublisher.registerSnapshot(() => _.last(this.marketTrades, 50));
        this._mdGateway.MarketTrade.on(this.handleNewMarketTrade);
    }
}