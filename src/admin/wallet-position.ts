import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions" *ngIf="value || quoteValue">
      <h4 class="col-md-12 col-xs-2"><small>
        {{ quoteCurrency }}:&nbsp;<span [ngClass]="quotePosition + quoteHeldPosition > buySize * fv ? 'text-danger' : 'text-muted'">{{ quotePosition | currency:quoteCurrency:true:'1.2-2' }}</span>
        <br/>(<span [ngClass]="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition | currency:quoteCurrency:true:'1.2-2' }}</span>)
      </small></h4>
      <h4 class="col-md-12 col-xs-2"><small>
        {{ baseCurrency }}:&nbsp;<span [ngClass]="basePosition + baseHeldPosition > sellSize ? 'text-danger' : 'text-muted'">฿{{ basePosition | number:'1.3-3' }}</span>
        <br/>(<span [ngClass]="baseHeldPosition ? 'sell' : 'text-muted'">฿{{ baseHeldPosition | number:'1.3-3' }}</span>)
      </small></h4>
      <h4 class="col-md-12 col-xs-2" style="margin-top: 1px;margin-bottom: 10px!important;">
        <small>Value:</small><br>฿<b>{{ value | number:'1.5-5' }}</b><br/><b>{{ quoteValue | currency:quoteCurrency:true:'1.2-2' }}</b>
      </h4>
    </div>`
})
export class WalletPositionComponent implements OnInit, OnDestroy {

  public baseCurrency: string;
  public basePosition: number;
  public quoteCurrency: string;
  public quotePosition: number;
  public baseHeldPosition: number;
  public quoteHeldPosition: number;
  public value: number;
  public quoteValue: number;
  public buySize: number;
  public sellSize: number;
  public fv: number;

  private subscriberQPChange: Messaging.ISubscribe<Models.QuotingParameters>;
  private subscriberPosition: Messaging.ISubscribe<Models.PositionReport>;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory
  ) {}

  ngOnInit() {
    this.subscriberQPChange = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.QuotingParametersChange)
      .registerDisconnectedHandler(this.clearQP)
      .registerSubscriber(this.updateQP);

    this.subscriberPosition = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.Position)
      .registerDisconnectedHandler(this.clearPosition)
      .registerSubscriber(this.updatePosition);
  }

  ngOnDestroy() {
    this.subscriberQPChange.disconnect();
    this.subscriberPosition.disconnect();
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
    this.fv = null;
  }

  private clearQP = () => {
    this.buySize = null;
    this.sellSize = null;
  }

  private updatePosition = (o: Models.Timestamped<any[]>) => {
    this.basePosition = o.data[0];
    this.quotePosition = o.data[1];
    this.baseHeldPosition = o.data[2];
    this.quoteHeldPosition = o.data[3];
    this.value = o.data[4];
    this.quoteValue = o.data[5];
    this.fv = o.data[5] / o.data[4];
    this.baseCurrency = Models.Currency[o.data[6]];
    this.quoteCurrency = Models.Currency[o.data[7]];
  }

  private updateQP = (qp: Models.QuotingParameters) => {
    this.buySize = qp.buySize;
    this.sellSize = qp.sellSize;
  }
}
