import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';
import {GridOptions, ColDef, RowNode} from "ag-grid/main";
import moment = require('moment');

import Models = require('../share/models');
import {SubscriberFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'market-trades',
  template: `<ag-grid-angular #marketList (click)="this.loadSubscriber()" class="ag-fresh ag-dark {{ subscribed ? \'ag-subscribed\' : \'ag-not-subscribed\' }} marketTrades" style="height: 349px;width: 100%;" rowHeight="21" [gridOptions]="gridOptions"></ag-grid-angular>`
})
export class MarketTradesComponent implements OnInit {

  private gridOptions: GridOptions = <GridOptions>{};

  @Input() product: Models.ProductState;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.enableSorting = true;
    this.gridOptions.overlayLoadingTemplate = `<span class="ag-overlay-no-rows-center">click to view data</span>`;
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty history</span>`;
  }

  private subscribed: boolean = false;
  public loadSubscriber = () => {
    if (this.subscribed) return;
    this.subscribed = true;
    this.gridOptions.rowData = [];

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.MarketTrade)
      .registerDisconnectedHandler(() => this.gridOptions.rowData.length = 0)
      .registerSubscriber(this.addRowData);
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      { width: 82, field: 'time', headerName: 'time', cellRenderer:(params) => {
        return (params.value) ? params.value.format('HH:mm:ss,SSS') : '';
      },
        comparator: (a: moment.Moment, b: moment.Moment) => a.diff(b),
        sort: 'desc', cellClass: (params) => {
          return 'fs11px '+(!params.data.recent ? "text-muted" : "");
      } },
      { width: 75, field: 'price', headerName: 'px', cellClass: (params) => {
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
      { width: 75, field: 'qB', headerName: 'qB', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 75, field: 'qA', headerName: 'qA', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 50, field: 'qAz', headerName: 'qAz', cellRendererFramework: BaseCurrencyCellComponent },
      { width: 50, field: 'mBz', headerName: 'mBz', cellRendererFramework: BaseCurrencyCellComponent },
      { width: 60, field: 'mB', headerName: 'mB', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 60, field: 'mA', headerName: 'mA', cellRendererFramework: QuoteCurrencyCellComponent },
      { width: 50, field: 'mAz', headerName: 'mAz', cellRendererFramework: BaseCurrencyCellComponent }
    ];
  }

  private addRowData = (trade: Models.MarketTrade) => {
    if (!this.gridOptions.api) return;
    if (trade != null)
      this.gridOptions.api.updateRowData({add:[{
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
        quoteSymbol: Models.Currency[trade.pair.quote],
        productFixed: this.product.fixed
      }]});

    this.gridOptions.api.forEachNode((node: RowNode) => {
      if (Math.abs(moment.utc().valueOf() - moment(node.data.time).valueOf()) > 3600000)
        this.gridOptions.api.updateRowData({remove:[node.data]});
      else if (Math.abs(moment.utc().valueOf() - moment(node.data.time).valueOf()) > 7000)
        node.setData(Object.assign(node.data, {recent: false}));
    });
  }
}
