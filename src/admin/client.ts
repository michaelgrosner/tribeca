/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts"/>
/// <reference path="shared_directives.ts"/>
/// <reference path="fairvalue-chart.ts"/>
/// <reference path="wallet-position.ts"/>
/// <reference path="target-base-position.ts"/>
/// <reference path="market-quoting.ts"/>
/// <reference path="market-trades.ts"/>
/// <reference path="trade-safety.ts"/>
/// <reference path="orders.ts"/>
/// <reference path="trades.ts"/>
/// <reference path="pair.ts"/>

import 'zone.js';
import 'reflect-metadata';

(<any>global).jQuery = require("jquery");

import {NgModule, Component} from '@angular/core';
import {BrowserModule} from '@angular/platform-browser';
import {platformBrowserDynamic} from '@angular/platform-browser-dynamic';

import moment = require("moment");
import io = require("socket.io-client");

import {NgbModule} from '@ng-bootstrap/ng-bootstrap';
// var ngGrid = require("../ui-grid.min");
// var bootstrap = require("../bootstrap.min");

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {FireFactory, SubscriberFactory, Mypopover, BindOnceDirective, MomentFullDatePipe, MomentShortDatePipe} from './shared_directives';
import Pair = require('./pair');
import {FairValueChartComponent} from './fairvalue-chart';
import {WalletPositionComponent} from './wallet-position';
import {TargetBasePositionComponent} from './target-base-position';
import {MarketQuotingComponent} from './market-quoting';
import {MarketTradesComponent} from './market-trades';
import {TradeSafetyComponent} from './trade-safety';
import {OrdersComponent} from './orders';
import {TradesComponent} from './trades';

class DisplayOrder {
  side : string;
  price : number;
  quantity : number;
  timeInForce : string;
  orderType : string;

  availableSides : string[];
  availableTifs : string[];
  availableOrderTypes : string[];

  private static getNames<T>(enumObject : T) {
    var names : string[] = [];
    for (var mem in enumObject) {
      if (!enumObject.hasOwnProperty(mem)) continue;
      if (parseInt(mem, 10) >= 0) {
        names.push(String(enumObject[mem]));
      }
    }
    return names;
  }

  private _fire : Messaging.IFire<Models.OrderRequestFromUI>;

  constructor(
    fireFactory : FireFactory,
    private _log : ng.ILogService
  ) {
    this.availableSides = DisplayOrder.getNames(Models.Side);
    this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
    this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);

    this._fire = fireFactory.getFire(Messaging.Topics.SubmitNewOrder);
  }

  public submit = () => {
    var msg = new Models.OrderRequestFromUI(this.side, this.price, this.quantity, this.timeInForce, this.orderType);
    // this._log.info("submitting order", msg);
    this._fire.fire(msg);
  };
}

@Component({
  selector: 'ui',
  templateUrl: 'ui.html'
})
class ClientComponent {

  public memory: string;
  public notepad: string;
  public connected: boolean;
  public order: DisplayOrder;
  public pair: Pair.DisplayPair;
  public exch_name: string;
  public pair_name: string;
  public cancelAllOrders = () => {};
  public cleanAllClosedOrders = () => {};
  public cleanAllOrders = () => {};
  public changeTheme = () => {};
  public changeNotepad = (content: string) => {};

