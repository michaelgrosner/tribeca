import {NgZone, Component, Inject, Output, EventEmitter, OnInit} from '@angular/core';
import moment = require('moment');
import Highcharts = require('highcharts');

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
  public ewmaShort: number;
  public ewmaLong: number;
  public ewmaQuote: number;
  public fvChart: any;
  public quoteChart: any;
  public baseChart: any;
  private saveInstance = (chartInstance, chartId) => {
    this[chartId+'Chart'] = Highcharts.charts.length;
    Highcharts.charts.push(chartInstance);
  }
  private syncExtremes = function (e) {
    var thisChart = this.chart;
    if (e.trigger !== 'syncExtremes') {
        (<any>Highcharts).each(Highcharts.charts, function (chart) {
            if (chart !== thisChart && chart.xAxis[0].setExtremes)
              chart.xAxis[0].setExtremes(e.min, e.max, undefined, true, { trigger: 'syncExtremes' });
        });
    }
  }
  public fvChartOptions = {
    title: 'fair value',
    chart: {
        zoomType: 'x',
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    xAxis: {
      type: 'datetime',
      crosshair: true,
      events: {
          setExtremes: this.syncExtremes
      },
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
          pointFormatter:function () {
            var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
            return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
          }
      },
      data: [],
      id: 'fvseries'
    },{
      name: 'Sell',
      type: 'spline',
      colorIndex: 5,
      tooltip: {
          pointFormatter:function () {
            var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
            return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
          }
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
          pointFormatter:function () {
            var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
            return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
          }
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
      name: 'Quote EWMA',
      type: 'spline',
      color: '#ffff00',
      tooltip: {
          pointFormatter:function () {
            var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
            return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
          }
      },
      data: []
    },{
      name: 'Short EWMA',
      type: 'spline',
      colorIndex: 6,
      tooltip: {
          pointFormatter:function () {
            var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
            return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
          }
      },
      data: []
    },{
      name: 'Long EWMA',
      type: 'spline',
      colorIndex: 3,
      tooltip: {
          pointFormatter:function () {
            var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
            return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
          }
      },
      data: []
    }]
  };
  public quoteChartOptions = {
    title: 'quote wallet',
    chart: {
        zoomType: 'x',
        resetZoomButton: {
            theme: {
                display: 'none'
            }
        },
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    tooltip: {
        shared: true,
        pointFormatter:function () {
          var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
          return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(2)+' €</b><br/>';
        }
    },
    plotOptions: {
        area: {
            stacking: 'normal',
            connectNulls: true
        }
    },
    xAxis: {
      type: 'datetime',
      crosshair: true,
      events: {
          setExtremes: this.syncExtremes
      },
      labels: {
          enabled: true
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
      name: 'Total Position',
      type: 'spline',
      zIndex: 1,
      colorIndex:2,
      lineWidth:3,
      marker: {
          enabled: false
      },
      data: []
    },{
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
    }]
  };
  public baseChartOptions = {
    title: 'base wallet',
    chart: {
        zoomType: 'x',
        resetZoomButton: {
            theme: {
                display: 'none'
            }
        },
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    tooltip: {
        shared: true,
        pointFormatter:function () {
          var symbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
          return '<span style="color:'+this.series.color+'">' + symbols[this.series.symbol] + '</span> '+this.series.name+': <b>'+this.y.toFixed(8)+' ฿</b><br/>';
        }
    },
    plotOptions: {
        area: {
            stacking: 'normal',
            connectNulls: true
        }
    },
    xAxis: {
      type: 'datetime',
      crosshair: true,
      events: {
          setExtremes: this.syncExtremes
      },
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
      name: 'Total Position',
      type: 'spline',
      zIndex: 1,
      colorIndex:2,
      lineWidth:3,
      marker: {
          enabled: false
      },
      data: []
    },{
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
    }]
  };

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    setTimeout(() => {
      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.FairValue)
        .registerSubscriber(this.addFairValueChartData);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.EWMAChart)
        .registerSubscriber(this.addEWMAChartData);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.TradesChart)
        .registerSubscriber(this.addTradesChartData);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.WalletChart)
        .registerSubscriber(this.addWalletChartData);

      jQuery('chart').bind('mousemove touchmove touchstart', function (e: any) {
        var chart, point, i, event;
        for (i = 0; i < Highcharts.charts.length; i = i + 1) {
          chart = Highcharts.charts[i];
          chart.pointer.reset = function () { return undefined; };
          let ev: any = jQuery.extend(jQuery.Event(e.originalEvent.type), {
              which: 1,
              chartX: e.originalEvent.chartX,
              chartY: e.originalEvent.chartY,
              clientX: (jQuery(chart.container).offset().left != jQuery(this).offset().left)?jQuery(chart.container).offset().left - jQuery(this).offset().left + e.originalEvent.clientX:e.originalEvent.clientX,
              clientY: e.originalEvent.clientY,
              pageX: (jQuery(chart.container).offset().left != jQuery(this).offset().left)?jQuery(chart.container).offset().left - jQuery(this).offset().left + e.originalEvent.pageX:e.originalEvent.pageX,
              pageY: e.originalEvent.pageY,
              screenX: (jQuery(chart.container).offset().left != jQuery(this).offset().left)?jQuery(chart.container).offset().left - jQuery(this).offset().left + e.originalEvent.screenX:e.originalEvent.screenX,
              screenY: e.originalEvent.screenY
          });
          event = chart.pointer.normalize(ev);
          point = chart.series[0].searchPoint(ev, true);
          if (point) {
            point.onMouseOver();
            point.series.chart.xAxis[0].drawCrosshair(ev, point);
          }
        }
      });
      jQuery('chart').bind('mouseleave', function (e) {
        var chart, point, i, event;
        for (i = 0; i < Highcharts.charts.length; ++i) {
          chart = Highcharts.charts[i];
          event = chart.pointer.normalize(e.originalEvent);
          point = chart.series[0].searchPoint(event, true);
          if (point) {
            point.onMouseOut();
            chart.tooltip.hide(point);
            chart.xAxis[0].hideCrosshair();
          }
        }
      });
    }, 10000);
  }

  private addFairValueChartData = (fv: Models.FairValue) => {
    if (fv == null) return;
    this.fairValue = ((fv.price * 100) / 100);
    let time = moment.utc().valueOf();
    this.removeOldPoints(Highcharts.charts[this.fvChart], time);
    if (this.ewmaQuote) Highcharts.charts[this.fvChart].series[5].addPoint([time, this.ewmaQuote], false);
    if (this.ewmaShort) Highcharts.charts[this.fvChart].series[6].addPoint([time, this.ewmaShort], false);
    if (this.ewmaLong) Highcharts.charts[this.fvChart].series[7].addPoint([time, this.ewmaLong], false);
    Highcharts.charts[this.fvChart].series[0].addPoint([time, this.fairValue], true);
  }

  private addEWMAChartData = (ewma: Models.EWMAChart) => {
    if (ewma == null) return;
    let time = moment.utc().valueOf();
    this.removeOldPoints(Highcharts.charts[this.fvChart], time);
    this.fairValue = ((ewma.fairValue * 100) / 100);
    if (ewma.ewmaQuote || this.ewmaQuote) {
      if (ewma.ewmaQuote) this.ewmaQuote = ewma.ewmaQuote;
      Highcharts.charts[this.fvChart].series[5].addPoint([time, this.ewmaQuote], false);
    }
    if (ewma.ewmaShort || this.ewmaShort) {
      if (ewma.ewmaShort) this.ewmaShort = ewma.ewmaShort;
      Highcharts.charts[this.fvChart].series[6].addPoint([time, this.ewmaShort], false);
    }
    if (ewma.ewmaLong || this.ewmaLong) {
      if (ewma.ewmaLong) this.ewmaLong = ewma.ewmaLong;
      Highcharts.charts[this.fvChart].series[7].addPoint([time, this.ewmaLong], false);
    }
    Highcharts.charts[this.fvChart].series[0].addPoint([time, this.fairValue], true);
  }

  private addTradesChartData = (t: Models.TradeChart) => {
    let time = moment.utc().valueOf();
    this.removeOldPoints(Highcharts.charts[this.fvChart], time);
    if (this.ewmaQuote) Highcharts.charts[this.fvChart].series[5].addPoint([time, this.ewmaQuote], false);
    if (this.ewmaShort) Highcharts.charts[this.fvChart].series[6].addPoint([time, this.ewmaShort], false);
    if (this.ewmaLong) Highcharts.charts[this.fvChart].series[7].addPoint([time, this.ewmaLong], false);
    Highcharts.charts[this.fvChart].series[Models.Side[t.side] == 'Bid' ? 3 : 1].addPoint([time, ((t.price * 100) / 100)], false);
    (<any>Highcharts).charts[this.fvChart].series[Models.Side[t.side] == 'Bid' ? 4 : 2].addPoint({
      x: time,
      title: Models.Side[t.side] == 'Bid' ? 'B' : 'S',
      useHTML:true,
      text: '<b><span style="color:'+(Models.Side[t.side] == 'Bid' ? '#0000FF':'#FF0000')+';">'+(Models.Side[t.side] == 'Bid' ? '▼':'▲')+'</span> '+(Models.Side[t.side] == 'Bid' ? 'Buy':'Sell')+'</b>:'
        + '<br/>' + 'Price: <b>' + ((t.price * 100) / 100) + ' €</b>'
        + '<br/>' + 'Qty: <b>' + t.quantity.toFixed(8) + ' ฿</b>'
        + '<br/>' + 'Value: <b>' + ((t.value+'').substring(0,(t.value+'').indexOf('.')+3)) + ' €</b>'
    }, !this.fairValue);
    if (this.fairValue) Highcharts.charts[this.fvChart].series[0].addPoint([time, this.fairValue], true);
  }

  private addWalletChartData = (w: Models.WalletChart) => {
    let time = moment(w.time).valueOf();
    this.removeOldPoints(Highcharts.charts[this.quoteChart], time);
    this.removeOldPoints(Highcharts.charts[this.baseChart], time);
    Highcharts.charts[this.quoteChart].series[0].addPoint([time, w.totalQuote], false);
    Highcharts.charts[this.quoteChart].series[1].addPoint([time, w.availQuote], false);
    Highcharts.charts[this.quoteChart].series[2].addPoint([time, w.heldQuote]), true;
    Highcharts.charts[this.baseChart].series[0].addPoint([time, w.totalBase], false);
    Highcharts.charts[this.baseChart].series[1].addPoint([time, w.availBase], false);
    Highcharts.charts[this.baseChart].series[2].addPoint([time, w.heldBase], true);
  }

  private removeOldPoints = (chart: any, time: number) => {
    chart.series.forEach(serie => {
      while(serie.data.length &&  Math.abs(time - serie.data[0].x) > 21600000)
        serie.data[0].remove(false);
    });
  }
}
