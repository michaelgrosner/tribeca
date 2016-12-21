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

class DisplayOrderStatusClassReport {
    orderId: string;
    price: number;
    quantity: number;
    side: Models.Side;

    constructor(public osr: Models.OrderStatusReport) {
        this.orderId = osr.orderId;
        this.side = osr.side;
        this.quantity = osr.quantity;
        this.price = osr.price;
    }
}

interface MarketQuotingScope extends ng.IScope {
    levels: Level[];
    fairValue: number;
    extVal: number;
    qBidSz: number;
    qBidPx: number;
    qAskPx: number;
    qAskSz: number;
    order_classes: DisplayOrderStatusClassReport[];

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

    var clearQuote = () => {
        $scope.order_classes = [];
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

        if (!angular.isUndefined($scope.order_classes)) {
          var bids = $scope.order_classes.filter(o => o.side === Models.Side.Bid);
          var asks = $scope.order_classes.filter(o => o.side === Models.Side.Ask);
          if (bids.length) {
            var bid = bids.reduce(function(a,b){return a.price>b.price?a:b;});
            $scope.qBidPx = bid.price;
            $scope.qBidSz = bid.quantity;
          }
          if (asks.length) {
            var ask = asks.reduce(function(a,b){return a.price<b.price?a:b;});
            $scope.qAskPx = ask.price;
            $scope.qAskSz = ask.quantity;
          }
        }
        for (var i = 0; i < update.bids.length; i++) {
            if (angular.isUndefined($scope.levels[i]))
                $scope.levels[i] = new Level();
            $scope.levels[i].bidPrice = update.bids[i].price;
            $scope.levels[i].bidSize = update.bids[i].size;

            $scope.levels[i].diffWidth = i==0
              ? $scope.levels[i].askPrice - $scope.levels[i].bidPrice : (
                (i==1 && $scope.qAskPx && $scope.qBidPx)
                  ? $scope.qAskPx - $scope.qBidPx : 0
              );
        }


        updateQuoteClass();
    };

    var updateQuote = (o: Models.OrderStatusReport) => {
        var idx = -1;
        for(var i=0;i<$scope.order_classes.length;i++)
          if ($scope.order_classes[i].orderId==o.orderId) {idx=i; break;}
        if (idx!=-1) {
            if (!o.leavesQuantity)
              $scope.order_classes.splice(idx,1);
        } else if (o.leavesQuantity)
          $scope.order_classes.push(new DisplayOrderStatusClassReport(o));

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
                if ($scope.bidIsLive) {
                  var bids = $scope.order_classes.filter(o => o.side === Models.Side.Bid);
                  for (var j = 0; j < bids.length; j++)
                    if (Math.abs(bids[j].price - level.bidPrice) < tol)
                        level.bidClass = 'success';
                }
                level.askClass = 'active';
                if ($scope.askIsLive) {
                  var asks = $scope.order_classes.filter(o => o.side === Models.Side.Ask);
                  for (var j = 0; j < asks.length; j++)
                    if (Math.abs(asks[j].price - level.askPrice) < tol)
                        level.askClass = 'success';
                }
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
    makeSubscriber<Models.OrderStatusReport>(Messaging.Topics.OrderStatusReports, updateQuote, clearQuote);
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
