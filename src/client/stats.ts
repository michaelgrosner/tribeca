import {NgZone, Component, Inject, OnInit} from '@angular/core';
import moment = require('moment');

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-stats',
  template: `<div class="col-md-6 col-xs-6">
    <chart style="height:400px;width:700px;" type="StockChart" [options]="fvChartOptions" (load)="saveInstance($event.context, 'fv')"></chart>
  </div>
  <div class="col-md-6 col-xs-6">
    <chart style="height:200px;width:700px;" [options]="baseChartOptions" (load)="saveInstance($event.context, 'base')"></chart>
    <chart style="height:200px;width:700px;" [options]="quoteChartOptions" (load)="saveInstance($event.context, 'quote')"></chart>
  </div>`
})
export class StatsComponent implements OnInit {

  public fairValue: number;
  public fvChart: any;
  public quoteChart: any;
  public baseChart: any;
  private saveInstance = (chartInstance, chartId) => {
    this[chartId+'Chart'] = chartInstance;
  }
  public fvChartOptions = {
    title: 'fair value',
    chart: {
        zoomType: 'x',
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    xAxis: {
      type: 'datetime'
    },
    yAxis: {
      title: {
        text: 'Fair Value and Trades'
      }
    },
    tooltip: {
        split: true
    },
    legend: {
        enabled: false
    },
    series: [{
      name: 'Fair Value',
      type: 'spline',
      colorIndex: 2,
      data: [],
      id: 'fvseries'
    },{
      name: 'Sell',
      type: 'scatter',
      colorIndex: 5,
      data: [],
      id: 'sellseries'
    },{
      type: 'flags',
      colorIndex: 5,
      data: [],
      onSeries: 'sellseries',
      shape: 'circlepin',
      width: 16
    },{
      name: 'Buy',
      type: 'scatter',
      colorIndex: 0,
      data: [],
      id: 'buyseries'
    },{
      type: 'flags',
      colorIndex: 0,
      data: [],
      onSeries: 'buyseries',
      shape: 'circlepin',
      width: 16
    }]
  };
  public quoteChartOptions = {
    title: 'quote wallet',
    chart: {
        zoomType: 'x',
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    tooltip: {
        split: true,
        pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
    },
    plotOptions: {
        series: {
            stacking: 'normal'
        }
    },
    xAxis: {
      type: 'datetime'
    },
    yAxis: [{
      title: {
        text: 'Total Position'
      },
      opposite: true
    },{
      title: {
        text: 'Available and Held'
      },
    }],
    legend: {
        enabled: false
    },
    series: [{
      name: 'Available',
      type: 'column',
      colorIndex:1,
      yAxis: 1,
      data: []
    },{
      name: 'Held',
      type: 'column',
      colorIndex:0,
      yAxis: 1,
      data: []
    },{
      name: 'Total Position',
      type: 'spline',
      colorIndex:2,
      yAxis: 0,
      data: []
    }]
  };
  public baseChartOptions = {
    title: 'base wallet',
    type: 'column',
    chart: {
        zoomType: 'x',
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    tooltip: {
        split: true,
        pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.8f} ฿</b><br/>'
    },
    plotOptions: {
        series: {
            stacking: 'normal'
        }
    },
    xAxis: {
      type: 'datetime'
    },
    yAxis: [{
      title: {
        text: 'Total Position'
      },
      opposite: true
    },{
      title: {
        text: 'Available and Held'
      }
    }],
    legend: {
        enabled: false
    },
    series: [{
      name: 'Available',
      type: 'column',
      yAxis: 1,
      colorIndex:1,
      data: []
    },{
      name: 'Held',
      type: 'column',
      yAxis: 1,
      colorIndex:5,
      data: []
    },{
      name: 'Total Position',
      type: 'spline',
      yAxis: 0,
      colorIndex:2,
      data: []
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
      .getSubscriber(this.zone, Models.Topics.TradesChart)
      .registerSubscriber(this.addTradesChartData);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.WalletChart)
      .registerSubscriber(this.addWalletChartData);
  }

  private updateFairValue = (fv: Models.FairValue) => {
    if (fv == null) return;
    this.fvChart.series[0].addPoint([
      new Date().getTime(),
      ((fv.price * 100) / 100)
    ]);
  }

  private addTradesChartData = (t: Models.TradeChart) => {
    let time = new Date().getTime();
    this.fvChart.series[Models.Side[t.side] == 'Bid' ? 3 : 1].addPoint([
      time,
      ((t.price * 100) / 100)
    ]);
    this.fvChart.series[Models.Side[t.side] == 'Bid' ? 4 : 2].addPoint({
      x: time,
      title: Models.Side[t.side] == 'Bid' ? 'B' : 'S',
      lineColor: Models.Side[t.side] == 'Bid' ? '#FF0000' : '#0000FF',
      useHTML:true,
      text: 'Price: ' + ((t.price * 100) / 100)
        + '<br/>' + 'Qty: ' + t.quantity
        + '<br/>' + 'Value: ' + ((t.value * 100) / 100)
    });
  }

  private addWalletChartData = (w: Models.WalletChart) => {
    let time = new Date().getTime();
    this.fvChart.series[0].addPoint([time, w.fairValue]);
    this.quoteChart.series[0].addPoint([time, w.availQuote]);
    this.quoteChart.series[1].addPoint([time, w.heldQuote]);
    this.quoteChart.series[2].addPoint([time, w.totalQuote]);
    this.baseChart.series[0].addPoint([time, w.availBase]);
    this.baseChart.series[1].addPoint([time, w.heldBase]);
    this.baseChart.series[2].addPoint([time, w.totalBase]);
  }
}
