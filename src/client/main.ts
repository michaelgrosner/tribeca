import 'zone.js';
import 'reflect-metadata';

(<any>global).jQuery = require("jquery");

import {NgModule, NgZone, Component, Inject, OnInit, enableProdMode} from '@angular/core';
import {platformBrowserDynamic} from '@angular/platform-browser-dynamic';
import {FormsModule} from '@angular/forms';
import {BrowserModule} from '@angular/platform-browser';
import {AgGridModule} from 'ag-grid-angular/main';
import {ChartModule} from 'angular2-highcharts';
import {PopoverModule} from "ng4-popover";
import Highcharts = require('highcharts');
import Highstock = require('highcharts/highstock');
require('highcharts/highcharts-more.js')(Highstock);

import moment = require("moment");

import Models = require('./models');
import Subscribe = require('./subscribe');
import {SharedModule, FireFactory, SubscriberFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';
import Pair = require('./pair');
import {WalletPositionComponent} from './wallet-position';
import {MarketQuotingComponent} from './market-quoting';
import {MarketTradesComponent} from './market-trades';
import {TradeSafetyComponent} from './trade-safety';
import {OrdersComponent} from './orders';
import {TradesComponent} from './trades';
import {StatsComponent} from './stats';

class DisplayOrder {
  side : string;
  price : number;
  quantity : number;
  timeInForce : string;
  orderType : string;

  availableSides : string[];
  availableTifs : string[];
  availableOrderTypes : string[];

  private static getNames(enumObject: any) {
    var names: string[] = [];
    for (var mem in enumObject) {
      if (!enumObject.hasOwnProperty(mem)) continue;
      if (parseInt(mem, 10) >= 0) {
        names.push(String(enumObject[mem]));
      }
    }
    return names;
  }

  private _fire: Subscribe.IFire<Models.OrderRequestFromUI>;

  constructor(
    fireFactory: FireFactory
  ) {
    this.availableSides = DisplayOrder.getNames(Models.Side).slice(0,-1);
    this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
    this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);
    this.timeInForce = this.availableTifs[2];
    this.orderType = this.availableOrderTypes[0];
    this._fire = fireFactory.getFire(Models.Topics.SubmitNewOrder);
  }

  public submit = () => {
    if (!this.side || !this.price || !this.quantity || !this.timeInForce || !this.orderType) return;
    this._fire.fire(new Models.OrderRequestFromUI(this.side, this.price, this.quantity, this.timeInForce, this.orderType));
  };
}

