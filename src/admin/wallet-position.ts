/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgZone, Component, Inject, OnInit, OnDestroy} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions">
      <h4 class="col-md-12 col-xs-2"><small>
        {{ quoteCurrency }}:&nbsp;<span [ngClass]="quotePosition + quoteHeldPosition > buySize * fv ? 'text-danger' : 'text-muted'">{{ quotePosition | currency:'USD':true:'1.2-2' }}</span>
        <br/>(<span [ngClass]="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition | currency:'USD':true:'1.2-2' }}</span>)
      </small></h4>
      <h4 class="col-md-12 col-xs-2"><small>
        {{ baseCurrency }}:&nbsp;<span [ngClass]="basePosition + baseHeldPosition > sellSize ? 'text-danger' : 'text-muted'"><span style="font-size: 82%;" class="glyphicon glyphicon-bitcoin"></span>{{ basePosition | number:'1.3-3' }}</span>
        <br/>(<span [ngClass]="baseHeldPosition ? 'sell' : 'text-muted'"><span style="font-size: 82%;" class="glyphicon glyphicon-bitcoin"></span>{{ baseHeldPosition | number:'1.3-3' }}</span>)
      </small></h4>
      <h4 class="col-md-12 col-xs-2">
        <small>Value:</small><br><b><span style="font-size: 76%;" class="glyphicon glyphicon-bitcoin"></span>{{ value | number:'1.5-5' }}</b><br/><b>{{ quoteValue | currency:'USD':true:'1.2-2' }}</b>
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
      .registerSubscriber(this.updateQP, qp => qp.forEach(this.updateQP));

    this.subscriberPosition = this.subscriberFactory
      .getSubscriber(this.zone, Messaging.Topics.Position)
      .registerDisconnectedHandler(this.clearPosition)
      .registerSubscriber(this.updatePosition, us => us.forEach(this.updatePosition));
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

  private updatePosition = (position : Models.PositionReport) => {
    this.baseCurrency = Models.Currency[position.pair.base];
    this.quoteCurrency = Models.Currency[position.pair.quote];
    this.basePosition = position.baseAmount;
    this.quotePosition = position.quoteAmount;
    this.baseHeldPosition = position.baseHeldAmount;
    this.quoteHeldPosition = position.quoteHeldAmount;
    this.value = position.value;
    this.quoteValue = position.quoteValue;
    this.fv = position.quoteValue / position.value;
  }

  private updateQP = (qp: Models.QuotingParameters) => {
    this.buySize = qp.buySize;
    this.sellSize = qp.sellSize;
  }
}
