import {Component, Input, Output, EventEmitter} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import {Shared, Models} from 'lib/K';

@Component({
  selector: 'markets',
  template: `<ag-grid-angular id="markets"
    class="ag-theme-fresh ag-theme-dark ag-theme-big"
    style="width: 560px;margin: 6px 0px;"
    (window:resize)="onGridReady()"
    (gridReady)="onGridReady()"
    (firstDataRendered)="onFirstDataRendered()"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class MarketsComponent {

  private _markets: any = null;
  private _market: any = null;

  @Output() rendered = new EventEmitter<boolean>();

  @Input() settings: Models.PortfolioParameters;

  @Input() set markets(o: any) {
    this._markets = o;
    this.addRowData();
  };

  @Input() set market(o: any) {
    this._market = o;
    this.addRowData();
  };

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    overlayNoRowsTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    defaultColDef: { sortable: true, resizable: true, flex: 1 },
    rowHeight:35,
    domLayout: 'autoHeight',
    animateRows:true,
    enableCellTextSelection: true,
    getRowNodeId: (data) => data.currency,
    columnDefs: [{
      width: 150,
      field: 'currency',
      headerName: 'markets',
      sort: 'asc',
      cellRenderer: (params) => `<a
        rel="noreferrer" target="_blank"
        title="` + params.data.web + `"
        href="` + params.data.web + `"><i class="beacon sym-_default-s sym-` + params.value.toLowerCase() + `-s" ></i> ` + params.value + `</a>`
    }, {
      width: 200,
      field: 'spread',
      headerName: 'bid/ask spread',
      type: 'rightAligned',
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_spread == "up-data"',
        'down-data': 'data.dir_spread == "down-data"'
      },
      comparator: Shared.comparator
    }, {
      width: 70,
      field: 'open',
      headerName: '24h price',
      type: 'rightAligned',
      cellRenderer: (params) => `<span class="open_percent val">` + ['','+'][+(Shared.num(params.value) > 0)] + params.value + `</span>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_open == "up-data"',
        'down-data': 'data.dir_open == "down-data"'
      },
      comparator: Shared.comparator
    }, {
      width: 140,
      field: 'volume',
      headerName: '24h volume',
      type: 'rightAligned',
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_volume == "up-data"',
        'down-data': 'data.dir_volume == "down-data"'
      },
      comparator: Shared.comparator
    }]
  };

  private onGridReady() {
    Shared.currencyHeaders(this.grid.api, this.settings.currency, this.settings.currency);
  };

  private onFirstDataRendered() {
    this.rendered.emit(true);
  };

  private addRowData = () => {
    if (!this.grid.api) return;
    if (!this._market || !this._markets)
      return this.grid.api.setRowData([]);
    var o = {};
    for (let x in this._markets)
      if (x == this._market)
        for (let z in this._markets[x])
          o[z] = this._markets[x][z];
      else if (this._markets[x].hasOwnProperty(this._market))
        o[x] = this._markets[x][this._market];
    if (!Object.keys(o).length)
      return this.grid.api.setRowData([]);
    for (let x in o) {
      const spread = Shared.str(o[x].spread, 8);
      const open   = Shared.str(o[x].open,   2);
      const volume = Shared.str(o[x].volume, 0);
      var node: RowNode = this.grid.api.getRowNode(x);
      if (!node)
        this.grid.api.applyTransaction({add: [{
          currency: x,
          web: o[x].web,
          spread: spread,
          open: open,
          volume: volume
        }]});
      else
        this.grid.api.flashCells({
          rowNodes: [node],
          columns: [].concat(Shared.resetRowData('spread', spread, node))
                     .concat(Shared.resetRowData('open',   open,   node))
                     .concat(Shared.resetRowData('volume', volume, node))
        });
    }

    this.grid.api.onSortChanged();
  };
};
