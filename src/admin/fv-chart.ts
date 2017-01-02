/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="shared_directives.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");
//import Highcharts = require('highcharts-ng');

interface fvChartScope extends ng.IScope {
    fairValueChart: Models.FairValue[];
}

var fvChartBlock = ($scope: fvChartScope,
                           $log: ng.ILogService,
                           subscriberFactory: Shared.SubscriberFactory) => {
    $scope.fairValueChart = [];

    var clear = () => {
        $scope.fairValueChart.length = 0;
    };

    var addFairValue = (fv: Models.FairValue) => {
        if (fv == null) {
            clear();
            return;
        }

        // $scope.fairValueChart.push(fv);
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.FairValue)
        .registerConnectHandler(clear)
        .registerDisconnectedHandler(clear)
        .registerSubscriber(addFairValue, fv => fv.forEach(addFairValue));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        // $log.info("destroy order list");
    });

    // $log.info("started order list");
};

export var fvChartDirective = "fvChartDirective";

angular.module(fvChartDirective, ['sharedDirectives'])
    .directive('fvChart', () => {
    var template = '<div></div>';

    return {
        template: template,
        restrict: "E",
        transclude: false,
        controller: fvChartBlock
    }
});
