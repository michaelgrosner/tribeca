import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import Broker = require("./broker");
import Quoter = require("./quoter");
import _ = require("lodash");
import Active = require("./active-state");
import QuotingParameters = require("./quoting-parameters");
import QuotingEngine = require("./quoting-engine");

export class QuoteSender {
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
            private _details: Broker.ExchangeBroker,
            private _activeRepo: Active.ActiveRepository) {
        _activeRepo.ExchangeConnectivity.on(this.sendQuote);
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
            console.warn('quotesender', 'crossing quote detected! gen quote at', px, 'would crossed with', Models.Side[oppSide], 'quote at', qs);
            return true;
        }
        return false;
    };

    private sendQuote = (): void => {
        var quote = this._quotingEngine.latestQuote;

        let askStatus = Models.QuoteStatus.Held;
        let bidStatus = Models.QuoteStatus.Held;

        if (quote !== null) {
          if (this._activeRepo.latest) {
            if (quote.ask !== null && (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Ask, quote.ask.price)))
              askStatus = Models.QuoteStatus.Live;

            if (quote.bid !== null && (this._details.hasSelfTradePrevention || !this.checkCrossedQuotes(Models.Side.Bid, quote.bid.price)))
              bidStatus = Models.QuoteStatus.Live;
          }

          if (askStatus === Models.QuoteStatus.Live)
            this._quoter.updateQuote(new Models.Timestamped(quote.ask, this._timeProvider.utcNow()), Models.Side.Ask);
          else
            this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Ask, this._timeProvider.utcNow()));

          if (bidStatus === Models.QuoteStatus.Live)
            this._quoter.updateQuote(new Models.Timestamped(quote.bid, this._timeProvider.utcNow()), Models.Side.Bid);
          else
            this._quoter.cancelQuote(new Models.Timestamped(Models.Side.Bid, this._timeProvider.utcNow()));
        }

        this.latestStatus = new Models.TwoSidedQuoteStatus(bidStatus, askStatus);
    };
}
