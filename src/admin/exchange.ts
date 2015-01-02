/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

interface ExchangesScope extends ng.IScope {
    exchanges : { [exch : number] : DisplayExchangeInformation};
    sendUpdatedParameters : (p : Models.ExchangePairMessage<Models.QuotingParameters>) => void;
}

class DisplayPair {
    name : string;
    base : string;
    quote : string;

    bidSize : number;
    bid : number;
    askSize : number;
    ask : number;

    bidAction : string;
    askAction : string;

    qBidPx : number;
    qBidSz : number;
    qAskPx : number;
    qAskSz : number;
    fairValue : number;

    constructor(private $scope : ExchangesScope,
                private $log : ng.ILogService,
                private exch : Models.Exchange,
                public pair : Models.CurrencyPair) {
        this.quote = Models.Currency[pair.quote];
        this.base = Models.Currency[pair.base];
        this.name = this.base + "/" + this.quote;
    }

    public updateMarket = (update : Models.Market) => {
        this.bidSize = update.bids[0].size;
        this.bid = update.bids[0].price;
        this.ask = update.asks[0].price;
        this.askSize = update.asks[0].size;
    };

    public updateDecision = (quote : Models.TradingDecision) => {
        this.bidAction = Models.QuoteAction[quote.bidAction];
        this.askAction = Models.QuoteAction[quote.askAction];
    };

    public updateQuote = (quote : Models.TwoSidedQuote) => {
        this.qBidPx = quote.bid.price;
        this.qBidSz = quote.bid.size;
        this.qAskPx = quote.ask.price;
        this.qAskSz = quote.ask.size;
    };

    public updateFairValue = (fv : Models.FairValue) => {
        this.fairValue = fv.price;
    };

    public updateParameters = (p : Models.QuotingParameters) => {
        this.width(p.width);
        this.size(p.size);
    };

    private _width : number = null;
    public width = (val? : number) : number => {
        if (arguments.length === 1) {
            if (this._width == null || Math.abs(val - this._width) > 1e-4) {
                this._width = val;
                this.sendNewParameters();
            }
        }
        return this._width;
    };

    private _size : number = null;
    public size = (val? : number) : number => {
        if (arguments.length === 1) {
            if (this._size == null || Math.abs(val - this._size) > 1e-4) {
                this._size = val;
                this.sendNewParameters();
            }
        }
        return this._size;
    };

    private sendNewParameters = () => {
        var parameters = new Models.QuotingParameters(this._width, this._size);
        this.$scope.sendUpdatedParameters(new Models.ExchangePairMessage(this.exch, this.pair, parameters));
    };
}

class DisplayExchangeInformation {
    connected : boolean;
    name : string;

    usdPosition : number = null;
    btcPosition : number = null;
    ltcPosition : number = null;

    constructor(private $scope : ExchangesScope, private _log : ng.ILogService, public exchange : Models.Exchange) {
        this.name = Models.Exchange[exchange];
    }

    public setConnectStatus = (cs : Models.ConnectivityStatus) => {
        this.connected = cs == Models.ConnectivityStatus.Connected;
    };

    public updatePosition = (position : Models.ExchangeCurrencyPosition) => {
        switch (position.currency) {
            case Models.Currency.BTC:
                this.btcPosition = position.amount;
                break;
            case Models.Currency.USD:
                this.usdPosition = position.amount;
                break;
            case Models.Currency.LTC:
                this.ltcPosition = position.amount;
                break;
        }
    };

    pairs : DisplayPair[] = [];

    public getOrAddDisplayPair = (pair : Models.CurrencyPair) : DisplayPair => {
        for (var i = 0; i < this.pairs.length; i++) {
            var p = this.pairs[i];
            if (pair.base === p.pair.base && pair.quote === p.pair.quote) {
                return p;
            }
        }

        this._log.info("adding new pair, base:", Models.Currency[pair.base], "quote:",
            Models.Currency[pair.quote], "to exchange", Models.Exchange[this.exchange]);

        var newPair = new DisplayPair(this.$scope, this._log, this.exchange, pair);
        this.pairs.push(newPair);
        return newPair;
    };
}

var ExchangesController = ($scope : ExchangesScope, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
    var getOrAddDisplayExchange = (exch : Models.Exchange) : DisplayExchangeInformation => {
        var disp = $scope.exchanges[exch];

        if (angular.isUndefined(disp)) {
            $log.info("adding new exchange", Models.Exchange[exch]);
            $scope.exchanges[exch] = new DisplayExchangeInformation($scope, $log, exch);
            disp = $scope.exchanges[exch];
        }

        return disp;
    };

    $scope.sendUpdatedParameters = (p : Models.ExchangePairMessage<Models.QuotingParameters>) => {
        socket.emit("parameters-update-request", p);
    };

    // ugh
    var subscriber = () => {
        $scope.exchanges = {};
        socket.emit("subscribe-position-report");
        socket.emit("subscribe-connection-status");
        socket.emit("subscribe-market-book");
        socket.emit("subscribe-new-trading-decision");
        socket.emit("subscribe-fair-value");
        socket.emit("subscribe-quote");
        socket.emit("subscribe-parameter-updates");
    };

    socket.on("hello", subscriber);
    subscriber();

    socket.on("position-report", (rpt : Models.ExchangeCurrencyPosition) =>
        getOrAddDisplayExchange(rpt.exchange).updatePosition(rpt));

    socket.on("connection-status", (exch : Models.Exchange, cs : Models.ConnectivityStatus) =>
        getOrAddDisplayExchange(exch).setConnectStatus(cs) );

    socket.on("market-book", (book : Models.ExchangePairMessage<Models.Market>) =>
        getOrAddDisplayExchange(book.exchange).getOrAddDisplayPair(book.pair).updateMarket(book.data));

    socket.on("new-trading-decision", (d : Models.ExchangePairMessage<Models.TradingDecision>) =>
        getOrAddDisplayExchange(d.exchange).getOrAddDisplayPair(d.pair).updateDecision(d.data));

    socket.on('fair-value', (fv : Models.ExchangePairMessage<Models.FairValue>) =>
        getOrAddDisplayExchange(fv.exchange).getOrAddDisplayPair(fv.pair).updateFairValue(fv.data));

    socket.on('quote', (q : Models.ExchangePairMessage<Models.TwoSidedQuote>) =>
        getOrAddDisplayExchange(q.exchange).getOrAddDisplayPair(q.pair).updateQuote(q.data));

    socket.on("parameter-updates", (p : Models.ExchangePairMessage<Models.QuotingParameters>) =>
        getOrAddDisplayExchange(p.exchange).getOrAddDisplayPair(p.pair).updateParameters(p.data));

    socket.on("disconnect", () => {
        $scope.exchanges = {};
    });

    $log.info("started exchanges");
};

var exchangeDirective = () : ng.IDirective => {
    return {
        restrict: "E",
        templateUrl: "exchange.html"
    }
};

angular.module('exchangesDirective', ['ui.bootstrap', 'sharedDirectives'])
       .controller('ExchangesController', ExchangesController)
       .directive("exchanges", exchangeDirective);