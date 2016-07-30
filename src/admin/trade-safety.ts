/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="shared_directives.ts"/>
///<reference path="pair.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Pair = require("./pair");
import Shared = require("./shared_directives");

interface TradeSafetyScope extends ng.IScope {
    buySafety: number;
    sellSafety: number;
    buySizeSafety: number;
    sellSizeSafety: number;
    tradeSafetyValue : number;
}

var TradeSafetyController = ($scope : TradeSafetyScope, $log : ng.ILogService, subscriberFactory : Shared.SubscriberFactory) => {

    var updateValue = (value : Models.TradeSafety) => {
        if (value == null) return;
        $scope.tradeSafetyValue = value.combined;
        $scope.buySafety = value.buy;
        $scope.sellSafety = value.sell;
        $scope.buySizeSafety = value.buyPing;
        $scope.sellSizeSafety = value.sellPong;
    };

    var clear = () => {
        $scope.tradeSafetyValue = null;
        $scope.buySafety = null;
        $scope.sellSafety = null;
        $scope.buySizeSafety = null;
        $scope.sellSizeSafety = null;
    };

    var subscriber = subscriberFactory.getSubscriber($scope, Messaging.Topics.TradeSafetyValue)
        .registerDisconnectedHandler(clear)
        .registerSubscriber(updateValue, us => us.forEach(updateValue));

    $scope.$on('$destroy', () => {
        subscriber.disconnect();
        // $log.info("destroy trade safety");
    });

    // $log.info("started trade safety");
};

export var tradeSafetyDirective = "tradeSafetyDirective";

angular
    .module(tradeSafetyDirective, ['sharedDirectives'])
    .directive("tradeSafety", () => {
        var template = '<span>BuyLT: <b>{{ buySizeSafety|number:2 }}</b>, SellLT: <b>{{ sellSizeSafety|number:2 }}</b>, BuyTS: {{ buySafety|number:2 }}, SellTS: {{ sellSafety|number:2 }}, TotalTS: {{ tradeSafetyValue|number:2 }}</span>';

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            template: template,
            controller: TradeSafetyController
        }
    });
