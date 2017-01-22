/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';
import {GridOptions, ColDef, RowNode} from "ag-grid/main";
import moment = require('moment');

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'trade-list',
  template: `<ag-grid-ng2 #tradeList class="ag-fresh ag-dark" style="height: 220px;width: 99.99%;" rowHeight="21" [gridOptions]="gridOptions"></ag-grid-ng2>`
})
export class TradesComponent implements OnInit, OnDestroy {

  private gridOptions: GridOptions = <GridOptions>{};

  public exch: Models.Exchange;
  public pair: Models.CurrencyPair;
  public audio: boolean;

  private subscriberTrades: Messaging.ISubscribe<Models.Trade>;
  private subscriberQPChange: Messaging.ISubscribe<Models.QuotingParameters>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.enableSorting = true;
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty history of trades</span>`;

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

  private createColumnDefs = (): ColDef[] => {
    return [
      {width: 121, field:'time', headerName:'t', cellRenderer:(params) => {
          return (params.value) ? Models.toUtcFormattedTime(params.value) : '';
        }, cellClass: 'fs11px', comparator: (aValue: moment.Moment, bValue: moment.Moment, aNode: RowNode, bNode: RowNode) => {
          return (aNode.data.Ktime||aNode.data.time).diff(bNode.data.Ktime||bNode.data.time);
      }, sort: 'desc'},
      {width: 121, field:'Ktime', hide:true, headerName:'timePong', cellRenderer:(params) => {
          return (params.value && params.value!='Invalid date') ? Models.toUtcFormattedTime(params.value) : '';
        }, cellClass: 'fs11px' },
      {width: 40, field:'side', headerName:'side', cellClass: (params) => {
        if (params.value === 'Buy') return 'buy';
        else if (params.value === 'Sell') return "sell";
        else if (params.value === 'K') return "kira";
        else return "unknown";
      }},
      {width: 65, field:'price', headerName:'px', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'quantity', headerName:'qty', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 69, field:'value', headerName:'val', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 69, field:'Kvalue', headerName:'valPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'Kqty', headerName:'qtyPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 65, field:'Kprice', headerName:'pxPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'Kdiff', headerName:'Kdiff', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return "kira"; else return "";
      }, cellRendererFramework: QuoteCurrencyCellComponent}
    ];
  }

  private addRowData = (t: Models.Trade) => {
    if (t.Kqty<0) {
      this.gridOptions.api.forEachNode((node: RowNode) => {
        if (node.data.tradeId==t.tradeId)
          this.gridOptions.api.removeItems([node]);
      });
    } else {
      let exists: boolean = false;
      this.gridOptions.api.forEachNode((node: RowNode) => {
        if (!exists && node.data.tradeId==t.tradeId) {
          exists = true;
          let merged = (node.data.quantity != t.quantity);
          if (t.Ktime && <any>t.Ktime=='Invalid date') t.Ktime = null;
          node.setData(Object.assign(node.data, {
            time: (moment.isMoment(t.time) ? t.time : moment(t.time)),
            quantity: t.quantity,
            value: t.value,
            Ktime: t.Ktime?(moment.isMoment(t.Ktime) ? t.Ktime : moment(t.Ktime)):null,
            Kqty: t.Kqty ? t.Kqty : null,
            Kprice: t.Kprice ? t.Kprice : null,
            Kvalue: t.Kvalue ? t.Kvalue : null,
            Kdiff: t.Kdiff?t.Kdiff:null,
            side: t.Kqty >= t.quantity ? 'K' : (t.side === Models.Side.Ask ? "Sell" : "Buy")
          }));
          if (t.loadedFromDB === false) {
            this.gridOptions.api.setSortModel([{colId: 'time', sort: 'desc'}]);
            this.gridOptions.api.refreshView();
            if (this.audio) {
              var audio = new Audio('/audio/'+(merged?'boom':'erang')+'.mp3');
              audio.volume = 0.5;
              audio.play();
            }
          }
        }
      });
      if (!exists) {
        if (t.Ktime && <any>t.Ktime=='Invalid date') t.Ktime = null;
        this.gridOptions.api.addItems([{
          tradeId: t.tradeId,
          time: (moment.isMoment(t.time) ? t.time : moment(t.time)),
          price: t.price,
          quantity: t.quantity,
          side: t.Kqty >= t.quantity ? 'K' : (t.side === Models.Side.Ask ? "Sell" : "Buy"),
          value: t.value,
          liquidity: (t.liquidity === 0 || t.liquidity === 1) ? Models.Liquidity[t.liquidity].charAt(0) : '?',
          Ktime: t.Ktime ? (moment.isMoment(t.Ktime) ? t.Ktime : moment(t.Ktime)) : null,
          Kqty: t.Kqty ? t.Kqty : null,
          Kprice: t.Kprice ? t.Kprice : null,
          Kvalue: t.Kvalue ? t.Kvalue : null,
          Kdiff: t.Kdiff && t.Kdiff!=0 ? t.Kdiff : null,
          quoteSymbol: Models.Currency[t.pair.quote]
        }]);
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
    var isK = (qp.mode === Models.QuotingMode.Boomerang || qp.mode === Models.QuotingMode.AK47);
    this.gridOptions.columnDefs.map((r: ColDef) => {
      ['Kqty','Kprice','Kvalue','Kdiff','Ktime',['time','timePing'],['price','pxPing'],['quantity','qtyPing'],['value','valPing']].map(t => {
        if (t[0]==r.field) r.headerName = isK ? t[1] : t[1].replace('Ping','');
        if (r.field[0]=='K') r.hide = !isK;
      });
      return r;
    });
    this.gridOptions.api.setColumnDefs(this.gridOptions.columnDefs);
  }
}
