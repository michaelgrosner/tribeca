import {Component, Input, Output, EventEmitter} from '@angular/core';

import {GridOptions, RowNode, ColDef} from '@ag-grid-community/all-modules';

import {Socket, Shared, Models} from 'lib/K';

@Component({
  selector: 'trades',
  template: `<ag-grid-angular
    class="ag-theme-fresh ag-theme-dark"
    style="height: 479px;width: 99.80%;"
    (window:resize)="onGridReady()"
    (gridReady)="onGridReady()"
    (cellClicked)="onCellClicked($event)"
    [gridOptions]="grid"></ag-grid-angular>`
})
export class TradesComponent {

  private audio: boolean;
  private hasPongs: boolean;
  private headerNameMod: string = "";

  private fireCxl: Socket.IFire<Models.CleanTradeRequestFromUI> = new Socket.Fire(Models.Topics.CleanTrade);

  @Input() product: Models.ProductAdvertisement;

  @Input() set quotingParameters(o: Models.QuotingParameters) {
    this.addRowConfig(o);
  };

  @Input() set trade(o: Models.Trade) {
    this.addRowData(o);
  };

  @Output() onTradesLength = new EventEmitter<number>();

  @Output() onTradesMatchedLength = new EventEmitter<number>();

  @Output() onTradesChartData = new EventEmitter<Models.TradeChart>();

  private grid: GridOptions = <GridOptions>{
    overlayLoadingTemplate: `<span class="ag-overlay-no-rows-center">0 closed orders</span>`,
    overlayNoRowsTemplate: `<span class="ag-overlay-no-rows-center">0 closed orders</span>`,
    defaultColDef: { sortable: true, resizable: true, flex: 1 },
    rowHeight:21,
    animateRows:true,
    getRowNodeId: (data) => data.tradeId,
    columnDefs: [{
      width: 30,
      field: 'cancel',
      headerName: 'cxl',
      suppressSizeToFit: true,
      cellRenderer: (params) => {
        return `<button type="button" class="btn btn-danger btn-xs">
          <span data-action-type="remove">&times;</span>
        </button>`;
      }
    }, {
      width: 95,
      field:'time',
      sort: 'desc',
      headerValueGetter:(params) => this.headerNameMod + 'time',
      suppressSizeToFit: true,
      comparator: (valueA: number, valueB: number, nodeA: RowNode, nodeB: RowNode, isInverted: boolean) => {
          return (nodeA.data.Ktime||nodeA.data.time) - (nodeB.data.Ktime||nodeB.data.time);
      },
      cellRenderer: (params) => {
        var d = new Date(params.value||0);
        return (d.getDate()+'')
          .padStart(2, "0")+'/'+((d.getMonth()+1)+'')
          .padStart(2, "0")+' '+(d.getHours()+'')
          .padStart(2, "0")+':'+(d.getMinutes()+'')
          .padStart(2, "0")+':'+(d.getSeconds()+'')
          .padStart(2, "0");
      }
    }, {
      width: 95,
      field:'Ktime',
      headerName:'⇋time',
      hide:true,
      suppressSizeToFit: true,
      cellRenderer: (params) => {
        if (params.value==0) return '';
        var d = new Date(params.value);
        return (d.getDate()+'')
          .padStart(2, "0")+'/'+((d.getMonth()+1)+'')
          .padStart(2, "0")+' '+(d.getHours()+'')
          .padStart(2, "0")+':'+(d.getMinutes()+'')
          .padStart(2, "0")+':'+(d.getSeconds()+'')
          .padStart(2, "0");
      }
    }, {
      width: 50,
      field:'side',
      headerName:'side',
      suppressSizeToFit: true,
      cellClassRules: {
        'sell': 'x == "Ask"',
        'buy': 'x == "Bid"',
        'kira': 'x == "&#10564;"'
      },
      cellRenderer: (params) => params.value === '&#10564;'
        ? '<span style="font-size:21px;padding-left:3px;font-weight:600;">' + params.value + '</span>'
        : params.value
    }, {
      width: 80,
      field:'price',
      headerValueGetter:(params) => this.headerNameMod + 'price',
      cellClassRules: {
        'sell': 'data._side == "Ask"',
        'buy': 'data._side == "Bid"'
      }
    }, {
      width: 95,
      field:'quantity',
      headerValueGetter:(params) => this.headerNameMod + 'qty',
      suppressSizeToFit: true,
      cellClassRules: {
        'sell': 'data._side == "Ask"',
        'buy': 'data._side == "Bid"'
      }
    }, {
      width: 69,
      field:'value',
      headerValueGetter:(params) => this.headerNameMod + 'value',
      cellClassRules: {
        'sell': 'data._side == "Ask"',
        'buy': 'data._side == "Bid"'
      }
    }, {
      width: 75,
      field:'Kvalue',
      headerName:'⥄value',
      cellClassRules: {
        'buy': 'data._side == "Ask"',
        'sell': 'data._side == "Bid"'
      }
    }, {
      width: 85,
      field:'Kqty',
      headerName:'⥄qty',
      suppressSizeToFit: true,
      cellClassRules: {
        'buy': 'data._side == "Ask"',
        'sell': 'data._side == "Bid"'
      }
    }, {
      width: 80,
      field:'Kprice',
      headerName:'⥄price',
      cellClassRules: {
        'buy': 'data._side == "Ask"',
        'sell': 'data._side == "Bid"'
      }
    }, {
      width: 65,
      field:'delta',
      headerName:'delta',
      cellClassRules: {
        'kira': 'data.side == "&#10564;"'
      },
      cellRenderer: (params) => params.value
        ? parseFloat(params.value.toFixed(8))
        : ''
    }]
  };

