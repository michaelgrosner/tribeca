import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';
import Highcharts = require('highcharts');

import Models = require('./models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-stats',
  template: `<div class="col-md-6 col-xs-6">
  <table><tr><td>
    <chart style="position:relative;top:5px;height:339px;width:700px;" type="StockChart" [options]="fvChartOptions" (load)="saveInstance($event.context, 'fv')"></chart>
  </td><td>
    <chart style="position:relative;top:10px;height:167px;width:700px;" [options]="baseChartOptions" (load)="saveInstance($event.context, 'base')"></chart>
    <chart style="position:relative;top:11px;height:167px;width:700px;" [options]="quoteChartOptions" (load)="saveInstance($event.context, 'quote')"></chart>
  </td></tr></table>
    </div>`
})
export class StatsComponent implements OnInit {

  public positionData: Models.PositionReport;
  public targetBasePosition: number;
  public fairValue: number;
  public width: number;
  public ewmaShort: number;
  public ewmaMedium: number;
  public ewmaLong: number;
  public ewmaQuote: number;
  public stdevWidth: Models.IStdev;
  public fvChart: any;
  public quoteChart: any;
  public baseChart: any;
  private saveInstance = (chartInstance, chartId) => {
    this[chartId+'Chart'] = Highcharts.charts.length;
    Highcharts.charts.push(chartInstance);
  }
  private pointFormatterBase = function () {
    return (this.series.type=='arearange')
      ? '<tr><td><span style="color:'+this.series.color+'">●</span>'+this.series.name+' High:</td><td style="text-align:right;"> <b>'+this.high.toFixed((<any>Highcharts).customProductFixed)+' ' + ((<any>Highcharts).customQuoteCurrency) + '</b></td></tr>'
        + '<tr><td><span style="color:'+this.series.color+'">●</span>'+this.series.name+' Low:</td><td style="text-align:right;"> <b>'+this.low.toFixed((<any>Highcharts).customProductFixed)+' ' + ((<any>Highcharts).customQuoteCurrency) + '</b></td></tr>'
      : '<tr><td><span style="color:'+this.series.color+'">' + (<any>Highcharts).customSymbols[this.series.symbol] + '</span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+this.y.toFixed((<any>Highcharts).customProductFixed)+' ' + ((<any>Highcharts).customQuoteCurrency) + '</b></td></tr>';
  }
  private pointFormatterQuote = function () {
    return '<tr><td><span style="color:'+this.series.color+'">' + (<any>Highcharts).customSymbols[this.series.symbol] + '</span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+this.y.toFixed(8)+' ' + ((<any>Highcharts).customBaseCurrency) + '</b></td></tr>';
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
        width: 700,
        height: 339,
        zoomType: false,
        backgroundColor:'rgba(255, 255, 255, 0)',
    },
    navigator: {enabled: false},
    rangeSelector:{enabled: false,height:0},
    scrollbar: {enabled: false},
    credits: {enabled: false},
    xAxis: {
      type: 'datetime',
      crosshair: true,
      // events: {setExtremes: this.syncExtremes},
      labels: {enabled: false},
      gridLineWidth: 0,
      dateTimeLabelFormats: {millisecond: '%H:%M:%S',second: '%H:%M:%S',minute: '%H:%M',hour: '%H:%M',day: '%m-%d',week: '%m-%d',month: '%m',year: '%Y'}
    },
    yAxis: [{
      title: {text: 'Fair Value and Trades'},
      labels: {enabled: false},
      gridLineWidth: 0
    },{
      title: {text: 'STDEV 20'},
      labels: {enabled: false},
      opposite: true,
      gridLineWidth: 0
    }],
    legend: {
      enabled:true,
      itemStyle: {
        color: 'lightgray'
      },
      itemHoverStyle: {
        color: 'gray'
      },
      itemHiddenStyle: {
        color: 'black'
      }
    },
    tooltip: {
        shared: true,
        useHTML: true,
        headerFormat: '<small>{point.x:%A} <b>{point.x:%H:%M:%S}</b></small><table>',
        footerFormat: '</table>'
    },
    series: [{
      name: 'Fair Value',
      type: 'spline',
      lineWidth:4,
      colorIndex: 2,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: [],
      id: 'fvseries'
    },{
      name: 'Width',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      colorIndex: 2,
      fillOpacity: 0.2,
      zIndex: -1,
      data: []
    },{
      name: 'Sell',
      type: 'spline',
      zIndex:1,
      colorIndex: 5,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: [],
      id: 'sellseries'
    },{
      name: 'Sell',
      type: 'flags',
      zIndex:2,
      colorIndex: 5,
      data: [],
      onSeries: 'sellseries',
      shape: 'circlepin',
      width: 16
    },{
      name: 'Buy',
      type: 'spline',
      zIndex:1,
      colorIndex: 0,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: [],
      id: 'buyseries'
    },{
      name: 'Buy',
      type: 'flags',
      zIndex:2,
      colorIndex: 0,
      data: [],
      onSeries: 'buyseries',
      shape: 'circlepin',
      width: 16
    },{
      name: 'EWMA Quote',
      type: 'spline',
      color: '#ffff00',
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: []
    },{
      name: 'EWMA Long',
      type: 'spline',
      colorIndex: 6,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: []
    },{
      name: 'EWMA Medium',
      type: 'spline',
      colorIndex: 6,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: []
    },{
      name: 'EWMA Short',
      type: 'spline',
      colorIndex: 3,
      tooltip: {pointFormatter:this.pointFormatterBase},
      data: []
    },{
      name: 'STDEV Fair',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      data: []
    },{
      name: 'STDEV Tops',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      data: []
    },{
      name: 'STDEV TopAsk',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      data: []
    },{
      name: 'STDEV TopBid',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      data: []
    },{
      name: 'STDEV BBFair',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      color:'#af451e',
      fillOpacity: 0.2,
      zIndex: -1,
      data: []
    },{
      name: 'STDEV BBTops',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      color:'#af451e',
      fillOpacity: 0.2,
      zIndex: -1,
      data: []
    },{
      name: 'STDEV BBTop',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      color:'#af451e',
      fillOpacity: 0.2,
      zIndex: -1,
      data: []
    }]
  };
  public quoteChartOptions = {
    title: 'quote wallet',
    chart: {
        width: 700,
        height: 167,
        zoomType: false,
        resetZoomButton: {theme: {display: 'none'}},
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    credits: {enabled: false},
    tooltip: {
        shared: true,
        useHTML: true,
        headerFormat: '<small>{point.x:%A} <b>{point.x:%H:%M:%S}</b></small><table>',
        footerFormat: '</table>',
        pointFormatter: this.pointFormatterBase
    },
    plotOptions: {
      area: {stacking: 'normal',connectNulls: true,marker: {enabled: false}},
      spline: {marker: {enabled: false}}
    },
    xAxis: {
      type: 'datetime',
      crosshair: true,
      // events: {setExtremes: this.syncExtremes},
      labels: {enabled: true},
      dateTimeLabelFormats: {millisecond: '%H:%M:%S',second: '%H:%M:%S',minute: '%H:%M',hour: '%H:%M',day: '%m-%d',week: '%m-%d',month: '%m',year: '%Y'}
    },
    yAxis: [{
      title: {text: 'Total Position'},
      opposite: true,
      labels: {enabled: false},
      gridLineWidth: 0
    },{
      title: {text: 'Available and Held'},
      min: 0,
      labels: {enabled: false},
      gridLineWidth: 0
    }],
    legend: {enabled: false},
    series: [{
      name: 'Total Value',
      type: 'spline',
      zIndex: 1,
      colorIndex:2,
      lineWidth:3,
      data: []
    },{
      name: 'Target',
      type: 'spline',
      yAxis: 1,
      zIndex: 2,
      colorIndex:6,
      data: []
    },{
      name: 'Available',
      type: 'area',
      colorIndex:0,
      fillOpacity: 0.2,
      yAxis: 1,
      data: []
    },{
      name: 'Held',
      type: 'area',
      colorIndex:0,
      yAxis: 1,
      marker:{symbol:'triangle-down'},
      data: []
    }]
  };
  public baseChartOptions = {
    title: 'base wallet',
    chart: {
        width: 700,
        height: 167,
        zoomType: false,
        resetZoomButton: {theme: {display: 'none'}},
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    credits: {enabled: false},
    tooltip: {
        shared: true,
        headerFormat: '<small>{point.x:%A} <b>{point.x:%H:%M:%S}</b></small><table>',
        footerFormat: '</table>',
        useHTML: true,
        pointFormatter: this.pointFormatterQuote
    },
    plotOptions: {
      area: {stacking: 'normal',connectNulls: true,marker: {enabled: false}},
      spline: {marker: {enabled: false}}
    },
    xAxis: {
      type: 'datetime',
      crosshair: true,
      // events: {setExtremes: this.syncExtremes},
      labels: {enabled: false},
      dateTimeLabelFormats: {millisecond: '%H:%M:%S',second: '%H:%M:%S',minute: '%H:%M',hour: '%H:%M',day: '%m-%d',week: '%m-%d',month: '%m',year: '%Y'}
    },
    yAxis: [{
      title: {text: 'Total Position'},
      opposite: true,
      labels: {enabled: false},
      gridLineWidth: 0
    },{
      title: {text: 'Available and Held'},
      min: 0,
      labels: {enabled: false},
      gridLineWidth: 0
    }],
    legend: {enabled: false},
    series: [{
      name: 'Total Value',
      type: 'spline',
      zIndex: 1,
      colorIndex:2,
      lineWidth:3,
      data: []
    },{
      name: 'Target (TBP)',
      type: 'spline',
      yAxis: 1,
      zIndex: 2,
      colorIndex:6,
      data: []
    },{
      name: 'Available',
      type: 'area',
      yAxis: 1,
      colorIndex:5,
      fillOpacity: 0.2,
      data: []
    },{
      name: 'Held',
      type: 'area',
      yAxis: 1,
      colorIndex:5,
      data: []
    }]
  };

  @Input() product: Models.ProductState;

  private showStats: boolean;
  @Input() set setShowStats(showStats: boolean) {
    if (!this.showStats && showStats)
      Highcharts.charts.forEach(chart => chart.redraw(false) );
    this.showStats = showStats;
  }

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    (<any>Highcharts).customBaseCurrency = '';
    (<any>Highcharts).customQuoteCurrency = '';
    (<any>Highcharts).customProductFixed = this.product.fixed;
    (<any>Highcharts).customSymbols = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};
    Highcharts.setOptions({global: {getTimezoneOffset: function () {return new Date().getTimezoneOffset(); }}});
    setTimeout(() => {
      jQuery('chart').bind('mousemove touchmove touchstart', function (e: any) {
        var chart, point, i, event, containerLeft, thisLeft;
        for (i = 0; i < Highcharts.charts.length; ++i) {
          chart = Highcharts.charts[i];
          containerLeft = jQuery(chart.container).offset().left;
          thisLeft = jQuery(this).offset().left;
          if (containerLeft == thisLeft && jQuery(chart.container).offset().top == jQuery(this).offset().top) continue;
          chart.pointer.reset = function () { return undefined; };
          let ev: any = jQuery.extend(jQuery.Event(e.originalEvent.type), {
              which: 1,
              chartX: e.originalEvent.chartX,
              chartY: e.originalEvent.chartY,
              clientX: (containerLeft != thisLeft)?containerLeft - thisLeft + e.originalEvent.clientX:e.originalEvent.clientX,
              clientY: e.originalEvent.clientY,
              pageX: (containerLeft != thisLeft)?containerLeft - thisLeft + e.originalEvent.pageX:e.originalEvent.pageX,
              pageY: e.originalEvent.pageY,
              screenX: (containerLeft != thisLeft)?containerLeft - thisLeft + e.originalEvent.screenX:e.originalEvent.screenX,
              screenY: e.originalEvent.screenY
          });
          event = chart.pointer.normalize(ev);
          point = chart.series[0].searchPoint(event, true);
          if (point) {
            point.onMouseOver();
            point.series.chart.xAxis[0].drawCrosshair(event, point);
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

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.FairValue)
        .registerSubscriber(this.updateFairValue);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.MarketData)
        .registerSubscriber(this.updateMarket);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.Position)
        .registerSubscriber(this.updatePosition);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.TargetBasePosition)
        .registerSubscriber(this.updateTargetBasePosition);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.EWMAChart)
        .registerSubscriber(this.addEWMAChartData);

      this.subscriberFactory
        .getSubscriber(this.zone, Models.Topics.TradesChart)
        .registerSubscriber(this.addTradesChartData);

      setInterval(() => this.updateCharts(new Date().getTime()), 10000);
    }, 10000);
  }

  private updateCharts = (time: number) => {
    this.removeOldPoints(time);
    if (this.fairValue) {
      if (this.stdevWidth) {
        if (this.stdevWidth.fv) Highcharts.charts[this.fvChart].series[10].addPoint([time, this.stdevWidth.fv], false);
        if (this.stdevWidth.tops) Highcharts.charts[this.fvChart].series[11].addPoint([time, this.stdevWidth.tops], false);
        if (this.stdevWidth.ask) Highcharts.charts[this.fvChart].series[12].addPoint([time, this.stdevWidth.ask], false);
        if (this.stdevWidth.bid) Highcharts.charts[this.fvChart].series[13].addPoint([time, this.stdevWidth.bid], false);
        if (this.stdevWidth.fv && this.stdevWidth.fvMean) Highcharts.charts[this.fvChart].series[14].addPoint([time, this.stdevWidth.fvMean-this.stdevWidth.fv, this.stdevWidth.fvMean+this.stdevWidth.fv], this.showStats, false, false);
        if (this.stdevWidth.tops && this.stdevWidth.topsMean) Highcharts.charts[this.fvChart].series[15].addPoint([time, this.stdevWidth.topsMean-this.stdevWidth.tops, this.stdevWidth.topsMean+this.stdevWidth.tops], this.showStats, false, false);
        if (this.stdevWidth.ask && this.stdevWidth.bid && this.stdevWidth.askMean && this.stdevWidth.bidMean) Highcharts.charts[this.fvChart].series[16].addPoint([time, this.stdevWidth.bidMean-this.stdevWidth.bid, this.stdevWidth.askMean+this.stdevWidth.ask], this.showStats, false, false);
      }
      if (this.ewmaQuote) Highcharts.charts[this.fvChart].series[6].addPoint([time, this.ewmaQuote], false);
      if (this.ewmaLong) Highcharts.charts[this.fvChart].series[7].addPoint([time, this.ewmaLong], false);
      if (this.ewmaMedium) Highcharts.charts[this.fvChart].series[8].addPoint([time, this.ewmaMedium], false);
      if (this.ewmaShort) Highcharts.charts[this.fvChart].series[9].addPoint([time, this.ewmaShort], false);
      Highcharts.charts[this.fvChart].series[0].addPoint([time, this.fairValue], this.showStats);
      if (this.width) Highcharts.charts[this.fvChart].series[1].addPoint([time, this.fairValue-this.width, this.fairValue+this.width], this.showStats, false, false);
    }
    if (this.positionData) {
      Highcharts.charts[this.quoteChart].yAxis[1].setExtremes(0, Math.max(this.positionData.quoteValue,Highcharts.charts[this.quoteChart].yAxis[1].getExtremes().dataMax), false, true, { trigger: 'syncExtremes' });
      Highcharts.charts[this.baseChart].yAxis[1].setExtremes(0, Math.max(this.positionData.value,Highcharts.charts[this.baseChart].yAxis[1].getExtremes().dataMax), false, true, { trigger: 'syncExtremes' });
      if (this.targetBasePosition) {
        Highcharts.charts[this.quoteChart].series[1].addPoint([time, (this.positionData.value-this.targetBasePosition)*this.positionData.quoteValue/this.positionData.value], false);
        Highcharts.charts[this.baseChart].series[1].addPoint([time, this.targetBasePosition], false);
      }
      Highcharts.charts[this.quoteChart].series[0].addPoint([time, this.positionData.quoteValue], false);
      Highcharts.charts[this.quoteChart].series[2].addPoint([time, this.positionData.quoteAmount], false);
      Highcharts.charts[this.quoteChart].series[3].addPoint([time, this.positionData.quoteHeldAmount], this.showStats);
      Highcharts.charts[this.baseChart].series[0].addPoint([time, this.positionData.value], false);
      Highcharts.charts[this.baseChart].series[2].addPoint([time, this.positionData.baseAmount], false);
      Highcharts.charts[this.baseChart].series[3].addPoint([time, this.positionData.baseHeldAmount], this.showStats);
    }
  }

  private updateFairValue = (fv: Models.FairValue) => {
    if (fv == null) return;
    this.fairValue = fv.price;
  }

  private addEWMAChartData = (ewma: Models.EWMAChart) => {
    if (ewma === null) return;
    this.fairValue = ewma.fairValue;
    if (ewma.ewmaQuote) this.ewmaQuote = ewma.ewmaQuote;
    if (ewma.ewmaShort) this.ewmaShort = ewma.ewmaShort;
    if (ewma.ewmaMedium) this.ewmaMedium = ewma.ewmaMedium;
    if (ewma.ewmaLong) this.ewmaLong = ewma.ewmaLong;
    if (ewma.stdevWidth) this.stdevWidth = ewma.stdevWidth;
  }

  private updateMarket = (update: Models.Market) => {
    if (update && update.bids && update.bids.length && update.asks && update.asks.length)
      this.width = (update.asks[0].price - update.bids[0].price) / 2;
  }

  private addTradesChartData = (t: Models.TradeChart) => {
    let time = new Date().getTime();
    Highcharts.charts[this.fvChart].series[Models.Side[t.side] == 'Bid' ? 4 : 2].addPoint([time, t.price], false);
    (<any>Highcharts).charts[this.fvChart].series[Models.Side[t.side] == 'Bid' ? 5 : 3].addPoint({
      x: time,
      title: (t.pong ? '¯' : '_')+(Models.Side[t.side] == 'Bid' ? 'B' : 'S'),
      useHTML:true,
      text: '<tr><td colspan="2"><b><span style="color:'+(Models.Side[t.side] == 'Bid' ? '#0000FF':'#FF0000')+';">'+(Models.Side[t.side] == 'Bid' ? '▼':'▲')+'</span> '+(Models.Side[t.side] == 'Bid' ? 'Buy':'Sell')+'</b> (P'+(t.pong?'o':'i')+'ng)</td></tr>'
        + '<tr><td>' + 'Price:</td><td style="text-align:right;"> <b>' + t.price.toFixed((<any>Highcharts).customProductFixed) + ' ' + ((<any>Highcharts).customQuoteCurrency) + '</b></td></tr>'
        + '<tr><td>' + 'Qty:</td><td style="text-align:right;"> <b>' + t.quantity.toFixed(8) + ' ' + ((<any>Highcharts).customBaseCurrency) + '</b></td></tr>'
        + '<tr><td>' + 'Value:</td><td style="text-align:right;"> <b>' + t.value.toFixed((<any>Highcharts).customProductFixed) + ' ' + ((<any>Highcharts).customQuoteCurrency) + '</b></td></tr>'
    }, this.showStats && !this.fairValue);
    this.updateCharts(time);
  }

  private updateTargetBasePosition = (value : Models.TargetBasePositionValue) => {
    if (value == null) return;
    this.targetBasePosition = value.tbp;
  }

  private updatePosition = (o: Models.PositionReport) => {
    if (o === null) return;
    let time = new Date().getTime();
    if (!(<any>Highcharts).customBaseCurrency) (<any>Highcharts).customBaseCurrency = Models.Currency[o.pair.base];
    if (!(<any>Highcharts).customQuoteCurrency) (<any>Highcharts).customQuoteCurrency = Models.Currency[o.pair.quote];
    this.positionData = o;
  }

  private removeOldPoints = (time: number) => {
    Highcharts.charts.forEach(chart => { chart.series.forEach(serie => {
      while(serie.data.length && Math.abs(time - serie.data[0].x) > 21600000)
        serie.data[0].remove(false);
    })});
  }
}
