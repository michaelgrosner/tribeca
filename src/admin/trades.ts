/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';
import {GridOptions} from "ag-grid/main";
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
  selector: 'trade-list',
  template: `<ag-grid-ng2 #tradeList class="ag-fresh ag-dark" style="height: 187px;width: 100%;" rowHeight="21" [gridOptions]="gridOptions"></ag-grid-ng2>`
})
export class TradesComponent implements OnInit, OnDestroy {

  private gridOptions: GridOptions = <GridOptions>{};

  public exch : Models.Exchange;
  public pair : Models.CurrencyPair;
  public audio: boolean;

  private subscriberTrades: Messaging.ISubscribe<Models.Trade>;
  private subscriberQPChange: Messaging.ISubscribe<Models.QuotingParameters>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty</span>`;

    this.subscriberQPChange = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.QuotingParametersChange)
      .registerSubscriber(this.updateQP, qp => qp.forEach(this.updateQP));

    this.subscriberTrades = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.Trades)
      .registerDisconnectedHandler(() => this.gridOptions.rowData.length = 0)
      .registerSubscriber(this.addRowData, trades => trades.forEach(this.addRowData));
  }

  ngOnDestroy() {
    this.subscriberQPChange.disconnect();
    this.subscriberTrades.disconnect();
  }

  private createColumnDefs = () => {
    return [
      {field:'sortTime', hide: true,
        comparator: (a: moment.Moment, b: moment.Moment) => a.diff(b),
        sort: 'desc' },
      {width: 121, field:'time', headerName:'t', cellRenderer:(params) => {
          return (params.value) ? Models.toUtcFormattedTime(params.value) : '';
        }, cellClass: 'fs11px' },
      {width: 121, field:'Ktime', hide:true, headerName:'timePong', cellRenderer:(params) => {
          return (params.value) ? Models.toUtcFormattedTime(params.value) : '';
        }, cellClass: 'fs11px' },
      {width: 42, field:'side', headerName:'side', cellClass: (params) => {
        if (params.value === 'Buy') return 'buy';
        else if (params.value === 'Sell') return "sell";
        else if (params.value === 'K') return "kira";
        else return "unknown";
      }},
      {width: 65, field:'price', headerName:'px', cellRenderer: (params) => {
          return params.value ? '$'+params.value : '';
      }, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }},
      {width: 65, field:'quantity', headerName:'qty', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }},
      {width: 69, field:'value', headerName:'val', cellRenderer: (params) => {
          return params.value ? '$'+params.value : '';
      }, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }},
      {width: 69, field:'Kvalue', headerName:'valPong', hide:true, cellRenderer: (params) => {
          return params.value ? '$'+params.value : '';
      }, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }},
      {width: 65, field:'Kqty', headerName:'qtyPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }},
      {width: 65, field:'Kprice', headerName:'pxPong', hide:true, cellRenderer: (params) => {
          return params.value ? '$'+params.value : '';
      }, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }},
      {width: 65, field:'Kdiff', headerName:'Kdiff', hide:true, cellRenderer: (params) => {
          return params.value ? '$'+params.value : '';
      }, cellClass: (params) => {
        if (params.data.side === 'K') return "kira"; else return "";
      }}
    ];
  }

  private addRowData = (t: Models.Trade) => {
    if (t.Kqty<0) {
      this.gridOptions.api.forEachNode((node) => {
        if (node.data.tradeId==t.tradeId)
          this.gridOptions.api.removeItems([node]);
      });
    } else {
      var exists = false;
      this.gridOptions.api.forEachNode((node) => {
        if (node.data.tradeId==t.tradeId) {
          exists = true;
          node.data.time = (moment.isMoment(t.time) ? t.time : moment(t.time));
          var merged = (node.data.quantity != t.quantity);
          node.data.quantity = t.quantity;
          node.data.value = t.value;
          node.data.Ktime = (moment.isMoment(t.Ktime) ? t.Ktime : moment(t.Ktime));
          node.data.Kqty = t.Kqty;
          node.data.Kprice = t.Kprice;
          node.data.Kvalue = t.Kvalue;
          node.data.Kdiff = t.Kdiff?t.Kdiff:null;
          node.data.sortTime = node.data.Ktime ? node.data.Ktime : node.data.time;
          if (node.data.Kqty >= node.data.quantity)
            node.data.side = 'K';
          this.gridOptions.api.refreshView();
          if (t.loadedFromDB === false && this.audio) {
            var audio = new Audio('/audio/'+(merged?'boom':'erang')+'.mp3');
            audio.volume = 0.5;
            audio.play();
          }
        }
      });
      if (!exists) {
        this.gridOptions.api.addItems([new DisplayTrade(t)]);
        if (t.loadedFromDB === false && this.audio) {
          var audio = new Audio('/audio/boom.mp3');
          audio.volume = 0.5;
          audio.play();
        }
      }
    }
  }

  private updateQP = (qp: Models.QuotingParameters) => {
    this.audio = qp.audio;
    var modGrid = (qp.mode === Models.QuotingMode.Boomerang || qp.mode === Models.QuotingMode.AK47);
    ['Kqty','Kprice','Kvalue','Kdiff','Ktime'].map(r => {
      this.gridOptions.columnApi.setColumnVisible(r, modGrid);
    });
    this.gridOptions.columnDefs.map((r:any) => {
      [['time','timePing'],['price','pxPing'],['quantity','qtyPing'],['value','valPing']].map(t => {
        if (r.field == t[0]) r.headerName = modGrid ? t[1] : t[1].replace('Ping','');
      });
      return r;
    });
    this.gridOptions.api.refreshHeader();
  }
}
