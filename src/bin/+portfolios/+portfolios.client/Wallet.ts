import {Component, Input} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

@Component({
  selector: 'wallet',
  template: `<ag-grid-angular
    class="ag-theme-fresh ag-theme-dark ag-theme-big"
    style="height: 479px;width: 100%;"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class WalletComponent {

  @Input() set asset(o: any) {
    this.addRowData(o);
  };

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    defaultColDef: { sortable: true, resizable: true },
    rowHeight:35,
    animateRows:true,
    getRowNodeId: function (data) { return data.currency; },
    columnDefs: [{
      width: 130,
      field: 'currency',
      headerName: 'currency',
      cellRenderer:(params) => {
        var sym = params.value.toLowerCase();
        if (sym == 'cgld') sym = 'celo';
        return '<i class="beacon-sym-_default-s beacon-sym-' + sym + '-s" ></i> ' + params.value;
      },
      cellClassRules: {
        'text-muted': 'data.total == "0.00000000"'
      }
    }, {
      width: 220,
      field: 'amount',
      headerName: 'amount',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_amount == "up-data"',
        'down-data': 'data.dir_amount == "down-data"'
      }
    }, {
      width: 220,
      field: 'held',
      headerName: 'held',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_held == "up-data"',
        'down-data': 'data.dir_held == "down-data"'
      }
    }, {
      width: 220,
      field: 'total',
      headerName: 'total',
      sort: 'desc',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_total == "up-data"',
        'down-data': 'data.dir_total == "down-data"'
      }
    }]
  };

  private addRowData = (o: any) => {
    if (!this.grid.api) return;
    if (o === null) this.grid.api.setRowData([]);
    else {
      const amount = o.wallet.amount.toFixed(8);
      const held = o.wallet.held.toFixed(8);
      const total = (o.wallet.amount + o.wallet.held).toFixed(8);
      var node: RowNode = this.grid.api.getRowNode(o.wallet.currency);
      if (!node) {
        this.grid.api.applyTransaction({add: [{
          currency: o.wallet.currency,
          amount: amount,
          held: held,
          total: total,
          dir_amount: '',
          dir_held: '',
          dir_total: ''
        }]});
      } else {
        var cols = [];

        if (this.resetRowData('amount', amount, node)) cols.push('amount');
        if (this.resetRowData('held',   held,   node)) cols.push('held');
        if (this.resetRowData('total',  total,  node)) cols.push('total');

        this.grid.api.flashCells({ rowNodes: [node], columns: cols});
      }
    }
  };

  private resetRowData = (name: string, val: string, node: RowNode) => {
      var dir = '';
      if      (val > node.data[name]) dir = 'up-data';
      else if (val < node.data[name]) dir = 'down-data';
      if (dir != '') {
        node.data['dir_' + name] = dir;
        node.setDataValue(name, val);
      }
      return dir != '';
  };
};
