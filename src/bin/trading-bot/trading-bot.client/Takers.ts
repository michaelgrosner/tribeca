import {Component, Input} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import {Shared, Models} from 'lib/K';

@Component({
  selector: 'takers',
  template: `<ag-grid-angular
    class="ag-theme-fresh ag-theme-dark marketTrades"
    style="height: 616px;width: 100%;"
    (window:resize)="onGridReady()"
    (gridReady)="onGridReady()"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class TakersComponent {

  @Input() product: Models.ProductAdvertisement;

  @Input() set taker(o: Models.MarketTrade) {
    this.addRowData(o);
  };

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">empty history</span>`,
    overlayNoRowsTemplate: `<span class="ag-overlay-no-rows-center">empty history</span>`,
    defaultColDef: { sortable: true, resizable: true, flex: 1 },
    rowHeight:21,
    columnDefs: [{
      field: 'time',
      width: 82,
      headerName: 'time',
      sort: 'desc',
      suppressSizeToFit: true,
      cellClassRules: {
        'text-muted': '!data.recent'
      },
      cellRenderer: (params) => {
        var d = new Date(params.value||0);
        return (d.getHours()+'')
          .padStart(2, "0")+':'+(d.getMinutes()+'')
          .padStart(2, "0")+':'+(d.getSeconds()+'')
          .padStart(2, "0")+','+(d.getMilliseconds()+'')
          .padStart(3, "0");
      }
    }, {
      field: 'price',
      width: 85,
      headerName: 'price',
      cellClassRules: {
        'sell': 'data.side == "Ask"',
        'buy': 'data.side == "Bid"'
      }
    }, {
      field: 'quantity',
      width: 50,
      headerName: 'qty',
      cellClassRules: {
        'sell': 'data.side == "Ask"',
        'buy': 'data.side == "Bid"'
      }
    }, {
      field: 'side',
      width: 40,
      headerName: 'side',
      cellClassRules: {
        'sell': 'x == "Ask"',
        'buy': 'x == "Bid"'
      },
    }]
  };

  private onGridReady() {
    Shared.currencyHeaders(this.grid.api, this.product.base, this.product.quote);
  };

  private addRowData = (o: Models.MarketTrade) => {
    if (!this.grid.api) return;

    if (o === null) this.grid.api.setRowData([]);
    else {
      this.grid.api.applyTransaction({add: [{
        price: o.price.toFixed(this.product.tickPrice),
        quantity: o.quantity.toFixed(this.product.tickSize),
        time: o.time,
        recent: true,
        side: Models.Side[o.side]
      }]});

      this.grid.api.forEachNode((node: RowNode) => {
        if (Math.abs(o.time - node.data.time) > 3600000)
          this.grid.api.applyTransaction({remove: [node.data]});
        else if (node.data.recent && Math.abs(o.time - node.data.time) > 7000)
          node.setData(Object.assign(node.data, {recent: false}));
      });
    }
  };
};
