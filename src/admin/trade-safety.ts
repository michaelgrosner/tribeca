/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>

import {NgModule, Component} from '@angular/core';

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SubscriberFactory} from './shared_directives';

@Component({
  selector: 'trade-safety',
  template: `<div>
      BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety|number:2 }}</span>,
      SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety|number:2 }}</span>,
      BuyTS: {{ buySafety|number:2 }},
      SellTS: {{ sellSafety|number:2 }},
      TotalTS: {{ tradeSafetyValue|number:2 }}
    </div>`
})
export class TradeSafetyComponent {

  public buySafety: number;
  public sellSafety: number;
  public buySizeSafety: number;
  public sellSizeSafety: number;
  public tradeSafetyValue: number;

  $scope: ng.IScope;
  subscriberFactory: SubscriberFactory;
  constructor() {
    var updateValue = (value : Models.TradeSafety) => {
      if (value == null) return;
      this.tradeSafetyValue = value.combined;
      this.buySafety = value.buy;
      this.sellSafety = value.sell;
      this.buySizeSafety = value.buyPing;
      this.sellSizeSafety = value.sellPong;
    };

    var clear = () => {
      this.tradeSafetyValue = null;
      this.buySafety = null;
      this.sellSafety = null;
      this.buySizeSafety = null;
      this.sellSizeSafety = null;
    };

    // var subscriberTradeSafetyValue = this.subscriberFactory.getSubscriber(this.$scope, Messaging.Topics.TradeSafetyValue)
      // .registerDisconnectedHandler(clear)
      // .registerSubscriber(updateValue, us => us.forEach(updateValue));

    // this.$scope.$on('$destroy', () => {
        // subscriberTradeSafetyValue.disconnect();
    // });
  }
}
