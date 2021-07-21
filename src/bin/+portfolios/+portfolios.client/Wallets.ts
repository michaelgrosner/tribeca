import {Component, Input} from '@angular/core';

import {GridOptions, RowNode} from '@ag-grid-community/all-modules';

import {Shared, Models} from 'lib/K';

@Component({
  selector: 'wallets',
  template: `<div hidden="true">
    <markets *ngIf="markets_view"
      (rendered)="onRendered($event)"
      [settings]="settings"
      [markets]="markets"
      [market]="market"></markets>
  </div>
  <ag-grid-angular id="portfolios"
    class="ag-theme-fresh ag-theme-dark ag-theme-big"
    style="width: 100%;"
    (window:resize)="onGridReady()"
    (gridReady)="onGridReady()"
    (rowClicked)="onRowClicked($event)"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class WalletsComponent {

  private deferredRender: any = null;

  private markets_view: boolean = false;
  private market: any = null;

  private selection: string = "";

  private pattern: string = "";

  @Input() markets: any;

  @Input() set wallets(o: any) {
    this.addRowData(o);
  };

  @Input() settings: Models.PortfolioParameters;

  private onRendered = ($event) => {
    if (this.deferredRender) setTimeout(this.deferredRender, 0);
  };

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
      this.markets_view = false;
      this.market = null;
      this.grid.api.forEachNode((node: RowNode) => {
        node.setRowHeight(this.grid.rowHeight);
      });
      var node: RowNode = this.grid.api.getSelectedNodes().reverse().pop();
      if (!node) return this.grid.api.onRowHeightChanged();
      this.deferredRender = () => {
        document.querySelector("#portfolios div[row-id='" + node.data.currency + "'] div[aria-colindex='4']").appendChild(document.querySelector('#markets'));
        var style = (<HTMLElement>document.querySelector('#markets')).style;
        node.setRowHeight(
          this.grid.rowHeight
          + parseInt(style.marginTop)
          + parseInt(style.marginBottom)
          + (<HTMLElement>document.querySelector('#markets div')).offsetHeight
        );
        this.grid.api.onRowHeightChanged();
      };
      setTimeout(() => {
        this.markets_view = true;
        setTimeout(() => {
          this.market = node.data.currency;
        }, 0);
      }, 0);
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
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_held == "up-data"',
        'down-data': 'data.dir_held == "down-data"'
      },
      comparator: Shared.comparator
    }, {
      width: 220,
      field: 'amount',
      headerName: 'available',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => ``,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_amount == "up-data"',
        'down-data': 'data.dir_amount == "down-data"'
      },
      comparator: Shared.comparator
    }, {
      width: 220,
      field: 'total',
      headerName: 'total',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => ``,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_total == "up-data"',
        'down-data': 'data.dir_total == "down-data"'
      },
      comparator: Shared.comparator
    }, {
      width: 130,
      field: 'currency',
      headerName: 'currency',
      pinnedRowCellRenderer: (params) => `<input type="text" class="form-control"
        style="background: #0000005c;width: 100%;height: 26px;font-size: 19px;margin-top: -1px;"
        title="filter" id="filter_pattern" />`,
      cellRenderer: (params) => '<span class="row_title"><i class="beacon sym-_default-s sym-' + params.value.toLowerCase() + '-s" ></i> ' + params.value + '</span>',
      cellClassRules: {
        'text-muted': '!parseFloat(data.total)'
      }
    }, {
      width: 220,
      field: 'price',
      headerName: 'price',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => `<span id="price_pin"></span>`,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_price == "up-data"',
        'down-data': 'data.dir_price == "down-data"'
      },
      comparator: Shared.comparator
    }, {
      width: 220,
      field: 'balance',
      headerName: 'balance',
      sort: 'desc',
      type: 'rightAligned',
      pinnedRowCellRenderer: (params) => `<span class="kira" id="balance_pin"></span><span id="total_pin" class="balance_percent"></span>`,
      cellRenderer: (params) => `<span class="val">` + params.value + `</span>`
        + `<small class="balance_percent" id="balance_percent_` + params.data.currency + `"></small>`,
      cellClassRules: {
        'text-muted': '!parseFloat(x)',
        'up-data': 'data.dir_balance == "up-data"',
        'down-data': 'data.dir_balance == "down-data"'
      },
      comparator: Shared.comparator
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
          document.getElementById("zeroed_checkbox").addEventListener("change", event => {
            this.cleanSelection();
          });

          document.getElementById("filter_pattern").addEventListener("keyup", event => {
            this.pattern =
            (<HTMLInputElement>event.target).value = (<HTMLInputElement>event.target).value.toUpperCase();
            this.grid.api.onFilterChanged();
            this.cleanSelection();
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

  private cleanSelection = () => {
    if (!this.selection) return;
    var node: RowNode = this.grid.api.getRowNode(this.selection);
    if (node && !this.grid.doesExternalFilterPass(node))
      this.onRowClicked({data:{currency:this.selection}});
  };

  private addRowData = (o: any) => {
    if (!this.grid.api) return;
    var sum = 0;
    if (o === null) {
      this.grid.api.setRowData([]);
      this.selection = null;
    }
    else o.forEach(o => {
      const amount  = Shared.str(o.wallet.amount,                 8);
      const held    = Shared.str(o.wallet.held,                   8);
      const total   = Shared.str(o.wallet.amount + o.wallet.held, 8);
      const balance = Shared.str(o.wallet.value,                  8);
      const price   = Shared.str(o.price,                         8);
      sum += o.wallet.value;
      var node: RowNode = this.grid.api.getRowNode(o.wallet.currency);
      if (!node)
        this.grid.api.applyTransaction({add: [{
          currency: o.wallet.currency,
          amount: amount,
          held: held,
          total: total,
          balance: balance,
          price: price
        }]});
      else
        this.grid.api.flashCells({
          rowNodes: [node],
          columns: [].concat(Shared.resetRowData('balance', balance, node))
                     .concat(Shared.resetRowData('price',   price,   node))
                     .concat(Shared.resetRowData('amount',  amount,  node))
                     .concat(Shared.resetRowData('held',    held,    node))
                     .concat(Shared.resetRowData('total',   total,   node))
        });
    });

    this.grid.api.onFilterChanged();

    if (!this.grid.api.getSelectedNodes().length)
      this.grid.api.onSortChanged();

    var pin_sum = document.getElementById('balance_pin');
    if (pin_sum) {
      pin_sum.innerHTML = Shared.str(sum, 8);

      this.grid.api.forEachNode((node: RowNode) => {
        var el = document.getElementById('balance_percent_' + node.data.currency);
        if (el) {
          var val = Shared.str(Shared.num(node.data.balance) / sum * 100, 2);
          if (val != el.innerHTML) el.innerHTML = val;
        }
      });
    }
  };
};
