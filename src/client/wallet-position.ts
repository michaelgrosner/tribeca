import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions" *ngIf="value || quoteValue">
    <h4 class="col-md-12 col-xs-2"><small>{{ quoteCurrency }}:<br><span title="{{ quoteCurrency }} Available" class="text-danger">{{ quotePosition }}</span><br/><span title="{{ quoteCurrency }} Held" [ngClass]="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition }}</span></small></h4>
    <h4 class="col-md-12 col-xs-2"><small>{{ baseCurrency }}:<br><span title="{{ baseCurrency }} Available" class="text-danger">{{ basePosition }}</span><br/><span title="{{ baseCurrency }} Held" [ngClass]="baseHeldPosition ? 'sell' : 'text-muted'">{{ baseHeldPosition }}</span></small></h4>
    <h4 class="col-md-12 col-xs-2" style="margin-bottom: 14px!important;"><small>Value:</small><br><b title="{{ baseCurrency }} Total">{{ value }}</b><br/><b title="{{ quoteCurrency }} Total">{{ quoteValue }}</b></h4>
  </div>`
})
export class WalletPositionComponent implements OnInit {

  public baseCurrency: string;
  public basePosition: string;
  public quoteCurrency: string;
  public quotePosition: string;
  public baseHeldPosition: string;
  public quoteHeldPosition: string;
  public value: string;
  public quoteValue: string;
  @Input() product: Models.ProductState;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Position)
      .registerDisconnectedHandler(this.clearPosition)
      .registerSubscriber(this.updatePosition);
  }

  private toAmt = (a: number) : string => a.toFixed(this.product.fixed);

  private toCry = (a: number) : string => a.toFixed(8);

  private clearPosition = () => {
    this.baseCurrency = null;
    this.quoteCurrency = null;
    this.basePosition = null;
    this.quotePosition = null;
    this.baseHeldPosition = null;
    this.quoteHeldPosition = null;
    this.value = null;
    this.quoteValue = null;
  }

  private updatePosition = (o: Models.Timestamped<any[]>) => {
    this.basePosition = this.toCry(o.data[0]);
    this.quotePosition = this.toAmt(o.data[1]);
    this.baseHeldPosition = this.toCry(o.data[2]);
    this.quoteHeldPosition = this.toAmt(o.data[3]);
    this.value = this.toCry(o.data[4]);
    this.quoteValue = this.toAmt(o.data[5]);
    this.baseCurrency = Models.Currency[o.data[6]];
    this.quoteCurrency = Models.Currency[o.data[7]];
  }
}