@Component({
  selector: 'ui',
  template: `<div>
    <div *ngIf="!online">
        <h4 class="text-danger text-center">{{ product.advert.environment ? product.advert.environment+' is d' : 'D' }}isconnected.</h4>
    </div>
    <div *ngIf="online">
        <div class="container-fluid">
            <div>
                <div style="padding: 5px;padding-top:10px;margin-top:7px;" [ngClass]="pair.connected ? 'bg-success img-rounded' : 'bg-danger img-rounded'">
                    <div class="row" [hidden]="!showConfigs">
                        <div class="col-md-12 col-xs-12">
                            <div class="row">
                              <table border="0" width="100%"><tr><td style="width:69px;text-align:center;border-bottom: 1px gray solid;">
                                <small>MARKET<br/>MAKING</small>
                              </td><td>
                                <table class="table table-responsive table-bordered" style="margin-bottom:0px;">
                                    <thead>
                                        <tr class="active">
                                            <th>%</th>
                                            <th>mode</th>
                                            <th *ngIf="pair.quotingParameters.display.mode==7">bullets</th>
                                            <th *ngIf="pair.quotingParameters.display.mode==7">range</th>
                                            <th *ngIf="[5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1">pingAt</th>
                                            <th *ngIf="[5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1">pongAt</th>
                                            <th>sop</th>
                                            <th [attr.colspan]="pair.quotingParameters.display.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && pair.quotingParameters.display.buySizeMax">minB</span><span *ngIf="!pair.quotingParameters.display.aggressivePositionRebalancing || !pair.quotingParameters.display.buySizeMax">b</span>idSize<span *ngIf="pair.quotingParameters.display.percentageValues">%</span><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && [5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1" style="float:right;">maxBidSize?</span></th>
                                            <th [attr.colspan]="pair.quotingParameters.display.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && pair.quotingParameters.display.sellSizeMax">minA</span><span *ngIf="!pair.quotingParameters.display.aggressivePositionRebalancing || !pair.quotingParameters.display.sellSizeMax">a</span>skSize<span *ngIf="pair.quotingParameters.display.percentageValues">%</span><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && [5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1" style="float:right;">maxAskSize?</span></th>
                                        </tr>
                                    </thead>
                                    <tbody>
                                        <tr class="active">
                                            <td style="width:25px;border-bottom: 3px solid #8BE296;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.percentageValues">
                                            </td>
                                            <td style="min-width:121px;border-bottom: 3px solid #DDE28B;">
                                                <select class="form-control input-sm"
                                                  [(ngModel)]="pair.quotingParameters.display.mode">
                                                  <option *ngFor="let option of pair.quotingParameters.availableQuotingModes" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:78px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.mode==7">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.bullets">
                                            </td>
                                            <td style="border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.mode==7">
                                                <input class="form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.range">
                                            </td>
                                            <td style="min-width:142px;border-bottom: 3px solid #8BE296;" *ngIf="[5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1">
                                                <select class="form-control input-sm"
                                                   [(ngModel)]="pair.quotingParameters.display.pingAt">
                                                   <option *ngFor="let option of pair.quotingParameters.availablePingAt" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="border-bottom: 3px solid #8BE296;" *ngIf="[5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1">
                                                <select class="form-control input-sm"
                                                   [(ngModel)]="pair.quotingParameters.display.pongAt">
                                                   <option *ngFor="let option of pair.quotingParameters.availablePongAt" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="min-width:121px;border-bottom: 3px solid #DDE28B;">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.superTrades">
                                                   <option *ngFor="let option of pair.quotingParameters.availableSuperTrades" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="!pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="0.01" min="0.01"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.buySize">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="1" min="1" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.buySizePercentage">
                                            </td>
                                            <td style="border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.buySizeMax">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="!pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="0.01" min="0.01"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.sellSize">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="1" min="1" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.sellSizePercentage">
                                            </td>
                                            <td style="border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.sellSizeMax">
                                            </td>
                                        </tr>
                                    </tbody>
                                </table>
                              </td></tr></table>
                              <table border="0" width="100%"><tr><td style="width:69px;text-align:center;border-bottom: 1px gray solid;">
                                <small>TECHNICAL<br/>ANALYSIS</small>
                              </td><td>
                                <table class="table table-responsive table-bordered" style="margin-bottom:0px;">
                                    <thead>
                                        <tr class="active">
                                            <th>apMode</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">long</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode==2">medium</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">short</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">sensibility</th>
                                            <th *ngIf="!pair.quotingParameters.display.autoPositionMode">tbp<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                            <th>pDiv<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                            <th>apr</th>
                                            <th>bw?</th>
                                            <th *ngIf="[9].indexOf(pair.quotingParameters.display.mode)==-1">%w?</th>
                                            <th *ngIf="[5,6,7,8].indexOf(pair.quotingParameters.display.mode)==-1"><span *ngIf="[9].indexOf(pair.quotingParameters.display.mode)==-1">width</span><span *ngIf="[9].indexOf(pair.quotingParameters.display.mode)>-1">depth</span><span *ngIf="pair.quotingParameters.display.widthPercentage && [9].indexOf(pair.quotingParameters.display.mode)==-1">%</span></th>
                                            <th *ngIf="[5,6,7,8].indexOf(pair.quotingParameters.display.mode)>-1">pingWidth<span *ngIf="pair.quotingParameters.display.widthPercentage">%</span></th>
                                            <th *ngIf="[5,6,7,8].indexOf(pair.quotingParameters.display.mode)>-1">pongWidth<span *ngIf="pair.quotingParameters.display.widthPercentage">%</span></th>
                                        </tr>
                                    </thead>
                                    <tbody>
                                        <tr class="active">
                                            <td style="min-width:121px;border-bottom: 3px solid #8BE296;">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.autoPositionMode">
                                                   <option *ngFor="let option of pair.quotingParameters.availableAutoPositionModes" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.longEwmaPeriods">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode==2">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.mediumEwmaPeriods">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.shortEwmaPeriods">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode">
                                                <input class="form-control input-sm"
                                                   type="number" step="0.01" min="0.01" max="1.00"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.ewmaSensiblityPercentage">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="!pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode==0">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="0.01" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.targetBasePosition">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode==0">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="1" min="0" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.targetBasePositionPercentage">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="!pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="0.01" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.positionDivergence">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="1" min="0" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.positionDivergencePercentage">
                                            </td>
                                            <td style="min-width:121px;border-bottom: 3px solid #D64A4A;">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.aggressivePositionRebalancing">
                                                   <option *ngFor="let option of pair.quotingParameters.availableAggressivePositionRebalancings" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:25px;border-bottom: 3px solid #8BE296;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.bestWidth">
                                            </td>
                                            <td style="width:25px;border-bottom: 3px solid #8BE296;" *ngIf="[9].indexOf(pair.quotingParameters.display.mode)==-1">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPercentage">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="!pair.quotingParameters.display.widthPercentage || [9].indexOf(pair.quotingParameters.display.mode)>-1">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPing">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.widthPercentage && [9].indexOf(pair.quotingParameters.display.mode)==-1">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="0.01" min="0.01" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPingPercentage">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="[5,6,7,8].indexOf(pair.quotingParameters.display.mode)>-1 && !pair.quotingParameters.display.widthPercentage">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPong">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="[5,6,7,8].indexOf(pair.quotingParameters.display.mode)>-1 && pair.quotingParameters.display.widthPercentage">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="0.01" min="0.01" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPongPercentage">
                                            </td>
                                    </tbody>
                                </table>
                              </td></tr></table>
                              <table border="0" width="100%"><tr><td style="width:69px;text-align:center;">
                                <small>PROTECTION</small>
                              </td><td>
                                <table class="table table-responsive table-bordered">
                                    <thead>
                                        <tr class="active">
                                            <th>fv</th>
                                            <th style="text-align:right;">trades</th>
                                            <th>/sec</th>
                                            <th>ewma?</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingEwmaProtection">periodsᵉʷᵐᵃ</th>
                                            <th>stdev</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingStdevProtection">periodsˢᵗᵈᶜᵛ</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingStdevProtection">factor</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingStdevProtection">BB?</th>
                                            <th>delayAPI</th>
                                            <th>cxl?</th>
                                            <th>profit</th>
                                            <th>Kmemory</th>
                                            <th>delayUI</th>
                                            <th>audio?</th>
                                            <th colspan="2">
                                                <span *ngIf="!pair.quotingParameters.pending && pair.quotingParameters.connected" class="text-success">
                                                    Applied
                                                </span>
                                                <span *ngIf="pair.quotingParameters.pending && pair.quotingParameters.connected" class="text-warning">
                                                    Pending
                                                </span>
                                                <span *ngIf="!pair.quotingParameters.connected" class="text-danger">
                                                    Not Connected
                                                </span>
                                            </th>
                                        </tr>
                                    </thead>
                                    <tbody>
                                        <tr class="active">
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.fvModel">
                                                   <option *ngFor="let option of pair.quotingParameters.availableFvModels" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;">
                                                <input class="form-control input-sm"
                                                   type="number" step="0.1" min="0"
                                                   style="text-align:right;"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.tradesPerMinute">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.tradeRateSeconds">
                                            </td>
                                            <td style="text-align: center;border-bottom: 3px solid #F0A0A0;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.quotingEwmaProtection">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #F0A0A0;" *ngIf="pair.quotingParameters.display.quotingEwmaProtection">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.quotingEwmaProtectionPeriods">
                                            </td>
                                            <td style="width:121px;border-bottom: 3px solid #AF451E;">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.quotingStdevProtection">
                                                   <option *ngFor="let option of pair.quotingParameters.availableSTDEV" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #AF451E;" *ngIf="pair.quotingParameters.display.quotingStdevProtection">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.quotingStdevProtectionPeriods">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #AF451E;" *ngIf="pair.quotingParameters.display.quotingStdevProtection">
                                                <input class="form-control input-sm"
                                                   type="number" step="0.1" min="0.1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.quotingStdevProtectionFactor">
                                            </td>
                                            <td style="text-align: center;border-bottom: 3px solid #AF451E;" *ngIf="pair.quotingParameters.display.quotingStdevProtection">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.quotingStdevBollingerBands">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #A0A0A0;">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.delayAPI">
                                            </td>
                                            <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.cancelOrdersAuto">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;">
                                                <input class="form-control input-sm"
                                                   type="number" step="0.01" min="0.01"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.profitHourInterval">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;">
                                                <input class="form-control input-sm"
                                                   type="number" step="0.1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.cleanPongsAuto">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #A0A0A0;">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.delayUI">
                                            </td>
                                            <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.audio">
                                            </td>
                                            <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                                                <input class="btn btn-default btn"
                                                    style="width:55px"
                                                    type="button"
                                                    (click)="pair.quotingParameters.reset()"
                                                    value="Reset" />
                                            </td>
                                            <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                                                <input class="btn btn-default btn"
                                                    style="width:50px"
                                                    type="submit"
                                                    [disabled]="!pair.quotingParameters.connected"
                                                    (click)="pair.quotingParameters.submit()"
                                                    value="Save" />
                                            </td>
                                        </tr>
                                    </tbody>
                                </table>
                              </td></tr></table>
                            </div>
                        </div>
                    </div>
                    <div class="row">
                        <div class="col-md-1 col-xs-12 text-center" style="padding-right:0px;">
                            <div class="row img-rounded exchange">
                                <div *ngIf="pair.connectionMessage">{{ pair.connectionMessage }}</div>
                                <button style="font-size:16px;" class="col-md-12 col-xs-3" [ngClass]="pair.active.getClass()" [disabled]="!pair.active.connected" (click)="pair.active.submit()">
                                    {{ exchange_name }}<br/>{{ pair_name.join('/') }}
                                </button>
                                <wallet-position [product]="product"></wallet-position>
                                <a [hidden]="!exchange_market" href="{{ exchange_market }}" target="_blank">Market</a><span [hidden]="!exchange_market || !exchange_orders ">,</span>
                                <a [hidden]="!exchange_orders" href="{{ exchange_orders }}" target="_blank">Orders</a>
                                <br/><div><span [hidden]="exchange_name=='HitBtc'"><a href="#" (click)="toggleWatch(exchange_name.toLowerCase(), this.pair_name.join('-').toLowerCase())">Watch</a>, </span><a href="#" (click)="toggleStats()">Stats</a></div>
                                <a href="#" (click)="toggleConfigs(showConfigs = !showConfigs)">Settings</a>
                            </div>
                        </div>

                        <div [hidden]="!showStats" [ngClass]="showStats == 2 ? 'col-md-11 col-xs-12 absolute-charts' : 'col-md-11 col-xs-12 relative-charts'">
                          <market-stats [setShowStats]="!!showStats" [product]="product"></market-stats>
                        </div>
                        <div [hidden]="showStats === 1" class="col-md-9 col-xs-12" style="padding-left:0px;padding-bottom:0px;">
                          <div class="row">
                            <trade-safety [tradeFreq]="tradeFreq" [product]="product"></trade-safety>
                          </div>
                          <div class="row" style="padding-top:0px;">
                            <div class="col-md-4 col-xs-12" style="padding-left:0px;padding-top:0px;padding-right:0px;">
                                <market-quoting [online]="!!pair.active.display.state" [product]="product"></market-quoting>
                            </div>
                            <div class="col-md-8 col-xs-12" style="padding-left:0px;padding-right:0px;padding-top:0px;">
                              <div class="row">
                                <div class="exchangeActions col-md-2 col-xs-12 text-center img-rounded">
                                  <div>
                                      <button type="button"
                                              class="btn btn-primary navbar-btn"
                                              id="order_form"
                                              [popover]="myPopover">Submit Order
                                      </button>
                                      <popover-content #myPopover
                                          placement="bottom"
                                          [animation]="true"
                                          [closeOnClickOutside]="true">
                                              <table border="0" style="width:139px;">
                                                <tr>
                                                    <td><label>Side:</label></td>
                                                    <td style="padding-bottom:5px;"><select class="form-control input-sm" [(ngModel)]="order.side">
                                                      <option *ngFor="let option of order.availableSides" [ngValue]="option">{{option}}</option>
                                                    </select></td>
                                                </tr>
                                                <tr>
                                                    <td><label>Price:&nbsp;</label></td>
                                                    <td style="padding-bottom:5px;"><input class="form-control input-sm" type="number" step="{{ product.advert.minTick}}" [(ngModel)]="order.price" /></td>
                                                </tr>
                                                <tr>
                                                    <td><label>Size:</label></td>
                                                    <td style="padding-bottom:5px;"><input class="form-control input-sm" type="number" step="0.01" [(ngModel)]="order.quantity" /></td>
                                                </tr>
                                                <tr>
                                                    <td><label>TIF:</label></td>
                                                    <td style="padding-bottom:5px;"><select class="form-control input-sm" [(ngModel)]="order.timeInForce">
                                                      <option *ngFor="let option of order.availableTifs" [ngValue]="option">{{option}}</option>
                                                    </select></td>
                                                </tr>
                                                <tr>
                                                    <td><label>Type:</label></td>
                                                    <td style="padding-bottom:5px;"><select class="form-control input-sm" [(ngModel)]="order.orderType">
                                                      <option *ngFor="let option of order.availableOrderTypes" [ngValue]="option">{{option}}</option>
                                                    </select></td>
                                                </tr>
                                                <tr><td colspan="2" class="text-center"><button type="button" class="btn btn-success" (click)="myPopover.hide()" (click)="order.submit()">Submit</button></td></tr>
                                              </table>
                                      </popover-content>
                                  </div>
                                  <div style="padding-top: 2px;padding-bottom: 2px;">
                                      <button type="button"
                                              class="btn btn-danger navbar-btn"
                                              (click)="cancelAllOrders()"
                                              data-placement="bottom">Cancel Orders
                                      </button>
                                  </div>
                                  <div style="padding-bottom: 2px;">
                                      <button type="button"
                                              class="btn btn-info navbar-btn"
                                              (click)="cleanAllClosedOrders()"
                                              *ngIf="[6,7,8].indexOf(pair.quotingParameters.display.mode)>-1"
                                              data-placement="bottom">Clean Pongs
                                      </button>
                                  </div>
                                  <div>
                                      <button type="button"
                                              class="btn btn-danger navbar-btn"
                                              (click)="cleanAllOrders()"
                                              *ngIf="[5,6,7,8,9].indexOf(pair.quotingParameters.display.mode)>-1"
                                              data-placement="bottom">Clean Pings
                                      </button>
                                  </div>
                                </div>
                                <div class="col-md-10 col-xs-12" style="padding-right:0px;padding-top:4px;">
                                  <order-list [online]="!!pair.active.display.state" [product]="product"></order-list>
                                </div>
                              </div>
                              <div class="row">
                                <trade-list (onTradesLength)="onTradesLength($event)" [product]="product"></trade-list>
                              </div>
                            </div>
                          </div>
                        </div>
                        <div [hidden]="showStats === 1" class="col-md-2 col-xs-12" style="padding-left:0px;">
                          <textarea [(ngModel)]="notepad" (ngModelChange)="changeNotepad(notepad)" placeholder="ephemeral notepad" class="ephemeralnotepad" style="height:69px;width: 100%;max-width: 100%;"></textarea>
                          <market-trades [product]="product"></market-trades>
                        </div>
                      </div>
                </div>
            </div>
        </div>
    </div>
    <address class="text-center">
      <small>
        <a href="{{ homepage }}/blob/master/README.md" target="_blank">README</a> - <a href="{{ homepage }}/blob/master/MANUAL.md" target="_blank">MANUAL</a> - <a href="{{ homepage }}" target="_blank">SOURCE</a> - <a href="#" (click)="changeTheme()">changeTheme(<span [hidden]="!system_theme">LIGHT</span><span [hidden]="system_theme">DARK</span>)</a> - <span title="Server used RAM" style="margin-top: 6px;display: inline-block;">{{ server_memory }}</span> - <span title="Client used RAM" style="margin-top: 6px;display: inline-block;">{{ client_memory }}</span> - <span title="Database Size" style="margin-top: 6px;display: inline-block;">{{ db_size }}</span> - <span title="Pings in memory" style="margin-top: 6px;display: inline-block;">{{ tradesLength }}</span> - <a href="#" (click)="openMatryoshka()">MATRYOSHKA</a> - <a href="{{ homepage }}/issues/new?title=%5Btopic%5D%20short%20and%20sweet%20description&body=description%0Aplease,%20consider%20to%20add%20all%20possible%20details%20%28if%20any%29%20about%20your%20new%20feature%20request%20or%20bug%20report%0A%0A%2D%2D%2D%0A%60%60%60%0Aapp%20exchange%3A%20{{ exchange_name }}/{{ pair_name.join('/') }}%0Aapp%20version%3A%20undisclosed%0A%60%60%60%0A![300px-spock_vulcan-salute3](https://cloud.githubusercontent.com/assets/1634027/22077151/4110e73e-ddb3-11e6-9d84-358e9f133d34.png)" target="_blank">CREATE ISSUE</a> - <a href="https://21.co/analpaper/" target="_blank">HELP</a> - <a title="irc://irc.domirc.net:6667/##tradingBot" href="irc://irc.domirc.net:6667/##tradingBot">IRC</a>
      </small>
    </address>
    <iframe id="matryoshka" style="margin:0px;padding:0px;border:0px;width:100%;height:0px;" src="about:blank"></iframe>
  </div>`
})
class ClientComponent implements OnInit {

