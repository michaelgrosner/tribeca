/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-quoting',
  template: `<table class="marketQuoting table table-hover table-bordered table-condensed table-responsive text-center">
      <tr class="active">
        <th></th>
        <th>bidSz&nbsp;</th>
        <th>bidPx</th>
        <th>FV</th>
        <th>askPx</th>
        <th>askSz&nbsp;</th>
      </tr>
      <tr class="info">
        <td class="text-left">q</td>
        <td [ngClass]="bidIsLive ? 'text-danger' : 'text-muted'">{{ qBidSz | number:'1.3-3' }}</td>
        <td [ngClass]="bidIsLive ? 'text-danger' : 'text-muted'">{{ qBidPx | number:'1.2-2' }}</td>
        <td class="fairvalue">{{ fairValue | number:'1.2-2' }}</td>
        <td [ngClass]="askIsLive ? 'text-danger' : 'text-muted'">{{ qAskPx | number:'1.2-2' }}</td>
        <td [ngClass]="askIsLive ? 'text-danger' : 'text-muted'">{{ qAskSz | number:'1.3-3' }}</td>
      </tr>
      <tr class="active" *ngFor="let level of levels; let i = index">
        <td class="text-left">mkt{{ i }}</td>
        <td [ngClass]="level.bidClass"><div [ngClass]="level.bidClassVisual">&nbsp;</div><div style="z-index:2;position:relative;">{{ level.bidSize | number:'1.3-3' }}</div></td>
        <td [ngClass]="level.bidClass">{{ level.bidPrice | number:'1.2-2' }}</td>
        <td><span *ngIf="level.diffWidth > 0">{{ level.diffWidth | number:'1.2-2' }}</span></td>
        <td [ngClass]="level.askClass">{{ level.askPrice | number:'1.2-2' }}</td>
        <td [ngClass]="level.askClass"><div [ngClass]="level.askClassVisual">&nbsp;</div><div style="z-index:2;position:relative;">{{ level.askSize | number:'1.3-3' }}</div></td>
      </tr>
    </table>`
})
export class MarketQuotingComponent implements OnInit, OnDestroy {

  public levels: any[];
  public fairValue: number;
  public qBidSz: number;
  public qBidPx: number;
  public qAskPx: number;
  public qAskSz: number;
  public order_classes: any[];
  public bidIsLive: boolean;
  public askIsLive: boolean;

  private subscribers: Messaging.ISubscribe<any>[] = [];

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {
    this.clearMarket();
    this.clearQuote();
  }

