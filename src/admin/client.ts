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
import 'core-js/es6/symbol';
import 'core-js/es6/object';
import 'core-js/es6/function';
import 'core-js/es6/parse-int';
import 'core-js/es6/parse-float';
import 'core-js/es6/number';
import 'core-js/es6/math';
import 'core-js/es6/string';
import 'core-js/es6/date';
import 'core-js/es6/array';
import 'core-js/es6/regexp';
import 'core-js/es6/map';
import 'core-js/es6/set';
import 'core-js/es6/reflect';

import 'core-js/es7/reflect';
import 'zone.js/dist/zone';

import 'zone.js';
import 'reflect-metadata';

(<any>global).jQuery = require("jquery");

import {NgModule, Component, ValueProvider, Inject, OnInit, OnDestroy, enableProdMode} from '@angular/core';
import {BrowserModule} from '@angular/platform-browser';
import {platformBrowserDynamic} from '@angular/platform-browser-dynamic';

import moment = require("moment");

import {NgbModule} from '@ng-bootstrap/ng-bootstrap';
// var ngGrid = require("../ui-grid.min");
// var bootstrap = require("../bootstrap.min");

import Models = require('../common/models');
import Messaging = require('../common/messaging');
import {SharedModule, /*FireFactory,*/ SubscriberFactory} from './shared_directives';
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
    // fireFactory : FireFactory
  ) {
    this.availableSides = DisplayOrder.getNames(Models.Side);
    this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
    this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);
    // this._fire = fireFactory.getFire(Messaging.Topics.SubmitNewOrder);
  }

  public submit = () => {
    var msg = new Models.OrderRequestFromUI(this.side, this.price, this.quantity, this.timeInForce, this.orderType);
    // this._fire.fire(msg);
  };
}

