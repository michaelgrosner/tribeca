/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Aggregators = require("./aggregators");
import Quoter = require("./quoter");

// computes theoretical values based off the market and theo parameters
export class FairValueAgent {
    NewValue = new Utils.Evt<Models.Exchange>();
    private _fvsByExch : { [ exch : number] : Models.FairValue} = {};

    constructor(private _mdAgg : Aggregators.MarketDataAggregator) {
        _mdAgg.MarketData.on(mkt => this.recalcMarkets(mkt.exchange));
    }

    private recalcMarkets = (exchange : Models.Exchange) => {
        var mkt = this._mdAgg.getCurrentBook(exchange);

        if (mkt != null) {
            var mid = (mkt.update.ask.price + mkt.update.bid.price) / 2.0;
            this._fvsByExch[mkt.exchange] = new Models.FairValue(mid, mkt);
            this.NewValue.trigger(mkt.exchange);
        }
    };

    public getFairValue = (exchange : Models.Exchange) : Models.FairValue => {
        return this._fvsByExch[exchange];
    };
}

class TwoSidedQuote {
    constructor(public bid : Models.Quote, public ask : Models.Quote) {}
}

// computes a quote based off my quoting parameters
export class QuoteGenerator {
    NewQuote = new Utils.Evt<Models.Exchange>();
    private _quotesByExch : { [ exch : number] : TwoSidedQuote} = {};

    constructor(private _fvAgent : FairValueAgent) {
        this._fvAgent.NewValue.on(this.handleNewFairValue);
    }

    private handleNewFairValue = (exchange : Models.Exchange) => {
        var fv = this._fvAgent.getFairValue(exchange);

        if (fv != null) {
            var bidPx = Math.max(fv.price - .02, 0);
            var bidQuote = new Models.Quote(Models.QuoteAction.New, Models.Side.Bid, exchange, fv.mkt.update.time, bidPx, .01);
            var askQuote = new Models.Quote(Models.QuoteAction.New, Models.Side.Ask, exchange, fv.mkt.update.time, fv.price + .02, .01);

            this._quotesByExch[exchange] = new TwoSidedQuote(bidQuote, askQuote);
            this.NewQuote.trigger(exchange);
        }
    };

    public getQuote = (exchange : Models.Exchange) : TwoSidedQuote => {
        return this._quotesByExch[exchange];
    };
}

// makes decisions about whether or not a quote should be submitted
export class Trader {
    private _log : Utils.Logger = Utils.log("tribeca:trader");
    Active : boolean = false;
    ActiveChanged = new Utils.Evt<boolean>();

    private _tradingDecsionsByExch : { [ exch : number] : Models.TradingDecision} = {};
    NewTradingDecision = new Utils.Evt<Models.TradingDecision>();

    changeActiveStatus = (to : boolean) => {
        if (this.Active != to) {
            this.Active = to;
            this._log("changing active status to %o", to);

            this._tradeableExchanges.forEach(this.sendQuote);
            this.ActiveChanged.trigger(this.Active);
        }
    };

    constructor(private _quoteGenerator : QuoteGenerator,
                private _quoter : Quoter.Quoter,
                private _fvAgent : FairValueAgent,
                private _tradeableExchanges : Models.Exchange[]) {
        this._quoteGenerator.NewQuote.on(this.sendQuote);
    }

    private sendQuote = (exchange : Models.Exchange) : void => {
        var quote = this._quoteGenerator.getQuote(exchange);
        var fv = this._fvAgent.getFairValue(exchange);

        var bidQt, askQt : Models.Quote;

        if (this.Active && quote != null) {
            bidQt = quote.bid;
            askQt = quote.ask;
        }
        else {
            bidQt = Trader.ConvertToStopQuote(quote.ask);
            askQt = Trader.ConvertToStopQuote(quote.bid);
        }

        var askAction = this._quoter.updateQuote(quote.ask);
        var bidAction = this._quoter.updateQuote(quote.bid);

        var decision = new Models.TradingDecision(bidAction, bidQt, askAction, askQt, fv);
        this._tradingDecsionsByExch[exchange] = decision;
        this.NewTradingDecision.trigger(decision);
        this._log("New trading decision: %s", decision);
    };

    public getTradingDecision = (exch : Models.Exchange) : Models.TradingDecision => {
        return this._tradingDecsionsByExch[exch];
    };

    private static ConvertToStopQuote(q : Models.Quote) {
        return new Models.Quote(Models.QuoteAction.Cancel, q.side, q.exchange, q.time);
    }

    private static isBrokerActive(b : Interfaces.IBroker) : boolean {
        return b.currentBook != null && b.connectStatus == Models.ConnectivityStatus.Connected;
    }

    private static hasEnoughPosition(b : Interfaces.IBroker, cur : Models.Currency, minAmt : number) {
        var pos = b.getPosition(cur);
        return pos != null && pos.amount > minAmt;
    }
}