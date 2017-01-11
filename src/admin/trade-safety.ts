/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'trade-safety',
  template: `<div>
      BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety | number:'1.2-2' }}</span>,
      SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety | number:'1.2-2' }}</span>,
      BuyTS: {{ buySafety | number:'1.2-2' }},
      SellTS: {{ sellSafety | number:'1.2-2' }},
      TotalTS: {{ tradeSafetyValue | number:'1.2-2' }}
    </div>`
})
export class TradeSafetyComponent {

  private buySafety: number;
  private sellSafety: number;
  private buySizeSafety: number;
  private sellSizeSafety: number;
  private tradeSafetyValue: number;

  private subscriberTradeSafetyValue: any;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberTradeSafetyValue = this.subscriberFactory.getSubscriber(this.zone, Messaging.Topics.TradeSafetyValue)
      .registerDisconnectedHandler(this.clear)
      .registerSubscriber(this.updateValue, us => us.forEach(this.updateValue));
  }

  ngOnDestroy() {
    this.subscriberTradeSafetyValue.disconnect();
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
