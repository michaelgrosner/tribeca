import {Component, Input} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import {Socket, Shared, Models} from 'lib/K';

@Component({
  selector: 'orders',
  template: `<ag-grid-angular
    class="ag-theme-fresh ag-theme-dark"
    style="height: 131px;width: 99.80%;"
    (gridReady)="onGridReady()"
    (window:resize)="onGridReady()"
    (cellClicked)="onCellClicked($event)"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class OrdersComponent {

  private fireCxl: Socket.IFire<Models.OrderCancelRequestFromUI> = new Socket.Fire(Models.Topics.CancelOrder);

  @Input() product: Models.ProductAdvertisement;

  @Input() set orders(o: Models.Order[]) {
    this.addRowData(o);
  };

  private grid: GridOptions = <GridOptions>{
    suppressNoRowsOverlay: true,
    defaultColDef: { sortable: true, resizable: true },
    rowHeight:21,
    columnDefs: [{
      width: 30,
      field: "cancel",
      headerName: 'cxl',
      suppressSizeToFit: true,
      cellRenderer: (params) => {
        return `<button type="button" class="btn btn-danger btn-xs">
          <span data-action-type="remove"'>&times;</span>
        </button>`;
      }
    }, {
      width: 82,
      field: 'time',
      headerName: 'time',
      suppressSizeToFit: true,
      cellRenderer: (params) => {
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
      cellClassRules: {
        'sell': 'data.side == "Ask"',
        'buy': 'data.side == "Bid"'
      },
      cellRenderer: (params) => {
        return (params.data.pong
          ? '&lrhar;'
          : '&rhard;'
        ) + params.value;
      }
    }, {
      width: 74,
      field: 'price',
      headerName: 'price',
      sort: 'desc',
      cellClassRules: {
        'sell': 'data.side == "Ask"',
        'buy': 'data.side == "Bid"'
      }
    }, {
      width: 95,
      field: 'quantity',
      headerName: 'qty',
      suppressSizeToFit: true,
      cellClassRules: {
        'sell': 'data.side == "Ask"',
        'buy': 'data.side == "Bid"'
      }
    }, {
      width: 74,
      field: 'value',
      headerName: 'value',
      cellClassRules: {
        'sell': 'data.side == "Ask"',
        'buy': 'data.side == "Bid"'
      }
    }, {
      width: 55,
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
      cellRenderer: (params) => {
        return (params.value)
          ? params.value.toString().split('-')[0]
          : '';
      }
    }]
  };

  private onGridReady() {
    Shared.currencyHeaders(this.grid.api, this.product.base, this.product.quote);
  };

  private onCellClicked = ($event) => {
    if ($event.event.target.getAttribute('data-action-type') != 'remove') return;
    this.fireCxl.fire(new Models.OrderCancelRequestFromUI($event.data.orderId, $event.data.exchange));
  };

  private addRowData = (o: Models.Order[]) => {
    if (!this.grid.api) return;

    var add = [];

    o.forEach(o => {
      add.push({
        orderId: o.orderId,
        exchangeId: o.exchangeId,
        side: Models.Side[o.side],
        price: o.price.toFixed(this.product.tickPrice),
        value: (this.product.margin == 0
                 ? Math.round(o.quantity * o.price * 100) / 100
                 : (this.product.margin == 1
                     ? Math.round((o.quantity / o.price) * 1e+8) / 1e+8
                     : Math.round((o.quantity * o.price) * 1e+8) / 1e+8
                 )).toFixed(this.product.tickPrice),
        type: Models.OrderType[o.type],
        tif: Models.TimeInForce[o.timeInForce],
        lat: o.latency + 'ms',
        quantity: o.quantity.toFixed(this.product.tickSize),
        pong: o.isPong,
        time: o.time
      });
    });

    this.grid.api.setRowData([]);

    if (add.length) this.grid.api.applyTransaction({add:add});
  };
};
