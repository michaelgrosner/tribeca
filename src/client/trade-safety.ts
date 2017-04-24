import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'trade-safety',
  template: `<div div class="tradeSafety img-rounded"><div>
      Fair Value: <span class="{{ fairValue ? \'text-danger fairvalue\' : \'text-muted\' }}">{{ fairValue }}</span>,
      BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety }}</span>,
      SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety }}</span>,
      BuyTS: <span class="{{ buySafety ? \'text-danger\' : \'text-muted\' }}">{{ buySafety | number:'1.2-2' }}</span>,
      SellTS: <span class="{{ sellSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSafety | number:'1.2-2' }}</span>,
      TotalTS: <span class="{{ tradeSafetyValue ? \'text-danger\' : \'text-muted\' }}">{{ tradeSafetyValue | number:'1.2-2' }}</span>,
      openOrders/min: <span class="{{ tradeFreq ? \'text-danger\' : \'text-muted\' }}">{{ tradeFreq }}</span>
    </div>
  </div>`
})
export class TradeSafetyComponent implements OnInit {

  public fairValue: string;
  private buySafety: number;
  private sellSafety: number;
  private buySizeSafety: string ;
  private sellSizeSafety: string;
  private tradeSafetyValue: number;
  @Input() tradeFreq: number;
  @Input() product: Models.ProductState;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.FairValue)
      .registerConnectHandler(this.clearFairValue)
      .registerSubscriber(this.updateFairValue);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.TradeSafetyValue)
      .registerConnectHandler(this.clear)
      .registerSubscriber(this.updateValues);
  }

  private toPrice = (px: number) : string => px.toFixed(this.product.fixed);

  private updateValues = (value : Models.TradeSafety) => {
    if (value == null) return;
    this.tradeSafetyValue = value.combined;
    this.buySafety = value.buy;
    this.sellSafety = value.sell;
    this.buySizeSafety = this.toPrice(value.buyPing);
    this.sellSizeSafety = this.toPrice(value.sellPong);
  }

  private updateFairValue = (fv: Models.FairValue) => {
    if (fv == null) {
      this.clearFairValue();
      return;
    }

    this.fairValue = this.toPrice(fv.price);
  }

  private clearFairValue = () => {
    this.fairValue = null;
  }

  private clear = () => {
    this.tradeSafetyValue = null;
    this.buySafety = null;
    this.sellSafety = null;
    this.buySizeSafety = null;
    this.sellSizeSafety = null;
  }
}
