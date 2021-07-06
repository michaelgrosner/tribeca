import {Component, Input} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import {Shared, Models} from 'lib/K';

@Component({
  selector: 'wallet',
  template: `<ag-grid-angular id="portfolios"
    class="ag-theme-fresh ag-theme-dark ag-theme-big"
    style="width: 100%;"
    (window:resize)="onGridReady()"
    (gridReady)="onGridReady()"
    (rowClicked)="onRowClicked($event)"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class WalletComponent {

  private selection: string = "";

  private pattern: string = "";

  @Input() links: any;

  @Input() set asset(o: any) {
    this.addRowData(o);
  };

  @Input() settings: Models.PortfolioParameters;

  private onRowClicked = ($event) => {
    if (!$event.data.currency) return;
    if (this.selection == $event.data.currency) {
      this.grid.api.deselectAll();
      this.selection = null;
    }
    else this.selection = $event.data.currency;
  };

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    overlayNoRowsTemplate: `<span class="ag-overlay-no-rows-center">missing data</span>`,
    defaultColDef: { sortable: true, resizable: true, flex: 1 },
    rowHeight:35,
    domLayout: 'autoHeight',
    animateRows:true,
    rowSelection:'single',
    enableCellTextSelection: true,
    onSelectionChanged: () => {
      if (document.getElementById("market_links")) document.getElementById("market_links").remove();
      var row = this.grid.api.getSelectedRows().reverse().pop();
      if (!row || !this.links.hasOwnProperty(row.currency)) return;
      var div = document.createElement('div');
      div.setAttribute('id', 'market_links');
      var tbl = document.createElement('table');
      div.appendChild(tbl);
      var tr;
      var i = 0;
      for (let x in this.links[row.currency]) {
        var a = document.createElement('a');
        a.setAttribute('rel', 'noreferrer');
        a.setAttribute('target', '_blank');
        a.setAttribute('title', this.links[row.currency][x]);
        a.setAttribute('href', this.links[row.currency][x]);
        a.onclick = (ev) => { ev.stopPropagation() };
        a.innerText = x;
        var td = document.createElement('td');
        td.appendChild(a);
        if (!(i++ % 5))
          tbl.appendChild(tr = document.createElement('tr'));
        tr.appendChild(td);
      }
      document.querySelectorAll('#portfolios div[row-id]').forEach((o: HTMLElement) => {
        o.style.zIndex = o.getAttribute('row-id') == row.currency ? '2' : '1';
      });
      document.querySelector("#portfolios div[row-id='" + row.currency + "'] div[aria-colindex='4']").appendChild(div);
    },
    isExternalFilterPresent: () => !this.settings.zeroed || !!this.pattern,
    doesExternalFilterPass: (node) => (
      this.settings.zeroed || !!parseFloat(node.data.total)
    ) && (
      !this.pattern || node.data.currency.toUpperCase().indexOf(this.pattern) > -1
    ),
    getRowNodeId: (data) => data.currency,
    columnDefs: [{
      width: 220,
      field: 'held',
      headerName: 'held',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => ``,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_held == "up-data"',
        'down-data': 'data.dir_held == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 220,
      field: 'amount',
      headerName: 'available',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => ``,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_amount == "up-data"',
        'down-data': 'data.dir_amount == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 220,
      field: 'total',
      headerName: 'total',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => ``,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_total == "up-data"',
        'down-data': 'data.dir_total == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }, {
      width: 130,
      field: 'currency',
      headerName: 'currency',
      pinnedRowCellRenderer: (params) => `<input type="text" class="form-control"
        style="background: #0000005c;width: 100%;height: 26px;font-size: 19px;margin-top: -1px;"
        title="filter" id="filter_pattern" />`,
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
      pinnedRowCellRenderer: (params) => `<span id="price_pin"></span>`,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
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
      pinnedRowCellRenderer: (params) => `<span class="kira" id="balance_pin"></span><span id="total_pin" class="balance_percent"></span>`,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`
        + `<small class="balance_percent" id="balance_percent_`+params.data.currency+`"></small>`,
      cellClassRules: {
        'text-muted': 'x == "0.00000000"',
        'up-data': 'data.dir_balance == "up-data"',
        'down-data': 'data.dir_balance == "down-data"'
      },
      comparator: (valueA, valueB, nodeA, nodeB, isInverted) => valueA - valueB
    }]
  };

  private onGridReady() {
    Shared.currencyHeaders(this.grid.api, this.settings.currency, this.settings.currency);

    this.pin();
  };

  private pin = () => {
    if (this.grid.api && !this.grid.api.getPinnedTopRowCount()) {
      this.grid.api.setPinnedTopRowData([{}]);

      var pin = (pin) => {
        if (document.getElementById("filter_pattern")
          && document.getElementById("price_pin")
          && document.getElementById("total_pin")
        ) {
          document.getElementById("filter_pattern").addEventListener("keyup", event => {
            this.pattern =
            (<HTMLInputElement>event.target).value = (<HTMLInputElement>event.target).value.toUpperCase();
            this.grid.api.onFilterChanged();
          });

          document.getElementById("price_pin").appendChild(
            document.getElementById("base_select")
          );

          document.getElementById("total_pin").appendChild(
            document.getElementById("zeroed_checkbox")
          );
        } else setTimeout(() => pin(pin), 69);
      }

      pin(pin);
    }
  };

  private addRowData = (o: any) => {
    if (!this.grid.api) return;
    var sum = 0;
    var pin_sum = document.getElementById('balance_pin');
    if (o === null) this.grid.api.setRowData([]);
    else o.forEach(o => {
      const amount = o.wallet.amount.toFixed(8);
      const held = o.wallet.held.toFixed(8);
      const total = (o.wallet.amount + o.wallet.held).toFixed(8);
      const balance = o.wallet.value.toFixed(8);
      const price = o.price.toFixed(8);
      sum += o.wallet.value;
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

      if (pin_sum) {
        var el = document.getElementById('balance_percent_' + o.wallet.currency);
        if (el) {
          var val = (o.wallet.value / parseFloat(pin_sum.innerHTML) * 100).toFixed(2);
          if (val != el.innerHTML) el.innerHTML = val;
        }
      }
    });

    if (pin_sum) pin_sum.innerHTML = sum.toFixed(8);
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
