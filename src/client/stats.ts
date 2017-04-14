import {NgZone, Component, Inject, OnInit} from '@angular/core';

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-stats',
  template: `<chart type="StockChart" [options]="chartOptions" (load)="saveInstance($event.context)"></chart>`
})
export class StatsComponent implements OnInit {

  public fairValue: number;
  public chart: any;
  private saveInstance = (chartInstance) => {
    this.chart = chartInstance;
  }
  public chartOptions = {
    title: { text: 'fair value and trades' },
    series: [{
      name: 'Fair Value',
      data: [],
      id: 'fvseries'
    },{
      type: 'flags',
      data: [],
      onSeries: 'fvseries',
      shape: 'circlepin',
      width: 16
    }]
  };

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.FairValue)
      .registerSubscriber(this.updateFairValue);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.OrderStatusReports)
      .registerSubscriber(this.addRowData);
  }

  private updateFairValue = (fv: Models.FairValue) => {
    if (fv == null) return;

    this.chart.series[0].addPoint([Date.now(), fv.price]);
  }

  private addRowData = (o: Models.Timestamped<any[]>) => {
    if (o.data[1] == Models.OrderStatus.Complete)
      this.chart.series[1].addPoint({
          x: Date.now(),
          y: o.data[3],
          title: Models.Side[o.data[2]].replace('Bid','buy').replace('Ask','sell'),
          text: 'Price: ' + o.data[3] + '<br/>' + 'Qty: ' + ((o.data[4] * 100) / 100)
      });
  }
}
