/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>
/// <amd-dependency path='ui.bootstrap'/>

import {NgModule, Component} from '@angular/core';
import moment = require('moment');

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

class DisplayTrade {
  tradeId: string;
  time: moment.Moment;
  price: number;
  quantity: number;
  side: string;
  value: number;
  liquidity: string;
  Ktime: moment.Moment;
  Kqty: number;
  Kprice: number;
  Kvalue: number;
  Kdiff: number;
  sortTime: moment.Moment;

  constructor(
    public trade: Models.Trade
  ) {
    this.tradeId = trade.tradeId;
    this.side = (trade.Kqty >= trade.quantity) ? 'K' : (trade.side === Models.Side.Ask ? "Sell" : "Buy");
    this.time = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));
    this.price = trade.price;
    this.quantity = trade.quantity;
    this.value = trade.value;
    if (trade.Ktime && <any>trade.Ktime=='Invalid date') trade.Ktime = null;
    this.Ktime = trade.Ktime ? (moment.isMoment(trade.Ktime) ? trade.Ktime : moment(trade.Ktime)) : null;
    this.Kqty = trade.Kqty ? trade.Kqty : null;
    this.Kprice = trade.Kprice ? trade.Kprice : null;
    this.Kvalue = trade.Kvalue ? trade.Kvalue : null;
    this.Kdiff = (trade.Kdiff && trade.Kdiff!=0) ? trade.Kdiff : null;
    this.sortTime = this.Ktime ? this.Ktime : this.time;

    if (trade.liquidity === 0 || trade.liquidity === 1) {
      this.liquidity = Models.Liquidity[trade.liquidity].charAt(0);
    } else {
      this.liquidity = "?";
    }
  }
}

@Component({
  selector: 'tradeList',
  template: `<div>
      <div ui-grid="gridOptions" class="table table-striped table-hover table-condensed" style="height: 180px" ></div>
    </div>`
})
export class TradesComponent {

