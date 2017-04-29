/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="utils.ts"/>
/// <reference path="quoter.ts"/>
/// <reference path="statistics.ts"/>
/// <reference path="active-state.ts"/>
/// <reference path="fair-value.ts"/>
/// <reference path="market-filtration.ts"/>
/// <reference path="quoting-parameters.ts"/>
/// <reference path="quoting-engine.ts"/>

import Config = require("./config");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import Safety = require("./safety");
import util = require("util");
import _ = require("lodash");
import Statistics = require("./statistics");
import Active = require("./active-state");
import FairValue = require("./fair-value");
import MarketFiltration = require("./market-filtration");
import QuotingParameters = require("./quoting-parameters");
import PositionManagement = require("./position-management");
import moment = require('moment');
import QuotingEngine = require("./quoting-engine");
import log from "./logging";

export class QuoteSender {
    private _log = log("quotesender");

    private _latest = new Models.TwoSidedQuoteStatus(Models.QuoteStatus.Held, Models.QuoteStatus.Held);
    public get latestStatus() { return this._latest; }
    public set latestStatus(val: Models.TwoSidedQuoteStatus) {
        if (_.isEqual(val, this._latest)) return;

        this._latest = val;
        this._statusPublisher.publish(this._latest);
    }

    constructor(
            private _timeProvider: Utils.ITimeProvider,
            private _quotingEngine: QuotingEngine.QuotingEngine,
            private _statusPublisher: Messaging.IPublish<Models.TwoSidedQuoteStatus>,
            private _quoter: Quoter.Quoter,
            private _activeRepo: Active.ActiveRepository,
            private _positionBroker: Interfaces.IPositionBroker,
            private _fv: FairValue.FairValueEngine,
            private _broker: Interfaces.IMarketDataBroker,
            private _details: Interfaces.IBroker) {
        _activeRepo.NewParameters.on(() => this.sendQuote(_timeProvider.utcNow()));
        _quotingEngine.QuoteChanged.on(() => this.sendQuote(Utils.timeOrDefault(_quotingEngine.latestQuote, _timeProvider)));
        _statusPublisher.registerSnapshot(() => this.latestStatus === null ? [] : [this.latestStatus]);
    }

    private checkCrossedQuotes = (side: Models.Side, px: number): boolean => {
        var oppSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;

        var doesQuoteCross = oppSide === Models.Side.Bid
            ? (a, b) => a.price >= b
            : (a, b) => a.price <= b;

        var qs = this._quoter.quotesSent(oppSide);
        for (var qi = 0; qi < qs.length; qi++) {
            if (doesQuoteCross(qs[qi].quote, px)) {
                this._log.warn("crossing quote detected! gen quote at %d would crossed with %s quote at",
                    px, Models.Side[oppSide], qs[qi]);
                return true;
            }
        }
        return false;
    };

    private sendQuote = (t: Date): void => {
        var quote = this._quotingEngine.latestQuote;

        var askStatus = Models.QuoteStatus.Held;
        var bidStatus = Models.QuoteStatus.Held;

        if (quote !== null && this._activeRepo.latest) {
            if (quote.ask !== null && this.hasEnoughPosition(this._details.pair.base, quote.ask.size) &&
                (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Ask, quote.ask.price))) {
                askStatus = Models.QuoteStatus.Live;
            }

            if (quote.bid !== null && this.hasEnoughPosition(this._details.pair.quote, quote.bid.size * quote.bid.price) &&
                (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Bid, quote.bid.price))) {
                bidStatus = Models.QuoteStatus.Live;
            }
        }
        
        var askAction: Models.QuoteSent;
        if (askStatus === Models.QuoteStatus.Live) {
            askAction = this._quoter.updateQuote(new Models.Timestamped(quote.ask, t), Models.Side.Ask);
        }
        else {
            askAction = this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Ask, t));
        }

        var bidAction: Models.QuoteSent;
        if (bidStatus === Models.QuoteStatus.Live) {
            bidAction = this._quoter.updateQuote(new Models.Timestamped(quote.bid, t), Models.Side.Bid);
        }
        else {
            bidAction = this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Bid, t));
        }

        this.latestStatus = new Models.TwoSidedQuoteStatus(bidStatus, askStatus);
    };

    private hasEnoughPosition = (cur: Models.Currency, minAmt: number): boolean => {
        var pos = this._positionBroker.getPosition(cur);
        return pos != null && pos.amount > minAmt;
    };
}
