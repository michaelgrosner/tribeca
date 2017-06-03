/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="shared_directives.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

class Level {
    bidPrice: string;
    bidSize: number;
    askPrice: string;
    askSize: number;

    bidClass: string;
    askClass: string;
}

interface MarketQuotingScope extends ng.IScope {
    levels: Level[];
    qBidSz: number;
    qBidPx: string;
    fairValue: string;
    spreadValue: string;
    qAskPx: string;
    qAskSz: number;
    extVal: string;

    bidIsLive: boolean;
    askIsLive: boolean;
}

var MarketQuotingController = ($scope: MarketQuotingScope,
        $log: ng.ILogService,
        subscriberFactory: Shared.SubscriberFactory,
        product: Shared.ProductState) => {

    var toPrice = (px: number) : string => px.toFixed(product.fixed);
    var toPercent = (askPx: number, bidPx: number): string => ((askPx - bidPx) / askPx * 100).toFixed(2);

    var clearMarket = () => {
        $scope.levels = [];
    };
    clearMarket();

    var clearBid = () => {
        $scope.qBidPx = null;
        $scope.qBidSz = null;
    };

    var clearAsk = () => {
        $scope.qAskPx = null;
        $scope.qAskSz = null;
    };

    var clearSpread = () => {
        $scope.spreadValue = null;
    }

    var clearQuote = () => {
        clearBid();
        clearAsk();
        clearSpread();
    };

    var clearFairValue = () => {
        $scope.fairValue = null;
    };

    var clearQuoteStatus = () => {
        $scope.bidIsLive = false;
        $scope.askIsLive = false;
    };

    var clearExtVal = () => {
        $scope.extVal = null;
    };

    var updateMarket = (update: Models.Market) => {
        if (update == null) {
            clearMarket();
            return;
        }

        for (var i = 0; i < update.asks.length; i++) {
            if (angular.isUndefined($scope.levels[i]))
                $scope.levels[i] = new Level();
            $scope.levels[i].askPrice = toPrice(update.asks[i].price);
            $scope.levels[i].askSize = update.asks[i].size;
        }

        for (var i = 0; i < update.bids.length; i++) {
            if (angular.isUndefined($scope.levels[i]))
                $scope.levels[i] = new Level();
            $scope.levels[i].bidPrice = toPrice(update.bids[i].price);
            $scope.levels[i].bidSize = update.bids[i].size;
        }

        updateQuoteClass();
    };

    var updateQuote = (quote: Models.TwoSidedQuote) => {
        if (quote !== null) {
            if (quote.bid !== null) {
                $scope.qBidPx = toPrice(quote.bid.price);
                $scope.qBidSz = quote.bid.size;
            }
            else {
                clearBid();
            }

            if (quote.ask !== null) {
                $scope.qAskPx = toPrice(quote.ask.price);
                $scope.qAskSz = quote.ask.size;
            }
            else {
                clearAsk();
            }

            if (quote.ask !== null && quote.bid !== null) {
                const spreadAbsolutePrice = (quote.ask.price - quote.bid.price).toFixed(2);
                const spreadPercent = toPercent(quote.ask.price, quote.bid.price);
                $scope.spreadValue = `${spreadAbsolutePrice} / ${spreadPercent}%`;
            }
            else {
                clearFairValue();
            }
        }
        else {
            clearQuote();
        }

        updateQuoteClass();
    };

    var updateQuoteStatus = (status: Models.TwoSidedQuoteStatus) => {
        if (status == null) {
            clearQuoteStatus();
            return;
        }

        $scope.bidIsLive = (status.bidStatus === Models.QuoteStatus.Live);
        $scope.askIsLive = (status.askStatus === Models.QuoteStatus.Live);
        updateQuoteClass();
    };

    var updateQuoteClass = () => {
        if (!angular.isUndefined($scope.levels) && $scope.levels.length > 0) {
            for (var i = 0; i < $scope.levels.length; i++) {
                var level = $scope.levels[i];

                if ($scope.qBidPx === level.bidPrice && $scope.bidIsLive) {
                    level.bidClass = 'success';
                }
                else {
                    level.bidClass = 'active';
                }

                if ($scope.qAskPx === level.askPrice && $scope.askIsLive) {
                    level.askClass = 'success';
                }
                else {
                    level.askClass = 'active';
                }
            }
        }
    };

    var updateFairValue = (fv: Models.FairValue) => {
        if (fv == null) {
            clearFairValue();
            return;
        }

        $scope.fairValue = toPrice(fv.price);
    };

    var subscribers = [];

    var makeSubscriber = <T>(topic: string, updateFn, clearFn) => {
        var sub = subscriberFactory.getSubscriber<T>($scope, topic)
            .registerSubscriber(updateFn, ms => ms.forEach(updateFn))
            .registerConnectHandler(clearFn);
        subscribers.push(sub);
    };

    makeSubscriber<Models.Market>(Messaging.Topics.MarketData, updateMarket, clearMarket);
    makeSubscriber<Models.TwoSidedQuote>(Messaging.Topics.Quote, updateQuote, clearQuote);
    makeSubscriber<Models.TwoSidedQuoteStatus>(Messaging.Topics.QuoteStatus, updateQuoteStatus, clearQuoteStatus);
    makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue, updateFairValue, clearFairValue);

    $scope.$on('$destroy', () => {
        subscribers.forEach(d => d.disconnect());
        $log.info("destroy market quoting grid");
    });

    $log.info("started market quoting grid");
};

export var marketQuotingDirective = "marketQuotingDirective";

angular
    .module(marketQuotingDirective, ['ui.bootstrap', 'ui.grid', Shared.sharedDirectives])
    .directive("marketQuotingGrid", () => {

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            templateUrl: "market_display.html",
            controller: MarketQuotingController
        }
    });