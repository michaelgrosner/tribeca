import {Component, Inject, Input, OnInit} from '@angular/core';
import {Module, ClientSideRowModelModule, GridOptions, ColDef, RowNode} from '@ag-grid-community/all-modules';

import * as Models from './models';
import * as Subscribe from './subscribe';
import {FireFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'order-list',
  template: `<ag-grid-angular #orderList class="ag-theme-fresh ag-theme-dark" style="height: 135px;width: 99.80%;" rowHeight="21" [gridOptions]="gridOptions" [modules]="modules" (cellClicked)="onCellClicked($event)"></ag-grid-angular>`
})
export class OrdersComponent implements OnInit {

  private modules: Module[] = [ClientSideRowModelModule];

  private gridOptions: GridOptions = <GridOptions>{};

  private fireCxl: Subscribe.IFire<any>;

  @Input() product: Models.ProductAdvertisement;

  @Input() set agree(agree: boolean) {
    if (agree) return;
    if (!this.gridOptions.api) return;
    this.gridOptions.api.setRowData([]);
    setTimeout(()=>{try{this.gridOptions.api.redrawRows();}catch(e){}},0);
  }

  @Input() set setOrderList(o) {
    this.addRowData(o);
  }

  constructor(
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.defaultColDef = { sortable: true, resizable: true };
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.suppressNoRowsOverlay = true;

    this.fireCxl = this.fireFactory
      .getFire(Models.Topics.CancelOrder);
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      { width: 30, suppressSizeToFit: true, field: "cancel", headerName: 'cxl', cellRenderer: (params) => {
        return '<button type="button" class="btn btn-danger btn-xs"><span data-action-type="remove" style="font-size: 16px;font-weight: bold;padding: 0px;line-height: 12px;">&times;</span></button>';
      } },
      { width: 82, suppressSizeToFit: true, field: 'time', headerName: 'time', cellRenderer:(params) => {
        var d = new Date(params.value||0);
        return (d.getHours()+'').padStart(2, "0")+':'+(d.getMinutes()+'').padStart(2, "0")+':'+(d.getSeconds()+'').padStart(2, "0")+','+(d.getMilliseconds()+'').padStart(3, "0");
      },
        cellClass: 'fs11px'
      },
      { width: 40, suppressSizeToFit: true, field: 'side', headerName: 'side', cellRenderer:(params) => {
        return (params.data.pong ? '¯' : '_') + params.value;
      }, cellClass: (params) => {
        if (params.value === 'Bid') return 'buy';
        else if (params.value === 'Ask') return "sell";
      }},
      { width: 74, field: 'price', headerName: 'px',
        sort: 'desc',  cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      { width: 85, suppressSizeToFit: true, field: 'qty', headerName: 'qty', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      { width: 74, field: 'value', headerName: 'value', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }},
      { width: 45, suppressSizeToFit: true, field: 'type', headerName: 'type' },
      { width: 40, field: 'tif', headerName: 'tif' },
      { width: 45, field: 'lat', headerName: 'lat'},
      { width: 110, suppressSizeToFit: true, field: 'exchangeId', headerName: 'openOrderId', cellRenderer:(params) => {
        return (params.value) ? params.value.toString().split('-')[0] : '';
      }}
    ];
  }

  public onCellClicked = ($event) => {
    if ($event.event.target.getAttribute("data-action-type")!='remove') return;
    this.fireCxl.fire({
      orderId: $event.data.orderId,
      exchange: $event.data.exchange
    });
    this.gridOptions.api.updateRowData({remove:[$event.data]});
  }

  private addRowData = (o) => {
    if (!this.gridOptions.api || this.product.base == null) return;
    if (!o || (typeof o.length == 'number' && !o.length)) {
      this.gridOptions.api.setRowData([]);
      return;
    } else if (typeof o.length == 'number' && typeof o[0] == 'object') {
      this.gridOptions.api.setRowData([]);
      return o.forEach(x => setTimeout(this.addRowData(x), 0));
    }

    let exists: boolean = false;
    let isClosed: boolean = (o.status == Models.OrderStatus.Terminated);
    this.gridOptions.api.forEachNode((node: RowNode) => {
      if (!exists && node.data.orderId==o.orderId) {
        exists = true;
        if (isClosed) this.gridOptions.api.updateRowData({remove:[node.data]});
        else {
          node.setData(Object.assign(node.data, {
            time: o.time,
            price: o.price,
            value: this.product.margin == 0
                     ? (Math.round(o.quantity * o.price * 100) / 100) + " " + this.product.quote
                     : (this.product.margin == 1
                         ? (Math.round((o.quantity / o.price) * 1e+8) / 1e+8) + " " + this.product.base
                         : (Math.round((o.quantity * o.price) * 1e+8) / 1e+8) + " " + this.product.base
                     ),
            tif: Models.TimeInForce[o.timeInForce],
            lat: o.latency+'ms',
            qty: o.quantity
          }));
        }
      }
    });
    setTimeout(()=>{try{this.gridOptions.api.redrawRows();}catch(e){}},0);
    if (!exists && !isClosed)
      this.gridOptions.api.updateRowData({add:[{
        orderId: o.orderId,
        exchangeId: o.exchangeId,
        side: Models.Side[o.side],
        price: o.price,
        value: this.product.margin == 0
                 ? (Math.round(o.quantity * o.price * 100) / 100) + " " + this.product.quote
                 : (this.product.margin == 1
                     ? (Math.round((o.quantity / o.price) * 1e+8) / 1e+8) + " " + this.product.base
                     : (Math.round((o.quantity * o.price) * 1e+8) / 1e+8) + " " + this.product.base
                 ),
        exchange: o.exchange,
        type: Models.OrderType[o.type],
        tif: Models.TimeInForce[o.timeInForce],
        lat: o.latency+'ms',
        qty: o.quantity,
        pong: o.isPong,
        time: o.time,
        quoteSymbol: this.product.quote,
        productFixedPrice: this.product.tickPrice,
        productFixedSize: this.product.tickSize
      }]});

    this.gridOptions.api.sizeColumnsToFit();
  }
}
