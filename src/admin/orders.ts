/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';
import {GridOptions, ColDef, RowNode} from "ag-grid/main";
import moment = require('moment');

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory, FireFactory} from './shared_directives';

class DisplayOrderStatusReport {
  orderId: string;
  time: moment.Moment;
  orderStatus: string;
  price: number;
  quantity: number;
  value: number;
  side: string;
  orderType: string;
  tif: string;
  computationalLatency: number;
  lastQuantity: number;
  lastPrice: number;
  leavesQuantity: number;
  cumQuantity: number;
  averagePrice: number;
  liquidity: string;
  rejectMessage: string;
  version: number;
  trackable: string;

  constructor(
    public osr: Models.OrderStatusReport,
    private _fireCxl: Messaging.IFire<Models.OrderStatusReport>
  ) {
    this.orderId = osr.orderId;
    this.side = Models.Side[osr.side];
    this.updateWith(osr);
  }

  public updateWith = (osr: Models.OrderStatusReport) => {
    this.time = (moment.isMoment(osr.time) ? osr.time : moment(osr.time));
    this.orderStatus = DisplayOrderStatusReport.getOrderStatus(osr);
    this.price = osr.price;
    this.quantity = osr.quantity;
    this.value = Math.round(osr.price * osr.quantity * 100) / 100;
    this.orderType = Models.OrderType[osr.type];
    this.tif = Models.TimeInForce[osr.timeInForce];
    this.computationalLatency = osr.computationalLatency;
    this.lastQuantity = osr.lastQuantity;
    this.lastPrice = osr.lastPrice;
    this.leavesQuantity = osr.leavesQuantity;
    this.cumQuantity = osr.cumQuantity;
    this.averagePrice = osr.averagePrice;
    this.liquidity = Models.Liquidity[osr.liquidity];
    this.rejectMessage = osr.rejectMessage;
    this.version = osr.version;
    this.trackable = osr.orderId + ":" + osr.version;
  };

  private static getOrderStatus(o: Models.OrderStatusReport): string {
    var endingModifier = (o: Models.OrderStatusReport) => {
      if (o.pendingCancel)
        return ", PndCxl";
      else if (o.pendingReplace)
        return ", PndRpl";
      else if (o.partiallyFilled)
        return ", PartFill";
      else if (o.cancelRejected)
        return ", CxlRj";
      return "";
    };
    return Models.OrderStatus[o.orderStatus] + endingModifier(o);
  }

  public cancel = () => {
    this._fireCxl.fire(this.osr);
  };
}

@Component({
  selector: 'order-list',
  template: `<ag-grid-ng2 #orderList class="ag-fresh ag-dark" style="height: 187px;width: 99.99%;" rowHeight="21" [gridOptions]="gridOptions" (cellClicked)="onCellClicked($event)"></ag-grid-ng2>`
})
export class OrdersComponent implements OnInit, OnDestroy {

  private gridOptions: GridOptions = <GridOptions>{};

  private fireCxl: Messaging.IFire<Models.OrderStatusReport>;
  private subscriberOSR: Messaging.ISubscribe<Models.OrderStatusReport>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory,
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.gridOptions.rowData = [];
    this.gridOptions.enableSorting = true;
    this.gridOptions.columnDefs = this.createColumnDefs();
    this.gridOptions.overlayNoRowsTemplate = `<span class="ag-overlay-no-rows-center">not trading</span>`;

    this.fireCxl = this.fireFactory
      .getFire(Messaging.Topics.CancelOrder);

