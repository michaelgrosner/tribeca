import {Component, Input, OnInit} from '@angular/core';

import * as Highcharts from 'highcharts';
require('highcharts/highcharts-more')(Highcharts);

import * as Models from 'lib/models';

interface PointOptionsObjectExtended extends Highcharts.PointOptionsObject {
  side: string;
  pong: string;
  price:string;
  qty:string;
  val:string;
  quote:string;
  base:string;
}

var self;

@Component({
  selector: 'stats',
  template: `<div class="col-md-12 col-xs-12" style="height:622px;">
    <div class="col-md-6 col-xs-6">
        <highcharts-chart [Highcharts]="Highcharts" style="display: block;" [options]="fvChartOptions"></highcharts-chart>
    </div>
    <div class="col-md-6 col-xs-6">
      <div style="height:306px;">
        <highcharts-chart [Highcharts]="Highcharts" style="display: block;" [options]="baseChartOptions"></highcharts-chart>
      </div>
      <div style="height:306px;">
        <highcharts-chart [Highcharts]="Highcharts" style="display: block;" [options]="quoteChartOptions"></highcharts-chart>
      </div>
    </div>
  </div>`
})
export class StatsComponent implements OnInit {

  private width: number;
  private ewma: Models.IEwma;
  private stdevWidth: Models.IStdev;
  private tradesBuySize: number;
  private tradesSellSize: number;
  private fvChart: number = 0;
  private baseChart: number = 1;
  private quoteChart: number = 2;
  private Highcharts: typeof Highcharts = Highcharts;

  @Input() product: Models.ProductAdvertisement;

  @Input() fairValue: Models.FairValue;

  @Input() quotingParameters: Models.QuotingParameters;

  @Input() position: Models.PositionReport;

  @Input() targetBasePosition: Models.TargetBasePositionValue;

  @Input() set setMarketWidth(o: number) {
    if (o == null) return;
    this.width = o / 2;
  };

  @Input() set setMarketChartData(o: Models.MarketChart) {
    if (o === null) return;
    if (o.ewma) this.ewma = o.ewma;
    if (o.stdevWidth) this.stdevWidth = o.stdevWidth;
    if (o.tradesBuySize) this.tradesBuySize = o.tradesBuySize;
    if (o.tradesSellSize) this.tradesSellSize = o.tradesSellSize;
  };

  @Input() set setTradesChartData(t: Models.TradeChart) {
    if (t===null) return;
    let time = new Date().getTime();
    Highcharts.charts[this.fvChart].series[Models.Side[t.side] == 'Bid' ? 4 : 2].addPoint([time, t.price], false);
    Highcharts.charts[this.fvChart].series[Models.Side[t.side] == 'Bid' ? 5 : 3].addPoint(<PointOptionsObjectExtended>{
      x: time,
      y: t.price,
      z: t.quantity,
      title: (t.pong ? '⇋' : '⇁')+(Models.Side[t.side] == 'Bid' ? 'B' : 'S'),
      side:(Models.Side[t.side] == 'Bid' ? 'Buy':'Sell'),
      pong:(t.pong?'o':'i'),
      price:t.price.toFixed(this.product.tickPrice),
      qty:t.quantity.toFixed(8),
      val:t.value.toFixed(this.product.tickPrice),
      quote:this.product.quote,
      base:this.product.base
    }, true);
    this.updateCharts(time);
  };

  private showStats: boolean;
  @Input() set setShowStats(showStats: boolean) {
    if (!this.showStats && showStats)
      Highcharts.charts.forEach(chart => chart.redraw(false) );
    this.showStats = showStats;
  };

  private syncExtremes = function (e) {
    var thisChart = this.chart;
    if (e.trigger !== 'syncExtremes') {
        Highcharts.each(Highcharts.charts, function (chart) {
            if (chart !== thisChart && chart.xAxis[0].setExtremes)
              chart.xAxis[0].setExtremes(e.min, e.max, undefined, true, { trigger: 'syncExtremes' });
        });
    }
  };

