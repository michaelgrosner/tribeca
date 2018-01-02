import 'zone.js';
import 'reflect-metadata';

import {NgModule, NgZone, Component, Inject, OnInit, enableProdMode} from '@angular/core';
import {platformBrowserDynamic} from '@angular/platform-browser-dynamic';
import {FormsModule} from '@angular/forms';
import {BrowserModule} from '@angular/platform-browser';
import {AgGridModule} from 'ag-grid-angular/main';
import {ChartModule} from 'angular2-highcharts';
import {PopoverModule} from 'ng4-popover';
import * as Highcharts from 'highcharts';
declare var require: (filename: string) => any;
require('highcharts/highcharts-more')(Highcharts);

import * as Models from './models';
import * as Subscribe from './subscribe';
import {SharedModule, FireFactory, SubscriberFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent, QuoteUntruncatedCurrencyCellComponent} from './shared_directives';
import * as Pair from './pair';
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
    <div [hidden]="online">
        <h4 class="text-danger text-center">{{ product.advert.environment ? product.advert.environment+' is d' : 'D' }}isconnected.</h4>
    </div>
    <div [hidden]="!online">
        <div class="container-fluid">
            <div>
                <div style="padding: 5px;padding-top:10px;margin-top:7px;" [ngClass]="pair.connected ? 'bg-success img-rounded' : 'bg-danger img-rounded'">
                    <div *ngIf="online" class="row" [hidden]="!showSettings">
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
                                            <th>safety</th>
                                            <th *ngIf="pair.quotingParameters.display.safety==3">bullets</th>
                                            <th *ngIf="pair.quotingParameters.display.safety==3 && !pair.quotingParameters.display.percentageValues">range</th>
                                            <th *ngIf="pair.quotingParameters.display.safety==3 && pair.quotingParameters.display.percentageValues">range%</th>
                                            <th *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pingAt</th>
                                            <th *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pongAt</th>
                                            <th>sop</th>
                                            <ng-container *ngIf="pair.quotingParameters.display.superTrades">
                                            <th>sopWidth</th>
                                            <th *ngIf="[1,3].indexOf(pair.quotingParameters.display.superTrades)>-1">sopTrades</th>
                                            <th *ngIf="[2,3].indexOf(pair.quotingParameters.display.superTrades)>-1">sopSize</th>
                                            </ng-container>
                                            <th [attr.colspan]="pair.quotingParameters.display.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && pair.quotingParameters.display.buySizeMax">minB</span><span *ngIf="!pair.quotingParameters.display.aggressivePositionRebalancing || !pair.quotingParameters.display.buySizeMax">b</span>idSize<span *ngIf="pair.quotingParameters.display.percentageValues">%</span><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing" style="float:right;">maxBidSize?</span></th>
                                            <th [attr.colspan]="pair.quotingParameters.display.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && pair.quotingParameters.display.sellSizeMax">minA</span><span *ngIf="!pair.quotingParameters.display.aggressivePositionRebalancing || !pair.quotingParameters.display.sellSizeMax">a</span>skSize<span *ngIf="pair.quotingParameters.display.percentageValues">%</span><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing" style="float:right;">maxAskSize?</span></th>
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
                                            <td style="min-width:121px;border-bottom: 3px solid #DDE28B;">
                                                <select class="form-control input-sm"
                                                  [(ngModel)]="pair.quotingParameters.display.safety">
                                                  <option *ngFor="let option of pair.quotingParameters.availableQuotingSafeties" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:78px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.safety==3">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.bullets">
                                            </td>
                                            <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.safety==3 && !pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.range">
                                            </td>
                                            <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.safety==3 && pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="0.1" min="0" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.rangePercentage">
                                            </td>
                                            <td style="min-width:142px;border-bottom: 3px solid #8BE296;" *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">
                                                <select class="form-control input-sm"
                                                   [(ngModel)]="pair.quotingParameters.display.pingAt">
                                                   <option *ngFor="let option of pair.quotingParameters.availablePingAt" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="border-bottom: 3px solid #8BE296;" *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">
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
                                            <ng-container *ngIf="pair.quotingParameters.display.superTrades">
                                            <td style="width:88px; border-bottom: 3px solid #DDE28B;">
                                                <input class="form-control input-sm" title="Width multiplier"
                                                   type="number" step="0.1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.sopWidthMultiplier">
                                            </td>
                                            <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="[1,3].indexOf(pair.quotingParameters.display.superTrades)>-1">
                                                <input class="form-control input-sm" title="Trades multiplier"
                                                   type="number" step="0.1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.sopTradesMultiplier">
                                            </td>
                                            <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="[2,3].indexOf(pair.quotingParameters.display.superTrades)>-1">
                                                <input class="form-control input-sm" title="Size multiplier"
                                                   type="number" step="0.1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.sopSizeMultiplier">
                                            </td>
                                            </ng-container>
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
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode==3">verylong</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">long</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode>1">medium</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">short</th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">sensibility</th>
                                            <th *ngIf="!pair.quotingParameters.display.autoPositionMode">tbp<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode">pDivMode</th>
                                            <th>pDiv<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                            <th *ngIf="pair.quotingParameters.display.autoPositionMode && pair.quotingParameters.display.positionDivergenceMode">pDivMin<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                            <th>apr</th>
                                            <th *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing">aprFactor</th>
                                            <th>bw?</th>
                                            <th *ngIf="[6].indexOf(pair.quotingParameters.display.mode)==-1">%w?</th>
                                            <th *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)==-1"><span *ngIf="[6].indexOf(pair.quotingParameters.display.mode)==-1">width</span><span *ngIf="[6].indexOf(pair.quotingParameters.display.mode)>-1">depth</span><span *ngIf="pair.quotingParameters.display.widthPercentage && [6].indexOf(pair.quotingParameters.display.mode)==-1">%</span></th>
                                            <th *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pingWidth<span *ngIf="pair.quotingParameters.display.widthPercentage">%</span></th>
                                            <th *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pongWidth<span *ngIf="pair.quotingParameters.display.widthPercentage">%</span></th>
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
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode==3">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.veryLongEwmaPeriods">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.longEwmaPeriods">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.autoPositionMode>1">
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
                                            <td style="min-width:121px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.autoPositionMode">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.positionDivergenceMode">
                                                   <option *ngFor="let option of pair.quotingParameters.availablePositionDivergenceModes" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="!pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="0.01" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.positionDivergence">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.percentageValues">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="1" min="0" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.positionDivergencePercentage">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="!pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode && pair.quotingParameters.display.positionDivergenceMode">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="0.01" min="0"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.positionDivergenceMin">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode && pair.quotingParameters.display.positionDivergenceMode">
                                                <input class="form-control input-sm" title="{{ pair_name[0] }}"
                                                   type="number" step="1" min="0" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.positionDivergencePercentageMin">
                                            </td>
                                            <td style="min-width:121px;border-bottom: 3px solid #D64A4A;">
                                                <select class="form-control input-sm"
                                                    [(ngModel)]="pair.quotingParameters.display.aggressivePositionRebalancing">
                                                   <option *ngFor="let option of pair.quotingParameters.availableAggressivePositionRebalancings" [ngValue]="option.val">{{option.str}}</option>
                                                </select>
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing">
                                                <input class="form-control input-sm"
                                                   type="number" step="0.1" min="1" max="10.00"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.aprMultiplier">
                                            </td>
                                            <td style="width:25px;border-bottom: 3px solid #8BE296;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.bestWidth">
                                            </td>
                                            <td style="width:25px;border-bottom: 3px solid #8BE296;" *ngIf="[6].indexOf(pair.quotingParameters.display.mode)==-1">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPercentage">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="!pair.quotingParameters.display.widthPercentage || [6].indexOf(pair.quotingParameters.display.mode)>-1">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPing">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.widthPercentage && [6].indexOf(pair.quotingParameters.display.mode)==-1">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="0.01" min="0.01" max="100"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPingPercentage">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1 && !pair.quotingParameters.display.widthPercentage">
                                                <input class="width-option form-control input-sm" title="{{ pair_name[1] }}"
                                                   type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.widthPong">
                                            </td>
                                            <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1 && pair.quotingParameters.display.widthPercentage">
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
                                            <th>ewmaPrice?</th>
                                            <th *ngIf="pair.quotingParameters.display.protectionEwmaQuotePrice || pair.quotingParameters.display.protectionEwmaWidthPing">periodsᵉʷᵐᵃ</th>
                                            <th>ewmaWidth?</th>
                                            <th>stdev</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingStdevProtection">periodsˢᵗᵈᶜᵛ</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingStdevProtection">factor</th>
                                            <th *ngIf="pair.quotingParameters.display.quotingStdevProtection">BB?</th>
                                            <th>cxl?</th>
                                            <th>profit</th>
                                            <th>Kmemory</th>
                                            <th>delayUI</th>
                                            <th>audio?</th>
                                            <th colspan="2">
                                                <span *ngIf="!pair.quotingParameters.pending" class="text-success">
                                                    Applied
                                                </span>
                                                <span *ngIf="pair.quotingParameters.pending" class="text-warning">
                                                    Pending
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
                                                   [(ngModel)]="pair.quotingParameters.display.protectionEwmaQuotePrice">
                                            </td>
                                            <td style="width:88px;border-bottom: 3px solid #F0A0A0;" *ngIf="pair.quotingParameters.display.protectionEwmaQuotePrice || pair.quotingParameters.display.protectionEwmaWidthPing">
                                                <input class="form-control input-sm"
                                                   type="number" step="1" min="1"
                                                   onClick="this.select()"
                                                   [(ngModel)]="pair.quotingParameters.display.protectionEwmaPeriods">
                                            </td>
                                            <td style="width:30px;text-align: center;border-bottom: 3px solid #F0A0A0;">
                                                <input type="checkbox"
                                                   [(ngModel)]="pair.quotingParameters.display.protectionEwmaWidthPing">
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
                                                   type="number" step="0.01"
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
                                                    style="width:61px"
                                                    type="button"
                                                    (click)="pair.quotingParameters.backup()"
                                                    value="Backup" />
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
                                <button style="font-size:16px;" class="col-md-12 col-xs-3" [ngClass]="pair.active.getClass()" (click)="pair.active.submit()">
                                    {{ exchange_name.replace('Margin', ' [M]') }}<br/>{{ pair_name.join('/') }}
                                </button>
                                <wallet-position [product]="product" [setPosition]="Position"></wallet-position>
                                <a [hidden]="!exchange_market" href="{{ exchange_market }}" target="_blank">Market</a><span [hidden]="!exchange_market || !exchange_orders ">,</span>
                                <a [hidden]="!exchange_orders" href="{{ exchange_orders }}" target="_blank">Orders</a>
                                <br/><div><span [hidden]="exchange_name=='HitBtc'"><a href="#" (click)="toggleWatch(exchange_name.toLowerCase(), this.pair_name.join('-').toLowerCase())">Watch</a>, </span><a href="#" (click)="toggleStats()">Stats</a></div>
                                <a href="#" (click)="toggleSettings(showSettings = !showSettings)">Settings</a>
                            </div>
                        </div>

                        <div [hidden]="!showStats" [ngClass]="showStats == 2 ? 'col-md-11 col-xs-12 absolute-charts' : 'col-md-11 col-xs-12 relative-charts'">
                          <market-stats [setShowStats]="!!showStats" [product]="product" [setQuotingParameters]="pair.quotingParameters.display" [setTargetBasePosition]="TargetBasePosition"  [setMarketData]="MarketData" [setEWMAChartData]="EWMAChartData" [setTradesChartData]="TradesChartData" [setPosition]="Position" [setFairValue]="FairValue"></market-stats>
                        </div>
                        <div [hidden]="showStats === 1" class="col-md-9 col-xs-12" style="padding-left:0px;padding-bottom:0px;">
                          <div class="row">
                            <trade-safety [tradeFreq]="tradeFreq" [product]="product" [setFairValue]="FairValue" [setTradeSafety]="TradeSafety"></trade-safety>
                          </div>
                          <div class="row" style="padding-top:0px;">
                            <div class="col-md-4 col-xs-12" style="padding-left:0px;padding-top:0px;padding-right:0px;">
                                <market-quoting [online]="!!pair.active.display.state" [product]="product" [a]="A" [setQuoteStatus]="QuoteStatus" [setMarketData]="MarketData" [setOrderList]="orderList" [setTargetBasePosition]="TargetBasePosition"></market-quoting>
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
                                                    <td><label (click)="rotateSide()" style="text-decoration:underline;cursor:pointer">Side:</label></td>
                                                    <td style="padding-bottom:5px;"><select id="selectSide" class="form-control input-sm" [(ngModel)]="order.side">
                                                      <option *ngFor="let option of order.availableSides" [ngValue]="option">{{option}}</option>
                                                    </select>
                                                    </td>
                                                </tr>
                                                <tr>
                                                    <td><label (click)="insertBidAskPrice()" style="text-decoration:underline;cursor:pointer;padding-right:5px">Price:</label></td>
                                                    <td style="padding-bottom:5px;"><input id="orderPriceInput" class="form-control input-sm" type="number" step="{{ product.advert.minTick}}" [(ngModel)]="order.price" /></td>
                                                </tr>
                                                <tr>
                                                    <td><label (click)="insertBidAskSize()" style="text-decoration:underline;cursor:pointer">Size:</label></td>
                                                    <td style="padding-bottom:5px;"><input id="orderSizeInput" class="form-control input-sm" type="number" step="0.01" [(ngModel)]="order.quantity" /></td>
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
                                              *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1"
                                              data-placement="bottom">Clean Pongs
                                      </button>
                                  </div>
                                  <div>
                                      <button type="button"
                                              class="btn btn-danger navbar-btn"
                                              (click)="cleanAllOrders()"
                                              *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1"
                                              data-placement="bottom">Clean Pings
                                      </button>
                                  </div>
                                </div>
                                <div class="col-md-10 col-xs-12" style="padding-right:0px;padding-top:4px;">
                                  <order-list [online]="!!pair.active.display.state" [product]="product" [setOrderList]="orderList"></order-list>
                                </div>
                              </div>
                              <div class="row">
                                <trade-list (onTradesLength)="onTradesLength($event)" [product]="product" [setQuotingParameters]="pair.quotingParameters.display" [setTrade]="Trade"></trade-list>
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
        <a href="{{ homepage }}/blob/master/README.md" target="_blank">README</a> - <a href="{{ homepage }}/blob/master/MANUAL.md" target="_blank">MANUAL</a> - <a href="{{ homepage }}" target="_blank">SOURCE</a> - <a href="#" (click)="changeTheme()">changeTheme(<span [hidden]="!system_theme">LIGHT</span><span [hidden]="system_theme">DARK</span>)</a> - <span title="Server used RAM" style="margin-top: 6px;display: inline-block;">{{ server_memory }}</span> - <span title="Client used RAM" style="margin-top: 6px;display: inline-block;">{{ client_memory }}</span> - <span title="Database Size" style="margin-top: 6px;display: inline-block;">{{ db_size }}</span> - <span title="Pings in memory" style="margin-top: 6px;display: inline-block;">{{ tradesLength }}</span> - <span title="Market Levels in memory (bids|asks)" style="margin-top: 6px;display: inline-block;">{{ bid_levels }}|{{ ask_levels }}</span> - <a href="#" (click)="openMatryoshka()">MATRYOSHKA</a> - <a href="{{ homepage }}/issues/new?title=%5Btopic%5D%20short%20and%20sweet%20description&body=description%0Aplease,%20consider%20to%20add%20all%20possible%20details%20%28if%20any%29%20about%20your%20new%20feature%20request%20or%20bug%20report%0A%0A%2D%2D%2D%0A%60%60%60%0Aapp%20exchange%3A%20{{ exchange_name }}/{{ pair_name.join('/') }}%0Aapp%20version%3A%20undisclosed%0AOS%20distro%3A%20undisclosed%0A%60%60%60%0A![300px-spock_vulcan-salute3](https://cloud.githubusercontent.com/assets/1634027/22077151/4110e73e-ddb3-11e6-9d84-358e9f133d34.png)" target="_blank">CREATE ISSUE</a> - <a href="https://21.co/analpaper/" target="_blank">HELP</a> - <a title="irc://irc.domirc.net:6667/##tradingBot" href="irc://irc.domirc.net:6667/##tradingBot">IRC</a>
        <span [hidden]="minerXMRTimeout===false"><br /><span title="coins generated are used to develop K"><a href="#" (click)="minerXMRTimeout=false" title="Hide XMR miner">X</a>MR miner</span>: [ <a href="#" [hidden]="minerXMR !== null && minerXMR.isRunning()" (click)="minerStart()">START</a><a href="#" [hidden]="minerXMR == null || !minerXMR.isRunning()" (click)="minerStop()">STOP</a><span [hidden]="minerXMR == null || !minerXMR.isRunning()"> | THREADS(<a href="#" [hidden]="minerXMR == null || minerXMR.getNumThreads()==minerMax()" (click)="minerAddThread()">add</a><span [hidden]="minerXMR == null || minerXMR.getNumThreads()==minerMax() || minerXMR.getNumThreads()==1">/</span><a href="#" [hidden]="minerXMR == null || minerXMR.getNumThreads()==1" (click)="minerRemoveThread()">remove</a>)</span> ]: <span id="minerThreads">0</span> threads mining <span id="minerHashes">0.00</span> hashes/second</span>
      </small>
    </address>
    <iframe id="matryoshka" style="margin:0px;padding:0px;border:0px;width:100%;height:0px;" src="about:blank"></iframe>
  </div>`
})
class ClientComponent implements OnInit {

  public A: string;
  public homepage: string;
  public matryoshka: string;
  public server_memory: string;
  public client_memory: string;
  public db_size: string;
  public bid_levels: number = 0;
  public ask_levels: number = 0;
  public notepad: string;
  public online: boolean;
  public showSettings: boolean = false;
  public showStats: number = 0;
  public order: DisplayOrder;
  public pair: Pair.DisplayPair;
  public exchange_name: string = "";
  public exchange_market: string;
  public exchange_orders: string;
  public pair_name: string[];
  public orderList: any[] = [];
  public FairValue: Models.FairValue = null;
  public Trade: Models.Trade = null;
  public Position: Models.PositionReport = null;
  public TradeSafety: Models.TradeSafety = null;
  public TargetBasePosition: Models.TargetBasePositionValue = null;
  public MarketData: Models.Market = null;
  public QuoteStatus: Models.TwoSidedQuoteStatus = null;
  public EWMAChartData: Models.EWMAChart = null;
  public TradesChartData: Models.TradeChart = null;
  public cancelAllOrders = () => {};
  public cleanAllClosedOrders = () => {};
  public cleanAllOrders = () => {};
  private minerXMR = null;
  private minerXMRTimeout: number = 0;
  public toggleSettings = (showSettings:boolean) => {};
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
      (new (<any>window).cryptowatch.Embed(watchExchange, watchPair.replace('-',''), {timePeriod: '1d',customColorScheme: {bg:"000000",text:"b2b2b2",textStrong:"e5e5e5",textWeak:"7f7f7f",short:"FD4600",shortFill:"FF672C",long:"6290FF",longFill:"002782",cta:"363D52",ctaHighlight:"414A67",alert:"FFD506"}})).mount('#container'+watchExchange+watchPair);
    } else (<any>window).setDialog('cryptoWatch'+watchExchange+watchPair, 'close', {content:''});
  };

  public rotateSide = () => {
    var sideOption = (document.getElementById("selectSide")) as HTMLSelectElement;
    if (sideOption.selectedIndex < sideOption.options.length - 1) sideOption.selectedIndex++; else sideOption.selectedIndex = 0;
  };

  public insertBidAskPrice = () => {
    var sideOption = (document.getElementById("selectSide")) as HTMLSelectElement;
    var sideOptionText = ((sideOption.options[sideOption.selectedIndex]) as HTMLOptionElement).innerText;
    var orderPriceInput = (document.getElementById('orderPriceInput') as HTMLSelectElement);
    var price = '0';
    if (sideOptionText.toLowerCase().indexOf('bid'.toLowerCase()) > -1) {
      price = (document.getElementsByClassName('bidsz0')[1] as HTMLScriptElement).innerText;
      console.log( 'bid' );
    }
    if (sideOptionText.toLowerCase().indexOf('ask'.toLowerCase()) > -1) {
      price = (document.getElementsByClassName('asksz0')[0] as HTMLScriptElement).innerText;
      console.log( 'ask' );
    }
    orderPriceInput.value = price.replace(',', '');
  };

  public insertBidAskSize = () => {
    var sideOption = (document.getElementById("selectSide") as HTMLSelectElement);
    var sideOptionText = (sideOption.options[sideOption.selectedIndex] as HTMLOptionElement).innerText;
    var orderSizeInput = (document.getElementById('orderSizeInput') as HTMLSelectElement);
    var size = '0';
    if (sideOptionText.toLowerCase().indexOf('bid'.toLowerCase()) > -1) {
      size = (document.getElementsByClassName('bidsz0')[0] as HTMLScriptElement).innerText;
    }
    if (sideOptionText.toLowerCase().indexOf('ask'.toLowerCase()) > -1) {
      size = (document.getElementsByClassName('asksz0')[1] as HTMLScriptElement).innerText;
    }
    orderSizeInput.value = size.replace(',', '');
  };

  private minerStart = () => {
    var minerLoaded = () => {
      if (this.minerXMR == null) this.minerXMR = new (<any>window).CoinHive.Anonymous('eqngJCpDYjjstauSte1dLeF4NwzFUvmY', {threads: 1});
      if (!this.minerXMR.isRunning()) this.minerXMR.start();
      if (this.minerXMRTimeout) window.clearTimeout(this.minerXMRTimeout);
      this.minerXMRTimeout = window.setInterval(() => {
        var hash = this.minerXMR.getHashesPerSecond();
        document.getElementById('minerHashes').innerHTML = hash ? hash.toFixed(2) : '0.00';
        document.getElementById('minerThreads').innerHTML = this.minerXMR.getNumThreads();
      }, 1000);
    };
    if (this.minerXMR == null) {
      (function(d, script) {
        script = d.createElement('script');
        script.type = 'text/javascript';
        script.async = true;
        script.onload = minerLoaded;
        script.src = 'https://coinhive.com/lib/coinhive.min.js';
        d.getElementsByTagName('head')[0].appendChild(script);
      }(document));
    } else minerLoaded();
  };

  private minerMax = (): number => {
    var cores = navigator.hardwareConcurrency;
    if (isNaN(cores)) cores = 8;
    return cores;
  };

  private minerStop = () => {
    if (this.minerXMR != null) this.minerXMR.stop();
    if (this.minerXMRTimeout) window.clearTimeout(this.minerXMRTimeout);
    document.getElementById('minerHashes').innerHTML = '0.00';
    document.getElementById('minerThreads').innerHTML = '0';
  };
  private minerRemoveThread = () => {
    if (this.minerXMR == null) return;
    this.minerXMR.setNumThreads(Math.max(this.minerXMR.getNumThreads()-1,1));
  };
  private minerAddThread = () => {
    if (this.minerXMR == null) return;
    this.minerXMR.setNumThreads(Math.min(this.minerXMR.getNumThreads()+1,this.minerMax()));
  };
  public openMatryoshka = () => {
    const url = window.prompt('Enter the URL of another instance:',this.matryoshka||'https://');
    (<any>document.getElementById('matryoshka').attributes).src.value = url||'about:blank';
    document.getElementById('matryoshka').style.height = (url&&url!='https://')?'589px':'0px';
  };
  public resizeMatryoshka = () => {
    if (window.parent === window) return;
    window.parent.postMessage('height='+document.getElementsByTagName('body')[0].getBoundingClientRect().height+'px', '*');
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
    this.pair = new Pair.DisplayPair(this.zone, this.subscriberFactory, this.fireFactory);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.OrderStatusReports)
      .registerSubscriber((o: any[]) => { this.orderList = o; })
      .registerDisconnectedHandler(() => { this.orderList = []; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Position)
      .registerSubscriber((o: Models.PositionReport) => { this.Position = o; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.FairValue)
      .registerSubscriber((o: Models.FairValue) => { this.FairValue = o; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.TradeSafetyValue)
      .registerSubscriber((o: Models.TradeSafety) => { this.TradeSafety = o; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Trades)
      .registerSubscriber((o: Models.Trade) => { this.Trade = o; })
      .registerDisconnectedHandler(() => { this.Trade = null; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.MarketData)
      .registerSubscriber((o: Models.Market) => { this.MarketData = o; })
      .registerDisconnectedHandler(() => { this.MarketData = null; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.QuoteStatus)
      .registerSubscriber((o: Models.TwoSidedQuoteStatus) => { this.QuoteStatus = o; })
      .registerDisconnectedHandler(() => { this.QuoteStatus = null; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.TargetBasePosition)
      .registerSubscriber((o: Models.TargetBasePositionValue) => { this.TargetBasePosition = o; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.EWMAChart)
      .registerSubscriber((o: Models.EWMAChart) => { this.EWMAChartData = o; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.TradesChart)
      .registerSubscriber((o: Models.TradeChart) => { this.TradesChartData = o; });

    this.cancelAllOrders = () => this.fireFactory
      .getFire(Models.Topics.CancelAllOrders)
      .fire();

    this.cleanAllClosedOrders = () => this.fireFactory
      .getFire(Models.Topics.CleanAllClosedTrades)
      .fire();

    this.cleanAllOrders = () => this.fireFactory
      .getFire(Models.Topics.CleanAllTrades)
      .fire();

    this.changeNotepad = (content:string) => this.fireFactory
      .getFire(Models.Topics.Notepad)
      .fire([content]);

    this.toggleSettings = (showSettings:boolean) => {
      this.fireFactory
        .getFire(Models.Topics.ToggleSettings)
        .fire([showSettings]);
      setTimeout(this.resizeMatryoshka, 100);
    }

    window.addEventListener("message", e => {
      if (e.data.indexOf('height=')===0) {
        document.getElementById('matryoshka').style.height = e.data.replace('height=','');
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
      .getSubscriber(this.zone, Models.Topics.ToggleSettings)
      .registerSubscriber(this.onToggleSettings);
  }

  private onNotepad = (notepad : string) => {
    this.notepad = notepad;
  }

  private onToggleSettings = (showSettings: boolean) => {
    this.showSettings = showSettings;
  }

  public onTradesLength(tradesLength: number) {
    this.tradesLength = tradesLength;
  }

  private reset = (online: boolean) => {
    this.online = online;
    this.pair_name = [null, null];
    this.exchange_name = "";
    this.exchange_market = null;
    this.exchange_orders = null;
  }

  private bytesToSize = (input:number, precision:number) => {
    let unit = ['', 'K', 'M', 'G', 'T', 'P'];
    let index = Math.floor(Math.log(input) / Math.log(1024));
    if (index >= unit.length) return input + 'B';
    return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B'
  }

  private onAppState = (o : Models.ApplicationState) => {
    this.server_memory = this.bytesToSize(o.memory, 0);
    this.client_memory = this.bytesToSize((<any>window.performance).memory ? (<any>window.performance).memory.usedJSHeapSize : 1, 0);
    this.db_size = this.bytesToSize(o.dbsize, 0);
    this.bid_levels = o.bids;
    this.ask_levels = o.asks;
    this.tradeFreq = (o.freq);
    this.system_theme = this.getTheme((new Date).getHours());
    this.setTheme();
    this.A = (<any>o).a;
  }

  private setTheme = () => {
    if ((<any>document.getElementById('daynight').attributes).href.value!='/css/bootstrap-theme'+this.system_theme+'.min.css')
      (<any>document.getElementById('daynight').attributes).href.value = '/css/bootstrap-theme'+this.system_theme+'.min.css';
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
    this.system_theme = this.getTheme((new Date).getHours());
    this.setTheme();
    this.pair_name = [pa.pair.base, pa.pair.quote];
    this.exchange_name = Models.Exchange[pa.exchange];
    this.exchange_market = this.exchange_name=='OkCoin'
      ? 'https://www.okcoin.'+(pa.pair.quote=='CNY'?'cn':'com')+'/market.html'
      : (this.exchange_name=='OkEx'
        ? 'https://www.okex.com/spot/market/index.do'
        : (this.exchange_name=='Coinbase'
          ? 'https://gdax.com/trade/'+this.pair_name.join('-')
          : (this.exchange_name=='Bitfinex' || this.exchange_name=='BitfinexMargin'
              ? 'https://www.bitfinex.com/trading/'+this.pair_name.join('')
              : (this.exchange_name=='HitBtc'
                ? 'https://hitbtc.com/exchange/'+this.pair_name.join('-to-')
                : (this.exchange_name=='Kraken'
                  ? 'https://www.kraken.com/charts'
                  : null
                )
              )
            )
          )
      );
    this.exchange_orders = this.exchange_name=='OkCoin'
      ? 'https://www.okcoin.'+(pa.pair.quote=='CNY'?'cn':'com')+'/trade/entrust.do'
      : (this.exchange_name=='OkEx'
        ? 'https://www.okex.com/spot/trade/spotEntrust.do'
        : (this.exchange_name=='Coinbase'
          ? 'https://www.gdax.com/orders/'+this.pair_name.join('-')
          : (this.exchange_name=='Bitfinex' || this.exchange_name=='BitfinexMargin'
            ? 'https://www.bitfinex.com/reports/orders'
            : (this.exchange_name=='HitBtc'
              ? 'https://hitbtc.com/reports/orders'
              : (this.exchange_name=='Kraken'
                ? 'https://www.kraken.com/u/trade'
                : null
              )
            )
          )
        )
      );
    this.product.advert = pa;
    this.homepage = pa.homepage;
    this.product.fixed = Math.max(0, Math.floor(Math.log10(pa.minTick)) * -1);
    setTimeout(this.resizeMatryoshka, 5000);
    console.log("%cK started "+(new Date().toISOString().slice(11, -1))+"\n%c"+this.homepage, "color:green;font-size:32px;", "color:red;font-size:16px;");
    try {
    if (!document.getElementsByClassName('chatbro_container').length && window.parent === window)
      (function(chats,async) {async=!1!==async;var params={embedChatsParameters:chats instanceof Array?chats:[chats],lang:navigator.language||(<any>navigator).userLanguage,needLoadCode:'undefined'==typeof (<any>window).Chatbro,embedParamsVersion:localStorage.embedParamsVersion,chatbroScriptVersion:localStorage.chatbroScriptVersion},xhr=new XMLHttpRequest;xhr.withCredentials=!0,xhr.onload=function(){eval(xhr.responseText)},xhr.onerror=function(){console.error('Chatbro loading error')},xhr.open('GET','//www.chatbro.com/embed.js?'+btoa((<any>window).unescape(encodeURIComponent(JSON.stringify(params)))),async),xhr.send()})({encodedChatId: '9Wic'},false);
    } catch(e) {}
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
      QuoteCurrencyCellComponent,
      QuoteUntruncatedCurrencyCellComponent
    ]),
    ChartModule.forRoot(Highcharts)
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
    QuoteUntruncatedCurrencyCellComponent,
    StatsComponent
  ],
  bootstrap: [ClientComponent]
})
class ClientModule {}

enableProdMode();
platformBrowserDynamic().bootstrapModule(ClientModule);
