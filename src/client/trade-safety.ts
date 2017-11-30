import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';

import Models = require('./models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'trade-safety',
  template: `<div div class="tradeSafety img-rounded"><div>
      Fair Value: <span class="{{ fairValue ? \'text-danger fairvalue\' : \'text-muted\' }}" style="font-size:121%;">{{ fairValue | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      BuyTS: <span class="{{ buySafety ? \'text-danger\' : \'text-muted\' }}">{{ buySafety | number:'1.2-2' }}</span>,
      SellTS: <span class="{{ sellSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSafety | number:'1.2-2' }}</span>,
      TotalTS: <span class="{{ tradeSafetyValue ? \'text-danger\' : \'text-muted\' }}">{{ tradeSafetyValue | number:'1.2-2' }}</span>,
      openOrders/60sec: <span class="{{ tradeFreq ? \'text-danger\' : \'text-muted\' }}">{{ tradeFreq | number:'1.0-0' }}</span>,
      Trend: <span class="{{ trendSMU < 0 ? 'text-danger' : 'text-success' }}">{{ trendSMU | number:'1.3-3' }}</span>
      MktAvg: <span class="{{ avgMktWidth ? 'text-danger' : 'text-success' }}">{{ avgMktWidth | number:'1.3-3' }}</span>
    </div>
  </div>`
})
export class TradeSafetyComponent implements OnInit {

  public fairValue: number;
  private buySafety: number;
  private sellSafety: number;
  private buySizeSafety: number;
  private sellSizeSafety: number;
  private tradeSafetyValue: number;
  private trendSMU: number;
  private avgMktWidth: number;
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
      .getSubscriber(this.zone, Models.Topics.TrendSMU)
      .registerConnectHandler(this.clearTrendSMU)
      .registerSubscriber(this.updateTrendSMU);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.TradeSafetyValue)
      .registerConnectHandler(this.clear)
      .registerSubscriber(this.updateValues);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.EWMAChart)
      .registerConnectHandler(this.clearAvgMktWidth)
      .registerSubscriber(this.updateAvgMktWidth);
  }

  private updateValues = (value: Models.TradeSafety) => {
    if (value == null) return;
    this.tradeSafetyValue = value.combined;
    this.buySafety = value.buy;
    this.sellSafety = value.sell;
    this.buySizeSafety = value.buyPing;
    this.sellSizeSafety = value.sellPong;
  }

  private updateFairValue = (fv: Models.FairValue) => {
    if (fv == null) {
      this.clearFairValue();
      return;
    }

    this.fairValue = fv.price;
  }
  private updateTrendSMU = (trend: Models.TrendSMU) => {
    if (trend == null) {
      this.clearTrendSMU();
      return;
    }

    this.trendSMU = trend.trend;
  }

  private updateAvgMktWidth = (value: Models.EWMAChart) => {
    if (value == null) {
      this.clearAvgMktWidth();
      return;
    }

    this.avgMktWidth = value.avgMktWidth;
  }

  private clearFairValue = () => {
    this.fairValue = null;
  }
  private clearTrendSMU = () => {
    this.trendSMU = null;
  }
  private clearAvgMktWidth = () => {
    this.avgMktWidth = null;
  }

  private clear = () => {
    this.tradeSafetyValue = null;
    this.buySafety = null;
    this.sellSafety = null;
    this.buySizeSafety = null;
    this.sellSizeSafety = null;
  }
}
