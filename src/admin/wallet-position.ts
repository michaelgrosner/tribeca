/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <amd-dependency path='ui.bootstrap'/>
/// <reference path='shared_directives.ts'/>

import angular = require('angular');
import Models = require('../common/models');
import Messaging = require('../common/messaging');
import Shared = require('./shared_directives');

class WalletPositionController {

  public baseCurrency: string;
  public basePosition: number;
  public quoteCurrency: string;
  public quotePosition: number;
  public baseHeldPosition: number;
  public quoteHeldPosition: number;
  public value: number;
  public quoteValue: number;
  public buySize: number;
  public sellSize: number;
  public fv: number;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: Shared.SubscriberFactory
  ) {
    var clearPosition = () => {
      this.baseCurrency = null;
      this.quoteCurrency = null;
      this.basePosition = null;
      this.quotePosition = null;
      this.baseHeldPosition = null;
      this.quoteHeldPosition = null;
      this.value = null;
      this.quoteValue = null;
      this.fv = null;
    };

    var clearQP = () => {
      this.buySize = null;
      this.sellSize = null;
    };

    var updatePosition = (position : Models.PositionReport) => {
      this.baseCurrency = Models.Currency[position.pair.base];
      this.quoteCurrency = Models.Currency[position.pair.quote];
      this.basePosition = position.baseAmount;
      this.quotePosition = position.quoteAmount;
      this.baseHeldPosition = position.baseHeldAmount;
      this.quoteHeldPosition = position.quoteHeldAmount;
      this.value = position.value;
      this.quoteValue = position.quoteValue;
      this.fv = position.quoteValue / position.value;
    };

    var updateQP = qp => {
      this.buySize = qp.buySize;
      this.sellSize = qp.sellSize;
    };

    var subscriberQPChange = subscriberFactory.getSubscriber($scope, Messaging.Topics.QuotingParametersChange)
      .registerDisconnectedHandler(clearQP)
      .registerSubscriber(updateQP, qp => qp.forEach(updateQP));

    var subscriberPosition = subscriberFactory.getSubscriber($scope, Messaging.Topics.Position)
      .registerDisconnectedHandler(clearPosition)
      .registerSubscriber(updatePosition, us => us.forEach(updatePosition));

    $scope.$on('$destroy', () => {
      subscriberQPChange.disconnect();
      subscriberPosition.disconnect();
    });
  }
}

export var walletPositionDirective = 'walletPositionDirective';

angular.module(walletPositionDirective, ['ui.bootstrap', 'sharedDirectives'])
  .directive('walletPosition', (): ng.IDirective => { return {
    template: `<div class="positions">
        <h4 class="col-md-12 col-xs-2"><small>
            {{ walletPositionScope.quoteCurrency }}:&nbsp;<span ng-class="walletPositionScope.quotePosition + walletPositionScope.quoteHeldPosition > walletPositionScope.buySize * walletPositionScope.fv ? 'text-danger' : 'text-muted'">{{ walletPositionScope.quotePosition|currency:undefined:2 }}</span><br/>(<span ng-class="walletPositionScope.quoteHeldPosition ? 'buy' : 'text-muted'">{{ walletPositionScope.quoteHeldPosition|currency:undefined:2 }}</span>)
        </small></h4>
        <h4 class="col-md-12 col-xs-2"><small>
            {{ walletPositionScope.baseCurrency }}:&nbsp;<span ng-class="walletPositionScope.basePosition + walletPositionScope.baseHeldPosition > walletPositionScope.sellSize ? 'text-danger' : 'text-muted'">{{ walletPositionScope.basePosition|currency:"B":3 }}</span><br/>(<span ng-class="walletPositionScope.baseHeldPosition ? 'sell' : 'text-muted'">{{ walletPositionScope.baseHeldPosition|currency:"B":3 }}</span>)
        </small></h4>
        <h4 class="col-md-12 col-xs-2">
          <small>Value: </small><b>{{ walletPositionScope.value|currency:"B":5 }}</b><br/>
          <b>{{ walletPositionScope.quoteValue|currency:undefined:2 }}</b>
        </h4>
    </div>`,
    restrict: "E",
    transclude: false,
    controller: WalletPositionController,
    controllerAs: 'walletPositionScope',
    scope: {},
    bindToController: true
  }});
