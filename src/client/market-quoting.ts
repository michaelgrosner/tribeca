import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';
import moment = require('moment');

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'market-quoting',
  template: `<div class="tradeSafety2 img-rounded" style="padding-top:0px;padding-right:0px;"><div style="padding-top:0px;padding-right:0px;">
      Market Width: <span class="{{ diffMD ? \'text-danger\' : \'text-muted\' }}">{{ diffMD | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      Quote Width: <span class="{{ diffPx ? \'text-danger\' : \'text-muted\' }}">{{ diffPx | number:'1.'+product.fixed+'-'+product.fixed }}</span>,&nbsp;
      <span style="z-index:1;position:absolute;white-space:pre;">Wallet TBP: <span class="text-danger">{{ targetBasePosition | number:'1.3-3' }}</span></span>
      </div></div><div style="padding-right:4px;padding-left:4px;padding-top:4px;"><table class="marketQuoting table table-hover table-bordered table-responsive text-center">
      <tr class="active">
        <th style="width:62px;">apr<span class="{{ sideAPRSafety!=\'Off\' ? \'text-danger\' : \'text-muted\' }}">{{ sideAPRSafety }}</span></th>
        <th>bidSize&nbsp;</th>
        <th>bidPrice</th>
        <th>askPrice</th>
        <th>askSize&nbsp;</th>
      </tr>
      <tr class="info">
        <td class="text-left">quote</td>
        <td [ngClass]="bidIsLive ? 'text-danger' : 'text-muted'">{{ qBidSz | number:'1.4-4' }}</td>
        <td [ngClass]="bidIsLive ? 'text-danger' : 'text-muted'">{{ qBidPx | number:'1.'+product.fixed+'-'+product.fixed }}</td>
        <td [ngClass]="askIsLive ? 'text-danger' : 'text-muted'">{{ qAskPx | number:'1.'+product.fixed+'-'+product.fixed }}</td>
        <td [ngClass]="askIsLive ? 'text-danger' : 'text-muted'">{{ qAskSz | number:'1.4-4' }}</td>
      </tr>
      <tr class="active" *ngFor="let level of levels; let i = index">
        <td class="text-left">mkt{{ i }}</td>
        <td [ngClass]="level.bidClass"><div [ngClass]="level.bidClassVisual">&nbsp;</div><div style="z-index:2;position:relative;">{{ level.bidSize | number:'1.4-4' }}</div></td>
        <td [ngClass]="level.bidClass">{{ level.bidPrice | number:'1.'+product.fixed+'-'+product.fixed }}</td>
        <td [ngClass]="level.askClass">{{ level.askPrice | number:'1.'+product.fixed+'-'+product.fixed }}</td>
        <td [ngClass]="level.askClass"><div [ngClass]="level.askClassVisual">&nbsp;</div><div style="z-index:2;position:relative;">{{ level.askSize | number:'1.4-4' }}</div></td>
      </tr>
    </table></div>`
})
export class MarketQuotingComponent implements OnInit {

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
  private sideAPRSafety: string;
  @Input() product: Models.ProductState;

  @Input() set connected(connected: boolean) {
    if (connected) return;
    this.clearQuote();
    this.updateQuoteClass();
  }

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {
    this.clearMarket();
    this.clearQuote();
  }

  ngOnInit() {
    [
      [Models.Topics.MarketData, this.updateMarket, this.clearMarket],
      [Models.Topics.OrderStatusReports, this.updateQuote, this.clearQuote],
      [Models.Topics.QuoteStatus, this.updateQuoteStatus, this.clearQuoteStatus],
      [Models.Topics.TargetBasePosition, this.updateTargetBasePosition, this.clearTargetBasePosition]
    ].forEach(x => (<T>(topic: string, updateFn, clearFn) => {
      this.subscriberFactory
        .getSubscriber<T>(this.zone, topic)
        .registerConnectHandler(clearFn)
        .registerSubscriber(updateFn);
    }).call(this, x[0], x[1], x[2]));
  }

  private clearMarket = () => {
    this.levels = [];
  }

