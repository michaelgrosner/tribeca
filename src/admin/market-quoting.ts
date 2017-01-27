/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-quoting',
  template: `<div class="tradeSafety2 img-rounded" style="padding-top:0px;padding-right:0px;"><div style="padding-top:0px;padding-right:0px;">
      Market Width: <span class="{{ diffMD ? \'text-danger\' : \'text-muted\' }}">{{ diffMD | number:'1.2-2' }}</span>,
      Quote Width: <span class="{{ diffPx ? \'text-danger\' : \'text-muted\' }}">{{ diffPx | number:'1.2-2' }}</span>,
      Wallet TBP: <span class="text-danger">{{ targetBasePosition | number:'1.3-3' }}</span>
      </div></div><div style="padding-right:4px;padding-left:4px;padding-top:4px;"><table class="marketQuoting table table-hover table-bordered table-responsive text-center">
      <tr class="active">
        <th></th>
        <th>bidSz&nbsp;</th>
        <th>bidPx</th>
        <th>askPx</th>
        <th>askSz&nbsp;</th>
      </tr>
      <tr class="info">
        <td class="text-left">quote</td>
        <td [ngClass]="bidIsLive ? 'text-danger' : 'text-muted'">{{ qBidSz | number:'1.3-3' }}</td>
        <td [ngClass]="bidIsLive ? 'text-danger' : 'text-muted'">{{ qBidPx | number:'1.2-2' }}</td>
        <td [ngClass]="askIsLive ? 'text-danger' : 'text-muted'">{{ qAskPx | number:'1.2-2' }}</td>
        <td [ngClass]="askIsLive ? 'text-danger' : 'text-muted'">{{ qAskSz | number:'1.3-3' }}</td>
      </tr>
      <tr class="active" *ngFor="let level of levels; let i = index">
        <td class="text-left">mkt{{ i }}</td>
        <td [ngClass]="level.bidClass"><div [ngClass]="level.bidClassVisual">&nbsp;</div><div style="z-index:2;position:relative;">{{ level.bidSize | number:'1.3-3' }}</div></td>
        <td [ngClass]="level.bidClass">{{ level.bidPrice | number:'1.2-2' }}</td>
        <td [ngClass]="level.askClass">{{ level.askPrice | number:'1.2-2' }}</td>
        <td [ngClass]="level.askClass"><div [ngClass]="level.askClassVisual">&nbsp;</div><div style="z-index:2;position:relative;">{{ level.askSize | number:'1.3-3' }}</div></td>
      </tr>
    </table></div>`
})
export class MarketQuotingComponent implements OnInit, OnDestroy {

  public levels: any[];
  public qBidSz: number;
  public qBidPx: number;
  public qAskPx: number;
  public qAskSz: number;
  public order_classes: any[];
  public bidIsLive: boolean;
  public askIsLive: boolean;
  public diffMD: number;
  public diffPx: number;
  private targetBasePosition: number;

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
    makeSubscriber<Models.TargetBasePositionValue>(Messaging.Topics.TargetBasePosition, this.updateTargetBasePosition, this.clearTargetBasePosition);
  }

  ngOnDestroy() {
    this.subscribers.forEach(d => d.disconnect());
  }

  private clearMarket = () => {
    this.levels = [];
  }

  private clearTargetBasePosition = () => {
    this.targetBasePosition = null;
  }

  private clearQuote = () => {
    this.order_classes = [];
  }

  private clearQuoteStatus = () => {
    this.bidIsLive = false;
    this.askIsLive = false;
  }

  private updateTargetBasePosition = (value : Models.TargetBasePositionValue) => {
    if (value == null) return;
    this.targetBasePosition = value.data;
  }

  private updateMarket = (update: Models.Timestamped<any[]>) => {
    if (update == null) {
      this.clearMarket();
      return;
    }

    let price: number = 0;
    for (let i: number = 0, j: number = 0; i < update.data[1].length; i++, j++) {
      if (j >= this.levels.length)
        this.levels[j] = <any>{};
      this.levels[j].askPrice = price + update.data[1][i] / 1e1;
      price = this.levels[j].askPrice;
      this.levels[j].askSize = update.data[1][++i] / 1e1;
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
    for (let i: number = 0, j: number = 0; i < update.data[0].length; i++, j++) {
      if (j >= this.levels.length)
        this.levels[j] = <any>{};
      this.levels[j].bidPrice = Math.abs(price - update.data[0][i] / 1e1);
      price = this.levels[j].bidPrice;
      this.levels[j].bidSize = update.data[0][++i] / 1e1;
      if (j==0) this.diffMD = this.levels[j].askPrice - this.levels[j].bidPrice;
      else if (j==1) this.diffPx = (this.qAskPx && this.qBidPx) ? this.qAskPx - this.qBidPx : 0;
    }

    this.updateQuoteClass();
  }

  private updateQuote = (o: Models.Timestamped<any[]>) => {
    if (o.data[1] == Models.OrderStatus.Cancelled
      || o.data[1] == Models.OrderStatus.Complete
      || o.data[1] == Models.OrderStatus.Rejected
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
}
