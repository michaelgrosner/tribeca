import {Component, Input} from '@angular/core';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'settings',
  template: `<table border="0" width="100%" class="params-market-making"><tr>
    <td class="params-title params-1">
      MARKET MAKING
    </td><td class="params-settings">
      <table class="table table-responsive" style="margin-bottom:10px;">
          <thead>
              <tr class="active">
                  <th title="If enabled, the values of bidSize, askSize, tbp, pDiv and range will be a percentage related to the total funds.">%</th>
                  <th title="Sets the quoting mode">mode</th>
                  <th title="Sets a quoting Safety">safety</th>
                  <th title="Maximum amount of trades placed in each side." *ngIf="params.safety==4">bullets</th>
                  <th title="Minimum width between bullets in USD (ex. a value of .3 is 30 cents)." *ngIf="params.safety==4 && !params.percentageValues">range</th>
                  <th title="Minimum width between bullets in USD (ex. a value of .3 is 30 cents)." *ngIf="params.safety==4 && params.percentageValues">range%</th>
                  <th title="Pongs are always placed in both sides." *ngIf="params.safety">pingAt</th>
                  <th title="" *ngIf="[3,4].indexOf(params.safety)>-1">pongAt</th>
                  <th title="Super opportunities, if enabled and if the market width is sopWidth times bigger than the width set, it multiplies sopTrades to trades and/or sopSize to size, in both sides at the same time.">sop</th>
                  <ng-container *ngIf="params.superTrades">
                  <th title="The value with the market width is multiplicated to define the activation point for Super opportunities.">sopWidth</th>
                  <th title="Multiplicates trades to rise the possible Trades per Minute if sop is in Trades or tradesSize state." *ngIf="[1,3].indexOf(params.superTrades)>-1">sopTrades</th>
                  <th title="Multiplicates width if sop is in Size or tradesSize state." *ngIf="[2,3].indexOf(params.superTrades)>-1">sopSize</th>
                  </ng-container>
                  <th title="Total funds to take percentage of when calculating order size." *ngIf="params.percentageValues">ordPctTot</th>
                  <th title="Exponent for TBP size dropoff, higher=faster." *ngIf="params.orderPctTotal == 3">exp</th>
                  <th title="Maximum bid size of our quote in BTC (ex. a value of 1.5 is 1.5 bitcoins). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack." [attr.colspan]="params.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="params.aggressivePositionRebalancing && params.buySizeMax">minB</span><span *ngIf="!params.aggressivePositionRebalancing || !params.buySizeMax">b</span>idSize<span *ngIf="params.percentageValues">%</span><span *ngIf="params.aggressivePositionRebalancing" style="float:right;">maxBidSize?</span></th>
                  <th title="Maximum ask size of our quote in BTC (ex. a value of 1.5 is 1.5 bitcoins). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack." [attr.colspan]="params.aggressivePositionRebalancing ? '2' : '1'"><span *ngIf="params.aggressivePositionRebalancing && params.sellSizeMax">minA</span><span *ngIf="!params.aggressivePositionRebalancing || !params.sellSizeMax">a</span>skSize<span *ngIf="params.percentageValues">%</span><span *ngIf="params.aggressivePositionRebalancing" style="float:right;">maxAskSize?</span></th>
              </tr>
          </thead>
          <tbody>
              <tr class="active">
                  <td style="width:25px;border-bottom: 3px solid #8BE296;">
                      <input type="checkbox"
                         [(ngModel)]="params.percentageValues">
                  </td>
                  <td style="min-width:121px;border-bottom: 3px solid #DDE28B;">
                      <select class="form-control input-sm"
                        [(ngModel)]="params.mode">
                        <option *ngFor="let option of availableQuotingModes" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="min-width:121px;border-bottom: 3px solid #DDE28B;">
                      <select class="form-control input-sm"
                        [(ngModel)]="params.safety">
                        <option *ngFor="let option of availableQuotingSafeties" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="width:78px;border-bottom: 3px solid #DDE28B;" *ngIf="params.safety==4">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.bullets">
                  </td>
                  <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="params.safety==4 && !params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.quote }}"
                         type="number"
                         [(ngModel)]="params.range">
                  </td>
                  <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="params.safety==4 && params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.quote }}"
                         type="number" step="0.001" min="0" max="100"
                         [(ngModel)]="params.rangePercentage">
                  </td>
                  <td style="min-width:142px;border-bottom: 3px solid #8BE296;" *ngIf="params.safety">
                      <select class="form-control input-sm"
                         [(ngModel)]="params.pingAt">
                         <option *ngFor="let option of availablePingAt" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="border-bottom: 3px solid #8BE296;" *ngIf="[3,4].indexOf(params.safety)>-1">
                      <select class="form-control input-sm"
                         [(ngModel)]="params.pongAt">
                         <option *ngFor="let option of availablePongAt" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="min-width:121px;border-bottom: 3px solid #DDE28B;">
                      <select class="form-control input-sm"
                          [(ngModel)]="params.superTrades">
                         <option *ngFor="let option of availableSuperTrades" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <ng-container *ngIf="params.superTrades">
                  <td style="width:88px; border-bottom: 3px solid #DDE28B;">
                      <input class="form-control input-sm" title="Width multiplier"
                         type="number" step="0.1" min="1"
                         [(ngModel)]="params.sopWidthMultiplier">
                  </td>
                  <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="[1,3].indexOf(params.superTrades)>-1">
                      <input class="form-control input-sm" title="Trades multiplier"
                         type="number" step="0.1" min="1"
                         [(ngModel)]="params.sopTradesMultiplier">
                  </td>
                  <td style="width:88px; border-bottom: 3px solid #DDE28B;" *ngIf="[2,3].indexOf(params.superTrades)>-1">
                      <input class="form-control input-sm" title="Size multiplier"
                         type="number" step="0.1" min="1"
                         [(ngModel)]="params.sopSizeMultiplier">
                  </td>
                  </ng-container>
                  <td style="border-bottom: 3px solid #D64A4A" *ngIf="params.percentageValues">
                    <select class="form-control input-sm"
                      [(ngModel)]="params.orderPctTotal">
                      <option *ngFor="let option of availableOrderPctTotals" [ngValue]="option.val">{{option.str}}</option>
                    </select>
                  </td>
                  <td style="border-bottom: 3px solid #D64A4A;" *ngIf="params.orderPctTotal == 3">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.001" min="1" max="16"
                         [(ngModel)]="params.tradeSizeTBPExp">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="!params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.margin ? 'Contracts' : product.base }}"
                         type="number" step="{{ product.stepSize }}" min="{{ product.minSize}}"
                         [(ngModel)]="params.buySize">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.margin ? 'Contracts' : product.base }}"
                         type="number" step="0.001" min="0.001" max="100"
                         [(ngModel)]="params.buySizePercentage">
                  </td>
                  <td style="border-bottom: 3px solid #D64A4A;" *ngIf="params.aggressivePositionRebalancing">
                      <input type="checkbox"
                         [(ngModel)]="params.buySizeMax">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="!params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.margin ? 'Contracts' : product.base }}"
                         type="number" step="{{ product.stepSize }}" min="{{ product.minSize }}"
                         [(ngModel)]="params.sellSize">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #D64A4A;" *ngIf="params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.margin ? 'Contracts' : product.base }}"
                         type="number" step="0.001" min="0.001" max="100"
                         [(ngModel)]="params.sellSizePercentage">
                  </td>
                  <td style="border-bottom: 3px solid #D64A4A;" *ngIf="params.aggressivePositionRebalancing">
                      <input type="checkbox"
                         [(ngModel)]="params.sellSizeMax">
                  </td>
              </tr>
          </tbody>
      </table>
    </td></tr></table>
    <table border="0" width="100%" class="params-tech-analysis"><tr>
    <td class="params-title params-1">
      TECHNICAL ANALYSIS
    </td><td class="params-settings">
      <table class="table table-responsive" style="margin-bottom:10px;">
          <thead>
              <tr class="active">
                  <th title="Automatic position management">apMode</th>
                  <th title="Sets the periods of EWMA VeryLong to automatically manage positions." *ngIf="params.autoPositionMode==3">verylong</th>
                  <th title="Sets the periods of EWMA Long to automatically manage positions." *ngIf="params.autoPositionMode">long</th>
                  <th title="Sets the periods of EWMA Medium to automatically manage positions." *ngIf="params.autoPositionMode>1">medium</th>
                  <th title="Sets the periods of EWMA Short to automatically manage positions." *ngIf="params.autoPositionMode">short</th>
                  <th title="Threshold removed from each period, affects EWMA Long, Medium and Short. The decimal value must be betweem 0 and 1." *ngIf="params.autoPositionMode">sensibility</th>
                  <th title="Sets a static Target Base Position for Krypto-trading-bot to stay near. Krypto-trading-bot will still try to respect pDiv and not make your position fluctuate by more than that value." *ngIf="!params.autoPositionMode">tbp<span *ngIf="params.percentageValues">%</span></th>
                  <th title="Sets a static minimum Target Base Position for Krypto-trading-bot. Krypto-trading-bot will still try to respect pDiv and not make your position reduce below that value." *ngIf="params.autoPositionMode>0">tbpmin<span *ngIf="params.percentageValues">%</span></th>
                  <th title="Sets a static maximum Target Base Position for Krypto-trading-bot. Krypto-trading-bot will still try to respect pDiv and not make your position increase beyond that value." *ngIf="params.autoPositionMode>0">tbpmax<span *ngIf="params.percentageValues">%</span></th>
                  <th title="Sets the strategy of dynamically adjusting the pDiv depending on the divergence from 50% of Base Value." *ngIf="params.autoPositionMode">pDivMode</th>
                  <th title="If your Target Base Position diverges more from this value, Krypto-trading-bot will stop sending orders to stop too much directional trading.">pDiv<span *ngIf="params.percentageValues">%</span></th>
                  <th title="It defines the minimal pDiv for the dynamic positon divergence." *ngIf="params.autoPositionMode && params.positionDivergenceMode">pDivMin<span *ngIf="params.percentageValues">%</span></th>
                  <th title="If you're in a state where Krypto-trading-bot has stopped sending orders because your position has diverged too far from Target Base Position, this setting will much more aggressively try to fix that discrepancy by placing orders much larger than size and at prices much more aggressive than width normally allows.">apr</th>
                  <th title="Defines the value with which the size is multiplicated when apr is in functional state." *ngIf="params.aggressivePositionRebalancing">aprFactor</th>
                  <th title="Enable Best Width to place orders avoiding hollows in the book, while accomodating new orders right near to existent orders in the book." >bw?</th>
                  <th title="Defines the size to ignore when looking for hollows in the book. The sum of orders up to this size will be ignored." *ngIf="params.bestWidth">bwSize</th>
                  <th title="The values of width or widthPing and widthPong will be a percentage related to the fair value; useful when calculating profits subtracting exchange's fees (that usually are percentages too)." *ngIf="[6].indexOf(params.mode)==-1">%w?</th>
                  <th title="Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack, width will always be respected." *ngIf="!params.safety"><span *ngIf="[6].indexOf(params.mode)==-1">width</span><span *ngIf="[6].indexOf(params.mode)>-1">depth</span><span *ngIf="params.widthPercentage && [6].indexOf(params.mode)==-1">%</span></th>
                  <th title="Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). With the exception for when apr is checked and the system is aggressively rebalancing positions after they get out of whack, width will always be respected." *ngIf="params.safety">pingWidth<span *ngIf="params.widthPercentage">%</span></th>
                  <th title="Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). Used only if previous Pings exists in the opposite side." *ngIf="params.safety">pongWidth<span *ngIf="params.widthPercentage">%</span></th>
              </tr>
          </thead>
          <tbody>
              <tr class="active">
                  <td style="min-width:121px;border-bottom: 3px solid #8BE296;">
                      <select class="form-control input-sm"
                          [(ngModel)]="params.autoPositionMode">
                         <option *ngFor="let option of availableAutoPositionModes" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.autoPositionMode==3">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.veryLongEwmaPeriods">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.autoPositionMode">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.longEwmaPeriods">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.autoPositionMode>1">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.mediumEwmaPeriods">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.autoPositionMode">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.shortEwmaPeriods">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.autoPositionMode">
                      <input class="form-control input-sm"
                         type="number" step="0.01" min="0.01" max="1.00"
                         [(ngModel)]="params.ewmaSensiblityPercentage">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="!params.percentageValues && params.autoPositionMode==0">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.01" min="0"
                         [(ngModel)]="params.targetBasePosition">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.percentageValues && params.autoPositionMode==0">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.1" min="0" max="100"
                         [(ngModel)]="params.targetBasePositionPercentage">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="!params.percentageValues && params.autoPositionMode">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.01" min="0"
                         [(ngModel)]="params.targetBasePositionMin">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.percentageValues && params.autoPositionMode">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.1" min="0" max="100"
                         [(ngModel)]="params.targetBasePositionPercentageMin">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="!params.percentageValues && params.autoPositionMode">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.01" min="0"
                         [(ngModel)]="params.targetBasePositionMax">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;" *ngIf="params.percentageValues && params.autoPositionMode">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.1" min="0" max="100"
                         [(ngModel)]="params.targetBasePositionPercentageMax">
                  </td>
                  <td style="min-width:121px;border-bottom: 3px solid #DDE28B;" *ngIf="params.autoPositionMode">
                      <select class="form-control input-sm"
                          [(ngModel)]="params.positionDivergenceMode">
                         <option *ngFor="let option of availablePdiv" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="!params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.01" min="0"
                         [(ngModel)]="params.positionDivergence">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="params.percentageValues">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.1" min="0" max="100"
                         [(ngModel)]="params.positionDivergencePercentage">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="!params.percentageValues && params.autoPositionMode && params.positionDivergenceMode">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.01" min="0"
                         [(ngModel)]="params.positionDivergenceMin">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;" *ngIf="params.percentageValues && params.autoPositionMode && params.positionDivergenceMode">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="0.1" min="0" max="100"
                         [(ngModel)]="params.positionDivergencePercentageMin">
                  </td>
                  <td style="min-width:121px;border-bottom: 3px solid #D64A4A;">
                      <select class="form-control input-sm"
                          [(ngModel)]="params.aggressivePositionRebalancing">
                         <option *ngFor="let option of availableAPR" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #D64A4A;" *ngIf="params.aggressivePositionRebalancing">
                      <input class="form-control input-sm"
                         type="number" step="0.1" min="1" max="10.00"
                         [(ngModel)]="params.aprMultiplier">
                  </td>
                  <td style="width:25px;border-bottom: 3px solid #8BE296;">
                      <input type="checkbox"
                         [(ngModel)]="params.bestWidth">
                  </td>
                  <td style="width:90px;border-bottom: 3px solid #8BE296;" *ngIf="params.bestWidth">
                      <input class="form-control input-sm" title="{{ product.base }}"
                         type="number" step="{{ product.stepPrice }}" min="0"
                         [(ngModel)]="params.bestWidthSize">
                  </td>
                  <td style="width:25px;border-bottom: 3px solid #8BE296;" *ngIf="[6].indexOf(params.mode)==-1">
                      <input type="checkbox"
                         [(ngModel)]="params.widthPercentage">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="!params.widthPercentage || [6].indexOf(params.mode)>-1">
                      <input class="width-option form-control input-sm" title="{{ product.quote }}"
                         type="number" step="{{ product.stepPrice }}" min="{{ product.stepPrice }}"
                         [(ngModel)]="params.widthPing">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="params.widthPercentage && [6].indexOf(params.mode)==-1">
                      <input class="width-option form-control input-sm" title="{{ product.quote }}"
                         type="number" step="0.001" min="0.001" max="100"
                         [(ngModel)]="params.widthPingPercentage">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="params.safety && !params.widthPercentage">
                      <input class="width-option form-control input-sm" title="{{ product.quote }}"
                         type="number" step="{{ product.stepPrice }}" min="{{ product.stepPrice }}"
                         [(ngModel)]="params.widthPong">
                  </td>
                  <td style="width:169px;border-bottom: 3px solid #8BE296;" *ngIf="params.safety && params.widthPercentage">
                      <input class="width-option form-control input-sm" title="{{ product.quote }}"
                         type="number" step="0.001" min="0.001" max="100"
                         [(ngModel)]="params.widthPongPercentage">
                  </td>
          </tbody>
      </table>
    </td></tr></table>
    <table border="0" width="100%" class="params-protection"><tr>
    <td class="params-title params-1">
      PROTECTION
    </td><td class="params-settings">
      <table class="table table-responsive" style="margin-bottom:10px;">
          <thead>
              <tr class="active">
                  <th title="Sets the fair value calculation mode">fv</th>
                  <th title="If you successfully complete more orders than trades in /sec seconds, the bot will stop sending more new orders until either /sec seconds has passed, or you have sold enough at a higher cost to make all those buy orders profitable." style="text-align:right;">trades</th>
                  <th title="If you successfully complete more orders than trades in /sec seconds, the bot will stop sending more new orders until either /sec seconds has passed, or you have sold enough at a higher cost to make all those buy orders profitable.">/sec</th>
                  <th title="Use a quote protection of periods smoothed line of the fair value to limit the price while sending new orders.">ewmaPrice?</th>
                  <th title="Maximum amount of values collected in the sequences used to calculate the ewmaPrice? and ewmaWidth? quote protection." *ngIf="params.protectionEwmaQuotePrice || params.protectionEwmaWidthPing">periods<sup>ewma</sup></th>
                  <th title="Use a quote protection of periods smoothed line of the width (between the top bid and the top ask) to limit the widthPing while sending new orders.">ewmaWidth?</th>
                  <th title="Use a trend protection of double periods (Ultra+Micro) smoothed lines of the price to limit uptrend sells and downtrend buys.">ewmaTrend?</th>
                  <th title="When trend stregth is above positive threshold value bot stops selling, when strength below negative threshold value bot stops buying" *ngIf="params.quotingEwmaTrendProtection">threshold</th>
                  <th title="Time in minutes to define Micro EMA" *ngIf="params.quotingEwmaTrendProtection">micro</th>
                  <th title="Time in minutes to define Ultra EMA" *ngIf="params.quotingEwmaTrendProtection">ultra</th>
                  <th title="Limit the price of new orders.">stdev</th>
                  <th title="Maximum amount of values collected in the sequences used to calculate the STDEV, each side may have its own STDEV calculation with the same amount of periods." *ngIf="params.quotingStdevProtection">periods<sup>stddev</sup></th>
                  <th title="Multiplier used to increase or decrease the value of the selected stdev calculation, a factor of 1 does effectively nothing." *ngIf="params.quotingStdevProtection">factor</th>
                  <th title="Enable Bollinger Bands with upper and lower bands calculated from the result of the selected stdev above or below its own moving average of periods." *ngIf="params.quotingStdevProtection">BB?</th>
                  <th title="Enable a timeout of 5 minutes to cancel all orders that exist as open in the exchange (in case you found yourself with zombie orders in the exchange, because the API integration have bugs or because the connection is interrupted).">cxl?</th>
                  <th title="Enable a timeout of lifetime milliseconds to keep orders open (otherwise open orders can be replaced anytime required).">lifetime</th>
                  <th title="Timeframe in hours to calculate the display of Profit (under wallet values) and also interval in hour to remove data points from the Stats.">profit</th>
                  <th title="Timeout in days for Pings (yet unmatched trades) and/or Pongs (K trades) to remain in memory, a value of 0 keeps the history in memory forever; a positive value remove only Pongs after Kmemory days; but a negative value remove both Pings and Pongs after Kmemory days.">Kmemory</th>
                  <th title="Relax the display of UI data by delayUI seconds. Set a value of 0 (zero) to display UI data in realtime, but this may penalize the communication with the exchange if you end up sending too much frequent UI data.">delayUI</th>
                  <th title="Plays a sound for each new trade (ping-pong modes have 2 sounds for each type of trade).">audio?</th>
                  <th colspan="2">
                      <span *ngIf="!pending" class="text-success">
                          Applied
                      </span>
                      <span *ngIf="pending" class="text-warning">
                          Pending
                      </span>
                  </th>
              </tr>
          </thead>
          <tbody>
              <tr class="active">
                  <td style="width:88px;border-bottom: 3px solid #8BE296;">
                      <select class="form-control input-sm"
                          [(ngModel)]="params.fvModel">
                         <option *ngFor="let option of availableFvModels" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;">
                      <input class="form-control input-sm"
                         type="number" step="1" min="0"
                         [(ngModel)]="params.tradesPerMinute">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;">
                      <input class="form-control input-sm"
                         type="number" step="1" min="0"
                         [(ngModel)]="params.tradeRateSeconds">
                  </td>
                  <td style="text-align: center;border-bottom: 3px solid #F0A0A0;">
                      <input type="checkbox"
                         [(ngModel)]="params.protectionEwmaQuotePrice">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #F0A0A0;" *ngIf="params.protectionEwmaQuotePrice || params.protectionEwmaWidthPing">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.protectionEwmaPeriods">
                  </td>
                  <td style="width:30px;text-align: center;border-bottom: 3px solid #F0A0A0;">
                      <input type="checkbox"
                         [(ngModel)]="params.protectionEwmaWidthPing">
                  </td>
                  <td style="text-align: center;border-bottom: 3px solid #fd00ff;">
                      <input type="checkbox"
                         [(ngModel)]="params.quotingEwmaTrendProtection">
                  </td>
                  <td style="width:60px;border-bottom: 3px solid #fd00ff;" *ngIf="params.quotingEwmaTrendProtection">
                  <input class="form-control input-sm"
                      type="number" step="0.1" min="0.1"
                         [(ngModel)]="params.quotingEwmaTrendThreshold">
                  </td>
                  <td style="width:60px;border-bottom: 3px solid #fd00ff;" *ngIf="params.quotingEwmaTrendProtection">
                      <input class="form-control input-sm"
                        type="number" step="1" min="1"
                          [(ngModel)]="params.extraShortEwmaPeriods">
                  </td>
                  <td style="width:60px;border-bottom: 3px solid #fd00ff;" *ngIf="params.quotingEwmaTrendProtection">
                      <input class="form-control input-sm"
                          type="number" step="1" min="1"
                          [(ngModel)]="params.ultraShortEwmaPeriods">
                </td>
                  <td style="width:121px;border-bottom: 3px solid #AF451E;">
                      <select class="form-control input-sm"
                          [(ngModel)]="params.quotingStdevProtection">
                          <option *ngFor="let option of availableSTDEV" [ngValue]="option.val">{{option.str}}</option>
                      </select>
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #AF451E;" *ngIf="params.quotingStdevProtection">
                      <input class="form-control input-sm"
                         type="number" step="1" min="1"
                         [(ngModel)]="params.quotingStdevProtectionPeriods">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #AF451E;" *ngIf="params.quotingStdevProtection">
                      <input class="form-control input-sm"
                         type="number" step="0.1" min="0.1"
                         [(ngModel)]="params.quotingStdevProtectionFactor">
                  </td>
                  <td style="text-align: center;border-bottom: 3px solid #AF451E;" *ngIf="params.quotingStdevProtection">
                      <input type="checkbox"
                         [(ngModel)]="params.quotingStdevBollingerBands">
                  </td>
                  <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                      <input type="checkbox"
                         [(ngModel)]="params.cancelOrdersAuto">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #A0A0A0;">
                      <input class="form-control input-sm"
                         type="number" step="1" min="0"
                         [(ngModel)]="params.lifetime">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #DDE28B;">
                      <input class="form-control input-sm"
                         type="number" step="0.01" min="0.01"
                         [(ngModel)]="params.profitHourInterval">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #8BE296;">
                      <input class="form-control input-sm"
                         type="number" step="0.01"
                         [(ngModel)]="params.cleanPongsAuto">
                  </td>
                  <td style="width:88px;border-bottom: 3px solid #A0A0A0;">
                      <input class="form-control input-sm"
                         type="number" step="1" min="0"
                         [(ngModel)]="params.delayUI">
                  </td>
                  <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                      <input type="checkbox"
                         [(ngModel)]="params.audio">
                  </td>
                  <td style="text-align: center;border-bottom: 3px solid #A0A0A0;">
                      <input class="btn btn-default btn"
                          style="width:61px;margin-right: 6px;"
                          type="button"
                          (click)="backup()"
                          value="Backup" />
                      <input class="btn btn-default btn"
                          style="width:55px;margin-right: 20px;"
                          type="button"
                          (click)="resetSettings()"
                          value="Reset" />
                      <input class="btn btn-default btn"
                          style="width:50px"
                          type="submit"
                          (click)="submitSettings()"
                          value="Save" />
                  </td>
              </tr>
          </tbody>
      </table>
    </td></tr></table>`
})
export class SettingsComponent {

