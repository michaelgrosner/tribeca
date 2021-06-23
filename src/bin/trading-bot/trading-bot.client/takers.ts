import {Component, Input} from '@angular/core';
import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import * as Shared from 'lib/shared';
import * as Models from 'lib/models';

@Component({
  selector: 'takers',
  template: `<ag-grid-angular #marketList
    class="ag-theme-fresh ag-theme-dark marketTrades"
    style="height: 616px;width: 100%;"
    rowHeight="21"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class TakersComponent {

  @Input() product: Models.ProductAdvertisement;

  @Input() set taker(o: Models.MarketTrade) {
    this.addRowData(o);
  };

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">empty history</span>`,
    defaultColDef: { sortable: true, resizable: true },
    columnDefs: [{
      field: 'time',
      width: 82,
      headerName: 'time',
      sort: 'desc',
      cellClass: (params) => {
        return 'fs11px ' + (!params.data.recent ? "text-muted" : "");
      },
      cellRenderer:(params) => {
        var d = new Date(params.value||0);
        return (d.getHours()+'')
          .padStart(2, "0")+':'+(d.getMinutes()+'')
          .padStart(2, "0")+':'+(d.getSeconds()+'')
          .padStart(2, "0")+','+(d.getMilliseconds()+'')
          .padStart(3, "0");
      }
    }, {
      field: 'price',
      width: 75,
      headerName: 'price',
      cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      },
      cellRendererFramework: Shared.QuoteCurrencyCellComponent
    }, {
      field: 'quantity',
      width: 50,
      headerName: 'qty',
      cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      },
      cellRendererFramework: Shared.BaseCurrencyCellComponent
    }, {
      field: 'side',
      width: 40,
      headerName: 'side',
      cellClass: (params) => {
        if (params.value === 'Bid') return 'buy';
        else if (params.value === 'Ask') return "sell";
      }
    }]
  };

  private addRowData = (o: Models.MarketTrade) => {
    if (!this.grid.api) return;
    if (o === null) {
      if (this.grid.rowData)
        this.grid.api.setRowData([]);
    } else {
      this.grid.api.applyTransaction({add: [{
        price: o.price,
        quantity: o.quantity,
        time: o.time,
        recent: true,
        side: Models.Side[o.side],
        quoteSymbol: this.product.quote,
        productFixedPrice: this.product.tickPrice,
        productFixedSize: this.product.tickSize
      }]});

      this.grid.api.forEachNode((node: RowNode) => {
        if (Math.abs(o.time - node.data.time) > 3600000)
          this.grid.api.applyTransaction({remove: [node.data]});
        else if (Math.abs(o.time - node.data.time) > 7000)
          node.setData(Object.assign(node.data, {recent: false}));
      });
    }
  };
};
