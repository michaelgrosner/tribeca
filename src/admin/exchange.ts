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

    constructor(public pair : Models.CurrencyPair) {
        this.quote = Models.Currency[pair.quote];
        this.base = Models.Currency[pair.base];
        this.name = this.base + "/" + this.quote;
    }

    public updateMarket = (update : Models.MarketUpdate) => {
        this.bidSize = update.bid.size;
        this.bid = update.bid.price;
        this.ask = update.ask.price;
        this.askSize = update.ask.size;
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
}

class DisplayExchangeInformation {
    connected : boolean;
    name : string;

    usdPosition : number = null;
    btcPosition : number = null;
    ltcPosition : number = null;

    constructor(public _log : ng.ILogService, public exchange : Models.Exchange) {
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

        var newPair = new DisplayPair(pair);
        this.pairs.push(newPair);
        return newPair;
    };
}

var ExchangesController = ($scope : ExchangesScope, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
    var getOrAddDisplayExchange = (exch : Models.Exchange) : DisplayExchangeInformation => {
        var disp = $scope.exchanges[exch];

        if (angular.isUndefined(disp)) {
            $log.info("adding new exchange", Models.Exchange[exch]);
            $scope.exchanges[exch] = new DisplayExchangeInformation($log, exch);
            disp = $scope.exchanges[exch];
        }

        return disp;
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
    };

    socket.on("hello", subscriber);
    subscriber();

    socket.on("position-report", (rpt : Models.ExchangeCurrencyPosition) =>
        getOrAddDisplayExchange(rpt.exchange).updatePosition(rpt));

    socket.on("connection-status", (exch : Models.Exchange, cs : Models.ConnectivityStatus) =>
        getOrAddDisplayExchange(exch).setConnectStatus(cs) );

    socket.on("market-book", (book : Models.ExchangePairMessage<Models.Market>) =>
        getOrAddDisplayExchange(book.exchange).getOrAddDisplayPair(book.pair).updateMarket(book.data.update));

    socket.on("new-trading-decision", (d : Models.ExchangePairMessage<Models.TradingDecision>) =>
        getOrAddDisplayExchange(d.exchange).getOrAddDisplayPair(d.pair).updateDecision(d.data));

    socket.on('fair-value', (fv : Models.ExchangePairMessage<Models.FairValue>) =>
        getOrAddDisplayExchange(fv.exchange).getOrAddDisplayPair(fv.pair).updateFairValue(fv.data));
    
    socket.on('quote', (fv : Models.ExchangePairMessage<Models.TwoSidedQuote>) =>
        getOrAddDisplayExchange(fv.exchange).getOrAddDisplayPair(fv.pair).updateQuote(fv.data));

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