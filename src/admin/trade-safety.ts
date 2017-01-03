/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import angular = require('angular');
import Models = require('../common/models');
import Messaging = require('../common/messaging');
import Shared = require('./shared_directives');

class TradeSafetyController {

  public buySafety: number;
  public sellSafety: number;
  public buySizeSafety: number;
  public sellSizeSafety: number;
  public tradeSafetyValue: number;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: Shared.SubscriberFactory
  ) {
    var updateValue = (value : Models.TradeSafety) => {
      if (value == null) return;
      this.tradeSafetyValue = value.combined;
      this.buySafety = value.buy;
      this.sellSafety = value.sell;
      this.buySizeSafety = value.buyPing;
      this.sellSizeSafety = value.sellPong;
    };

    var clear = () => {
      this.tradeSafetyValue = null;
      this.buySafety = null;
      this.sellSafety = null;
      this.buySizeSafety = null;
      this.sellSizeSafety = null;
    };

    var subscriberTradeSafetyValue = subscriberFactory.getSubscriber($scope, Messaging.Topics.TradeSafetyValue)
      .registerDisconnectedHandler(clear)
      .registerSubscriber(updateValue, us => us.forEach(updateValue));

    $scope.$on('$destroy', () => {
        subscriberTradeSafetyValue.disconnect();
    });
  }
}

export var tradeSafetyDirective = 'tradeSafetyDirective';

angular.module(tradeSafetyDirective, [Shared.sharedDirectives])
  .directive('tradeSafety', (): ng.IDirective => { return {
    template: `<div>
      BuyPing: <span class="{{ tradeSafetyScope.buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ tradeSafetyScope.buySizeSafety|number:2 }}</span>,
      SellPing: <span class="{{ tradeSafetyScope.sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ tradeSafetyScope.sellSizeSafety|number:2 }}</span>,
      BuyTS: {{ tradeSafetyScope.buySafety|number:2 }},
      SellTS: {{ tradeSafetyScope.sellSafety|number:2 }},
      TotalTS: {{ tradeSafetyScope.tradeSafetyValue|number:2 }}
    </div>`,
    restrict: "E",
    transclude: false,
    controller: TradeSafetyController,
    controllerAs: 'tradeSafetyScope',
    scope: {},
    bindToController: true
  }});
