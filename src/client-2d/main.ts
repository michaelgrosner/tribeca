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
import {SharedModule, FireFactory, SubscriberFactory, BaseCurrencyCellComponent, QuoteCurrencyCellComponent} from './shared_directives';
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
            <div id="hud" [ngClass]="pair.connected ? 'bg-success img-rounded' : 'bg-danger img-rounded'">
                <div *ngIf="online" class="row" [hidden]="!showSettings">
                    <div class="col-md-12 col-xs-12">
                        <div class="row">
                          <table border="0" width="100%"><tr><td style="width:69px;text-align:center;border-bottom: 1px gray solid;">
                            <small>MARKET<br/>MAKING</small>
                          </td><td>
                            <table class="table table-responsive" style="margin-bottom:0px;">
                                <thead>
                                    <tr class="active">
                                        <th title="If enabled, the values of bidSize, askSize, tbp, pDiv and range will be a percentage related to the total funds.">%</th>
                                        <th title="Sets the quoting mode">mode</th>
                                        <th title="Sets a quoting Safety">safety</th>
                                        <th title="Maximum amount of trades placed in each side." *ngIf="pair.quotingParameters.display.safety==3">bullets</th>
                                        <th title="Minimum width between bullets in USD (ex. a value of .3 is 30 cents)." *ngIf="pair.quotingParameters.display.safety==3 && !pair.quotingParameters.display.percentageValues">range</th>
                                        <th title="Minimum width between bullets in USD (ex. a value of .3 is 30 cents)." *ngIf="pair.quotingParameters.display.safety==3 && pair.quotingParameters.display.percentageValues">range%</th>
                                        <th title="Pongs are always placed in both sides." *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pingAt</th>
                                        <th title="" *ngIf="[2,3].indexOf(pair.quotingParameters.display.safety)>-1">pongAt</th>
                                        <th title="Super opportunities, if enabled and if the market width is sopWidth times bigger than the width set, it multiplies sopTrades to trades and/or sopSize to size, in both sides at the same time.">sop</th>
                                        <ng-container *ngIf="pair.quotingParameters.display.superTrades">
                                        <th title="The value with the market width is multiplicated to define the activation point for Super opportunities.">sopWidth</th>
                                        <th title="Multiplicates trades to rise the possible Trades per Minute if sop is in Trades or tradesSize state." *ngIf="[1,3].indexOf(pair.quotingParameters.display.superTrades)>-1">sopTrades</th>
                                        <th title="Multiplicates width if sop is in Size or tradesSize state." *ngIf="[2,3].indexOf(pair.quotingParameters.display.superTrades)>-1">sopSize</th>
                                        </ng-container>
                                        <th title="Maximum bid size of our quote in BTC (ex. a value of 1.5 is 1.5 bitcoins). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack." [attr.colspan]="pair.quotingParameters.display.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && pair.quotingParameters.display.buySizeMax">minB</span><span *ngIf="!pair.quotingParameters.display.aggressivePositionRebalancing || !pair.quotingParameters.display.buySizeMax">b</span>idSize<span *ngIf="pair.quotingParameters.display.percentageValues">%</span><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing" style="float:right;">maxBidSize?</span></th>
                                        <th title="Maximum ask size of our quote in BTC (ex. a value of 1.5 is 1.5 bitcoins). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack." [attr.colspan]="pair.quotingParameters.display.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing && pair.quotingParameters.display.sellSizeMax">minA</span><span *ngIf="!pair.quotingParameters.display.aggressivePositionRebalancing || !pair.quotingParameters.display.sellSizeMax">a</span>skSize<span *ngIf="pair.quotingParameters.display.percentageValues">%</span><span *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing" style="float:right;">maxAskSize?</span></th>
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
                                            <input class="form-control input-sm" title="{{ quoteCurrency }}"
                                               type="number"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.range">
                                        </td>
                                        <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.safety==3 && pair.quotingParameters.display.percentageValues">
                                            <input class="form-control input-sm" title="{{ quoteCurrency }}"
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
                                        <td style="border-bottom: 3px solid #8BE296;" *ngIf="[2,3].indexOf(pair.quotingParameters.display.safety)>-1">
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
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="0.01" min="0.01"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.buySize">
                                        </td>
                                        <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.percentageValues">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="1" min="1" max="100"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.buySizePercentage">
                                        </td>
                                        <td style="border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing">
                                            <input type="checkbox"
                                               [(ngModel)]="pair.quotingParameters.display.buySizeMax">
                                        </td>
                                        <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="!pair.quotingParameters.display.percentageValues">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="0.01" min="0.01"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.sellSize">
                                        </td>
                                        <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="pair.quotingParameters.display.percentageValues">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
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
                            <table class="table table-responsive" style="margin-bottom:0px;">
                                <thead>
                                    <tr class="active">
                                        <th title="Automatic position management">apMode</th>
                                        <th title="Sets the periods of EWMA VeryLong to automatically manage positions." *ngIf="pair.quotingParameters.display.autoPositionMode==3">verylong</th>
                                        <th title="Sets the periods of EWMA Long to automatically manage positions." *ngIf="pair.quotingParameters.display.autoPositionMode">long</th>
                                        <th title="Sets the periods of EWMA Medium to automatically manage positions." *ngIf="pair.quotingParameters.display.autoPositionMode>1">medium</th>
                                        <th title="Sets the periods of EWMA Short to automatically manage positions." *ngIf="pair.quotingParameters.display.autoPositionMode">short</th>
                                        <th title="Threshold removed from each period, affects EWMA Long, Medium and Short. The decimal value must be betweem 0 and 1." *ngIf="pair.quotingParameters.display.autoPositionMode">sensibility</th>
                                        <th title="Sets a static Target Base Position for Krypto-trading-bot to stay near. Krypto-trading-bot will still try to respect pDiv and not make your position fluctuate by more than that value." *ngIf="!pair.quotingParameters.display.autoPositionMode">tbp<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                        <th title="Sets the strategy of dynamically adjusting the pDiv depending on the divergence from 50% of Base Value." *ngIf="pair.quotingParameters.display.autoPositionMode">pDivMode</th>
                                        <th title="If your Target Base Position diverges more from this value, Krypto-trading-bot will stop sending orders to stop too much directional trading.">pDiv<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                        <th title="It defines the minimal pDiv for the dynamic positon divergence." *ngIf="pair.quotingParameters.display.autoPositionMode && pair.quotingParameters.display.positionDivergenceMode">pDivMin<span *ngIf="pair.quotingParameters.display.percentageValues">%</span></th>
                                        <th title="If you're in a state where Krypto-trading-bot has stopped sending orders because your position has diverged too far from Target Base Position, this setting will much more aggressively try to fix that discrepancy by placing orders much larger than size and at prices much more aggressive than width normally allows.">apr</th>
                                        <th title="Defines the value with which the size is multiplicated when apr is in functional state." *ngIf="pair.quotingParameters.display.aggressivePositionRebalancing">aprFactor</th>
                                        <th title="Enable Best Width to place orders avoiding hollows in the book, while accomodating new orders right near to existent orders in the book." >bw?</th>
                                        <th title="The values of width or widthPing and widthPong will be a percentage related to the fair value; useful when calculating profits subtracting exchange's fees (that usually are percentages too)." *ngIf="[6].indexOf(pair.quotingParameters.display.mode)==-1">%w?</th>
                                        <th title="Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack, width will always be respected." *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)==-1"><span *ngIf="[6].indexOf(pair.quotingParameters.display.mode)==-1">width</span><span *ngIf="[6].indexOf(pair.quotingParameters.display.mode)>-1">depth</span><span *ngIf="pair.quotingParameters.display.widthPercentage && [6].indexOf(pair.quotingParameters.display.mode)==-1">%</span></th>
                                        <th title="Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack, width will always be respected." *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pingWidth<span *ngIf="pair.quotingParameters.display.widthPercentage">%</span></th>
                                        <th title="Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). Used only if previous Pings exists in the opposite side." *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1">pongWidth<span *ngIf="pair.quotingParameters.display.widthPercentage">%</span></th>
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
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="0.01" min="0"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.targetBasePosition">
                                        </td>
                                        <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode==0">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
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
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="0.01" min="0"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.positionDivergence">
                                        </td>
                                        <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.percentageValues">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="1" min="0" max="100"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.positionDivergencePercentage">
                                        </td>
                                        <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="!pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode && pair.quotingParameters.display.positionDivergenceMode">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
                                               type="number" step="0.01" min="0"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.positionDivergenceMin">
                                        </td>
                                        <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="pair.quotingParameters.display.percentageValues && pair.quotingParameters.display.autoPositionMode && pair.quotingParameters.display.positionDivergenceMode">
                                            <input class="form-control input-sm" title="{{ baseCurrency }}"
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
                                            <input class="width-option form-control input-sm" title="{{ quoteCurrency }}"
                                               type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.widthPing">
                                        </td>
                                        <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="pair.quotingParameters.display.widthPercentage && [6].indexOf(pair.quotingParameters.display.mode)==-1">
                                            <input class="width-option form-control input-sm" title="{{ quoteCurrency }}"
                                               type="number" step="0.001" min="0.001" max="100"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.widthPingPercentage">
                                        </td>
                                        <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1 && !pair.quotingParameters.display.widthPercentage">
                                            <input class="width-option form-control input-sm" title="{{ quoteCurrency }}"
                                               type="number" step="{{ product.advert.minTick}}" min="{{ product.advert.minTick}}"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.widthPong">
                                        </td>
                                        <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="[1,2,3].indexOf(pair.quotingParameters.display.safety)>-1 && pair.quotingParameters.display.widthPercentage">
                                            <input class="width-option form-control input-sm" title="{{ quoteCurrency }}"
                                               type="number" step="0.001" min="0.001" max="100"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.widthPongPercentage">
                                        </td>
                                </tbody>
                            </table>
                          </td></tr></table>
                          <table border="0" width="100%"><tr><td style="width:69px;text-align:center;">
                            <small>PROTECTION</small>
                          </td><td>
                            <table class="table table-responsive">
                                <thead>
                                    <tr class="active">
                                        <th title="Sets the fair value calculation mode">fv</th>
                                        <th title="If you successfully complete more orders than trades in /sec seconds, Krypto-trading-bot will stop sending more buy orders until either /sec seconds has passed, or you have sold enough at a higher cost to make all those buy orders profitable." style="text-align:right;">trades</th>
                                        <th title="If you successfully complete more orders than trades in /sec seconds, Krypto-trading-bot will stop sending more buy orders until either /sec seconds has passed, or you have sold enough at a higher cost to make all those buy orders profitable.">/sec</th>
                                        <th title="Use a quote protection of periods smoothed line of the fair value to limit the price while sending new orders.">ewmaPrice?</th>
                                        <th title="Maximum amount of values collected in the sequences used to calculate the ewmaPrice? and ewmaWidth? quote protection." *ngIf="pair.quotingParameters.display.protectionEwmaQuotePrice || pair.quotingParameters.display.protectionEwmaWidthPing">periodsᵉʷᵐᵃ</th>
                                        <th title="Use a quote protection of periods smoothed line of the width (between the top bid and the top ask) to limit the widthPing while sending new orders.">ewmaWidth?</th>
                                        <th title="Use a trend protection of double periods (Ultra+Micro) smoothed lines of the price to limit uptrend sells and downtrend buys.">ewmaTrend?</th>
                                        <th title="When trend stregth is above positive threshold value bot stops selling, when strength below negative threshold value bot stops buying" *ngIf="pair.quotingParameters.display.quotingEwmaTrendProtection">threshold</th>
                                        <th title="Time in minutes to define Micro EMA" *ngIf="pair.quotingParameters.display.quotingEwmaTrendProtection">micro</th>
                                        <th title="Time in minutes to define Ultra EMA" *ngIf="pair.quotingParameters.display.quotingEwmaTrendProtection">ultra</th>
                                        <th title="Limit the price of new orders.">stdev</th>
                                        <th title="Maximum amount of values collected in the sequences used to calculate the STDEV, each side may have its own STDEV calculation with the same amount of periods." *ngIf="pair.quotingParameters.display.quotingStdevProtection">periodsˢᵗᵈᶜᵛ</th>
                                        <th title="Multiplier used to increase or decrease the value of the selected stdev calculation, a factor of 1 does effectively nothing." *ngIf="pair.quotingParameters.display.quotingStdevProtection">factor</th>
                                        <th title="Enable Bollinger Bands with upper and lower bands calculated from the result of the selected stdev above or below its own moving average of periods." *ngIf="pair.quotingParameters.display.quotingStdevProtection">BB?</th>
                                        <th title="Enable a timeout of 5 minutes to cancel all orders that exist as open in the exchange (in case you found yourself with zombie orders in the exchange, because the API integration have bugs or because the connection is interrupted).">cxl?</th>
                                        <th title="Timeframe in hours to calculate the display of Profit (under wallet values) and also interval in hour to remove data points from the Stats.">profit</th>
                                        <th title="Timeout in days for Pings (yet unmatched trades) and/or Pongs (K trades) to remain in memory, a value of 0 keeps the history in memory forever; a positive value remove only Pongs after Kmemory days; but a negative value remove both Pings and Pongs after Kmemory days.">Kmemory</th>
                                        <th title="Relax the display of UI data by delayUI seconds. Set a value of 0 (zero) to display UI data in realtime, but this may penalize the communication with the exchange if you end up sending too much frequent UI data.">delayUI</th>
                                        <th title="Plays a sound for each new trade (ping-pong modes have 2 sounds for each type of trade).">audio?</th>
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
                                        <td style="text-align: center;border-bottom: 3px solid #fd00ff;">
                                            <input type="checkbox"
                                               [(ngModel)]="pair.quotingParameters.display.quotingEwmaTrendProtection">
                                        </td>
                                        <td style="width:60px;border-bottom: 3px solid #fd00ff;" *ngIf="pair.quotingParameters.display.quotingEwmaTrendProtection">
                                        <input class="form-control input-sm"
                                            type="number" step="0.1" min="0.1"
                                               onClick="this.select()"
                                               [(ngModel)]="pair.quotingParameters.display.quotingEwmaTrendThreshold">
                                        </td>
                                        <td style="width:60px;border-bottom: 3px solid #fd00ff;" *ngIf="pair.quotingParameters.display.quotingEwmaTrendProtection">
                                            <input class="form-control input-sm"
                                              type="number" step="1" min="1"
                                                onClick="this.select()"
                                                [(ngModel)]="pair.quotingParameters.display.extraShortEwmaPeriods">
                                        </td>
                                        <td style="width:60px;border-bottom: 3px solid #fd00ff;" *ngIf="pair.quotingParameters.display.quotingEwmaTrendProtection">
                                            <input class="form-control input-sm"
                                                type="number" step="1" min="1"
                                                onClick="this.select()"
                                                [(ngModel)]="pair.quotingParameters.display.ultraShortEwmaPeriods">
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
                                {{ exchange_name.replace('Margin', ' [M]') }}<br/>{{ baseCurrency+'/'+quoteCurrency }}
                            </button>
                            <wallet-position [baseCurrency]="baseCurrency" [quoteCurrency]="quoteCurrency" [product]="product" [setPosition]="Position"></wallet-position>
                            <div>
                              <a [hidden]="!exchange_market" href="{{ exchange_market }}" target="_blank">Market</a>, <a [hidden]="!exchange_orders" href="{{ exchange_orders }}" target="_blank">Orders</a>
                              <br/><br/><div>
                                  <button type="button"
                                          class="btn btn-xs btn-primary navbar-btn"
                                          id="order_form"
                                          [popover]="myPopover">Submit Order
                                  </button>
                              </div>
                              <div style="padding-top: 2px;padding-bottom: 2px;">
                                  <button type="button"
                                          class="btn btn-xs btn-danger navbar-btn"
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
                                          class="btn btn-xs btn-danger navbar-btn"
                                          (click)="cleanAllOrders()"
                                          data-placement="bottom">{{ [1,2,3].indexOf(pair.quotingParameters.display.safety)>-1 ? 'Clean Pings' : 'Clean Trades' }}
                                  </button>
                              </div>
                              <br [hidden]="exchange_name=='HitBtc'" /><a [hidden]="exchange_name=='HitBtc'" href="#" (click)="toggleWatch(exchange_name.toLowerCase(), (baseCurrency+'-'+quoteCurrency).toLowerCase())">Watch</a><br [hidden]="exchange_name=='HitBtc'" />
                              <br/><a href="#" (click)="toggleTakers()">Takers</a>, <a href="#" (click)="toggleStats()">Stats</a>
                              <br/><a href="#" (click)="toggleSettings(showSettings = !showSettings)">Settings</a>
                              <br/><a href="#" (click)="changeTheme()">{{ system_theme ? 'Light' : 'Dark' }}</a>
                            </div>
                        </div>
                    </div>

                    <div [hidden]="!showStats" [ngClass]="showStats == 2 ? 'col-md-11 col-xs-12 absolute-charts' : 'col-md-11 col-xs-12 relative-charts'">
                      <market-stats ondblclick="this.style.opacity=this.style.opacity<1?1:0.4" [baseCurrency]="baseCurrency" [quoteCurrency]="quoteCurrency" [setMarketWidth]="marketWidth" [setShowStats]="!!showStats" [product]="product" [setQuotingParameters]="pair.quotingParameters.display" [setTargetBasePosition]="TargetBasePosition" [setMarketChartData]="MarketChartData" [setTradesChartData]="TradesChartData" [setPosition]="Position" [setFairValue]="FairValue"></market-stats>
                    </div>
                    <div [hidden]="showStats === 1" class="col-md-{{ showTakers ? '9' : '11' }} col-xs-12" style="padding-left:0px;padding-bottom:0px;">
                      <div class="row">
                        <trade-safety [tradeFreq]="tradeFreq" [product]="product" [setFairValue]="FairValue" [setTradeSafety]="TradeSafety"></trade-safety>
                      </div>
                      <div class="row" style="padding-top:0px;">
                        <div class="col-md-4 col-xs-12" style="padding-left:0px;padding-top:0px;padding-right:0px;">
                            <market-quoting (onBidsLength)="onBidsLength($event)" (onAsksLength)="onAsksLength($event)" (onMarketWidth)="onMarketWidth($event)" [online]="!!pair.active.display.state" [product]="product" [a]="A" [setQuoteStatus]="QuoteStatus" [setMarketData]="MarketData" [setOrderList]="orderList" [setTargetBasePosition]="TargetBasePosition"></market-quoting>
                        </div>
                        <div class="col-md-8 col-xs-12" style="padding-left:0px;padding-right:0px;padding-top:0px;">
                          <div class="row">
                            <div class="exchangeActions col-md-2 col-xs-12 text-center img-rounded">
                              <textarea [(ngModel)]="notepad" (ngModelChange)="changeNotepad(notepad)" placeholder="ephemeral notepad" class="ephemeralnotepad" style="height:131px;width: 100%;max-width: 100%;"></textarea>
                            </div>
                            <div class="col-md-10 col-xs-12" style="padding-right:0px;padding-top:4px;">
                              <order-list [online]="!!pair.active.display.state" [product]="product" [setOrderList]="orderList"></order-list>
                            </div>
                          </div>
                          <div class="row">
                            <trade-list (onTradesChartData)="onTradesChartData($event)" (onTradesLength)="onTradesLength($event)" [product]="product" [setQuotingParameters]="pair.quotingParameters.display" [setTrade]="Trade"></trade-list>
                          </div>
                        </div>
                      </div>
                    </div>
                    <div [hidden]="!showTakers || showStats === 1" class="col-md-2 col-xs-12" style="padding-left:0px;">
                      <market-trades [product]="product"></market-trades>
                    </div>
                </div>
                <popover-content #myPopover
                    placement="right"
                    [animation]="true"
                    [closeOnClickOutside]="true">
                        <table border="0" style="width:139px;z-index:3;">
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
        </div>
    </div>
    <address class="text-center">
      <small>
        <a href="{{ homepage }}/blob/master/README.md" target="_blank">README</a> - <a href="{{ homepage }}/blob/master/MANUAL.md" target="_blank">MANUAL</a> - <a href="{{ homepage }}" target="_blank">SOURCE</a> - <span [hidden]="!online"><span [hidden]="!inet"><span title="non-default Network Interface for outgoing traffic">{{ inet }}</span> - </span><span title="Server used RAM" style="margin-top: 6px;display: inline-block;">{{ server_memory }}</span> - <span title="Client used RAM" style="margin-top: 6px;display: inline-block;">{{ client_memory }}</span> - <span title="Database Size" style="margin-top: 6px;display: inline-block;">{{ db_size }}</span> - <span title="Pings in memory" style="margin-top: 6px;display: inline-block;">{{ tradesLength }}</span> - <span title="Market Levels in memory (bids|asks)" style="margin-top: 6px;display: inline-block;">{{ bidsLength }}|{{ asksLength }}</span> - </span><a href="#" (click)="openMatryoshka()">MATRYOSHKA</a> - <a href="{{ homepage }}/issues/new?title=%5Btopic%5D%20short%20and%20sweet%20description&body=description%0Aplease,%20consider%20to%20add%20all%20possible%20details%20%28if%20any%29%20about%20your%20new%20feature%20request%20or%20bug%20report%0A%0A%2D%2D%2D%0A%60%60%60%0Aapp%20exchange%3A%20{{ exchange_name }}/{{ baseCurrency+'/'+quoteCurrency }}%0Aapp%20version%3A%20undisclosed%0AOS%20distro%3A%20undisclosed%0A%60%60%60%0A![300px-spock_vulcan-salute3](https://cloud.githubusercontent.com/assets/1634027/22077151/4110e73e-ddb3-11e6-9d84-358e9f133d34.png)" target="_blank">CREATE ISSUE</a> - <a href="https://earn.com/analpaper/" target="_blank">HELP</a> - <a title="irc://irc.freenode.net:6697/#tradingBot" href="irc://irc.freenode.net:6697/#tradingBot">IRC</a>|<a target="_blank" href="https://kiwiirc.com/client/irc.freenode.net:6697/?theme=cli#tradingBot" rel="nofollow">www</a>
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
  public inet: string = "";
  public db_size: string;
  public notepad: string;
  public online: boolean;
  public showSettings: boolean = true;
  public showTakers: boolean = false;
  public showStats: number = 0;
  public order: DisplayOrder;
  public pair: Pair.DisplayPair;
  public exchange_name: string = "";
  public exchange_market: string;
  public exchange_orders: string;
  public orderList: any[] = [];
  public FairValue: Models.FairValue = null;
  public Trade: Models.Trade = null;
  public Position: Models.PositionReport = null;
  public TradeSafety: Models.TradeSafety = null;
  public TargetBasePosition: Models.TargetBasePositionValue = null;
  public MarketData: Models.Market = null;
  public QuoteStatus: Models.TwoSidedQuoteStatus = null;
  public MarketChartData: Models.MarketChart = null;
  public TradesChartData: Models.TradeChart = null;
  public cancelAllOrders = () => {};
  public cleanAllClosedOrders = () => {};
  public cleanAllOrders = () => {};
  public toggleSettings = (showSettings:boolean) => {
    setTimeout(this.resizeMatryoshka, 100);
  };
  public changeNotepad = (content: string) => {};
  public toggleTakers = () => {
    this.showTakers = !this.showTakers;
  };
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
  public baseCurrency: string = "?";
  public quoteCurrency: string = "?";

  private user_theme: string = null;
  private system_theme: string = null;
  public tradeFreq: number = 0;
  public tradesChart: Models.TradeChart = null;
  public tradesLength: number = 0;
  public bidsLength: number = 0;
  public asksLength: number = 0;
  public marketWidth: number = 0;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(SubscriberFactory) private subscriberFactory: SubscriberFactory,
    @Inject(FireFactory) private fireFactory: FireFactory
  ) {}

  ngOnInit() {
    this.pair = new Pair.DisplayPair(this.zone, this.subscriberFactory, this.fireFactory);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.ProductAdvertisement)
      .registerSubscriber(this.onAdvert)
      .registerDisconnectedHandler(() => this.reset(false));

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
      .getSubscriber(this.zone, Models.Topics.MarketChart)
      .registerSubscriber((o: Models.MarketChart) => { this.MarketChartData = o; });

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

    window.addEventListener("message", e => {
      if (!e.data.indexOf) return;

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
      .getSubscriber(this.zone, Models.Topics.ApplicationState)
      .registerSubscriber(this.onAppState);

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Notepad)
      .registerSubscriber(this.onNotepad);
  }

  private onNotepad = (notepad : string) => {
    this.notepad = notepad;
  }

  public onTradesChartData(tradesChart: Models.TradeChart) {
    this.TradesChartData = tradesChart;
  }
  public onTradesLength(tradesLength: number) {
    this.tradesLength = tradesLength;
  }
  public onBidsLength(bidsLength: number) {
    this.bidsLength = bidsLength;
  }
  public onAsksLength(asksLength: number) {
    this.asksLength = asksLength;
  }
  public onMarketWidth(marketWidth: number) {
    this.marketWidth = marketWidth;
  }

  private reset = (online: boolean) => {
    this.online = online;
    this.baseCurrency = "";
    this.quoteCurrency = "";
    this.exchange_name = "";
    this.exchange_market = null;
    this.exchange_orders = null;
  }

  private bytesToSize = (input:number, precision:number) => {
    if (!input) return '0B';
    let unit = ['', 'K', 'M', 'G', 'T', 'P'];
    let index = Math.floor(Math.log(input) / Math.log(1024));
    if (index >= unit.length) return input + 'B';
    return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B'
  }

  private onAppState = (o : Models.ApplicationState) => {
    this.inet = o.inet;
    this.server_memory = this.bytesToSize(o.memory, 0);
    this.client_memory = this.bytesToSize((<any>window.performance).memory ? (<any>window.performance).memory.usedJSHeapSize : 1, 0);
    this.db_size = this.bytesToSize(o.dbsize, 0);
    this.tradeFreq = (o.freq);
    this.user_theme = this.user_theme!==null ? this.user_theme : (o.theme==1 ? '' : (o.theme==2 ? '-dark' : this.user_theme));
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
    this.baseCurrency = pa.pair.base;
    this.quoteCurrency = pa.pair.quote;
    this.exchange_name = Models.Exchange[pa.exchange];
    this.exchange_market = this.exchange_name=='OkCoin'
      ? 'https://www.okcoin.'+(pa.pair.quote=='CNY'?'cn':'com')+'/market.html'
      : (this.exchange_name=='OkEx'
        ? 'https://www.okex.com/spot/market/index.do'
        : (this.exchange_name=='Coinbase'
          ? 'https://gdax.com/trade/'+this.baseCurrency+'-'+this.quoteCurrency
          : (this.exchange_name=='Bitfinex' || this.exchange_name=='BitfinexMargin'
              ? 'https://www.bitfinex.com/trading/'+this.baseCurrency+this.quoteCurrency
              : (this.exchange_name=='HitBtc'
                ? 'https://hitbtc.com/exchange/'+this.baseCurrency+'-to-'+this.quoteCurrency
                : (this.exchange_name=='Kraken'
                  ? 'https://www.kraken.com/charts'
                  : (this.exchange_name=='Poloniex'
                    ? 'https://poloniex.com/exchange'
                    : null
                  )
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
          ? 'https://www.gdax.com/orders/'+this.baseCurrency+'-'+this.quoteCurrency
          : (this.exchange_name=='Bitfinex' || this.exchange_name=='BitfinexMargin'
            ? 'https://www.bitfinex.com/reports/orders'
            : (this.exchange_name=='HitBtc'
              ? 'https://hitbtc.com/reports/orders'
              : (this.exchange_name=='Kraken'
                ? 'https://www.kraken.com/u/trade'
                : (this.exchange_name=='Poloniex'
                  ? 'https://poloniex.com/tradeHistory'
                  : null
                )
              )
            )
          )
        )
      );
    this.product.advert = pa;
    this.homepage = pa.homepage;
    this.product.fixed = Math.max(0, Math.floor(Math.log10(pa.minTick)) * -1);
    setTimeout(this.resizeMatryoshka, 5000);
    console.log("%cK started "+(new Date().toISOString().slice(11, -1))+"  %c"+this.homepage, "color:green;font-size:32px;", "color:red;font-size:16px;");
    // try {
    // if (!document.getElementsByClassName('chatbro_container').length && window.parent === window)
      // (function(chats,async) {async=!1!==async;var params={embedChatsParameters:chats instanceof Array?chats:[chats],lang:navigator.language||(<any>navigator).userLanguage,needLoadCode:'undefined'==typeof (<any>window).Chatbro,embedParamsVersion:localStorage.embedParamsVersion,chatbroScriptVersion:localStorage.chatbroScriptVersion},xhr=new XMLHttpRequest;xhr.withCredentials=!0,xhr.onload=function(){try{eval(xhr.responseText)}catch(e){}},xhr.onerror=function(){console.error('Chatbro loading error')},xhr.open('GET','//www.chatbro.com/embed.js?'+btoa((<any>window).unescape(encodeURIComponent(JSON.stringify(params)))),async),xhr.send()})({encodedChatId: '9Wic'},false);
    // } catch(e) {}
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
    StatsComponent
  ],
  bootstrap: [ClientComponent]
})
class ClientModule {}

enableProdMode();
platformBrowserDynamic().bootstrapModule(ClientModule);