@Component({
  selector: 'ui',
  template: `<div>
    <div ng-if="!connected">
        Not connected
    </div>

    <div ng-if="connected">
        <div class="navbar navbar-default" role="navigation">
            <div class="container-fluid">
                <div class="navbar-header">
                    <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target=".navbar-collapse">
                        <span class="icon-bar"></span>
                    </button>
                    <a class="navbar-brand" href="#" ng-click="changeTheme()">tribeca</a> <small title="Memory Used" style="margin-top: 6px;display: inline-block;">{{ memory }}</small>
                </div>
                <div class="navbar-collapse collapse">
                    <ul class="nav navbar-nav navbar-right">
                        <li><p class="navbar-text">Target Base Position: <target-base-position></target-base-position></p></li>
                        <li><p class="navbar-text"><trade-safety></trade-safety></p></li>
                        <li>
                            <button type="button"
                                    class="btn btn-primary navbar-btn"
                                    id="order_form"
                                    mypopover popover-template="order_form.html"
                                    data-placement="bottom">Submit order
                            </button>
                        </li>
                        <li>
                            <button type="button"
                                    class="btn btn-danger navbar-btn"
                                    ng-click="cancelAllOrders()"
                                    data-placement="bottom">Cancel All Open Orders
                            </button>
                        </li>
                        <li>
                            <button type="button"
                                    class="btn btn-info navbar-btn"
                                    ng-click="cleanAllClosedOrders()"
                                    ng-show="[6,7].indexOf(pair.quotingParameters.display.mode)>-1"
                                    data-placement="bottom">Clean All Closed Pongs
                            </button>
                        </li>
                        <li>
                            <button type="button"
                                    class="btn btn-danger navbar-btn"
                                    ng-click="cleanAllOrders()"
                                    ng-show="[5,6,7].indexOf(pair.quotingParameters.display.mode)>-1"
                                    data-placement="bottom">Clean All Open Pings
                            </button>
                        </li>
                    </ul>
                </div>
            </div>
        </div>

        <div class="container-fluid">
            <div>
                <div style="padding: 5px" ng-class="pair.connected ? 'bg-success img-rounded' : 'bg-danger img-rounded'">
                    <div class="row">
                        <div class="col-md-1 col-xs-12 text-center">
                            <div class="row img-rounded exchange">
                                <button class="col-md-12 col-xs-3" ng-class="pair.active.getClass()" ng-click="pair.active.submit()">
                                    {{ pair_name }}
                                </button>

                                <h4 style="font-size: 20px" class="col-md-12 col-xs-3">{{ exch_name }}</h4>
                                <wallet-position></wallet-position>
                            </div>
                        </div>

                        <div class="col-md-3 col-xs-12">
                            <market-quoting></market-quoting>
                        </div>

                        <div class="col-md-6 col-xs-12">
                            <trade-list></trade-list>
                        </div>

                        <div class="col-md-2 col-xs-12">
                            <market-trades></market-trades>
                        </div>
                    </div>

                    <div class="row">
                        <div class="col-md-12 col-xs-12">
                            <div class="row">
                                <table class="table table-responsive table-bordered">
                                    <thead>
                                        <tr class="active">
                                            <th>mode</th>
                                            <th ng-show="pair.quotingParameters.display.mode==7">bullets</th>
                                            <th ng-show="pair.quotingParameters.display.mode==7">magazine</th>
                                            <th ng-show="[5,6,7].indexOf(pair.quotingParameters.display.mode)>-1">pingAt</th>
                                            <th ng-show="[5,6,7].indexOf(pair.quotingParameters.display.mode)>-1">pongAt</th>
                                            <th>fv</th>
                                            <th>apMode</th>
                                            <th>width</th>
                                            <th>bidSz</th>
                                            <th>askSz</th>
                                            <th>tbp</th>
                                            <th>pDiv</th>
                                            <th>ewma?</th>
                                            <th>apr?</th>
                                            <th>trds</th>
                                            <th>/sec</th>
                                            <th>audio?</th>
                                            <th colspan="2">
                                                <span ng-if="!pair.quotingParameters.pending && pair.quotingParameters.connected" class="text-success">
                                                    Applied
                                                </span>
                                                <span ng-if="pair.quotingParameters.pending && pair.quotingParameters.connected" class="text-warning">
                                                    Pending
                                                </span>
                                                <span ng-if="!pair.quotingParameters.connected" class="text-danger">
                                                    Not Connected
                                                </span>
                                            </th>
                                        </tr>
                                    </thead>
                                    <tbody>
                                        <tr class="active">
                                            <td style="width:121px;">
                                                <select class="form-control input-sm"
                                                    ng-model="pair.quotingParameters.display.mode"
                                                    ng-options="x.val as x.str for x in pair.quotingParameters.availableQuotingModes"></select>
                                            </td>
                                            <td style="width:78px;" ng-show="pair.quotingParameters.display.mode==7">
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.bullets">
                                            </td>
                                            <td style="width:121px;" ng-show="pair.quotingParameters.display.mode==7">
                                                <select class="form-control input-sm"
                                                   ng-model="pair.quotingParameters.display.magazine"
                                                   ng-options="x.val as x.str for x in pair.quotingParameters.availableMagazine"></select>
                                            </td>
                                            <td style="width:142px;" ng-show="[5,6,7].indexOf(pair.quotingParameters.display.mode)>-1">
                                                <select class="form-control input-sm"
                                                   ng-model="pair.quotingParameters.display.pingAt"
                                                   ng-options="x.val as x.str for x in pair.quotingParameters.availablePingAt"></select>
                                            </td>
                                            <td style="width:148px;" ng-show="[5,6,7].indexOf(pair.quotingParameters.display.mode)>-1">
                                                <select class="form-control input-sm"
                                                   ng-model="pair.quotingParameters.display.pongAt"
                                                   ng-options="x.val as x.str for x in pair.quotingParameters.availablePongAt"></select>
                                            </td>
                                            <td style="width:88px;">
                                                <select class="form-control input-sm"
                                                    ng-model="pair.quotingParameters.display.fvModel"
                                                    ng-options="x.val as x.str for x in pair.quotingParameters.availableFvModels"></select>
                                            </td>
                                            <td style="width:121px;">
                                                <select class="form-control input-sm"
                                                    ng-model="pair.quotingParameters.display.autoPositionMode"
                                                    ng-options="x.val as x.str for x in pair.quotingParameters.availableAutoPositionModes"></select>
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.width">
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.buySize">
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.sellSize">
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.targetBasePosition">
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.positionDivergence">
                                            </td>
                                            <td>
                                                <input type="checkbox"
                                                   ng-model="pair.quotingParameters.display.ewmaProtection">
                                            </td>
                                            <td>
                                                <input type="checkbox"
                                                   ng-model="pair.quotingParameters.display.aggressivePositionRebalancing">
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.tradesPerMinute">
                                            </td>
                                            <td>
                                                <input class="form-control input-sm"
                                                   type="number"
                                                   onClick="this.select()"
                                                   ng-model="pair.quotingParameters.display.tradeRateSeconds">
                                            </td>
                                            <td>
                                                <input type="checkbox"
                                                   ng-model="pair.quotingParameters.display.audio">
                                            </td>
                                            <td>
                                                <input class="btn btn-default btn col-md-1 col-xs-6"
                                                    style="width:55px"
                                                    type="button"
                                                    ng-click="pair.quotingParameters.reset()"
                                                    value="Reset" />
                                            </td>
                                            <td>
                                                <input class="btn btn-default btn col-md-1 col-xs-6"
                                                    style="width:50px"
                                                    type="submit"
                                                    ng-click="pair.quotingParameters.submit()"
                                                    value="Save" />
                                            </td>
                                        </tr>
                                    </tbody>

                                </table>
                            </div>

                        </div>
                    </div>

                    <div class="row">
                        <div class="col-md-10 col-xs-12">
                            <order-list></order-list>
                        </div>
                        <div  class="col-md-2 col-xs-12">
                          <textarea ng-model="notepad" ng-change="changeNotepad(notepad)" placeholder="ephemeral notepad" class="ephemeralnotepad" style="height:273px;width: 100%;max-width: 100%;"></textarea>
                      </div>
                    </div>
                </div>
            </div>

          <div class="container-fluid">
              <div class="row">
                  <div class="col-md-4 col-xs-12">
                  </div>
                  <div class="col-md-4 col-xs-12">
                      <fair-value-chart></fair-value-chart>
                  </div>
                  <div class="col-md-4 col-xs-12">
                  </div>
              </div>
          </div>
        </div>
    </div>
    <address class="text-center">
      <small>
        <a href="/view/README.md" target="_blank">README</a> - <a href="/view/HOWTO.md" target="_blank">HOWTO</a>
      </small>
    </address>
  </div>`
})
class ClientComponent implements OnInit, OnDestroy {