  public homepage: string;
  public matryoshka: string;
  public server_memory: string;
  public client_memory: string;
  public db_size: string;
  public notepad: string;
  public online: boolean;
  public showConfigs: boolean = false;
  public showStats: number = 0;
  public order: DisplayOrder;
  public pair: Pair.DisplayPair;
  public exchange_name: string;
  public exchange_market: string;
  public exchange_orders: string;
  public pair_name: string[];
  public cancelAllOrders = () => {};
  public cleanAllClosedOrders = () => {};
  public cleanAllOrders = () => {};
  public toggleConfigs = (showConfigs:boolean) => {};
  public changeNotepad = (content: string) => {};
  public toggleStats = () => {
    if (++this.showStats>=3) this.showStats = 0;
  };
  public toggleWatch = (watchExchange: string, watchPair: string) => {
    if (window.parent !== window) {
      window.parent.postMessage('cryptoWatch='+watchExchange+','+watchPair, '*');
      return;
    }
    var self = this;
    var toggleWatch = function() {
      self._toggleWatch(watchExchange, watchPair);
     };
    if (!(<any>window).cryptowatch) (function(d, script) {
        script = d.createElement('script');
        script.type = 'text/javascript';
        script.async = true;
        script.onload = toggleWatch;
        script.src = 'https://static.cryptowat.ch/assets/scripts/embed.bundle.js';
        d.getElementsByTagName('head')[0].appendChild(script);
      }(document));
    else toggleWatch();
  };
  public _toggleWatch = (watchExchange: string, watchPair: string) => {
    if (!document.getElementById('cryptoWatch'+watchExchange+watchPair)) {
      (<any>window).setDialog('cryptoWatch'+watchExchange+watchPair, 'open', {title: watchExchange.toUpperCase()+' '+watchPair.toUpperCase().replace('-','/'),width: 800,height: 400,content: `<div id="container`+watchExchange+watchPair+`" style="width:100%;height:100%;"></div>`});
      if (!jQuery('#cryptoWatch'+watchExchange+watchPair+'.resizable').length) (<any>jQuery)('#cryptoWatch'+watchExchange+watchPair).resizable({handleSelector: '#cryptoWatch'+watchExchange+watchPair+' .dialog-resize'});
      (new (<any>window).cryptowatch.Embed(watchExchange, watchPair.replace('-',''), {timePeriod: '1d',customColorScheme: {bg:"000000",text:"b2b2b2",textStrong:"e5e5e5",textWeak:"7f7f7f",short:"FD4600",shortFill:"FF672C",long:"6290FF",longFill:"002782",cta:"363D52",ctaHighlight:"414A67",alert:"FFD506"}})).mount('#container'+watchExchange+watchPair);
    } else (<any>window).setDialog('cryptoWatch'+watchExchange+watchPair, 'close', {content:''});
  };
  public openMatryoshka = () => {
    const url = window.prompt('Enter the URL of another instance:',this.matryoshka||'https://');
    jQuery('#matryoshka').attr('src', url||'about:blank').height((url&&url!='https://')?589:0);
  };
  public resizeMatryoshka = () => {
    if (window.parent === window) return;
    window.parent.postMessage('height='+jQuery('body').height(), '*');
  };
  public product: Models.ProductState = {
    advert: new Models.ProductAdvertisement(null, null, null, null, null, .01),
    fixed: 2
  };

