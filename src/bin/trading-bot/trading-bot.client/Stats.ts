import {Component, Input, OnInit} from '@angular/core';

import {Shared, Models} from 'lib/K';

@Component({
  selector: 'stats',
  template: `<div class="col-md-12 col-xs-12" style="height:622px;">
    <div class="col-md-6 col-xs-6">
        <highcharts-chart
          [Highcharts]="Highcharts"
          style="display: block;"
          [options]="fvChartOptions"></highcharts-chart>
    </div>
    <div class="col-md-6 col-xs-6">
      <div style="height:306px;">
        <highcharts-chart
          [Highcharts]="Highcharts"
          style="display: block;"
          [options]="baseChartOptions"></highcharts-chart>
      </div>
      <div style="height:306px;">
        <highcharts-chart
          [Highcharts]="Highcharts"
          style="display: block;"
          [options]="quoteChartOptions"></highcharts-chart>
      </div>
    </div>
  </div>`
})
export class StatsComponent implements OnInit {

  private fvChart:    number = 0;
  private baseChart:  number = 1;
  private quoteChart: number = 2;

  private showStats: boolean;

  private Highcharts: typeof Shared.Highcharts = Shared.Highcharts;

  @Input() product: Models.ProductAdvertisement;

  @Input() fairValue: Models.FairValue;

  @Input() quotingParameters: Models.QuotingParameters;

  @Input() position: Models.PositionReport;

  @Input() marketWidth: number;

  @Input() targetBasePosition: Models.TargetBasePositionValue;

  @Input() marketChart: Models.MarketChart;

  @Input() set _showStats(showStats: boolean) {
    if (!this.showStats && showStats)
      this.Highcharts.charts.forEach(chart => chart.redraw(false));
    this.showStats = showStats;
  };

  @Input() set tradesChart(o: Models.TradeChart) {
    if (!o.price) return;
    let time = new Date().getTime();
    this.Highcharts.charts[this.fvChart].series[Models.Side[o.side] == 'Bid' ? 4 : 2].addPoint([time, o.price], false);
    this.Highcharts.charts[this.fvChart].series[Models.Side[o.side] == 'Bid' ? 5 : 3].addPoint(<any>{
      x: time,
      y: o.price,
      z: o.quantity,
      title: (o.pong ? '⇋' : '⇁')+(Models.Side[o.side] == 'Bid' ? 'B' : 'S'),
      side: (Models.Side[o.side] == 'Bid' ? 'Buy':'Sell'),
      pong: o.pong?'o':'i',
      price: o.price.toFixed(this.product.tickPrice),
      qty: o.quantity.toFixed(8),
      val: o.value.toFixed(this.product.tickPrice)
    }, true);
    this.updateCharts(time);
  };

  private syncExtremes = function (e) {
    var thisChart = this.chart;
    if (e.trigger !== 'syncExtremes') {
        this.Highcharts.each(this.Highcharts.charts, function (chart) {
            if (chart !== thisChart && chart.xAxis[0].setExtremes)
              chart.xAxis[0].setExtremes(e.min, e.max, undefined, true, { trigger: 'syncExtremes' });
        });
    }
  };

  private pointFormatterBase = function () {
    return this.series.type=='arearange'
      ? '<tr><td><span style="color:'+this.series.color+'">●</span>'+this.series.name+(this.series.name=="Width" && this.series.chart.options.self.quotingParameters.protectionEwmaWidthPing ?" EWMA":"")+' High:</td><td style="text-align:right;"> <b>'+ this.high.toFixed(this.series.chart.options.self.product.tickPrice) +' ' + this.series.chart.options.self.product.quote + '</b></td></tr>'
        + '<tr><td><span style="color:'+this.series.color+'">●</span>'+this.series.name+(this.series.name=="Width" && this.series.chart.options.self.quotingParameters.protectionEwmaWidthPing?" EWMA":"")+' Low:</td><td style="text-align:right;"> <b>'+ this.low.toFixed(this.series.chart.options.self.product.tickPrice) +' ' + this.series.chart.options.self.product.quote + '</b></td></tr>'
      : ( this.series.type=='bubble'
        ? '<tr><td colspan="2"><b><span style="color:'+this.series.color+';">'+(this.side == 'Buy' ? '▼':'▲')+'</span> '+this.side+'</b> (P'+this.pong+'ng)</td></tr>'
          + '<tr><td>' + 'Price:</td><td style="text-align:right;"> <b>'+this.price+' '+this.series.chart.options.self.product.quote+'</b></td></tr>'
          + '<tr><td>' + 'Qty:</td><td style="text-align:right;"> <b>'+this.qty+' '+this.series.chart.options.self.product.base+'</b></td></tr>'
          + '<tr><td>' + 'Value:</td><td style="text-align:right;"> <b>'+this.val+' '+this.series.chart.options.self.product.quote+'</b></td></tr>'
        :'<tr><td><span style="color:'+this.series.color+'" class="point-'+this.series.symbol+'"></span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+ this.y.toFixed(this.series.chart.options.self.product.tickPrice) +' ' + this.series.chart.options.self.product.quote + '</b></td></tr>'
      );
  };

