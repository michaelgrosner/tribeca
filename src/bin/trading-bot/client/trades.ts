import {Component, Inject, EventEmitter, Input, Output, OnInit} from '@angular/core';
import {Module, ClientSideRowModelModule, GridOptions, ColDef, RowNode} from '@ag-grid-community/all-modules';

import * as Models from './models';
import * as Subscribe from './subscribe';
import {FireFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'trade-list',
  template: `<ag-grid-angular #tradeList class="ag-theme-fresh ag-theme-dark" style="height: 361px;width: 99.80%;" rowHeight="21" [gridOptions]="gridOptions" [modules]="modules" (cellClicked)="onCellClicked($event)"></ag-grid-angular>`
})
export class TradesComponent implements OnInit {

  private modules: Module[] = [ClientSideRowModelModule];

  private gridOptions: GridOptions = <GridOptions>{};

  private fireCxl: Subscribe.IFire<object>;

  public audio: boolean;

  public hasPongs: boolean;

  public headerNameMod: string = "";

  private sortTimeout: number;

  @Input() product: Models.ProductAdvertisement;

  @Input() set setQuotingParameters(o: Models.QuotingParameters) {
    this.audio = o.audio;
    if (!this.gridOptions.api) return;
    this.hasPongs = (o.safety === Models.QuotingSafety.Boomerang || o.safety === Models.QuotingSafety.AK47);
    this.headerNameMod = this.hasPongs ? "Ping" : "";
    this.gridOptions.columnDefs.map((r: ColDef) => {
      if (['Kqty','Kprice','Kvalue','Kdiff','Ktime'].indexOf(r.field) > -1)
        this.gridOptions.columnApi.setColumnVisible(r.field, this.hasPongs);
      return r;
    });
    this.gridOptions.api.refreshHeader();
    this.emitLengths();
  }

  @Input() set setTrade(o: Models.Trade) {
    if (o === null) {
      if (this.gridOptions.rowData)
        this.gridOptions.api.setRowData([]);
    }
    else this.addRowData(o);
  }

  @Output() onTradesLength = new EventEmitter<number>();

  @Output() onTradesMatchedLength = new EventEmitter<number>();

  @Output() onTradesChartData = new EventEmitter<Models.TradeChart>();

