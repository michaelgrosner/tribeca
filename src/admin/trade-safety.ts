/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'trade-safety',
  template: `<div div class="tradeSafety">
      <div>BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety | number:'1.2-2' }}</span></div>
      <div>SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety | number:'1.2-2' }}</span></div>
      <div>Target Base Position: <span class="text-danger">{{ targetBasePosition | number:'1.2-2' }}</span></div>
      <div>BuyTS: {{ buySafety | number:'1.2-2' }}</div>
      <div>SellTS: {{ sellSafety | number:'1.2-2' }}</div>
      <div>TotalTS: {{ tradeSafetyValue | number:'1.2-2' }}</div>
    </div>`
})
export class TradeSafetyComponent implements OnInit, OnDestroy {

  private buySafety: number;
  private sellSafety: number;
  private targetBasePosition: number;
  private buySizeSafety: number;
  private sellSizeSafety: number;
  private tradeSafetyValue: number;

  private subscriberTargetBasePosition: Messaging.ISubscribe<Models.TargetBasePositionValue>;
  private subscriberTradeSafetyValue: Messaging.ISubscribe<Models.TradeSafety>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberTargetBasePosition = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.TargetBasePosition)
      .registerDisconnectedHandler(() => this.targetBasePosition = null)
      .registerSubscriber(this.updateTargetBasePosition, us => us.forEach(this.updateTargetBasePosition));

    this.subscriberTradeSafetyValue = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.TradeSafetyValue)
      .registerDisconnectedHandler(this.clear)
      .registerSubscriber(this.updateValue, us => us.forEach(this.updateValue));
  }

  ngOnDestroy() {
    this.subscriberTargetBasePosition.disconnect();
    this.subscriberTradeSafetyValue.disconnect();
  }

  private updateTargetBasePosition = (value : Models.TargetBasePositionValue) => {
    if (value == null) return;
    this.targetBasePosition = value.data;
  }

  private updateValue = (value : Models.TradeSafety) => {
    if (value == null) return;
    this.tradeSafetyValue = value.combined;
    this.buySafety = value.buy;
    this.sellSafety = value.sell;
    this.buySizeSafety = value.buyPing;
    this.sellSizeSafety = value.sellPong;
  }

  private clear = () => {
    this.tradeSafetyValue = null;
    this.buySafety = null;
    this.sellSafety = null;
    this.buySizeSafety = null;
    this.sellSizeSafety = null;
  }
}
