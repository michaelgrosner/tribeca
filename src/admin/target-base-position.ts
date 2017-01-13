/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'target-base-position',
  template: `<span>{{ targetBasePosition | number:'1.2-2' }}</span>`
})
export class TargetBasePositionComponent implements OnInit, OnDestroy {

  private targetBasePosition: number;

  private subscriberTargetBasePosition: Messaging.ISubscribe<Models.TargetBasePositionValue>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberTargetBasePosition = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.TargetBasePosition)
      .registerDisconnectedHandler(() => this.targetBasePosition = null)
      .registerSubscriber(this.update, us => us.forEach(this.update));
  }

  ngOnDestroy() {
    this.subscriberTargetBasePosition.disconnect();
  }

  private update = (value : Models.TargetBasePositionValue) => {
    if (value == null) return;
    this.targetBasePosition = value.data;
  }
}