  private user_theme: string = null;
  private system_theme: string = null;
  public tradeFreq: number = 0;
  public tradesLength: number = 0;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory,
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.cancelAllOrders = () => this.fireFactory
      .getFire(Models.Topics.CancelAllOrders)
      .fire();

    this.cleanAllClosedOrders = () => this.fireFactory
      .getFire(Models.Topics.CleanAllClosedOrders)
      .fire();

    this.cleanAllOrders = () => this.fireFactory
      .getFire(Models.Topics.CleanAllOrders)
      .fire();

    this.changeNotepad = (content:string) => this.fireFactory
      .getFire(Models.Topics.Notepad)
      .fire([content]);

    this.toggleConfigs = (showConfigs:boolean) => {
      this.fireFactory
        .getFire(Models.Topics.ToggleConfigs)
        .fire([showConfigs]);
      setTimeout(this.resizeMatryoshka, 100);
    }

    window.addEventListener("message", e => {
      if (e.data.indexOf('height=')===0) {
        jQuery('#matryoshka').height(e.data.replace('height=',''));
        this.resizeMatryoshka();
      }
      else if (e.data.indexOf('cryptoWatch=')===0) {
        var data = e.data.replace('cryptoWatch=','').split(',');
        this._toggleWatch(data[0], data[1]);
      }
    }, false);

