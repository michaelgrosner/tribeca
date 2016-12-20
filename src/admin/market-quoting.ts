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
    bidPrice: number;
    bidSize: number;
    askPrice: number;
    askSize: number;
    diffWidth: number;

    bidClass: string;
    askClass: string;
}

interface MarketQuotingScope extends ng.IScope {
    levels: Level[];
    qBidSz: number[];
    qBidPx: number[];
    fairValue: number;
    qAskPx: number[];
    qAskSz: number[];
    extVal: number;

    bidIsLive: boolean;
    askIsLive: boolean;
}

var MarketQuotingController = ($scope: MarketQuotingScope,
    $log: ng.ILogService,
    subscriberFactory: Shared.SubscriberFactory) => {
    var clearMarket = () => {
        $scope.levels = [];
    };
    clearMarket();

    var clearBid = () => {
        $scope.qBidPx = [];
        $scope.qBidSz = [];
    };

    var clearAsk = () => {
        $scope.qAskPx = [];
        $scope.qAskSz = [];
    };

    var clearQuote = () => {
        clearBid();
        clearAsk();
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
            $scope.levels[i].askPrice = update.asks[i].price;
            $scope.levels[i].askSize = update.asks[i].size;
        }

        for (var i = 0; i < update.bids.length; i++) {
            if (angular.isUndefined($scope.levels[i]))
                $scope.levels[i] = new Level();
            $scope.levels[i].bidPrice = update.bids[i].price;
            $scope.levels[i].bidSize = update.bids[i].size;

            $scope.levels[i].diffWidth = i==0
              ? $scope.levels[i].askPrice - $scope.levels[i].bidPrice : (
                (i==1 && $scope.qBidPx.length && $scope.qAskPx.length)
                  ? Math.min.apply(Math, $scope.qAskPx) - Math.max.apply(Math, $scope.qBidPx) : 0
              );
        }


        updateQuoteClass();
    };

    var updateQuote = (quote: Models.TwoSidedQuote) => {
        if (quote !== null) {
            if (quote.bid !== null) {
                if (!$scope.qBidPx || !$scope.qBidSz) clearBid();
                $scope.qBidPx.push(quote.bid.price);
                $scope.qBidSz.push(quote.bid.size);
            }
            else {
                clearBid();
            }

            if (quote.ask !== null) {
                if (!$scope.qAskPx || !$scope.qAskSz) clearAsk();
                $scope.qAskPx.push(quote.ask.price);
                $scope.qAskSz.push(quote.ask.size);
            }
            else {
                clearAsk();
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
            var tol = .005;
            for (var i = 0; i < $scope.levels.length; i++) {
                var level = $scope.levels[i];
                level.bidClass = 'active';
                if ($scope.bidIsLive)
                  for (var j = 0; j < $scope.qBidPx.length; j++)
                    if (Math.abs($scope.qBidPx[j] - level.bidPrice) < tol)
                        level.bidClass = 'success';
                level.askClass = 'active';
                if ($scope.askIsLive)
                  for (var j = 0; j < $scope.qAskPx.length; j++)
                    if (Math.abs($scope.qAskPx[j] - level.askPrice) < tol)
                        level.askClass = 'success';
            }
        }
    };

    var updateFairValue = (fv: Models.FairValue) => {
        if (fv == null) {
            clearFairValue();
            return;
        }

        $scope.fairValue = fv.price;
    };

    var subscribers = [];

    var makeSubscriber = <T>(topic: string, updateFn, clearFn) => {
        var sub = subscriberFactory.getSubscriber<T>($scope, topic)
            .registerSubscriber(updateFn, ms => ms.forEach(updateFn))
            .registerDisconnectedHandler(clearFn);
        subscribers.push(sub);
    };

    makeSubscriber<Models.Market>(Messaging.Topics.MarketData, updateMarket, clearMarket);
    makeSubscriber<Models.TwoSidedQuote>(Messaging.Topics.Quote, updateQuote, clearQuote);
    makeSubscriber<Models.TwoSidedQuoteStatus>(Messaging.Topics.QuoteStatus, updateQuoteStatus, clearQuoteStatus);
    makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue, updateFairValue, clearFairValue);

    $scope.$on('$destroy', () => {
        subscribers.forEach(d => d.disconnect());
        // $log.info("destroy market quoting grid");
    });

    clearQuote();
    // $log.info("started market quoting grid");
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
