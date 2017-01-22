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
  selector: 'market-trades',
  template: `<ag-grid-ng2 #marketList class="ag-fresh ag-dark marketTrades" style="height: 406px;width: 100%;" rowHeight="21" [gridOptions]="gridOptions"></ag-grid-ng2>`
})
export class MarketTradesComponent implements OnInit, OnDestroy {

  private gridOptions: GridOptions = <GridOptions>{};

  private subscriberMarketTrade: Messaging.ISubscribe<Models.MarketTrade>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.enableSorting = true;
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty</span>`;

    this.subscriberMarketTrade = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.MarketTrade)
      .registerDisconnectedHandler(() => this.gridOptions.rowData.length = 0)
      .registerSubscriber(this.addRowData, x => x.forEach(this.addRowData));
  }

  ngOnDestroy() {
    this.subscriberMarketTrade.disconnect();
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      { width: 82, field: 'time', headerName: 'time', cellRenderer:(params) => {
        return (params.value) ? Models.toShortTimeString(params.value) : '';
      },
        comparator: (a: moment.Moment, b: moment.Moment) => a.diff(b),
        sort: 'desc', cellClass: (params) => {
          return 'fs11px '+(!params.data.recent ? "text-muted" : "");
      } },
      { width: 60, field: 'price', headerName: 'px', cellClass: (params) => {
          return (params.data.make_side === 'Ask') ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      { width: 50, field: 'size', headerName: 'sz', cellClass: (params) => {
          return (params.data.make_side === 'Ask') ? "sell" : "buy";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      { width: 40, field: 'make_side', headerName: 'ms' , cellClass: (params) => {
        if (params.value === 'Bid') return 'buy';
        else if (params.value === 'Ask') return "sell";
      }},
      { width: 50, field: 'qBz', headerName: 'qBz', cellRendererFramework: BaseCurrencyCellComponent },
      { width: 60, field: 'qB', headerName: 'qB', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 60, field: 'qA', headerName: 'qA', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 50, field: 'qAz', headerName: 'qAz', cellRendererFramework: BaseCurrencyCellComponent },
      { width: 50, field: 'mBz', headerName: 'mBz', cellRendererFramework: BaseCurrencyCellComponent },
      { width: 60, field: 'mB', headerName: 'mB', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 60, field: 'mA', headerName: 'mA', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 50, field: 'mAz', headerName: 'mAz', cellRendererFramework: BaseCurrencyCellComponent }
    ];
  }

  private addRowData = (trade: Models.MarketTrade) => {
    if (trade != null)
      this.gridOptions.api.addItems([{
        price: trade.price,
        size: trade.size,
        time: (moment.isMoment(trade.time) ? trade.time : moment(trade.time)),
        recent: true,
        qA: (trade.quote != null && trade.quote.ask !== null ? trade.quote.ask.price : null),
        qB: (trade.quote != null && trade.quote.bid !== null ? trade.quote.bid.price : null),
        qAz: (trade.quote != null && trade.quote.ask !== null ? trade.quote.ask.size : null),
        qBz: (trade.quote != null && trade.quote.bid !== null ? trade.quote.bid.size : null),
        mA: (trade.ask != null ? trade.ask.price : null),
        mB: (trade.bid != null ? trade.bid.price : null),
        mAz: (trade.ask != null ? trade.ask.size : null),
        mBz: (trade.bid != null ? trade.bid.size : null),
        make_side: Models.Side[trade.make_side],
        quoteSymbol: Models.Currency[trade.pair.quote]
      }]);

    this.gridOptions.api.forEachNode((node: RowNode) => {
      if (Math.abs(moment.utc().valueOf() - moment(node.data.time).valueOf()) > 3600000)
        this.gridOptions.api.removeItems([node]);
      else if (Math.abs(moment.utc().valueOf() - moment(node.data.time).valueOf()) > 7000)
        node.setData(Object.assign(node.data, {recent: false}));
    });
  }
}
