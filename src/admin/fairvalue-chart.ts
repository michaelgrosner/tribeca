/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {Component} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';
//import Highcharts = require('highcharts-ng');

@Component({
  selector: 'fairValueChart',
  template: ''
})
export class FairValueChartComponent {

  public fairValueChart: number;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: SubscriberFactory
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

    var subscriberFairValue = subscriberFactory.getSubscriber($scope, Messaging.Topics.FairValue)
      .registerConnectHandler(clear)
      .registerDisconnectedHandler(clear)
      .registerSubscriber(addFairValue, fv => fv.forEach(addFairValue));

    $scope.$on('$destroy', () => {
      subscriberFairValue.disconnect();
    });
  }
}