  constructor(
    $scope : ng.IScope,
    $window: ng.IWindowService,
    $timeout : ng.ITimeoutService,
    $log : ng.ILogService,
    subscriberFactory : SubscriberFactory,
    fireFactory : FireFactory
  ) {
    var cancelAllFirer = fireFactory.getFire(Messaging.Topics.CancelAllOrders);
    this.cancelAllOrders = () => cancelAllFirer.fire(new Models.CancelAllOrdersRequest());

    var cleanAllClosedFirer = fireFactory.getFire(Messaging.Topics.CleanAllClosedOrders);
    this.cleanAllClosedOrders = () => cleanAllClosedFirer.fire(new Models.CleanAllClosedOrdersRequest());

    var cleanAllFirer = fireFactory.getFire(Messaging.Topics.CleanAllOrders);
    this.cleanAllOrders = () => cleanAllFirer.fire(new Models.CleanAllOrdersRequest());

    var changeNotepadFirer = fireFactory.getFire(Messaging.Topics.ChangeNotepad);
    this.changeNotepad = (content:string) => changeNotepadFirer.fire(new Models.Notepad(content));

    this.order = new DisplayOrder(fireFactory, $log);
    this.pair = null;

    var unit = ['', 'K', 'M', 'G', 'T', 'P'];

    var bytesToSize = (input:number, precision:number) => {
      var index = Math.floor(Math.log(input) / Math.log(1024));
      if (index >= unit.length) return input + 'B';
      return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B'
    };

    var user_theme = null;
    var system_theme = null;

    var setTheme = () => {
      if (jQuery('#daynight').attr('href')!='/css/bootstrap-theme'+system_theme+'.min.css')
        jQuery('#daynight').attr('href', '/css/bootstrap-theme'+system_theme+'.min.css');
    };

    this.changeTheme = () => {
      user_theme = user_theme!==null?(user_theme==''?'-dark':''):(system_theme==''?'-dark':'');
      system_theme = user_theme;
      setTheme();
      $window.setTimeout(function(){$window.dispatchEvent(new Event('resize'));}, 1000);
    };

    var getTheme = (hour: number) => {
      return user_theme!==null?user_theme:((hour<9 || hour>=21)?'-dark':'');
    };

    this.notepad = null;
    var onNotepad = (np : Models.Notepad) => {
      this.notepad = np ? np.content : "";
    };

    var onAppState = (as : Models.ApplicationState) => {
      this.memory = bytesToSize(as.memory, 3);
      system_theme = getTheme(as.hour);
      setTheme();
    };

    var onAdvert = (pa : Models.ProductAdvertisement) => {
      // $log.info("advert", pa);
      this.connected = true;
      $window.document.title = 'tribeca ['+pa.environment+']';
      system_theme = getTheme(moment.utc().hours());
      setTheme();
      this.pair_name = Models.Currency[pa.pair.base] + "/" + Models.Currency[pa.pair.quote];
      this.exch_name = Models.Exchange[pa.exchange];
      this.pair = new Pair.DisplayPair($scope, subscriberFactory, fireFactory);
      $window.setTimeout(function(){$window.dispatchEvent(new Event('resize'));}, 1000);
    };

    var reset = (reason : string) => {
      // $log.info("reset", reason);
      this.connected = false;
      this.pair_name = null;
      this.exch_name = null;

      if (this.pair !== null)
        this.pair.dispose();
      this.pair = null;
    };
    reset("startup");

    var subscriberProductAdvertisement = subscriberFactory.getSubscriber($scope, Messaging.Topics.ProductAdvertisement)
      .registerSubscriber(onAdvert, a => a.forEach(onAdvert))
      .registerDisconnectedHandler(() => reset("disconnect"));

    var subscriberApplicationState = subscriberFactory.getSubscriber($scope, Messaging.Topics.ApplicationState)
      .registerSubscriber(onAppState, a => a.forEach(onAppState))
      .registerDisconnectedHandler(() => reset("disconnect"));

    var subscriberNotepad = subscriberFactory.getSubscriber($scope, Messaging.Topics.Notepad)
      .registerSubscriber(onNotepad, a => a.forEach(onNotepad))
      .registerDisconnectedHandler(() => reset("disconnect"));

    $scope.$on('$destroy', () => {
      subscriberProductAdvertisement.disconnect();
      subscriberApplicationState.disconnect();
      subscriberNotepad.disconnect();
    });
  }
}

@NgModule({
  imports: [BrowserModule, NgbModule.forRoot()],
  bootstrap: [ClientComponent],
  declarations: [
    ClientComponent,
    FairValueChartComponent,
    OrdersComponent,
    TradesComponent,
    MarketQuotingComponent,
    MarketTradesComponent,
    WalletPositionComponent,
    TargetBasePositionComponent,
    TradeSafetyComponent,
    BindOnceDirective,
    Mypopover,
    MomentFullDatePipe,
    MomentShortDatePipe
  ],
  providers:[ /*'ui.grid',*/ FireFactory, SubscriberFactory, {
    provide: 'socket',
    useFactory: io
  }]
})
class ClientModule {}

platformBrowserDynamic().bootstrapModule(ClientModule);
