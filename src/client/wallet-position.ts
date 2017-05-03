import {NgZone, Component, Inject, Input, OnInit} from '@angular/core';

import Models = require('../share/models');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions" *ngIf="value || quoteValue">
    <h4 class="col-md-12 col-xs-2"><small>{{ baseCurrency }}:<br><span title="{{ baseCurrency }} Available" class="text-danger">{{ basePosition | number:'1.8-8' }}</span><br/><span title="{{ baseCurrency }} Held" [ngClass]="baseHeldPosition ? 'sell' : 'text-muted'">{{ baseHeldPosition | number:'1.8-8' }}</span></small></h4>
    <h4 class="col-md-12 col-xs-2"><small>{{ quoteCurrency }}:<br><span title="{{ quoteCurrency }} Available" class="text-danger">{{ quotePosition | number:'1.'+product.fixed+'-'+product.fixed }}</span><br/><span title="{{ quoteCurrency }} Held" [ngClass]="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition | number:'1.'+product.fixed+'-'+product.fixed }}</span></small></h4>
    <h4 class="col-md-12 col-xs-2"><small>Value:</small><br><b title="{{ baseCurrency }} Total">{{ value | number:'1.8-8' }}</b><br/><b title="{{ quoteCurrency }} Total">{{ quoteValue | number:'1.'+product.fixed+'-'+product.fixed }}</b></h4>
  </div>`
})
export class WalletPositionComponent implements OnInit {

  public baseCurrency: string;
  public basePosition: string;
  public quoteCurrency: string;
  public quotePosition: number;
  public baseHeldPosition: number;
  public quoteHeldPosition: number;
  public value: number;
  public quoteValue: number;
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
    this.basePosition = o.data[0];
    this.quotePosition = o.data[1];
    this.baseHeldPosition = o.data[2];
    this.quoteHeldPosition = o.data[3];
    this.value = o.data[4];
    this.quoteValue = o.data[5];
    this.baseCurrency = Models.Currency[o.data[6]];
    this.quoteCurrency = Models.Currency[o.data[7]];
  }
}