  private onGridReady() {
    Shared.currencyHeaders(this.grid.api, this.product.base, this.product.quote);
  };

  private onCellClicked = ($event) => {
    if ($event.event.target.getAttribute('data-action-type') != 'remove') return;
    this.fireCxl.fire(new Models.CleanTradeRequestFromUI($event.data.tradeId));
  }

  private addRowConfig = (o: Models.QuotingParameters) => {
    this.audio = o.audio;

    this.hasPongs = (o.safety === Models.QuotingSafety.Boomerang || o.safety === Models.QuotingSafety.AK47);

    this.headerNameMod = this.hasPongs ? "➜" : "";

    if (!this.grid.api) return;

    this.grid.columnDefs.map((x: ColDef)  => {
      if (['Ktime','Kqty','Kprice','Kvalue','delta'].indexOf(x.field) > -1)
        this.grid.columnApi.setColumnVisible(x.field, this.hasPongs);
      return x;
    });

    this.grid.api.refreshHeader();

    this.emitLengths();
  };

  private addRowData = (o: Models.Trade) => {
    if (!this.grid.api) return;

    if (o === null) this.grid.api.setRowData([]);
    else {
      var node: RowNode = this.grid.api.getRowNode(o.tradeId);
      if (o.Kqty < 0) {
        if (node)
          this.grid.api.applyTransaction({remove: [node.data]});
      } else {
        var edit = {
          time: o.time,
          quantity: o.quantity.toFixed(this.product.tickSize),
          value: o.value.toFixed(this.product.tickPrice),
          Ktime: o.Ktime,
          Kqty: o.Kqty ? o.Kqty.toFixed(this.product.tickSize) : '',
          Kprice: o.Kprice ? o.Kprice.toFixed(this.product.tickPrice) : '',
          Kvalue: o.Kvalue ? o.Kvalue.toFixed(this.product.tickPrice) : '',
          delta: o.delta,
          side: o.Kqty >= o.quantity ? '&#10564;' : (o.side === Models.Side.Ask ? "Ask" : "Bid"),
          _side: o.side === Models.Side.Ask ? "Ask" : "Bid",
        };

        if (node) node.setData(Object.assign(node.data, edit));
        else this.grid.api.applyTransaction({add: [Object.assign(edit, {
          tradeId: o.tradeId,
          price: o.price.toFixed(this.product.tickPrice)
        })]});

        if (o.loadedFromDB === false) {
          if (this.audio) Shared.playAudio(o.isPong?'1':'0');

          this.onTradesChartData.emit(new Models.TradeChart(
            (o.isPong && o.Kprice)?o.Kprice:o.price,
            (o.isPong && o.Kprice)?(o.side === Models.Side.Ask ? Models.Side.Bid : Models.Side.Ask):o.side,
            (o.isPong && o.Kprice)?o.Kqty:o.quantity,
            (o.isPong && o.Kprice)?o.Kvalue:o.value,
            o.isPong
          ));
        }
      }
    }

    this.emitLengths();
  };

  private emitLengths = () => {
    this.onTradesLength.emit(this.grid.api.getModel().getRowCount());
    var tradesMatched: number = 0;
    if (this.hasPongs) {
      this.grid.api.forEachNode((node: RowNode) => {
        if (node.data.Kqty) tradesMatched++;
      });
    } else tradesMatched = -1;
    this.onTradesMatchedLength.emit(tradesMatched);
  };
};
