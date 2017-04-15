import {NgZone, Component, Inject, OnInit} from '@angular/core';
import moment = require('moment');

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-stats',
  template: `<div style="position:absolute;">
    <div class="col-md-6 col-xs-6">
      <chart style="position:relative;top:5px;height:400px;width:700px;" type="StockChart" [options]="fvChartOptions" (load)="saveInstance($event.context, 'fv')"></chart>
    </div>
    <div class="col-md-6 col-xs-6">
      <chart style="position:relative;top:15px;height:200px;width:700px;" [options]="baseChartOptions" (load)="saveInstance($event.context, 'base')"></chart>
      <chart style="position:relative;top:21px;height:200px;width:700px;" [options]="quoteChartOptions" (load)="saveInstance($event.context, 'quote')"></chart>
    </div>
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
      type: 'datetime',
      labels: {
          enabled: false
      },
      gridLineWidth: 0
    },
    yAxis: {
      title: {
        text: 'Fair Value and Trades'
      },
      labels: {
          enabled: false
      },
      gridLineWidth: 0
    },
    legend: {
        enabled: false
    },
    tooltip: {
        shared: true
    },
    series: [{
      name: 'Fair Value',
      type: 'spline',
      lineWidth:4,
      colorIndex: 2,
      tooltip: {
          pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
      },
      data: [],
      id: 'fvseries'
    },{
      name: 'Sell',
      type: 'spline',
      colorIndex: 5,
      tooltip: {
          pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
      },
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
      type: 'spline',
      colorIndex: 0,
      tooltip: {
          pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
      },
      data: [],
      id: 'buyseries'
    },{
      name: 'Buy',
      type: 'flags',
      colorIndex: 0,
      data: [],
      onSeries: 'buyseries',
      shape: 'circlepin',
      width: 16
    },{
      name: 'Short EWMA',
      type: 'spline',
      colorIndex: 6,
      tooltip: {
          pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
      },
      data: []
    },{
      name: 'Long EWMA',
      type: 'spline',
      colorIndex: 3,
      tooltip: {
          pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
      },
      data: []
    }]
  };
  public quoteChartOptions = {
    title: 'quote wallet',
    chart: {
        zoomType: 'x',
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    tooltip: {
        shared: true,
        pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.2f} €</b><br/>'
    },
    plotOptions: {
        area: {
            stacking: 'normal',
            connectNulls: true
        }
    },
    xAxis: {
      type: 'datetime',
      labels: {
          enabled: false
      }
    },
    yAxis: [{
      title: {
        text: 'Total Position'
      },
      opposite: true,
      labels: {
          enabled: false
      },
      gridLineWidth: 0
    },{
      title: {
        text: 'Available and Held'
      },
      min: 0,
      labels: {
          enabled: false
      },
      gridLineWidth: 0
    }],
    legend: {
        enabled: false
    },
    series: [{
      name: 'Available',
      type: 'area',
      colorIndex:1,
      marker: {
          enabled: false
      },
      yAxis: 1,
      data: []
    },{
      name: 'Held',
      type: 'area',
      colorIndex:0,
      marker: {
          enabled: false
      },
      yAxis: 1,
      data: []
    },{
      name: 'Total Position',
      type: 'spline',
      colorIndex:2,
      lineWidth:3,
      marker: {
          enabled: false
      },
      data: []
    }]
  };
  public baseChartOptions = {
    title: 'base wallet',
    chart: {
        zoomType: 'x',
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    tooltip: {
        shared: true,
        pointFormat:'<span style="color:{point.color}">\u25CF</span> {series.name}: <b>{point.y:.8f} ฿</b><br/>'
    },
    plotOptions: {
        area: {
            stacking: 'normal',
            connectNulls: true
        }
    },
    xAxis: {
      type: 'datetime',
      labels: {
          enabled: false
      }
    },
    yAxis: [{
      title: {
        text: 'Total Position'
      },
      opposite: true,
      labels: {
          enabled: false
      },
      gridLineWidth: 0
    },{
      title: {
        text: 'Available and Held'
      },
      min: 0,
      labels: {
          enabled: false
      },
      gridLineWidth: 0
    }],
    legend: {
        enabled: false
    },
    series: [{
      name: 'Available',
      type: 'area',
      yAxis: 1,
      colorIndex:1,
      marker: {
          enabled: false
      },
      data: []
    },{
      name: 'Held',
      type: 'area',
      yAxis: 1,
      colorIndex:5,
      marker: {
          enabled: false
      },
      data: []
    },{
      name: 'Total Position',
      type: 'spline',
      colorIndex:2,
      lineWidth:3,
      marker: {
          enabled: false
      },
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
      .registerSubscriber(this.addFairValueChartData);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.EWMAChart)
      .registerSubscriber(this.addEWMAChartData);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.WalletChart)
      .registerSubscriber(this.addWalletChartData);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.TradesChart)
      .registerSubscriber(this.addTradesChartData);
  }

  private addFairValueChartData = (fv: Models.FairValue) => {
    if (fv == null) return;
    this.fairValue = ((fv.price * 100) / 100);
    this.fvChart.series[0].addPoint([moment(fv.time).valueOf(), this.fairValue]);
  }

  private addEWMAChartData = (ewma: Models.EWMAChart) => {
    if (ewma == null) return;
    let time = moment(ewma.time).valueOf();
    this.fairValue = ((ewma.fairValue * 100) / 100);
    this.fvChart.series[0].addPoint([time, this.fairValue]);
    this.fvChart.series[5].addPoint([time, ewma.ewmaShort]);
    this.fvChart.series[6].addPoint([time, ewma.ewmaLong]);
  }

  private addWalletChartData = (w: Models.WalletChart) => {
    let time = moment(w.time).valueOf();
    this.fairValue = ((w.fairValue * 100) / 100);
    this.fvChart.series[0].addPoint([time, this.fairValue]);
    this.quoteChart.series[0].addPoint([time, w.availQuote]);
    this.quoteChart.series[1].addPoint([time, w.heldQuote]);
    this.quoteChart.series[2].addPoint([time, w.totalQuote]);
    this.baseChart.series[0].addPoint([time, w.availBase]);
    this.baseChart.series[1].addPoint([time, w.heldBase]);
    this.baseChart.series[2].addPoint([time, w.totalBase]);
  }

  private addTradesChartData = (t: Models.TradeChart) => {
    let time = moment(t.time).valueOf();
    if (this.fairValue)
      this.fvChart.series[0].addPoint([time, this.fairValue]);
    this.fvChart.series[Models.Side[t.side] == 'Bid' ? 3 : 1].addPoint([time, ((t.price * 100) / 100)]);
    this.fvChart.series[Models.Side[t.side] == 'Bid' ? 4 : 2].addPoint({
      x: time,
      title: Models.Side[t.side] == 'Bid' ? 'B' : 'S',
      useHTML:true,
      text: '<b><span style="color:'+(Models.Side[t.side] == 'Bid' ? '#0000FF':'#FF0000')+';">'+(Models.Side[t.side] == 'Bid' ? '▼':'▲')+'</span> '+(Models.Side[t.side] == 'Bid' ? 'Buy':'Sell')+'</b>:'
        + '<br/>' + 'Price: <b>' + ((t.price * 100) / 100) + ' €</b>'
        + '<br/>' + 'Qty: <b>' + t.quantity + ' ฿</b>'
        + '<br/>' + 'Value: <b>' + ((t.value * 100) / 100) + ' €</b>'
    });
  }
}
