/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>
/// <amd-dependency path='ui.bootstrap'/>

import {Component} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions">
      <h4 class="col-md-12 col-xs-2"><small>
        {{ quoteCurrency }}:&nbsp;<span ng-class="quotePosition + quoteHeldPosition > buySize * fv ? 'text-danger' : 'text-muted'">{{ quotePosition|currency:undefined:2 }}</span>
        <br/>(<span ng-class="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition|currency:undefined:2 }}</span>)
      </small></h4>
      <h4 class="col-md-12 col-xs-2"><small>
        {{ baseCurrency }}:&nbsp;<span ng-class="basePosition + baseHeldPosition > sellSize ? 'text-danger' : 'text-muted'">{{ basePosition|currency:"B":3 }}</span>
        <br/>(<span ng-class="baseHeldPosition ? 'sell' : 'text-muted'">{{ baseHeldPosition|currency:"B":3 }}</span>)
      </small></h4>
      <h4 class="col-md-12 col-xs-2">
        <small>Value: </small><b>{{ value|currency:"B":5 }}</b>
        <br/><b>{{ quoteValue|currency:undefined:2 }}</b>
      </h4>
    </div>`
})
export class WalletPositionComponent {

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

  $scope: ng.IScope;
  subscriberFactory: SubscriberFactory;
  constructor() {
    var clearPosition = () => {
      this.baseCurrency = null;
      this.quoteCurrency = null;
      this.basePosition = null;
      this.quotePosition = null;
      this.baseHeldPosition = null;
      this.quoteHeldPosition = null;
      this.value = null;
      this.quoteValue = null;
      this.fv = null;
    };

    var clearQP = () => {
      this.buySize = null;
      this.sellSize = null;
    };

    var updatePosition = (position : Models.PositionReport) => {
      this.baseCurrency = Models.Currency[position.pair.base];
      this.quoteCurrency = Models.Currency[position.pair.quote];
      this.basePosition = position.baseAmount;
      this.quotePosition = position.quoteAmount;
      this.baseHeldPosition = position.baseHeldAmount;
      this.quoteHeldPosition = position.quoteHeldAmount;
      this.value = position.value;
      this.quoteValue = position.quoteValue;
      this.fv = position.quoteValue / position.value;
    };

    var updateQP = qp => {
      this.buySize = qp.buySize;
      this.sellSize = qp.sellSize;
    };

    // var subscriberQPChange = this.subscriberFactory.getSubscriber(this.$scope, Messaging.Topics.QuotingParametersChange)
      // .registerDisconnectedHandler(clearQP)
      // .registerSubscriber(updateQP, qp => qp.forEach(updateQP));

    // var subscriberPosition = this.subscriberFactory.getSubscriber(this.$scope, Messaging.Topics.Position)
      // .registerDisconnectedHandler(clearPosition)
      // .registerSubscriber(updatePosition, us => us.forEach(updatePosition));

    // this.$scope.$on('$destroy', () => {
      // subscriberQPChange.disconnect();
      // subscriberPosition.disconnect();
    // });
  }
}
