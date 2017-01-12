/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';
//import Highcharts = require('highcharts-ng');

@Component({
  selector: 'fair-value-chart',
  template: ''//'{{ fairValueChart }}'
})
export class FairValueChartComponent implements OnInit, OnDestroy {

  private fairValueChart: number;

  private subscriberFairValue: Messaging.ISubscribe<Models.FairValue>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {
    this.clear();
  }

  ngOnInit() {
    // this.subscriberFairValue = this.subscriberFactory.getSubscriber(this.zone, Messaging.Topics.FairValue)
      // .registerConnectHandler(this.clear)
      // .registerDisconnectedHandler(this.clear)
      // .registerSubscriber(this.addFairValue, fv => fv.forEach(this.addFairValue));
  }

  ngOnDestroy() {
    // this.subscriberFairValue.disconnect();
  }

  private clear = () => {
    this.fairValueChart = 0;
  }

  private addFairValue = (fv: Models.FairValue) => {
    if (fv == null) {
      this.clear();
      return;
    }

    this.fairValueChart = fv.price;
  }
}