  public trade_statuses : DisplayTrade[];
  public exch : Models.Exchange;
  public pair : Models.CurrencyPair;
  public gridOptions : any;
  public gridApi : any;
  public audio: boolean;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: SubscriberFactory,
    uiGridConstants: any
  ) {
    this.trade_statuses = [];
    this.gridOptions = {
      data: 'trade_statuses',
      treeRowHeaderAlwaysVisible: false,
      primaryKey: 'tradeId',
      groupsCollapsedByDefault: true,
      enableColumnResize: true,
      sortInfo: {fields: ['sortTime'], directions: ['desc']},
      rowHeight: 20,
      headerRowHeight: 20,
      columnDefs: [
        {field:'sortTime', visible: false,
          sortingAlgorithm: (a: moment.Moment, b: moment.Moment) => a.diff(b),
          sort: { direction: uiGridConstants.DESC, priority: 1} },
        {width: 121, field:'time', displayName:'t', cellFilter: 'momentFullDate', cellClass: 'fs11px' },
        {width: 121, field:'Ktime', visible:false, displayName:'timePong', cellFilter: 'momentFullDate', cellClass: 'fs11px' },
        {width: 42, field:'side', displayName:'side', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (grid.getCellValue(row, col) === 'Buy') {
            return 'buy';
          }
          else if (grid.getCellValue(row, col) === 'Sell') {
            return "sell";
          }
          else if (grid.getCellValue(row, col) === 'K') {
            return "kira";
          }
          else {
            return "unknown";
          }
        }},
        {width: 65, field:'price', displayName:'px', cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return (row.entity.price > row.entity.Kprice) ? "sell" : "buy"; else return row.entity.side === 'Sell' ? "sell" : "buy";
        }},
        {width: 65, field:'quantity', displayName:'qty', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return (row.entity.price > row.entity.Kprice) ? "sell" : "buy"; else return row.entity.side === 'Sell' ? "sell" : "buy";
        }},
        {width: 69, field:'value', displayName:'val', cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return (row.entity.price > row.entity.Kprice) ? "sell" : "buy"; else return row.entity.side === 'Sell' ? "sell" : "buy";
        }},
        {width: 69, field:'Kvalue', displayName:'valPong', visible:false, cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return (row.entity.price < row.entity.Kprice) ? "sell" : "buy"; else return row.entity.Kqty ? ((row.entity.price < row.entity.Kprice) ? "sell" : "buy") : "";
        }},
        {width: 65, field:'Kqty', displayName:'qtyPong', visible:false, cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return (row.entity.price < row.entity.Kprice) ? "sell" : "buy"; else return row.entity.Kqty ? ((row.entity.price < row.entity.Kprice) ? "sell" : "buy") : "";
        }},
        {width: 65, field:'Kprice', displayName:'pxPong', visible:false, cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return (row.entity.price < row.entity.Kprice) ? "sell" : "buy"; else return row.entity.Kqty ? ((row.entity.price < row.entity.Kprice) ? "sell" : "buy") : "";
        }},
        {width: 65, field:'Kdiff', displayName:'Kdiff', visible:false, cellFilter: 'currency:"$":3', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
          if (row.entity.side === 'K') return "kira"; else return "";
        }}
      ],
      onRegisterApi: function(gridApi) {
        this.gridApi = gridApi;
      }
    };

    var addTrade = t => {
      if (t.Kqty<0) {
        for(var i = 0;i<this.trade_statuses.length;i++) {
          if (this.trade_statuses[i].tradeId==t.tradeId) {
            this.trade_statuses.splice(i, 1);
            break;
          }
        }
      } else {
        var exists = false;
        for(var i = 0;i<this.trade_statuses.length;i++) {
          if (this.trade_statuses[i].tradeId==t.tradeId) {
            exists = true;
            this.trade_statuses[i].time = (moment.isMoment(t.time) ? t.time : moment(t.time));
            var merged = (this.trade_statuses[i].quantity != t.quantity);
            this.trade_statuses[i].quantity = t.quantity;
            this.trade_statuses[i].value = t.value;
            this.trade_statuses[i].Ktime = (moment.isMoment(t.Ktime) ? t.Ktime : moment(t.Ktime));
            this.trade_statuses[i].Kqty = t.Kqty;
            this.trade_statuses[i].Kprice = t.Kprice;
            this.trade_statuses[i].Kvalue = t.Kvalue;
            this.trade_statuses[i].Kdiff = t.Kdiff?t.Kdiff:null;
            this.trade_statuses[i].sortTime = this.trade_statuses[i].Ktime ? this.trade_statuses[i].Ktime : this.trade_statuses[i].time;
            if (this.trade_statuses[i].Kqty >= this.trade_statuses[i].quantity)
              this.trade_statuses[i].side = 'K';
            if (this.gridApi && uiGridConstants)
              this.gridApi.grid.notifyDataChange(uiGridConstants.dataChange.ALL);
            if (t.loadedFromDB === false && this.audio) {
              var audio = new Audio('/audio/'+(merged?'boom':'erang')+'.mp3');
              audio.volume = 0.5;
              audio.play();
            }
            break;
          }
        }
        if (!exists) {
          this.trade_statuses.push(new DisplayTrade(t));
          if (t.loadedFromDB === false && this.audio) {
            var audio = new Audio('/audio/boom.mp3');
            audio.volume = 0.5;
            audio.play();
          }
        }
      }
    };

    var updateQP = qp => {
      this.audio = qp.audio;
      var modGrid = (qp.mode === Models.QuotingMode.Boomerang || qp.mode === Models.QuotingMode.AK47);
      ['Kqty','Kprice','Kvalue','Kdiff','Ktime'].map(r => {
        this.gridOptions.columnDefs[this.gridOptions.columnDefs.map(e => { return e.field; }).indexOf(r)].visible = modGrid;
      });
      [['time','timePing'],['price','pxPing'],['quantity','qtyPing'],['value','valPing']].map(r => {
        this.gridOptions.columnDefs[this.gridOptions.columnDefs.map(e => { return e.field; }).indexOf(r[0])].displayName = modGrid ? r[1] : r[1].replace('Ping','');
      });
      if (this.gridApi) this.gridApi.grid.refresh();
    };

    var subscriberTrades = subscriberFactory.getSubscriber($scope, Messaging.Topics.Trades)
      .registerConnectHandler(() => this.trade_statuses.length = 0)
      .registerDisconnectedHandler(() => this.trade_statuses.length = 0)
      .registerSubscriber(addTrade, trades => trades.forEach(addTrade));

    var subscriberQPChange = subscriberFactory.getSubscriber($scope, Messaging.Topics.QuotingParametersChange)
      .registerDisconnectedHandler(() => this.trade_statuses.length = 0)
      .registerSubscriber(updateQP, qp => qp.forEach(updateQP));

    $scope.$on('$destroy', () => {
      subscriberTrades.disconnect();
      subscriberQPChange.disconnect();
    });
  }
}
