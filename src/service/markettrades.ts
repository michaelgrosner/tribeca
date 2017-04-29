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
import * as moment from "moment";
import log from "./logging";

export class MarketTradeBroker implements Interfaces.IMarketTradeBroker {
    private _log = log("mt:broker");

    // TOOD: is this event needed?
    MarketTrade = new Utils.Evt<Models.MarketTrade>();
    public get marketTrades() { return this._marketTrades; }

    private _marketTrades: Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u: Models.GatewayMarketTrade) => {
        const qt = u.onStartup ? null : this._quoteEngine.latestQuote;
        const mkt = u.onStartup ? null : this._mdBroker.currentBook;

        const px = Utils.roundNearest(u.price, this._base.minTickIncrement);
        const t = new Models.MarketTrade(this._base.exchange(), this._base.pair, px, u.size, u.time, qt, 
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
        this._persister.persist(t);
    };

    constructor(
        private _mdGateway: Interfaces.IMarketDataGateway,
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