  private clearTargetBasePosition = () => {
    this.targetBasePosition = null;
    this.sideAPRSafety = null;
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
    this.sideAPRSafety = value.sideAPR.length ? value.sideAPR.join(', ') : 'Off';
  }

  private updateMarket = (update: Models.Timestamped<any[]>) => {
    if (update == null) {
      this.clearMarket();
      return;
    }

    var bids = this.order_classes.filter(o => o.side === Models.Side.Bid);
    var asks = this.order_classes.filter(o => o.side === Models.Side.Ask);
    for (let i: number = 0; i < asks.length; i++)
      if (!update.data[1].filter(x => x===asks[i].price).length) {
        for (var j: number = 0; j < update.data[1].length;j++)
          if (update.data[1][j++]>asks[i].price) break;
        update.data[1].splice(j-(j==update.data[1].length?0:1), 0, asks[i].price, asks[i].quantity);
        update.data[1] = update.data[1].slice(0, -2);
      }
    for (let i: number = 0; i < bids.length; i++)
      if (!update.data[0].filter(x => x===bids[i].price).length) {
        for (var j: number = 0; j < update.data[0].length;j++)
          if (update.data[0][j++]<bids[i].price) break;
        update.data[0].splice(j-(j==update.data[0].length?0:1), 0, bids[i].price, bids[i].quantity);
        update.data[0] = update.data[0].slice(0, -2);
      }

    for (let i: number = 0, j: number = 0; i < update.data[1].length; i++, j++) {
      if (j >= this.levels.length) this.levels[j] = <any>{};
      this.levels[j] = { askPrice: update.data[1][i], askSize: update.data[1][++i] };
    }

    if (bids.length) {
      var bid = bids.reduce(function(a,b){return a.price>b.price?a:b;});
      this.qBidPx = bid.price;
      this.qBidSz = bid.quantity;
    } else {
      this.qBidPx = null;
      this.qBidSz = null;
    }
    if (asks.length) {
      var ask = asks.reduce(function(a,b){return a.price<b.price?a:b;});
      this.qAskPx = ask.price;
      this.qAskSz = ask.quantity;
    } else {
      this.qAskPx = null;
      this.qAskSz = null;
    }

    for (let i: number = 0, j: number = 0; i < update.data[0].length; i++, j++) {
      if (j >= this.levels.length) this.levels[j] = <any>{};
      this.levels[j] = Object.assign(this.levels[j], { bidPrice: update.data[0][i], bidSize: update.data[0][++i] });
      if (j==0) this.diffMD = this.levels[j].askPrice - this.levels[j].bidPrice;
      else if (j==1) this.diffPx = Math.max((this.qAskPx && this.qBidPx) ? this.qAskPx - this.qBidPx : 0, 0);
    }

    this.updateQuoteClass();
  }

  private updateQuote = (o: Models.Timestamped<any[]>) => {
    if (typeof o.data[0] === 'object') {
      this.clearQuote();
      return o.data.forEach(x => setTimeout(this.updateQuote(x), 0));
    }
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
      for (var i = 0; i < this.levels.length; i++) {
        var level = this.levels[i];
        level.bidClass = 'active ';
        var bids = this.order_classes.filter(o => o.side === Models.Side.Bid);
        for (var j = 0; j < bids.length; j++)
          if (bids[j].price === level.bidPrice)
            level.bidClass = 'success buy ';
        level.bidClassVisual = String('vsBuy visualSize').concat(<any>Math.round(Math.max(Math.min((Math.log(level.bidSize)/Math.log(2))*4,19),1)));
        level.askClass = 'active ';
        var asks = this.order_classes.filter(o => o.side === Models.Side.Ask);
        for (var j = 0; j < asks.length; j++)
          if (asks[j].price === level.askPrice)
            level.askClass = 'success sell ';
        level.askClassVisual = String('vsAsk visualSize').concat(<any>Math.round(Math.max(Math.min((Math.log(level.askSize)/Math.log(2))*4,19),1)));
      }
    }
  }
}