    this.reset(false);

    this.order = new DisplayOrder(this.fireFactory);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.ProductAdvertisement)
      .registerSubscriber(this.onAdvert)
      .registerDisconnectedHandler(() => this.reset(false));

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.ApplicationState)
      .registerSubscriber(this.onAppState);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Notepad)
      .registerSubscriber(this.onNotepad);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.ToggleConfigs)
      .registerSubscriber(this.onToggleConfigs);
  }

  private onNotepad = (notepad : string) => {
    this.notepad = notepad;
  }

  private onToggleConfigs = (showConfigs: boolean) => {
    this.showConfigs = showConfigs;
  }

  public onTradesLength(tradesLength: number) {
    this.tradesLength = tradesLength;
  }

  private reset = (online: boolean) => {
    this.online = online;
    this.pair_name = [null, null];
    this.exchange_name = null;
    this.exchange_market = null;
    this.exchange_orders = null;
    this.pair = null;
  }

  private bytesToSize = (input:number, precision:number) => {
    let unit = ['', 'K', 'M', 'G', 'T', 'P'];
    let index = Math.floor(Math.log(input) / Math.log(1024));
    if (index >= unit.length) return input + 'B';
    return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B'
  }

  private onAppState = (as : Models.ApplicationState) => {
    this.server_memory = this.bytesToSize(as.memory, 0);
    this.client_memory = this.bytesToSize((<any>window.performance).memory ? (<any>window.performance).memory.usedJSHeapSize : 1, 0);
    this.db_size = this.bytesToSize(as.dbsize, 0);
    this.system_theme = this.getTheme(as.hour);
    this.tradeFreq = (as.freq);
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
  }

  private getTheme = (hour: number) => {
    return this.user_theme!==null?this.user_theme:((hour<9 || hour>=21)?'-dark':'');
  }

  private onAdvert = (pa : Models.ProductAdvertisement) => {
    this.online = true;
    window.document.title = '['+pa.environment+']';
    this.matryoshka = pa.matryoshka;
    this.system_theme = this.getTheme(moment.utc().hours());
    this.setTheme();
    this.pair_name = [Models.Currency[pa.pair.base], Models.Currency[pa.pair.quote]];
    this.exchange_name = Models.Exchange[pa.exchange];
    this.exchange_market = this.exchange_name=='OkCoin'
      ? 'https://www.okcoin.'+(Models.Currency[pa.pair.quote]=='CNY'?'cn':'com')+'/market.html'
      : (this.exchange_name=='Coinbase'
        ? 'https://gdax.com/trade/'+this.pair_name.join('-')
        : (this.exchange_name=='Bitfinex'
            ? 'https://www.bitfinex.com/trading/'+this.pair_name.join('')
            : (this.exchange_name=='HitBtc'
              ? 'https://hitbtc.com/exchange/'+this.pair_name.join('-to-')
              : null
            )
          )
      );
    this.exchange_orders = this.exchange_name=='OkCoin'
      ? 'https://www.okcoin.'+(Models.Currency[pa.pair.quote]=='CNY'?'cn':'com')+'/trade/entrust.do'
      : (this.exchange_name=='Coinbase'
        ? 'https://www.gdax.com/orders/'+this.pair_name.join('-')
        : (this.exchange_name=='Bitfinex'
          ? 'https://www.bitfinex.com/reports/orders'
          : (this.exchange_name=='HitBtc'
            ? 'https://hitbtc.com/reports/orders'
            : null
          )
        )
      );
    this.pair = new Pair.DisplayPair(this.zone, this.subscriberFactory, this.fireFactory);
    this.product.advert = pa;
    this.homepage = pa.homepage;
    this.product.fixed = Math.max(0, Math.floor(Math.log10(pa.minTick)) * -1);
    setTimeout(this.resizeMatryoshka, 5000);
    console.log("%cK started "+(new Date().toISOString().slice(11, -1))+"\n%c"+this.homepage, "color:green;font-size:32px;", "color:red;font-size:16px;");
  }
}

@NgModule({
  imports: [
    SharedModule,
    BrowserModule,
    FormsModule,
    PopoverModule,
    AgGridModule.withComponents([
      BaseCurrencyCellComponent,
      QuoteCurrencyCellComponent
    ]),
    ChartModule.forRoot(Highcharts),
    ChartModule.forRoot(Highstock)
  ],
  declarations: [
    ClientComponent,
    OrdersComponent,
    TradesComponent,
    MarketQuotingComponent,
    MarketTradesComponent,
    WalletPositionComponent,
    TradeSafetyComponent,
    BaseCurrencyCellComponent,
    QuoteCurrencyCellComponent,
    StatsComponent
  ],
  bootstrap: [ClientComponent]
})
class ClientModule {}

enableProdMode();
platformBrowserDynamic().bootstrapModule(ClientModule);
