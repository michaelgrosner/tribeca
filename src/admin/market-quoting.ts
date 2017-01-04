/// <reference path='../common/models.ts' />
/// <reference path='../common/messaging.ts' />
/// <reference path='shared_directives.ts'/>
/// <amd-dependency path='ui.bootstrap'/>

import angular = require('angular');
import Models = require('../common/models');
import Messaging = require('../common/messaging');
import Shared = require('./shared_directives');

class DisplayLevel {
  bidPrice: number;
  bidSize: number;
  bidPercent: number;
  askPrice: number;
  askSize: number;
  askPercent: number;
  diffWidth: number;

  bidClass: string;
  askClass: string;
}

class DisplayOrderStatusClassReport {
  orderId: string;
  price: number;
  quantity: number;
  side: Models.Side;

  constructor(
    public osr: Models.OrderStatusReport
  ) {
    this.orderId = osr.orderId;
    this.side = osr.side;
    this.quantity = osr.quantity;
    this.price = osr.price;
  }
}

class MarketQuotingController {

  public levels: DisplayLevel[];
  public fairValue: number;
  public extVal: number;
  public qBidSz: number;
  public qBidPx: number;
  public qAskPx: number;
  public qAskSz: number;
  public order_classes: DisplayOrderStatusClassReport[];
  public bidIsLive: boolean;
  public askIsLive: boolean;

  constructor(
    $scope: ng.IScope,
    $log: ng.ILogService,
    subscriberFactory: Shared.SubscriberFactory
  ) {
    var clearMarket = () => {
        this.levels = [];
    };
    clearMarket();

    var clearQuote = () => {
        this.order_classes = [];
    };
    clearQuote();

    var clearFairValue = () => {
        this.fairValue = null;
    };

    var clearQuoteStatus = () => {
        this.bidIsLive = false;
        this.askIsLive = false;
    };

    var clearExtVal = () => {
        this.extVal = null;
    };

    var updateMarket = (update: Models.Market) => {
      if (update == null) {
        clearMarket();
        return;
      }

      for (var i = 0; i < update.asks.length; i++) {
        if (i >= this.levels.length)
          this.levels[i] = new DisplayLevel();
        this.levels[i].askPrice = update.asks[i].price;
        this.levels[i].askSize = update.asks[i].size;
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
      for (var i = 0; i < update.bids.length; i++) {
        if (i >= this.levels.length)
          this.levels[i] = new DisplayLevel();
        this.levels[i].bidPrice = update.bids[i].price;
        this.levels[i].bidSize = update.bids[i].size;
        this.levels[i].bidPercent = Math.max(Math.min((Math.log(update.bids[i].size)/Math.log(2))*4,19),1);
        if (i < update.asks.length)
          this.levels[i].askPercent = Math.max(Math.min((Math.log(update.asks[i].size)/Math.log(2))*4,19),1);

        this.levels[i].diffWidth = i==0
          ? this.levels[i].askPrice - this.levels[i].bidPrice : (
            (i==1 && this.qAskPx && this.qBidPx)
              ? this.qAskPx - this.qBidPx : 0
          );
      }

      updateQuoteClass();
    };

    var updateQuote = (o: Models.OrderStatusReport) => {
      var idx = -1;
      for(var i=0;i<this.order_classes.length;i++)
        if (this.order_classes[i].orderId==o.orderId) {idx=i; break;}
      if (idx!=-1) {
        if (!o.leavesQuantity)
          this.order_classes.splice(idx,1);
      } else if (o.leavesQuantity)
        this.order_classes.push(new DisplayOrderStatusClassReport(o));

      updateQuoteClass();
    };

    var updateQuoteStatus = (status: Models.TwoSidedQuoteStatus) => {
      if (status == null) {
        clearQuoteStatus();
        return;
      }

      this.bidIsLive = (status.bidStatus === Models.QuoteStatus.Live);
      this.askIsLive = (status.askStatus === Models.QuoteStatus.Live);
      updateQuoteClass();
    };

    var updateQuoteClass = () => {
      if (this.levels && this.levels.length > 0) {
        var tol = .005;
        for (var i = 0; i < this.levels.length; i++) {
          var level = this.levels[i];
          level.bidClass = 'active';
          var bids = this.order_classes.filter(o => o.side === Models.Side.Bid);
          for (var j = 0; j < bids.length; j++)
            if (Math.abs(bids[j].price - level.bidPrice) < tol)
              level.bidClass = 'success buy';
          level.askClass = 'active';
          var asks = this.order_classes.filter(o => o.side === Models.Side.Ask);
          for (var j = 0; j < asks.length; j++)
            if (Math.abs(asks[j].price - level.askPrice) < tol)
              level.askClass = 'success sell';
        }
      }
    };

    var updateFairValue = (fv: Models.FairValue) => {
      if (fv == null) {
        clearFairValue();
        return;
      }

      this.fairValue = fv.price;
    };

    var subscribers = [];

    var makeSubscriber = <T>(topic: string, updateFn, clearFn) => {
      var subscriber = subscriberFactory.getSubscriber<T>($scope, topic)
        .registerSubscriber(updateFn, ms => ms.forEach(updateFn))
        .registerDisconnectedHandler(clearFn);
      subscribers.push(subscriber);
    };

    makeSubscriber<Models.Market>(Messaging.Topics.MarketData, updateMarket, clearMarket);
    makeSubscriber<Models.OrderStatusReport>(Messaging.Topics.OrderStatusReports, updateQuote, clearQuote);
    makeSubscriber<Models.TwoSidedQuoteStatus>(Messaging.Topics.QuoteStatus, updateQuoteStatus, clearQuoteStatus);
    makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue, updateFairValue, clearFairValue);

    $scope.$on('$destroy', () => {
      subscribers.forEach(d => d.disconnect());
    });

    clearQuote();
  }
}

