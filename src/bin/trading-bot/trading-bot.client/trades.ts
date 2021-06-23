import {Component, EventEmitter, Input, Output} from '@angular/core';
import {GridOptions, RowNode, ColDef} from '@ag-grid-community/all-modules';

import * as Socket from 'lib/socket';
import * as Shared from 'lib/shared';
import * as Models from 'lib/models';

@Component({
  selector: 'trades',
  template: `<ag-grid-angular #tradeList
    class="ag-theme-fresh ag-theme-dark"
    style="height: 479px;width: 99.80%;"
    rowHeight="21"
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
    defaultColDef: { sortable: true, resizable: true },
    columnDefs: [{
      width: 30,
      field: "cancel",
      headerName: 'cxl',
      suppressSizeToFit: true,
      cellRenderer: (params) => {
        return `<button type="button" class="btn btn-danger btn-xs">
          <span data-action-type="remove"
            style="font-size: 16px;font-weight: bold;padding: 0px;line-height: 12px;"
          >&times;</span>
        </button>`;
      }
    }, {
      width: 95,
      field:'time',
      sort: 'desc',
      headerValueGetter:(params) => { return this.headerNameMod + 'time'; },
      suppressSizeToFit: true,
      comparator: (valueA: number, valueB: number, nodeA: RowNode, nodeB: RowNode, isInverted: boolean) => {
          return (nodeA.data.Ktime||nodeA.data.time) - (nodeB.data.Ktime||nodeB.data.time);
      },
      cellClass: 'fs11px',
      cellRenderer:(params) => {
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
      cellClass: 'fs11px',
      cellRenderer:(params) => {
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
      width: 40,
      field:'side',
      headerName:'side',
      suppressSizeToFit: true,
      cellClass: (params) => {
        if (params.value === 'Bid') return 'buy';
        else if (params.value === 'Ask') return "sell";
        else if (params.value === '&lrhar;') return "kira";
        else return "unknown";
      },
      cellRenderer:(params) => {
        return params.value === '&lrhar;' ? '<span style="font-size:15px;padding-left:3px;">' + params.value + '</span>' : params.value;
      }
    }, {
      width: 80,
      field:'price',
      headerValueGetter:(params) => { return this.headerNameMod + 'price'; },
      cellClass: (params) => { return params.data.pingSide; },
      cellRendererFramework: Shared.QuoteCurrencyCellComponent
    }, {
      width: 85,
      field:'quantity',
      headerValueGetter:(params) => { return this.headerNameMod + 'qty'; },
      suppressSizeToFit: true,
      cellClass: (params) => { return params.data.pingSide; },
      cellRendererFramework: Shared.BaseCurrencyCellComponent
    }, {
      width: 69,
      field:'value',
      headerValueGetter:(params) => { return this.headerNameMod + 'value'; },
      cellClass: (params) => { return params.data.pingSide; },
      cellRendererFramework: Shared.QuoteCurrencyCellComponent
    }, {
      width: 75,
      field:'Kvalue',
      headerName:'⇋value',
      hide:true,
      cellClass: (params) => { return params.data.pongSide; },
      cellRendererFramework: Shared.QuoteCurrencyCellComponent
    }, {
      width: 85,
      field:'Kqty',
      headerName:'⇋qty',
      hide:true,
      suppressSizeToFit: true,
      cellClass: (params) => { return params.data.pongSide; },
      cellRendererFramework: Shared.BaseCurrencyCellComponent
    }, {
      width: 80,
      field:'Kprice',
      headerName:'⇋price',
      hide:true,
      cellClass: (params) => { return params.data.pongSide; },
      cellRendererFramework: Shared.QuoteCurrencyCellComponent
    }, {
      width: 65,
      field:'delta',
      headerName:'delta',
      hide:true,
      cellClass: (params) => { if (params.data.side === '&lrhar;') return "kira"; else return ""; },
      cellRenderer: (params) => {
        return (!params.value) ? "" : params.data.quoteSymbol + parseFloat(params.value.toFixed(8));
      }
    }]
  };

  private onCellClicked = ($event) => {
    if ($event.event.target.getAttribute("data-action-type") != 'remove') return;
    this.fireCxl.fire(new Models.CleanTradeRequestFromUI($event.data.tradeId));
  }

  private addRowConfig = (o: Models.QuotingParameters) => {
    this.audio = o.audio;
    if (!this.grid.api) return;
    this.hasPongs = (o.safety === Models.QuotingSafety.Boomerang || o.safety === Models.QuotingSafety.AK47);
    this.headerNameMod = this.hasPongs ? "⇁" : "";
    this.grid.columnDefs.map((r: ColDef) => {
      if (['Ktime','Kqty','Kprice','Kvalue','delta'].indexOf(r.field) > -1)
        this.grid.columnApi.setColumnVisible(r.field, this.hasPongs);
      return r;
    });
    this.grid.api.refreshHeader();
    this.emitLengths();
  };

  private addRowData = (o: Models.Trade) => {
    if (!this.grid.api) return;
    if (o === null) {
      if (this.grid.rowData)
        this.grid.api.setRowData([]);
    } else {
      if (o.Kqty < 0) {
        this.grid.api.forEachNode((node: RowNode) => {
          if (node.data.tradeId == o.tradeId)
            this.grid.api.applyTransaction({remove: [node.data]});
        });
      } else {
        let exists: boolean = false;
        this.grid.api.forEachNode((node: RowNode) => {
          if (!exists && node.data.tradeId == o.tradeId) {
            exists = true;
            node.setData(Object.assign(node.data, {
              time: o.time,
              quantity: o.quantity,
              value: o.value,
              Ktime: o.Ktime || 0,
              Kqty: o.Kqty ? o.Kqty : null,
              Kprice: o.Kprice ? o.Kprice : null,
              Kvalue: o.Kvalue ? o.Kvalue : null,
              delta: o.delta?o.delta:null,
              side: o.Kqty >= o.quantity ? '&lrhar;' : (o.side === Models.Side.Ask ? "Ask" : "Bid"),
              pingSide: o.side == Models.Side.Ask ? "sell" : "buy",
              pongSide: o.side == Models.Side.Ask ? "buy" : "sell"
            }));
          }
        });
        if (!exists) {
          this.grid.api.applyTransaction({add: [{
            tradeId: o.tradeId,
            time: o.time,
            price: o.price,
            quantity: o.quantity,
            side: o.Kqty >= o.quantity ? '&lrhar;' : (o.side === Models.Side.Ask ? "Ask" : "Bid"),
            pingSide: o.side == Models.Side.Ask ? "sell" : "buy",
            pongSide: o.side == Models.Side.Ask ? "buy" : "sell",
            value: o.value,
            Ktime: o.Ktime || 0,
            Kqty: o.Kqty ? o.Kqty : null,
            Kprice: o.Kprice ? o.Kprice : null,
            Kvalue: o.Kvalue ? o.Kvalue : null,
            delta: o.delta && o.delta!=0 ? o.delta : null,
            quoteSymbol: this.product.quote.replace('EUR','€').replace('USD','$'),
            productFixedPrice: this.product.tickPrice,
            productFixedSize: this.product.tickSize
          }]});
        }
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

      this.grid.api.sizeColumnsToFit();
      this.emitLengths();
    }
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