  private master: Models.QuotingParameters = JSON.parse(JSON.stringify(<Models.QuotingParameters>{}));
  private params: Models.QuotingParameters = JSON.parse(JSON.stringify(<Models.QuotingParameters>{}));
  private pending: boolean = false;

  private availableQuotingModes:      Models.Map[] = Models.getMap(Models.QuotingMode);
  private availableOrderPctTotals:    Models.Map[] = Models.getMap(Models.OrderPctTotal);
  private availableQuotingSafeties:   Models.Map[] = Models.getMap(Models.QuotingSafety);
  private availableFvModels:          Models.Map[] = Models.getMap(Models.FairValueModel);
  private availableAutoPositionModes: Models.Map[] = Models.getMap(Models.AutoPositionMode);
  private availablePdiv:              Models.Map[] = Models.getMap(Models.DynamicPDivMode);
  private availableAPR:               Models.Map[] = Models.getMap(Models.APR);
  private availableSuperTrades:       Models.Map[] = Models.getMap(Models.SOP);
  private availablePingAt:            Models.Map[] = Models.getMap(Models.PingAt);
  private availablePongAt:            Models.Map[] = Models.getMap(Models.PongAt);
  private availableSTDEV:             Models.Map[] = Models.getMap(Models.STDEV);

  private fireCxl: Socket.IFire<Models.QuotingParameters> = new Socket.Fire(Models.Topics.QuotingParametersChange);

  @Input() product: Models.ProductAdvertisement;

  @Input() set quotingParameters(o: Models.QuotingParameters) {
    this.master = JSON.parse(JSON.stringify(o));
    this.params = JSON.parse(JSON.stringify(o));
    this.pending = false;
  };

  private backup = () => {
    try {
      this.params = JSON.parse(window.prompt(
        'Backup quoting parameters\n\nTo export the current setup,'
          + ' copy all from the input below and paste it somewhere else.\n\nTo import'
          + ' a setup replacement, replace the quoting parameters below by some new,'
          + ' then click [Save] to apply.\n\nCurrent/New setup of quoting parameters:',
        JSON.stringify(this.params)
      )) || this.params;
    } catch(e) {}
  };

  private resetSettings = () => {
    this.params = JSON.parse(JSON.stringify(this.master));
  };

  private submitSettings = () => {
    this.pending = true;
    this.fireCxl.fire(this.params);
  };
};
