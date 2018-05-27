import {Component, Inject, EventEmitter, Input, Output, OnInit} from '@angular/core';
import {GridOptions, ColDef, RowNode} from 'ag-grid/main';

import * as Models from './models';
import * as Subscribe from './subscribe';
import {FireFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'trade-list',
  template: `<ag-grid-angular #tradeList class="ag-fresh ag-dark" style="height: 364px;width: 99.80%;" rowHeight="21" [gridOptions]="gridOptions" (cellClicked)="onCellClicked($event)"></ag-grid-angular>`
})
export class TradesComponent implements OnInit {

  private gridOptions: GridOptions = <GridOptions>{};

  private fireCxl: Subscribe.IFire<object>;

  public exch: Models.Exchange;
  public pair: Models.CurrencyPair;
  public audio: boolean;

  private sortTimeout: number;

  @Input() product: Models.ProductState;

  @Input() set setQuotingParameters(o: Models.QuotingParameters) {
    this.audio = o.audio;
    if (!this.gridOptions.api) return;
    var isK = (o.safety === Models.QuotingSafety.Boomerang || o.safety === Models.QuotingSafety.AK47);
    this.gridOptions.columnDefs.map((r: ColDef) => {
      ['Kqty','Kprice','Kvalue','Kdiff','Ktime',['time','timePing'],['price','pxPing'],['quantity','qtyPing'],['value','valPing']].map(t => {
        if (t[0]==r.field) r.headerName = isK ? t[1] : t[1].replace('Ping','');
        if (r.field[0]=='K') r.hide = !isK;
      });
      return r;
    });
    this.gridOptions.api.setColumnDefs(this.gridOptions.columnDefs);
  }

  @Input() set setTrade(o: Models.Trade) {
    if (o === null) {
      if (this.gridOptions.rowData)
        this.gridOptions.api.setRowData([]);
    }
    else this.addRowData(o);
  }

  @Output() onTradesLength = new EventEmitter<number>();

  constructor(
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.enableSorting = true;
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty history of trades</span>`;
    this.gridOptions.enableColResize = true;

    this.fireCxl = this.fireFactory
      .getFire(Models.Topics.CleanTrade);
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      {width: 30, suppressSizeToFit: true, field: "cancel", headerName: 'cxl', cellRenderer: (params) => {
        return '<button type="button" class="btn btn-danger btn-xs"><span data-action-type="remove" style="font-size: 16px;font-weight: bold;padding: 0px;line-height: 12px;">&times;</span></button>';
      } },
      {width: 95, suppressSizeToFit: true, field:'time', headerName:'t', cellRenderer:(params) => {
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
      {width: 80, field:'price', headerName:'px', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, suppressSizeToFit: true, field:'quantity', headerName:'qty', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 69, field:'value', headerName:'val', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 75, field:'Kvalue', headerName:'valPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, suppressSizeToFit: true, field:'Kqty', headerName:'qtyPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 80, field:'Kprice', headerName:'pxPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
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
    if (!this.gridOptions.api) return;
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
          const merged = (node.data.quantity != t.quantity);
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
            side: t.Kqty >= t.quantity ? 'K' : (t.side === Models.Side.Ask ? "Sell" : "Buy")
          }));
          if (t.loadedFromDB === false) {
            if (this.sortTimeout) window.clearTimeout(this.sortTimeout);
            this.sortTimeout = window.setTimeout(() => {
              this.gridOptions.api.setSortModel([{colId: 'time', sort: 'desc'}]);
              setTimeout(()=>{try{this.gridOptions.api.redrawRows();}catch(e){}},0);
            }, 269);
            if (this.audio) {
              var audio = new Audio('/audio/'+(merged?'0':'1')+'.mp3');
              audio.volume = 0.5;
              audio.play();
            }
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
          value: t.value,
          Ktime: t.Ktime || 0,
          Kqty: t.Kqty ? t.Kqty : null,
          Kprice: t.Kprice ? t.Kprice : null,
          Kvalue: t.Kvalue ? t.Kvalue : null,
          Kdiff: t.Kdiff && t.Kdiff!=0 ? t.Kdiff : null,
          quoteSymbol: t.pair.quote.substr(0,3).replace('USD','$').replace('EUR','€'),
          productFixed: this.product.fixed
        }]});
        if (t.loadedFromDB === false && this.audio) {
          var audio = new Audio('/audio/0.mp3');
          audio.volume = 0.5;
          audio.play();
        }
      }
    }

    this.gridOptions.api.sizeColumnsToFit();
    this.onTradesLength.emit(this.gridOptions.api.getModel().getRowCount());
  }
}