  constructor(
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.defaultColDef = { sortable: true, resizable: true };
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty history of trades</span>`;

    this.fireCxl = this.fireFactory
      .getFire(Models.Topics.CleanTrade);
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      {width: 30, suppressSizeToFit: true, field: "cancel", headerName: 'cxl', cellRenderer: (params) => {
        return '<button type="button" class="btn btn-danger btn-xs"><span data-action-type="remove" style="font-size: 16px;font-weight: bold;padding: 0px;line-height: 12px;">&times;</span></button>';
      } },
      {width: 95, suppressSizeToFit: true, field:'time', headerValueGetter:(params) => { return 'time' + this.headerNameMod; }, cellRenderer:(params) => {
        var d = new Date(params.value||0);
        return (d.getDate()+'').padStart(2, "0")+'/'+((d.getMonth()+1)+'').padStart(2, "0")+' '+(d.getHours()+'').padStart(2, "0")+':'+(d.getMinutes()+'').padStart(2, "0")+':'+(d.getSeconds()+'').padStart(2, "0");
      }, cellClass: 'fs11px', sort: 'desc', comparator: (valueA: any, valueB: any, nodeA: RowNode, nodeB: RowNode, isInverted: boolean) => {
          return (nodeA.data.Ktime||nodeA.data.time) - (nodeB.data.Ktime||nodeB.data.time);
      }},
      {width: 95, suppressSizeToFit: true, field:'Ktime', hide:true, headerName:'timePong', cellRenderer:(params) => {
        if (params.value==0) return '';
        var d = new Date(params.value);
        return (d.getDate()+'').padStart(2, "0")+'/'+((d.getMonth()+1)+'').padStart(2, "0")+' '+(d.getHours()+'').padStart(2, "0")+':'+(d.getMinutes()+'').padStart(2, "0")+':'+(d.getSeconds()+'').padStart(2, "0");
      }, cellClass: 'fs11px' },
      {width: 40, suppressSizeToFit: true, field:'side', headerName:'side', cellClass: (params) => {
        if (params.value === 'Buy') return 'buy';
        else if (params.value === 'Sell') return "sell";
        else if (params.value === 'K') return "kira";
        else return "unknown";
      }},
      {width: 80, field:'price', headerValueGetter:(params) => { return 'px' + this.headerNameMod; }, cellClass: (params) => {
        return params.data.pingSide;
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 85, suppressSizeToFit: true, field:'quantity', headerValueGetter:(params) => { return 'qty' + this.headerNameMod; }, cellClass: (params) => {
        return params.data.pingSide;
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 69, field:'value', headerValueGetter:(params) => { return 'val' + this.headerNameMod; }, cellClass: (params) => {
        return params.data.pingSide;
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 75, field:'Kvalue', headerName:'valPong', hide:true, cellClass: (params) => {
        return params.data.pongSide;
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 85, suppressSizeToFit: true, field:'Kqty', headerName:'qtyPong', hide:true, cellClass: (params) => {
        return params.data.pongSide;
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 80, field:'Kprice', headerName:'pxPong', hide:true, cellClass: (params) => {
        return params.data.pongSide;
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'Kdiff', headerName:'Kdiff', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return "kira"; else return "";
      }, cellRenderer: (params) => {
        return (!params.value) ? "" : params.data.quoteSymbol + parseFloat(params.value.toFixed(8));
      }}
    ];
  }

  public onCellClicked = ($event) => {
    if ($event.event.target.getAttribute("data-action-type")!='remove') return;
    this.fireCxl.fire({
      tradeId: $event.data.tradeId
    });
  }

  private addRowData = (t: Models.Trade) => {
    if (!this.gridOptions.api || this.product.base == null) return;
    if (t.Kqty<0) {
      this.gridOptions.api.forEachNode((node: RowNode) => {
        if (node.data.tradeId==t.tradeId)
          this.gridOptions.api.updateRowData({remove:[node.data]});
      });
    } else {
      let exists: boolean = false;
      this.gridOptions.api.forEachNode((node: RowNode) => {
        if (!exists && node.data.tradeId==t.tradeId) {
          exists = true;
          if (t.Ktime && <any>t.Ktime=='Invalid date') t.Ktime = null;
          node.setData(Object.assign(node.data, {
            time: t.time,
            quantity: t.quantity,
            value: t.value,
            Ktime: t.Ktime || 0,
            Kqty: t.Kqty ? t.Kqty : null,
            Kprice: t.Kprice ? t.Kprice : null,
            Kvalue: t.Kvalue ? t.Kvalue : null,
            Kdiff: t.Kdiff?t.Kdiff:null,
            side: t.Kqty >= t.quantity ? 'K' : (t.side === Models.Side.Ask ? "Sell" : "Buy"),
            pingSide: t.side == Models.Side.Ask ? "sell" : "buy",
            pongSide: t.side == Models.Side.Ask ? "buy" : "sell"
          }));
          if (t.loadedFromDB === false) {
            if (this.sortTimeout) window.clearTimeout(this.sortTimeout);
            this.sortTimeout = window.setTimeout(() => {
              this.gridOptions.api.setSortModel([{colId: 'time', sort: 'desc'}]);
              setTimeout(()=>{try{this.gridOptions.api.redrawRows();}catch(e){}},0);
            }, 269);
          }
        }
      });
      if (!exists) {
        if (t.Ktime && <any>t.Ktime=='Invalid date') t.Ktime = null;
        this.gridOptions.api.updateRowData({add:[{
          tradeId: t.tradeId,
          time: t.time,
          price: t.price,
          quantity: t.quantity,
          side: t.Kqty >= t.quantity ? 'K' : (t.side === Models.Side.Ask ? "Sell" : "Buy"),
          pingSide: t.side == Models.Side.Ask ? "sell" : "buy",
          pongSide: t.side == Models.Side.Ask ? "buy" : "sell",
          value: t.value,
          Ktime: t.Ktime || 0,
          Kqty: t.Kqty ? t.Kqty : null,
          Kprice: t.Kprice ? t.Kprice : null,
          Kvalue: t.Kvalue ? t.Kvalue : null,
          Kdiff: t.Kdiff && t.Kdiff!=0 ? t.Kdiff : null,
          quoteSymbol: this.product.quote.replace('EUR','€').replace('USD','$'),
          productFixedPrice: this.product.tickPrice,
          productFixedSize: this.product.tickSize
        }]});
      }
      if (t.loadedFromDB === false) {
        if (this.audio) {
          var audio = new Audio('audio/'+(t.isPong?'1':'0')+'.mp3');
          audio.volume = 0.5;
          audio.play();
        }
        this.onTradesChartData.emit(new Models.TradeChart(
          (t.isPong && t.Kprice)?t.Kprice:t.price,
          (t.isPong && t.Kprice)?(t.side === Models.Side.Ask ? Models.Side.Bid : Models.Side.Ask):t.side,
          (t.isPong && t.Kprice)?t.Kqty:t.quantity,
          (t.isPong && t.Kprice)?t.Kvalue:t.value,
          t.isPong
        ));
      }
    }

    this.gridOptions.api.sizeColumnsToFit();
    this.emitLengths();
  }

  private emitLengths = () => {
    this.onTradesLength.emit(this.gridOptions.api.getModel().getRowCount());
    var tradesMatched: number = 0;
    if (this.hasPongs) {
      this.gridOptions.api.forEachNode((node: RowNode) => {
        if (node.data.Kqty) tradesMatched++;
      });
    } else tradesMatched = -1;
    this.onTradesMatchedLength.emit(tradesMatched);
  }
}