    this.subscriberOSR = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.OrderStatusReports)
      .registerDisconnectedHandler(() => this.gridOptions.rowData.length = 0)
      .registerSubscriber(this.addRowData, os => os.forEach(this.addRowData));
  }

  ngOnDestroy() {
    this.subscriberOSR.disconnect();
  }

  private createColumnDefs = (): ColDef[] => {
    return [
      { width: 82, field: 'time', headerName: 'time', cellRenderer:(params) => {
          return (params.value) ? Models.toShortTimeString(params.value) : '';
        },
        comparator: (a: moment.Moment, b: moment.Moment) => a.diff(b),
        cellClass: (params) => {
          return 'fs11px'+(Math.abs(moment.utc().valueOf() - params.data.time.valueOf()) > 7000 ? " text-muted" : "");
      }  },
      { width: 90, field: 'orderId', headerName: 'id' },
      { width: 35, field: 'computationalLatency', headerName: 'lat'},
      // { width: 35, field: 'version', headerName: 'v' },
      { width: 110, field: 'orderStatus', headerName: 'status', cellClass: (params) => {
        return params.data.orderStatus == "New" ? "text-muted" : "";
      }  },
      { width: 40, field: 'side', headerName: 'side' , cellClass: (params) => {
        if (params.value === 'Bid') return 'buy';
        else if (params.value === 'Ask') return "sell";
      }},
      { width: 60, field: 'leavesQuantity', headerName: 'lvQty', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRenderer:(params) => {
        return params.value?new Intl.NumberFormat('en-US', {  maximumFractionDigits: 3, minimumFractionDigits: 3 }).format(params.value):'';
      }},
      { width: 65, field: 'price', headerName: 'px',
      sort: 'desc',  cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRenderer:(params) => {
        return params.value?new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD',  maximumFractionDigits: 2, minimumFractionDigits: 2 }).format(params.value):'';
      } },
      { width: 60, field: 'quantity', headerName: 'qty', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRenderer:(params) => {
        return new Intl.NumberFormat('en-US', {  maximumFractionDigits: 3, minimumFractionDigits: 3 }).format(params.value);
      }},
      { width: 60, field: 'value', headerName: 'value', cellClass: (params) => {
        return (params.data.side === 'Ask') ? "sell" : "buy";
      }, cellRenderer:(params) => {
        return params.value?new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD',  maximumFractionDigits: 2, minimumFractionDigits: 2 }).format(params.value):'';
      }},
      { width: 45, field: 'orderType', headerName: 'type' },
      { width: 50, field: 'tif', headerName: 'tif' },
      // { width: 60, field: 'lastQuantity', headerName: 'lQty' },
      // { width: 65, field: 'lastPrice', headerName: 'lPx', cellRenderer:(params) => {
        // return params.value?new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD',  maximumFractionDigits: 2, minimumFractionDigits: 2 }).format(params.value):'';
      // } },
      // { width: 60, field: 'cumQuantity', headerName: 'cum' },
      // { width: 65, field: 'averagePrice', headerName: 'avg', cellRenderer:(params) => {
        // return params.value?new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD',  maximumFractionDigits: 2, minimumFractionDigits: 2 }).format(params.value):'';
      // } },
      // { width: 40, field: 'liquidity', headerName: 'liq' },
      // { minWidth: 69, field: 'rejectMessage', headerName: 'msg' },
      { width: 40, field: "cancel", headerName: 'cxl', cellRenderer: (params) => {
        return '<button type="button" class="btn btn-danger btn-xs"><span data-action-type="remove" class="glyphicon glyphicon-remove"></span></button>';
      } },
    ];
  }

  public onCellClicked = ($event) => {
    if ($event.event.target.getAttribute("data-action-type")=='remove')
      $event.data.cancel();
  }

  private addRowData = (o: Models.OrderStatusReport) => {
    let exists: boolean = false;
    this.gridOptions.api.forEachNode((node: RowNode) => {
      if (!exists && node.data.orderId==o.orderId) {
        if (o.leavesQuantity) {
          if (node.data.version < o.version)
            node.data.updateWith(o);
        } else this.gridOptions.api.removeItems([node]);
        exists = true;
      }
    });
    if (!exists && (o.leavesQuantity || o.orderStatus == Models.OrderStatus.New))
      this.gridOptions.api.addItems([new DisplayOrderStatusReport(o, this.fireCxl)]);
  }
}