export var marketQuotingDirective = 'marketQuotingDirective';

angular.module(marketQuotingDirective, ['ui.bootstrap', 'ui.grid', Shared.sharedDirectives])
  .directive('marketQuoting', (): ng.IDirective => { return {
    template: `<table class="table table-hover table-bordered table-condensed table-responsive text-center">
      <tr class="active">
        <th></th>
        <th>bidSz</th>
        <th>bidPx</th>
        <th>FV</th>
        <th>askPx</th>
        <th>askSz</th>
      </tr>
      <tr class="info">
        <td class="text-left">q</td>
        <td ng-class="marketQuotingScope.bidIsLive ? 'text-danger' : 'text-muted'">{{ marketQuotingScope.qBidSz|number:2 }}</td>
        <td ng-class="marketQuotingScope.bidIsLive ? 'text-danger' : 'text-muted'">{{ marketQuotingScope.qBidPx|number:2 }}</td>
        <td class="fairvalue">{{ marketQuotingScope.fairValue|number:2 }}</td>
        <td ng-class="marketQuotingScope.askIsLive ? 'text-danger' : 'text-muted'">{{ marketQuotingScope.qAskPx|number:2 }}</td>
        <td ng-class="marketQuotingScope.askIsLive ? 'text-danger' : 'text-muted'">{{ marketQuotingScope.qAskSz|number:2 }}</td>
      </tr>
      <tr class="active" ng-repeat="level in marketQuotingScope.levels">
        <td class="text-left">mkt{{ $index }}</td>
        <td ng-class="level.bidClass"><div style="width:100%;background: -webkit-linear-gradient(left, #8de2ff {{ level.bidPercent|number:2 }}%,trasnparent {{ level.bidPercent|number:2 }}%);background: -moz-linear-gradient(left, #8de2ff {{ level.bidPercent|number:2 }}%,transparent {{ level.bidPercent|number:2 }}%);background: -ms-linear-gradient(left, #8de2ff {{ level.bidPercent|number:2 }}%,transparent {{ level.bidPercent|number:2 }}%);background: -o-linear-gradient(left, #8de2ff {{ level.bidPercent|number:2 }}%,transparent {{ level.bidPercent|number:2 }}%);background: linear-gradient(to right, #8de2ff {{ level.bidPercent|number:2 }}%,transparent {{ level.bidPercent|number:2 }}%);">{{ level.bidSize|number:2 }}</div></td>
        <td ng-class="level.bidClass">{{ level.bidPrice|number:2 }}</td>
        <td><span ng-show="level.diffWidth > 0">{{ level.diffWidth|number:2 }}</span></td>
        <td ng-class="level.askClass">{{ level.askPrice|number:2 }}</td>
        <td ng-class="level.askClass"><div style="width:100%;background: -webkit-linear-gradient(left, #ff8e8c {{ level.askPercent|number:2 }}%,trasnparent {{ level.askPercent|number:2 }}%);background:    -moz-linear-gradient(left, #ff8e8c {{ level.askPercent|number:2 }}%,transparent {{ level.askPercent|number:2 }}%);background:     -ms-linear-gradient(left, #ff8e8c {{ level.askPercent|number:2 }}%,transparent {{ level.askPercent|number:2 }}%);background:      -o-linear-gradient(left, #ff8e8c {{ level.askPercent|number:2 }}%,transparent {{ level.askPercent|number:2 }}%);background:         linear-gradient(to right, #ff8e8c {{ level.askPercent|number:2 }}%,transparent {{ level.askPercent|number:2 }}%);">{{ level.askSize|number:2 }}</div></td>
      </tr>
    </table>`,
    restrict: 'E',
    transclude: false,
    controller: MarketQuotingController,
    controllerAs: 'marketQuotingScope',
    scope: {},
    bindToController: true
  }});