  public memory: string;
  public notepad: string;
  public connected: boolean;
  public order: DisplayOrder;
  // public pair: Pair.DisplayPair;
  public exch_name: string;
  public pair_name: string;
  public cancelAllOrders = () => {};
  public cleanAllClosedOrders = () => {};
  public cleanAllOrders = () => {};
  public changeNotepad = (content: string) => {};

  subscriberProductAdvertisement: any;
  subscriberApplicationState: any;
  subscriberNotepad: any;

  user_theme: string;
  system_theme: string;

  // $scope : ng.IScope,
  // fireFactory : FireFactory
  constructor(@Inject(SubscriberFactory) private subscriberFactory:SubscriberFactory) {
    // var cancelAllFirer = fireFactory.getFire(Messaging.Topics.CancelAllOrders);
    // this.cancelAllOrders = () => cancelAllFirer.fire(new Models.CancelAllOrdersRequest());

    // var cleanAllClosedFirer = fireFactory.getFire(Messaging.Topics.CleanAllClosedOrders);
    // this.cleanAllClosedOrders = () => cleanAllClosedFirer.fire(new Models.CleanAllClosedOrdersRequest());

    // var cleanAllFirer = fireFactory.getFire(Messaging.Topics.CleanAllOrders);
    // this.cleanAllOrders = () => cleanAllFirer.fire(new Models.CleanAllOrdersRequest());

    // var changeNotepadFirer = fireFactory.getFire(Messaging.Topics.ChangeNotepad);
    // this.changeNotepad = (content:string) => changeNotepadFirer.fire(new Models.Notepad(content));

    // this.order = new DisplayOrder(fireFactory);
    // this.pair = null;


    this.notepad = null;
    var onNotepad = (np : Models.Notepad) => {
      this.notepad = np ? np.content : "";
    };

    this.reset("startup");
  }