  private pointFormatterBase = function () {
    return this.series.type=='arearange'
      ? '<tr><td><span style="color:'+this.series.color+'">●</span>'+this.series.name+(this.series.name=="Width" && self.quotingParameters.protectionEwmaWidthPing ?" EWMA":"")+' High:</td><td style="text-align:right;"> <b>'+ this.high.toFixed(self.product.tickPrice) +' ' + self.product.quote + '</b></td></tr>'
        + '<tr><td><span style="color:'+this.series.color+'">●</span>'+this.series.name+(this.series.name=="Width" && self.quotingParameters.protectionEwmaWidthPing?" EWMA":"")+' Low:</td><td style="text-align:right;"> <b>'+ this.low.toFixed(self.product.tickPrice) +' ' + self.product.quote + '</b></td></tr>'
      : ( this.series.type=='bubble'
        ? '<tr><td colspan="2"><b><span style="color:'+this.series.color+';">'+(this.side == 'Buy' ? '▼':'▲')+'</span> '+this.side+'</b> (P'+this.pong+'ng)</td></tr>'
          + '<tr><td>' + 'Price:</td><td style="text-align:right;"> <b>'+this.price+' '+this.quote+'</b></td></tr>'
          + '<tr><td>' + 'Qty:</td><td style="text-align:right;"> <b>'+this.qty+' '+this.base+'</b></td></tr>'
          + '<tr><td>' + 'Value:</td><td style="text-align:right;"> <b>'+this.val+' '+this.quote+'</b></td></tr>'
        :'<tr><td><span style="color:'+this.series.color+'">' + self.customSymbols[this.series.symbol] + '</span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+ this.y.toFixed(self.product.tickPrice) +' ' + self.product.quote + '</b></td></tr>'
      );
  };

  private pointFormatterQuote = function () {
    return '<tr><td><span style="color:'+this.series.color+'">' + self.customSymbols[this.series.symbol||'square'] + '</span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+this.y.toFixed(8)+' ' + self.product.base + '</b></td></tr>';
  };

  private pointFormatterPercentage = function () {
    return '<tr><td><span style="color:'+this.series.color+'">' + self.customSymbols[this.series.symbol||'square'] + '</span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+this.y.toFixed(4)+' % </b></td></tr>';
  };

  private fvChartOptions = {
    title: 'fair value',
    chart: {
        width: null,
        height: 615,
        type: 'bubble',
        zoomType: false,
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    plotOptions: {series: {marker: {enabled: false}}},
    navigator: {enabled: false},
    rangeSelector:{enabled: false,height:0},
    scrollbar: {enabled: false},
    credits: {enabled: false},
    xAxis: {
      type: 'datetime',
      crosshair: true,
      // events: {setExtremes: this.syncExtremes},
      labels: {enabled: true},
      gridLineWidth: 0,
      dateTimeLabelFormats: {millisecond: '%H:%M:%S',second: '%H:%M:%S',minute: '%H:%M',hour: '%H:%M',day: '%m-%d',week: '%m-%d',month: '%m',year: '%Y'}
    },
    yAxis: [{
      title: {text: 'Fair Value and Trades'},
      labels: {enabled: true},
      crosshair: true,
      gridLineWidth: 0
    },{
      title: {text: 'STDEV 20'},
      labels: {enabled: false},
      opposite: true,
      gridLineWidth: 0
    },{
      title: {text: 'Market Size'},
      labels: {enabled: false},
      opposite: true,
      pointPadding: 0,
      groupPadding: 0,
      borderWidth: 0,
      shadow: false,
      gridLineWidth: 0
    },{
      title: {text: 'Percentage'},
      min: -100,
      max: 100,
      labels: {enabled: false},
      opposite: true,
      gridLineWidth: 0
    }],
    legend: {
      enabled:true,
      itemStyle: {
        color: 'yellowgreen'
      },
      itemHoverStyle: {
        color: 'gold'
      },
      itemHiddenStyle: {
        color: 'darkgrey'
      }
    },
    tooltip: {
        shared: true,
        useHTML: true,
        headerFormat: 'Fair value:<br><small>{point.x:%A} <b>{point.x:%H:%M:%S}</b></small><table>',
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
      type: 'bubble',
      zIndex:2,
      color: 'white',
      marker: {enabled: true,fillColor: 'transparent',lineColor: Highcharts.getOptions().colors[5]},
      tooltip: {pointFormatter: this.pointFormatterBase},
      dataLabels: {enabled: true,format: '{point.title}'},
      data: [],
      linkedTo: ':previous'
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
      type: 'bubble',
      zIndex:2,
      color: 'white',
      marker: {enabled: true,fillColor: 'transparent',lineColor: Highcharts.getOptions().colors[0]},
      tooltip: {pointFormatter: this.pointFormatterBase},
      dataLabels: {enabled: true,format: '{point.title}'},
      data: [],
      linkedTo: ':previous'
    },{
      name: 'EWMA Quote',
      type: 'spline',
      color: '#ffff00',
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: []
    },{
      name: 'EWMA Very Long',
      type: 'spline',
      colorIndex: 6,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: []
    },{
      name: 'EWMA Long',
      type: 'spline',
      colorIndex: 5,
      tooltip: {pointFormatter: this.pointFormatterBase},
      data: []
    },{
      name: 'EWMA Medium',
      type: 'spline',
      colorIndex: 4,
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
      visible: false,
      data: []
    },{
      name: 'STDEV Tops',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      visible: false,
      data: []
    },{
      name: 'STDEV TopAsk',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      visible: false,
      data: []
    },{
      name: 'STDEV TopBid',
      type: 'spline',
      lineWidth:1,
      color:'#af451e',
      tooltip: {pointFormatter: this.pointFormatterBase},
      yAxis: 1,
      visible: false,
      data: []
    },{
      name: 'STDEV BBFair',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      color:'#af451e',
      fillOpacity: 0.2,
      zIndex: -1,
      visible: false,
      data: []
    },{
      name: 'STDEV BBTops',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      color:'#af451e',
      fillOpacity: 0.2,
      zIndex: -1,
      visible: false,
      data: []
    },{
      name: 'STDEV BBTop',
      type: 'arearange',
      tooltip: {pointFormatter: this.pointFormatterBase},
      lineWidth: 0,
      color:'#af451e',
      fillOpacity: 0.2,
      zIndex: -1,
      visible: false,
      data: []
    },{
      type: 'column',
      name: 'Market Buy Size',
      tooltip: {pointFormatter: this.pointFormatterQuote},
      yAxis: 2,
      colorIndex:0,
      data: [],
      zIndex: -2
    },{
      type: 'column',
      name: 'Market Sell Size',
      tooltip: {pointFormatter: this.pointFormatterQuote},
      yAxis: 2,
      colorIndex:5,
      data: [],
      zIndex: -2
    },{
      name: 'EWMA Trend Diff',
      type: 'spline',
      color: '#fd00ff',
      tooltip: {pointFormatter: this.pointFormatterPercentage},
      yAxis: 3,
      visible: false,
      data: []
    }]
  };

