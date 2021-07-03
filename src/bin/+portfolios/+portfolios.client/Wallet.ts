import {Component, Input, Output, EventEmitter} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import {Shared, Models} from 'lib/K';

@Component({
  selector: 'wallet',
  template: `<ag-grid-angular
    class="ag-theme-fresh ag-theme-dark ag-theme-big"
    style="height: 479px;width: 100%;"
    (window:resize)="onGridReady()"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class WalletComponent {

  @Input() set asset(o: any) {
    this.addRowData(o);
  };

  @Input() settings: Models.PortfolioParameters;

  @Output() onBalance = new EventEmitter<number>();

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    overlayNoRowsTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    defaultColDef: { sortable: true, resizable: true },
    rowHeight:35,
    animateRows:true,
    getRowNodeId: function (data) { return data.currency; },
    columnDefs: [{
      width: 130,
      field: 'currency',
      headerName: 'currency',
      cellRenderer: (params) => {
        var sym = params.value.toLowerCase();
        if (sym == 'cgld') sym = 'celo';
        return '<i class="beacon-sym-_default-s beacon-sym-' + sym + '-s" ></i> ' + params.value;
      },
      cellClassRules: {
        'text-muted': 'data.total == "0.00000000"'
      }
    }, {
      width: 220,
      field: 'price',
      headerName: 'price',
      type: 'rightAligned',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_price == "up-data"',
        'down-data': 'data.dir_price == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 220,
      field: 'balance',
      headerName: 'balance',
      sort: 'desc',
      type: 'rightAligned',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_balance == "up-data"',
        'down-data': 'data.dir_balance == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 220,
      field: 'total',
      headerName: 'total',
      type: 'rightAligned',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_total == "up-data"',
        'down-data': 'data.dir_total == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 220,
      field: 'amount',
      headerName: 'available',
      type: 'rightAligned',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_amount == "up-data"',
        'down-data': 'data.dir_amount == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 220,
      field: 'held',
      headerName: 'held',
      type: 'rightAligned',
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_held == "up-data"',
        'down-data': 'data.dir_held == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }]
  };

  private onGridReady() {
    Shared.currencyHeaders(this.grid.api, this.settings.currency, this.settings.currency);
  };

  private addRowData = (o: any) => {
    if (!this.grid.api) return;
    if (o === null) this.grid.api.setRowData([]);
    else o.forEach(o => {
      const amount = o.wallet.amount.toFixed(8);
      const held = o.wallet.held.toFixed(8);
      const total = (o.wallet.amount + o.wallet.held).toFixed(8);
      const balance = o.wallet.value.toFixed(8);
      const price = o.price.toFixed(8);
      var node: RowNode = this.grid.api.getRowNode(o.wallet.currency);
      if (!node) {
        this.grid.api.applyTransaction({add: [{
          currency: o.wallet.currency,
          amount: amount,
          held: held,
          total: total,
          balance: balance,
          price: price
        }]});
      } else {
        var cols = [];

        if (this.resetRowData('balance', balance, node)) cols.push('balance');
        if (this.resetRowData('price',   price,   node)) cols.push('price');
        if (this.resetRowData('amount',  amount,  node)) cols.push('amount');
        if (this.resetRowData('held',    held,    node)) cols.push('held');
        if (this.resetRowData('total',   total,   node)) cols.push('total');

        this.grid.api.flashCells({ rowNodes: [node], columns: cols});
      }
    });

    var sum = 0;
    this.grid.api.forEachNode((node: RowNode) => {
      if (node.data.balance) sum += parseFloat(node.data.balance);
    });
    this.onBalance.emit(sum);
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