  private reset = (reason : string) => {
    this.connected = false;
    this.pair_name = null;
    this.exch_name = null;

    // if (this.pair !== null)
      // this.pair.dispose();
    // this.pair = null;
  };

  private unit = ['', 'K', 'M', 'G', 'T', 'P'];

  private bytesToSize = (input:number, precision:number) => {
    var index = Math.floor(Math.log(input) / Math.log(1024));
    if (index >= this.unit.length) return input + 'B';
    return (input / Math.pow(1024, index)).toFixed(precision) + this.unit[index] + 'B'
  }

  private onAppState = (as : Models.ApplicationState) => {
    this.memory = this.bytesToSize(as.memory, 3);
    this.system_theme = this.getTheme(as.hour);
    this.setTheme();
  }

  private setTheme = () => {
    if (jQuery('#daynight').attr('href')!='/css/bootstrap-theme'+this.system_theme+'.min.css')
      jQuery('#daynight').attr('href', '/css/bootstrap-theme'+this.system_theme+'.min.css');
  }

  public changeTheme = () => {
    this.user_theme = this.user_theme!==null?(this.user_theme==''?'-dark':''):(this.system_theme==''?'-dark':'');
    this.system_theme = this.user_theme;
    this.setTheme();
    window.setTimeout(function(){window.dispatchEvent(new Event('resize'));}, 1000);
  }

  private getTheme = (hour: number) => {
    return this.user_theme!==null?this.user_theme:((hour<9 || hour>=21)?'-dark':'');
  }

  private onAdvert = (pa : Models.ProductAdvertisement) => {
    this.connected = true;
    window.document.title = 'tribeca ['+pa.environment+']';
    this.system_theme = this.getTheme(moment.utc().hours());
    this.setTheme();
    // this.pair_name = Models.Currency[pa.pair.base] + "/" + Models.Currency[pa.pair.quote];
    // this.exch_name = Models.Exchange[pa.exchange];
    // this.pair = new Pair.DisplayPair(this, subscriberFactory, fireFactory);
    window.setTimeout(function(){window.dispatchEvent(new Event('resize'));}, 1000);
  }

  ngOnInit() {
    // this.connection = this.chatService.getMessages().subscribe(message => {
      // this.messages.push(message);
    // });

    this.subscriberProductAdvertisement = this.subscriberFactory.getSubscriber(this, Messaging.Topics.ProductAdvertisement)
      .registerSubscriber(this.onAdvert, a => a.forEach(this.onAdvert))
      .registerDisconnectedHandler(() => this.reset("disconnect"));

    // this.subscriberApplicationState = this.subscriberFactory.getSubscriber(this, Messaging.Topics.ApplicationState)
      // .registerSubscriber(this.onAppState, a => a.forEach(this.onAppState))
      // .registerDisconnectedHandler(() => reset("disconnect"));

    // this.subscriberNotepad = this.subscriberFactory.getSubscriber(this, Messaging.Topics.Notepad)
      // .registerSubscriber(onNotepad, a => a.forEach(onNotepad))
      // .registerDisconnectedHandler(() => reset("disconnect"));

  }

  ngOnDestroy() {
    // this.connection.unsubscribe();
    this.subscriberProductAdvertisement.disconnect();
    // this.subscriberApplicationState.disconnect();
    // this.subscriberNotepad.disconnect();
  }
}

// const ioFactory = (): SocketIOClient.Socket => io();

@NgModule({
  imports: [BrowserModule, SharedModule, NgbModule.forRoot()],
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
  ]
})
class ClientModule {}

// enableProdMode();
platformBrowserDynamic().bootstrapModule(ClientModule);
