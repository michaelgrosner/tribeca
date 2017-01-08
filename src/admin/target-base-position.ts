/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgModule, Component} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'targetBasePosition',
  template: '<span>{{ targetBasePosition|number:2 }}</span>'
})
export class TargetBasePositionComponent {

  public targetBasePosition: number;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: SubscriberFactory
  ) {
    var update = (value : Models.TargetBasePositionValue) => {
      if (value == null) return;
      this.targetBasePosition = value.data;
    };

    var subscriberTargetBasePosition = subscriberFactory.getSubscriber($scope, Messaging.Topics.TargetBasePosition)
      .registerDisconnectedHandler(() => this.targetBasePosition = null)
      .registerSubscriber(update, us => us.forEach(update));

    $scope.$on('$destroy', () => {
      subscriberTargetBasePosition.disconnect();
    });
  }
}
