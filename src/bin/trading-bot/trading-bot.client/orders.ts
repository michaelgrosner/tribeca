import {Component, Input} from '@angular/core';
import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import * as Models from 'lib/models';
import * as Socket from 'lib/socket';
import * as Shared from 'lib/shared';

@Component({
  selector: 'orders',
  template: `<ag-grid-angular #orderList
    class="ag-theme-fresh ag-theme-dark"
    style="height: 131px;width: 99.80%;"
    rowHeight="21"
    (cellClicked)="onCellClicked($event)"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class OrdersComponent {

  private fireCxl: Socket.IFire<Models.OrderCancelRequestFromUI> = new Socket.Fire(Models.Topics.CancelOrder);

  @Input() product: Models.ProductAdvertisement;

  @Input() set orderList(o: Models.Order[]) {
    this.addRowData(o);
  };

  private grid: GridOptions = <GridOptions>{
    suppressNoRowsOverlay: true,
    defaultColDef: { sortable: true, resizable: true },
    columnDefs: [{
      width: 30,
      field: "cancel",
      headerName: 'cxl',
      suppressSizeToFit: true,
      cellRenderer: (params) => {
        return `<button type="button" class="btn btn-danger btn-xs">
          <span data-action-type="remove"'
            style="font-size: 16px;font-weight: bold;padding: 0px;line-height: 12px;"
          >&times;</span>
        </button>`;
      }
    }, {
      width: 82,
      field: 'time',
      headerName: 'time',
      suppressSizeToFit: true,
      cellClass: 'fs11px',
      cellRenderer:(params) => {
        var d = new Date(params.value||0);
        return (d.getHours()+'')
          .padStart(2, "0")+':'+(d.getMinutes()+'')
          .padStart(2, "0")+':'+(d.getSeconds()+'')
          .padStart(2, "0")+','+(d.getMilliseconds()+'')
          .padStart(3, "0");
      }
    }, {
      width: 40,
      field: 'side',
      headerName: 'side',
      suppressSizeToFit: true,
      cellClass: (params) => {
        if (params.value === 'Bid') return 'buy';
        else if (params.value === 'Ask') return "sell";
      },
      cellRenderer:(params) => { return (params.data.pong ? '&lrhar;' : '&rhard;') + params.value; }
    }, {
      width: 74,
      field: 'price',
      headerName: 'price',
      sort: 'desc',
      cellClass: (params) => { return (params.data.side === 'Ask') ? "sell" : "buy"; },
      cellRendererFramework: Shared.QuoteCurrencyCellComponent
    }, {
      width: 85,
      field: 'qty',
      headerName: 'qty',
      suppressSizeToFit: true,
      cellClass: (params) => { return (params.data.side === 'Ask') ? "sell" : "buy"; },
      cellRendererFramework: Shared.BaseCurrencyCellComponent
    }, {
      width: 74,
      field: 'value',
      headerName: 'value',
      cellClass: (params) => { return (params.data.side === 'Ask') ? "sell" : "buy"; }
    }, {
      width: 45,
      field: 'type',
      headerName: 'type',
      suppressSizeToFit: true
    }, {
      width: 40,
      field: 'tif',
      headerName: 'tif'
    }, {
      width: 45,
      field: 'lat',
      headerName: 'lat'
    }, {
      width: 110,
      field: 'exchangeId',
      headerName: 'openOrderId',
      suppressSizeToFit: true,
      cellRenderer:(params) => { return (params.value) ? params.value.toString().split('-')[0] : ''; }
    }]
  };

  private onCellClicked = ($event) => {
    if ($event.event.target.getAttribute("data-action-type") != 'remove') return;
    this.fireCxl.fire(new Models.OrderCancelRequestFromUI($event.data.orderId, $event.data.exchange));
    // this.grid.api.applyTransaction({remove:[$event.data]});
  };

  private addRowData = (o) => {
    if (!this.grid.api) return;
    if (!o || (typeof o.length == 'number' && !o.length)) {
      this.grid.api.setRowData([]);
      return;
    } else if (typeof o.length == 'number' && typeof o[0] == 'object') {
      this.grid.api.setRowData([]);
      return o.forEach(x => setTimeout(this.addRowData(x), 0));
    }

    let exists: boolean = false;
    let isClosed: boolean = (o.status == Models.OrderStatus.Terminated);
    this.grid.api.forEachNode((node: RowNode) => {
      if (!exists && node.data.orderId==o.orderId) {
        exists = true;
        if (isClosed) this.grid.api.applyTransaction({remove:[node.data]});
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
    setTimeout(()=>{try{this.grid.api.redrawRows();}catch(e){}},0);
    if (!exists && !isClosed)
      this.grid.api.applyTransaction({add:[{
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

    this.grid.api.sizeColumnsToFit();
  };
};