  private pointFormatterQuote = function () {
    return '<tr><td><span style="color:'+this.series.color+'" class="point-'+(this.series.symbol||'square')+'"></span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+this.y.toFixed(8)+' ' + this.series.chart.options.self.product.base + '</b></td></tr>';
  };

  private pointFormatterPercentage = function () {
    return '<tr><td><span style="color:'+this.series.color+'" class="point-'+(this.series.symbol||'square')+'"></span> '+this.series.name+':</td><td style="text-align:right;"> <b>'+this.y.toFixed(4)+' % </b></td></tr>';
  };

  private fvChartOptions = {
    self: this,
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
      marker: {enabled: true,fillColor: 'transparent',lineColor: this.Highcharts.getOptions().colors[5]},
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
      marker: {enabled: true,fillColor: 'transparent',lineColor: this.Highcharts.getOptions().colors[0]},
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
    self: this,
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
    self: this,
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

  ngOnInit() {
    setInterval(() => this.updateCharts(new Date().getTime()), 10e+3);
  };

  private updateCharts = (time: number) => {
    this.removeOldPoints(time);
    if (this.fairValue.price) {
      if (this.marketChart.stdevWidth) {
        if (this.marketChart.stdevWidth.fv)
          this.Highcharts.charts[this.fvChart].series[11].addPoint([time, this.marketChart.stdevWidth.fv], false);
        if (this.marketChart.stdevWidth.tops)
          this.Highcharts.charts[this.fvChart].series[12].addPoint([time, this.marketChart.stdevWidth.tops], false);
        if (this.marketChart.stdevWidth.ask)
          this.Highcharts.charts[this.fvChart].series[13].addPoint([time, this.marketChart.stdevWidth.ask], false);
        if (this.marketChart.stdevWidth.bid)
          this.Highcharts.charts[this.fvChart].series[14].addPoint([time, this.marketChart.stdevWidth.bid], false);
        if (this.marketChart.stdevWidth.fv && this.marketChart.stdevWidth.fvMean)
          this.Highcharts.charts[this.fvChart].series[15].addPoint([time, this.marketChart.stdevWidth.fvMean-this.marketChart.stdevWidth.fv, this.marketChart.stdevWidth.fvMean+this.marketChart.stdevWidth.fv], this.showStats, false, false);
        if (this.marketChart.stdevWidth.tops && this.marketChart.stdevWidth.topsMean)
          this.Highcharts.charts[this.fvChart].series[16].addPoint([time, this.marketChart.stdevWidth.topsMean-this.marketChart.stdevWidth.tops, this.marketChart.stdevWidth.topsMean+this.marketChart.stdevWidth.tops], this.showStats, false, false);
        if (this.marketChart.stdevWidth.ask && this.marketChart.stdevWidth.bid && this.marketChart.stdevWidth.askMean && this.marketChart.stdevWidth.bidMean)
          this.Highcharts.charts[this.fvChart].series[17].addPoint([time, this.marketChart.stdevWidth.bidMean-this.marketChart.stdevWidth.bid, this.marketChart.stdevWidth.askMean+this.marketChart.stdevWidth.ask], this.showStats, false, false);
      }
      this.Highcharts.charts[this.fvChart].yAxis[2].setExtremes(0, Math.max(this.marketChart.tradesBuySize*4,this.marketChart.tradesSellSize*4,this.Highcharts.charts[this.fvChart].yAxis[2].getExtremes().dataMax*4), false, true, { trigger: 'syncExtremes' });
      if (this.marketChart.tradesBuySize)
        this.Highcharts.charts[this.fvChart].series[18].addPoint([time, this.marketChart.tradesBuySize], false);
      if (this.marketChart.tradesSellSize)
        this.Highcharts.charts[this.fvChart].series[19].addPoint([time, this.marketChart.tradesSellSize], false);
      this.marketChart.tradesBuySize = 0;
      this.marketChart.tradesSellSize = 0;
      if (this.marketChart.ewma) {
        if (this.marketChart.ewma.ewmaQuote)
          this.Highcharts.charts[this.fvChart].series[6].addPoint([time, this.marketChart.ewma.ewmaQuote], false);
        if (this.marketChart.ewma.ewmaVeryLong)
          this.Highcharts.charts[this.fvChart].series[7].addPoint([time, this.marketChart.ewma.ewmaVeryLong], false);
        if (this.marketChart.ewma.ewmaLong)
          this.Highcharts.charts[this.fvChart].series[8].addPoint([time, this.marketChart.ewma.ewmaLong], false);
        if (this.marketChart.ewma.ewmaMedium)
          this.Highcharts.charts[this.fvChart].series[9].addPoint([time, this.marketChart.ewma.ewmaMedium], false);
        if (this.marketChart.ewma.ewmaShort)
          this.Highcharts.charts[this.fvChart].series[10].addPoint([time, this.marketChart.ewma.ewmaShort], false);
        if (this.marketChart.ewma.ewmaTrendDiff)
          this.Highcharts.charts[this.fvChart].series[20].addPoint([time, this.marketChart.ewma.ewmaTrendDiff], false);
      }
      this.Highcharts.charts[this.fvChart].series[0].addPoint([time, this.fairValue.price], this.showStats);
      if (this.quotingParameters.protectionEwmaWidthPing && (this.marketChart.ewma && this.marketChart.ewma.ewmaWidth))
        this.Highcharts.charts[this.fvChart].series[1].addPoint([time, this.fairValue.price-this.marketChart.ewma.ewmaWidth, this.fairValue.price+this.marketChart.ewma.ewmaWidth], this.showStats, false, false);
      else if (this.marketWidth)
        this.Highcharts.charts[this.fvChart].series[1].addPoint([time, this.fairValue.price-this.marketWidth, this.fairValue.price+this.marketWidth], this.showStats, false, false);
    }
    if (this.position.base.value || this.position.quote.value) {
      this.Highcharts.charts[this.quoteChart].yAxis[1].setExtremes(0, Math.max(this.position.quote.value,this.Highcharts.charts[this.quoteChart].yAxis[1].getExtremes().dataMax), false, true, { trigger: 'syncExtremes' });
      this.Highcharts.charts[this.baseChart].yAxis[1].setExtremes(0, Math.max(this.position.base.value,this.Highcharts.charts[this.baseChart].yAxis[1].getExtremes().dataMax), false, true, { trigger: 'syncExtremes' });
      this.Highcharts.charts[this.quoteChart].series[1].addPoint([time, (this.position.base.value-this.targetBasePosition.tbp)*this.position.quote.value/this.position.base.value], false);
      this.Highcharts.charts[this.baseChart].series[1].addPoint([time, this.targetBasePosition.tbp], false);
      this.Highcharts.charts[this.quoteChart].series[2].addPoint([time, Math.max(0, this.position.base.value-this.targetBasePosition.tbp-this.targetBasePosition.pDiv)*this.position.quote.value/this.position.base.value, Math.min(this.position.base.value, this.position.base.value-this.targetBasePosition.tbp+this.targetBasePosition.pDiv)*this.position.quote.value/this.position.base.value], this.showStats, false, false);
      this.Highcharts.charts[this.baseChart].series[2].addPoint([time, Math.max(0,this.targetBasePosition.tbp-this.targetBasePosition.pDiv), Math.min(this.position.base.value, this.targetBasePosition.tbp+this.targetBasePosition.pDiv)], this.showStats, false, false);
      this.Highcharts.charts[this.quoteChart].series[0].addPoint([time, this.position.quote.value], false);
      this.Highcharts.charts[this.quoteChart].series[3].addPoint([time, this.position.quote.amount], false);
      this.Highcharts.charts[this.quoteChart].series[4].addPoint([time, this.position.quote.held], this.showStats);
      this.Highcharts.charts[this.baseChart].series[0].addPoint([time, this.position.base.value], false);
      this.Highcharts.charts[this.baseChart].series[3].addPoint([time, this.position.base.amount], false);
      this.Highcharts.charts[this.baseChart].series[4].addPoint([time, this.position.base.held], this.showStats);
    }
  };

  private removeOldPoints = (time: number) => {
    this.Highcharts.charts.forEach(chart => { chart.series.forEach(serie => {
      while(serie.data.length && Math.abs(time - serie.data[0].x) > this.quotingParameters.profitHourInterval * 36e+5)
        serie.data[0].remove(false);
    })});
  };
};
