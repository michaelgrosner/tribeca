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

export class MarketTradeBroker implements Interfaces.IMarketTradeBroker {
    // TOOD: is this event needed?
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._marketTrades; }

    private _marketTrades : Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u : Models.MarketSide) => {
        var t = new Models.MarketTrade(u.price, u.size, u.time, this._quoteGenerator.latestQuote);
        this.marketTrades.push(t);
        this.MarketTrade.trigger(t);
        this._marketTradePublisher.publish(t);
    };

    constructor(private _mdGateway : Interfaces.IMarketDataGateway,
                private _marketTradePublisher : Messaging.IPublish<Models.MarketTrade>,
                private _quoteGenerator : Agent.QuoteGenerator) {
        _marketTradePublisher.registerSnapshot(() => this.marketTrades);
        this._mdGateway.MarketTrade.on(this.handleNewMarketTrade);
    }
}