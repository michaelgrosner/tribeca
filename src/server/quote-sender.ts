import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Quoter = require("./quoter");
import _ = require("lodash");
import Active = require("./active-state");
import QuotingParameters = require("./quoting-parameters");
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
            private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
            private _quotingEngine: QuotingEngine.QuotingEngine,
            private _statusPublisher: Publish.IPublish<Models.TwoSidedQuoteStatus>,
            private _quoter: Quoter.Quoter,
            private _details: Interfaces.IBroker,
            private _activeRepo: Active.ActiveRepository) {
        _activeRepo.NewParameters.on(this.sendQuote);
        _quotingEngine.QuoteChanged.on(this.sendQuote);
        _statusPublisher.registerSnapshot(() => this.latestStatus === null ? [] : [this.latestStatus]);
    }

    private checkCrossedQuotes = (side: Models.Side, px: number): boolean => {
        var oppSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;

        var doesQuoteCross = oppSide === Models.Side.Bid
            ? (a, b) => a.price >= b
            : (a, b) => a.price <= b;

        let qs = this._quotingEngine.latestQuote[oppSide === Models.Side.Bid ? 'bid' : 'ask'];
        if (qs && doesQuoteCross(qs.price, px)) {
            this._log.warn("crossing quote detected! gen quote at %d would crossed with %s quote at",
                px, Models.Side[oppSide], qs);
            return true;
        }
        return false;
    };

    private sendQuote = (): void => {
        var quote = this._quotingEngine.latestQuote;

        var askStatus = Models.QuoteStatus.Held;
        var bidStatus = Models.QuoteStatus.Held;

        if (quote !== null) {
          if (this._activeRepo.latest) {
              if (quote.ask !== null && (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Ask, quote.ask.price)))
                  askStatus = Models.QuoteStatus.Live;

              if (quote.bid !== null && (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Bid, quote.bid.price)))
                  bidStatus = Models.QuoteStatus.Live;
          }

          if (askStatus === Models.QuoteStatus.Live) {
              this._quoter.updateQuote(new Models.Timestamped(quote.ask, this._timeProvider.utcNow()), Models.Side.Ask);
          } else if (this._quoter.quotesSent(Models.Side.Ask).filter(o => (quote.ask !== null && this._qlParamRepo.latest.mode === Models.QuotingMode.AK47)?quote.ask.price === o.quote.price:true).length)
              this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Ask, this._timeProvider.utcNow()));

          if (bidStatus === Models.QuoteStatus.Live) {
              this._quoter.updateQuote(new Models.Timestamped(quote.bid, this._timeProvider.utcNow()), Models.Side.Bid);
          } else if (this._quoter.quotesSent(Models.Side.Bid).filter(o => (quote.bid !== null && this._qlParamRepo.latest.mode === Models.QuotingMode.AK47)?quote.bid.price === o.quote.price:true).length)
              this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Bid, this._timeProvider.utcNow()));
        }

        this.latestStatus = new Models.TwoSidedQuoteStatus(bidStatus, askStatus);
    };
}
