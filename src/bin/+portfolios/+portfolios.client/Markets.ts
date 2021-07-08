import {Component, Input, Output, EventEmitter} from '@angular/core';

import {GridOptions,} from '@ag-grid-community/all-modules';

@Component({
  selector: 'markets',
  template: `<ag-grid-angular id="markets"
    class="ag-theme-fresh ag-theme-dark ag-theme-big"
    style="width: 440px;margin: 6px 0px;"
    (firstDataRendered)="onFirstDataRendered()"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class MarketsComponent {

  @Output() rendered = new EventEmitter<boolean>();

  @Input() set markets(o: any) {
    this.addRowData(o);
  };

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    overlayNoRowsTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    defaultColDef: { sortable: true, resizable: true, flex: 1 },
    rowHeight:35,
    domLayout: 'autoHeight',
    animateRows:true,
    columnDefs: [{
      width: 140,
      field: 'currency',
      headerName: 'markets',
      sort: 'asc',
      cellRenderer: (params) => `<a
        rel="noreferrer" target="_blank"
        title="` + params.data.url + `"
        href="` + params.data.url + `"><i class="beacon-sym-_default-s beacon-sym-` + params.value.toLowerCase() + `-s" ></i> ` + params.value + `</a>`
    }, {
      width: 100,
      field: 'spread',
      headerName: 'spread'
    }, {
      width: 100,
      field: '24h price',
      headerName: 'diff'
    }, {
      width: 100,
      field: '1d volume',
      headerName: 'volume'
    }]
  };

  private onFirstDataRendered() {
    this.rendered.emit(true);
  };

  private addRowData = (o: any) => {
    if (!this.grid.api) return;
    this.grid.api.setRowData([]);
    var rows = [];
    for (let x in o) rows.push({
      currency: x,
      url: o[x]
    });
    if (rows.length)
      this.grid.api.applyTransaction({add: rows});
  };

};