  ngOnInit() {
    var makeSubscriber = <T>(topic: string, updateFn, clearFn) => {
      this.subscribers.push(
        this.subscriberFactory.getSubscriber<T>(this.zone, topic)
          .registerSubscriber(updateFn, ms => ms.forEach(updateFn))
          .registerDisconnectedHandler(clearFn)
      );
    };

    makeSubscriber<Models.Timestamped<any[]>>(Messaging.Topics.MarketData, this.updateMarket, this.clearMarket);
    makeSubscriber<Models.Timestamped<any[]>>(Messaging.Topics.OrderStatusReports, this.updateQuote, this.clearQuote);
    makeSubscriber<Models.TwoSidedQuoteStatus>(Messaging.Topics.QuoteStatus, this.updateQuoteStatus, this.clearQuoteStatus);
    makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue, this.updateFairValue, this.clearFairValue);
  }

  ngOnDestroy() {
    this.subscribers.forEach(d => d.disconnect());
  }

  private clearMarket = () => {
    this.levels = [];
  }

  private clearQuote = () => {
    this.order_classes = [];
  }

  private clearFairValue = () => {
    this.fairValue = null;
  }

  private clearQuoteStatus = () => {
    this.bidIsLive = false;
    this.askIsLive = false;
  }

  private updateMarket = (update: Models.Timestamped<any[]>) => {
    if (update == null) {
      this.clearMarket();
      return;
    }

    let price: number = 0;
    for (var i = 0; i < update.data[1].length; i++) {
      if (i >= this.levels.length)
        this.levels[i] = <any>{};
      this.levels[i].askPrice = price + update.data[1][i][0];
      price = this.levels[i].askPrice;
      this.levels[i].askSize = update.data[1][i][1];
    }

    if (this.order_classes.length) {
      var bids = this.order_classes.filter(o => o.side === Models.Side.Bid);
      var asks = this.order_classes.filter(o => o.side === Models.Side.Ask);
      if (bids.length) {
        var bid = bids.reduce(function(a,b){return a.price>b.price?a:b;});
        this.qBidPx = bid.price;
        this.qBidSz = bid.quantity;
      }
      if (asks.length) {
        var ask = asks.reduce(function(a,b){return a.price<b.price?a:b;});
        this.qAskPx = ask.price;
        this.qAskSz = ask.quantity;
      }
    }

    price = 0;
    for (var i = 0; i < update.data[0].length; i++) {
      if (i >= this.levels.length)
        this.levels[i] = <any>{};
      this.levels[i].bidPrice = Math.abs(price - update.data[0][i][0]);
      price = this.levels[i].bidPrice;
      this.levels[i].bidSize = update.data[0][i][1];
      this.levels[i].diffWidth = i==0
        ? this.levels[i].askPrice - this.levels[i].bidPrice : (
          (i==1 && this.qAskPx && this.qBidPx)
            ? this.qAskPx - this.qBidPx : 0
        );
    }

    this.updateQuoteClass();
  }

  private updateQuote = (o: Models.Timestamped<any[]>) => {
    if (o.data[11] == Models.OrderStatus.Cancelled
      || o.data[11] == Models.OrderStatus.Complete
      || o.data[11] == Models.OrderStatus.Rejected
    ) this.order_classes = this.order_classes.filter(x => x.orderId !== o.data[0]);
    else if (!this.order_classes.filter(x => x.orderId === o.data[0]).length)
      this.order_classes.push({
        orderId: o.data[0],
        side: o.data[5],
        quantity: o.data[4],
        price: o.data[3]
      });

    this.updateQuoteClass();
  }

  private updateQuoteStatus = (status: Models.TwoSidedQuoteStatus) => {
    if (status == null) {
      this.clearQuoteStatus();
      return;
    }

    this.bidIsLive = (status.bidStatus === Models.QuoteStatus.Live);
    this.askIsLive = (status.askStatus === Models.QuoteStatus.Live);
    this.updateQuoteClass();
  }

  private updateQuoteClass = () => {
    if (this.levels && this.levels.length > 0) {
      var tol = 5e-3;
      for (var i = 0; i < this.levels.length; i++) {
        var level = this.levels[i];
        level.bidClass = 'active ';
        var bids = this.order_classes.filter(o => o.side === Models.Side.Bid);
        for (var j = 0; j < bids.length; j++)
          if (Math.abs(bids[j].price - level.bidPrice) < tol)
            level.bidClass = 'success buy ';
        level.bidClassVisual = String('vsBuy visualSize').concat(<any>Math.round(Math.max(Math.min((Math.log(level.bidSize)/Math.log(2))*4,19),1)));
        level.askClass = 'active ';
        var asks = this.order_classes.filter(o => o.side === Models.Side.Ask);
        for (var j = 0; j < asks.length; j++)
          if (Math.abs(asks[j].price - level.askPrice) < tol)
            level.askClass = 'success sell ';
        level.askClassVisual = String('vsAsk visualSize').concat(<any>Math.round(Math.max(Math.min((Math.log(level.askSize)/Math.log(2))*4,19),1)));
      }
    }
  }

  private updateFairValue = (fv: Models.FairValue) => {
    if (fv == null) {
      this.clearFairValue();
      return;
    }

    this.fairValue = fv.price;
  }
}
