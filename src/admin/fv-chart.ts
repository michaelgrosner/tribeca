/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import angular = require('angular');
import Models = require('../common/models');
import io = require('socket.io-client');
import moment = require('moment');
import Messaging = require('../common/messaging');
import Shared = require('./shared_directives');
//import Highcharts = require('highcharts-ng');

class fvChartCtrl {

  public fairValueChart: number;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: Shared.SubscriberFactory
  ) {
    var clear = () => {
      this.fairValueChart = 0;
    };
    clear();

    var addFairValue = (fv: Models.FairValue) => {
      if (fv == null) {
        clear();
        return;
      }

      // this.fairValueChart = fv.price;
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.FairValue)
      .registerConnectHandler(clear)
      .registerDisconnectedHandler(clear)
      .registerSubscriber(addFairValue, fv => fv.forEach(addFairValue));

    $scope.$on('$destroy', () => {
      sub.disconnect();
    });
  }
}

export var fvChartDirective = 'fvChartDirective';

angular.module(fvChartDirective, ['sharedDirectives'])
  .directive('fvChart', (): ng.IDirective => { return {
    template: '',//'<div>{{ fvChartScope.fairValueChart|number:2 }}</div>',
    restrict: "E",
    transclude: false,
    controller: fvChartCtrl,
    controllerAs: 'fvChartScope',
    scope: {},
    bindToController: {
      fairValueChart: '@'
    }
  }});