  private quoteChartOptions = {
    title: 'quote wallet',
    chart: {
        width: null,
        height: 306,
        zoomType: false,
        resetZoomButton: {theme: {display: 'none'}},
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    credits: {enabled: false},
    tooltip: {
        shared: true,
        useHTML: true,
        headerFormat: 'Quote wallet:<br><small>{point.x:%A} <b>{point.x:%H:%M:%S}</b></small><table>',
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
      name: 'pDiv',
      type: 'arearange',
      lineWidth: 0,
      yAxis: 1,
      colorIndex: 6,
      fillOpacity: 0.3,
      zIndex: 0,
      marker: { enabled: false },
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
      yAxis: 1,
      marker:{symbol:'triangle-down'},
      data: []
    }]
  };

  private baseChartOptions = {
    title: 'base wallet',
    chart: {
        width: null,
        height: 306,
        zoomType: false,
        resetZoomButton: {theme: {display: 'none'}},
        backgroundColor:'rgba(255, 255, 255, 0)'
    },
    credits: {enabled: false},
    tooltip: {
        shared: true,
        headerFormat: 'Base wallet:<br><small>{point.x:%A} <b>{point.x:%H:%M:%S}</b></small><table>',
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
      name: 'pDiv',
      type: 'arearange',
      lineWidth: 0,
      yAxis: 1,
      colorIndex: 6,
      fillOpacity: 0.3,
      zIndex: 0,
      marker: { enabled: false },
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

  private customSymbols: object = {'circle': '●','diamond': '♦','square': '■','triangle': '▲','triangle-down': '▼'};

  ngOnInit() {
    self = this;

    setInterval(() => this.updateCharts(new Date().getTime()), 10000);
  };

  private updateCharts = (time: number) => {
    this.removeOldPoints(time);
    if (this.fairValue.price) {
      if (this.stdevWidth) {
        if (this.stdevWidth.fv) Highcharts.charts[this.fvChart].series[11].addPoint([time, this.stdevWidth.fv], false);
        if (this.stdevWidth.tops) Highcharts.charts[this.fvChart].series[12].addPoint([time, this.stdevWidth.tops], false);
        if (this.stdevWidth.ask) Highcharts.charts[this.fvChart].series[13].addPoint([time, this.stdevWidth.ask], false);
        if (this.stdevWidth.bid) Highcharts.charts[this.fvChart].series[14].addPoint([time, this.stdevWidth.bid], false);
        if (this.stdevWidth.fv && this.stdevWidth.fvMean) Highcharts.charts[this.fvChart].series[15].addPoint([time, this.stdevWidth.fvMean-this.stdevWidth.fv, this.stdevWidth.fvMean+this.stdevWidth.fv], this.showStats, false, false);
        if (this.stdevWidth.tops && this.stdevWidth.topsMean) Highcharts.charts[this.fvChart].series[16].addPoint([time, this.stdevWidth.topsMean-this.stdevWidth.tops, this.stdevWidth.topsMean+this.stdevWidth.tops], this.showStats, false, false);
        if (this.stdevWidth.ask && this.stdevWidth.bid && this.stdevWidth.askMean && this.stdevWidth.bidMean) Highcharts.charts[this.fvChart].series[17].addPoint([time, this.stdevWidth.bidMean-this.stdevWidth.bid, this.stdevWidth.askMean+this.stdevWidth.ask], this.showStats, false, false);
      }
      Highcharts.charts[this.fvChart].yAxis[2].setExtremes(0, Math.max(this.tradesBuySize*4,this.tradesSellSize*4,Highcharts.charts[this.fvChart].yAxis[2].getExtremes().dataMax*4), false, true, { trigger: 'syncExtremes' });
      if (this.tradesBuySize) Highcharts.charts[this.fvChart].series[18].addPoint([time, this.tradesBuySize], false);
      if (this.tradesSellSize) Highcharts.charts[this.fvChart].series[19].addPoint([time, this.tradesSellSize], false);
      this.tradesBuySize = 0;
      this.tradesSellSize = 0;
      if (this.ewma) {
        if (this.ewma.ewmaQuote) Highcharts.charts[this.fvChart].series[6].addPoint([time, this.ewma.ewmaQuote], false);
        if (this.ewma.ewmaVeryLong) Highcharts.charts[this.fvChart].series[7].addPoint([time, this.ewma.ewmaVeryLong], false);
        if (this.ewma.ewmaLong) Highcharts.charts[this.fvChart].series[8].addPoint([time, this.ewma.ewmaLong], false);
        if (this.ewma.ewmaMedium) Highcharts.charts[this.fvChart].series[9].addPoint([time, this.ewma.ewmaMedium], false);
        if (this.ewma.ewmaShort) Highcharts.charts[this.fvChart].series[10].addPoint([time, this.ewma.ewmaShort], false);
        if (this.ewma.ewmaTrendDiff) Highcharts.charts[this.fvChart].series[20].addPoint([time, this.ewma.ewmaTrendDiff], false);
      }
      Highcharts.charts[this.fvChart].series[0].addPoint([time, this.fairValue.price], this.showStats);
      if (this.quotingParameters.protectionEwmaWidthPing && (this.ewma && this.ewma.ewmaWidth)) Highcharts.charts[this.fvChart].series[1].addPoint([time, this.fairValue.price-this.ewma.ewmaWidth, this.fairValue.price+this.ewma.ewmaWidth], this.showStats, false, false);
      else if (this.width) Highcharts.charts[this.fvChart].series[1].addPoint([time, this.fairValue.price-this.width, this.fairValue.price+this.width], this.showStats, false, false);
    }
    if (this.position.base.value || this.position.quote.value) {
      Highcharts.charts[this.quoteChart].yAxis[1].setExtremes(0, Math.max(this.position.quote.value,Highcharts.charts[this.quoteChart].yAxis[1].getExtremes().dataMax), false, true, { trigger: 'syncExtremes' });
      Highcharts.charts[this.baseChart].yAxis[1].setExtremes(0, Math.max(this.position.base.value,Highcharts.charts[this.baseChart].yAxis[1].getExtremes().dataMax), false, true, { trigger: 'syncExtremes' });
      Highcharts.charts[this.quoteChart].series[1].addPoint([time, (this.position.base.value-this.targetBasePosition.tbp)*this.position.quote.value/this.position.base.value], false);
      Highcharts.charts[this.baseChart].series[1].addPoint([time, this.targetBasePosition.tbp], false);
      Highcharts.charts[this.quoteChart].series[2].addPoint([time, Math.max(0, this.position.base.value-this.targetBasePosition.tbp-this.targetBasePosition.pDiv)*this.position.quote.value/this.position.base.value, Math.min(this.position.base.value, this.position.base.value-this.targetBasePosition.tbp+this.targetBasePosition.pDiv)*this.position.quote.value/this.position.base.value], this.showStats, false, false);
      Highcharts.charts[this.baseChart].series[2].addPoint([time, Math.max(0,this.targetBasePosition.tbp-this.targetBasePosition.pDiv), Math.min(this.position.base.value, this.targetBasePosition.tbp+this.targetBasePosition.pDiv)], this.showStats, false, false);
      Highcharts.charts[this.quoteChart].series[0].addPoint([time, this.position.quote.value], false);
      Highcharts.charts[this.quoteChart].series[3].addPoint([time, this.position.quote.amount], false);
      Highcharts.charts[this.quoteChart].series[4].addPoint([time, this.position.quote.held], this.showStats);
      Highcharts.charts[this.baseChart].series[0].addPoint([time, this.position.base.value], false);
      Highcharts.charts[this.baseChart].series[3].addPoint([time, this.position.base.amount], false);
      Highcharts.charts[this.baseChart].series[4].addPoint([time, this.position.base.held], this.showStats);
    }
  };

  private removeOldPoints = (time: number) => {
    Highcharts.charts.forEach(chart => { chart.series.forEach(serie => {
      while(serie.data.length && Math.abs(time - serie.data[0].x) > this.quotingParameters.profitHourInterval * 36e+5)
        serie.data[0].remove(false);
    })});
  };
};
