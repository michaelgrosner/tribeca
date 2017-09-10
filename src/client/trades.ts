import {NgZone, Component, Inject, EventEmitter, Input, Output, OnInit} from '@angular/core';
import {GridOptions, ColDef, RowNode} from "ag-grid/main";
import moment = require('moment');

import Models = require('./models');
import Subscribe = require('./subscribe');
import {SubscriberFactory, FireFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';

@Component({
  selector: 'trade-list',
  template: `<ag-grid-angular #tradeList class="ag-fresh ag-dark" style="height: 159px;width: 99.99%;" rowHeight="21" [gridOptions]="gridOptions" (cellClicked)="onCellClicked($event)"></ag-grid-angular>`
})
export class TradesComponent implements OnInit {

  private gridOptions: GridOptions = <GridOptions>{};

  private fireCxl: Subscribe.IFire<object>;

  public exch: Models.Exchange;
  public pair: Models.CurrencyPair;
  public audio: boolean;

  private sortTimeout: number;

  @Input() product: Models.ProductState;

  @Output() onTradesLength = new EventEmitter<number>();

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory,
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.enableSorting = true;
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">empty history of trades</span>`;
    setTimeout(this.loadSubscriber, 3321);
  }

  private subscribed: boolean = false;
  public loadSubscriber = () => {
    if (this.subscribed) return;
    this.subscribed = true;

    this.fireCxl = this.fireFactory
      .getFire(Models.Topics.CleanTrade);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.QuotingParametersChange)
      .registerSubscriber(this.updateQP);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Trades)
      .registerConnectHandler(() => this.gridOptions.rowData.length = 0)
      .registerSubscriber(this.addRowData);
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      { width: 30, field: "cancel", headerName: 'cxl', cellRenderer: (params) => {
        return '<button type="button" class="btn btn-danger btn-xs"><span data-action-type="remove" style="font-size: 16px;font-weight: bold;padding: 0px;line-height: 12px;">&times;</span></button>';
      } },
      {width: 95, field:'time', headerName:'t', cellRenderer:(params) => {
          return (params.value) ? params.value.format('D/M HH:mm:ss') : '';
        }, cellClass: 'fs11px', comparator: (aValue: moment.Moment, bValue: moment.Moment, aNode: RowNode, bNode: RowNode) => {
          return (aNode.data.Ktime||aNode.data.time).diff(bNode.data.Ktime||bNode.data.time);
      }, sort: 'desc'},
      {width: 95, field:'Ktime', hide:true, headerName:'timePong', cellRenderer:(params) => {
          return (params.value && params.value!='Invalid date') ? params.value.format('D/M HH:mm:ss') : '';
        }, cellClass: 'fs11px' },
      {width: 40, field:'side', headerName:'side', cellClass: (params) => {
        if (params.value === 'Buy') return 'buy';
        else if (params.value === 'Sell') return "sell";
        else if (params.value === 'K') return "kira";
        else return "unknown";
      }},
      {width: 80, field:'price', headerName:'px', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'quantity', headerName:'qty', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 69, field:'value', headerName:'val', cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price > params.data.Kprice) ? "sell" : "buy"; else return params.data.side === 'Sell' ? "sell" : "buy";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 75, field:'Kvalue', headerName:'valPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'Kqty', headerName:'qtyPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: BaseCurrencyCellComponent},
      {width: 80, field:'Kprice', headerName:'pxPong', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return (params.data.price < params.data.Kprice) ? "sell" : "buy"; else return params.data.Kqty ? ((params.data.price < params.data.Kprice) ? "sell" : "buy") : "";
      }, cellRendererFramework: QuoteCurrencyCellComponent},
      {width: 65, field:'Kdiff', headerName:'Kdiff', hide:true, cellClass: (params) => {
        if (params.data.side === 'K') return "kira"; else return "";
      }, cellRendererFramework: QuoteCurrencyCellComponent}
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
            time: (moment.isMoment(t.time) ? t.time : moment(t.time)),
            quantity: t.quantity,
            value: t.value,
            Ktime: t.Ktime?(moment.isMoment(t.Ktime) ? t.Ktime : moment(t.Ktime)):null,
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
              setTimeout(()=>this.gridOptions.api.refreshView(),0);
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
          time: (moment.isMoment(t.time) ? t.time : moment(t.time)),
          price: t.price,
          quantity: t.quantity,
          side: t.Kqty >= t.quantity ? 'K' : (t.side === Models.Side.Ask ? "Sell" : "Buy"),
          value: t.value,
          Ktime: t.Ktime ? (moment.isMoment(t.Ktime) ? t.Ktime : moment(t.Ktime)) : null,
          Kqty: t.Kqty ? t.Kqty : null,
          Kprice: t.Kprice ? t.Kprice : null,
          Kvalue: t.Kvalue ? t.Kvalue : null,
          Kdiff: t.Kdiff && t.Kdiff!=0 ? t.Kdiff : null,
          quoteSymbol: Models.Currency[t.pair.quote],
          productFixed: this.product.fixed
        }]});
        if (t.loadedFromDB === false && this.audio) {
          var audio = new Audio('/audio/0.mp3');
          audio.volume = 0.5;
          audio.play();
        }
      }
    }

    this.onTradesLength.emit(this.gridOptions.api.getModel().getRowCount());
  }

  private updateQP = (qp: Models.QuotingParameters) => {
    this.audio = qp.audio;
    if (!this.gridOptions.api) return;
    var isK = (qp.mode === Models.QuotingMode.Boomerang || qp.mode === Models.QuotingMode.HamelinRat || qp.mode === Models.QuotingMode.AK47);
    this.gridOptions.columnDefs.map((r: ColDef) => {
      ['Kqty','Kprice','Kvalue','Kdiff','Ktime',['time','timePing'],['price','pxPing'],['quantity','qtyPing'],['value','valPing']].map(t => {
        if (t[0]==r.field) r.headerName = isK ? t[1] : t[1].replace('Ping','');
        if (r.field[0]=='K') r.hide = !isK;
      });
      return r;
    });
    this.gridOptions.api.setColumnDefs(this.gridOptions.columnDefs);
  }
}
