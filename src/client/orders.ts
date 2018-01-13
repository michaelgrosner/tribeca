import {Component, Inject, Input, OnInit} from '@angular/core';
import {GridOptions, ColDef, RowNode} from 'ag-grid/main';

import * as Models from './models';
import * as Subscribe from './subscribe';
import {FireFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'order-list',
  template: `<ag-grid-angular #orderList class="ag-fresh ag-dark" style="height: 135px;width: 99.99%;" rowHeight="21" [gridOptions]="gridOptions" (cellClicked)="onCellClicked($event)"></ag-grid-angular>`
})
export class OrdersComponent implements OnInit {

  private gridOptions: GridOptions = <GridOptions>{};

  private fireCxl: Subscribe.IFire<any>;

  @Input() product: Models.ProductState;

  @Input() set online(online: boolean) {
    if (online) return;
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
    this.gridOptions.enableSorting = true;
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.suppressNoRowsOverlay = true;
    this.gridOptions.enableColResize = true;

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
      { width: 60, suppressSizeToFit: true, field: 'qty', headerName: 'qty', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      { width: 74, field: 'value', headerName: 'value', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      { width: 45, suppressSizeToFit: true, field: 'type', headerName: 'type' },
      { width: 40, field: 'tif', headerName: 'tif' },
      { width: 45, field: 'lat', headerName: 'lat'},
      { width: 90, suppressSizeToFit: true, field: 'orderId', headerName: 'openOrderId', cellRenderer:(params) => {
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
    if (!this.gridOptions.api || this.product.advert.pair == null) return;
    if (!o || (typeof o.length == 'number' && !o.length)) {
      this.gridOptions.api.setRowData([]);
      return;
    } else if (typeof o.length == 'number' && typeof o[0] == 'object') {
      this.gridOptions.api.setRowData([]);
      return o.forEach(x => setTimeout(this.addRowData(x), 0));
    }

    let exists: boolean = false;
    let isClosed: boolean = (o.orderStatus == Models.OrderStatus.Cancelled
      || o.orderStatus == Models.OrderStatus.Complete);
    this.gridOptions.api.forEachNode((node: RowNode) => {
      if (!exists && node.data.orderId==o.orderId) {
        exists = true;
        if (isClosed) this.gridOptions.api.updateRowData({remove:[node.data]});
        else {
          node.setData(Object.assign(node.data, {
            time: o.time,
            price: o.price,
            value: Math.round(o.price * o.quantity * 100) / 100,
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
        side: Models.Side[o.side],
        price: o.price,
        value: Math.round(o.price * o.quantity * 100) / 100,
        exchange: o.exchange,
        type: Models.OrderType[o.type],
        tif: Models.TimeInForce[o.timeInForce],
        lat: o.latency+'ms',
        qty: o.quantity,
        pong: o.isPong,
        time: o.time,
        quoteSymbol: this.product.advert.pair.quote,
        productFixed: this.product.fixed
      }]});
    this.gridOptions.api.sizeColumnsToFit();
  }
}
