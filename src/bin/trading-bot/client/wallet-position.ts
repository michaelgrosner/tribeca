import {Component, Input} from '@angular/core';

import * as Models from './models';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions">
    <h4 class="col-md-12 col-xs-2"><small>{{ product.base }}:<br><span title="{{ product.base }} Available" class="text-danger">{{ basePosition.toFixed(8) }}</span><br/><span title="{{ product.base }} Held" [ngClass]="baseHeldPosition ? 'sell' : 'text-muted'">{{ baseHeldPosition.toFixed(8) }}</span>
    <hr style="margin:0 30px;color:#fff;opacity:.7">
    <span class="text-muted" title="{{ product.base}} Total">{{ (basePosition + baseHeldPosition).toFixed(8)}}</span></small></h4>
    <div *ngIf="!product.margin">
    <h4 class="col-md-12 col-xs-2"><small>{{ product.quote }}:<br><span title="{{ product.quote }} Available" class="text-danger">{{ quotePosition.toFixed(product.tickPrice) }}</span><br/><span title="{{ product.quote }} Held" [ngClass]="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition.toFixed(product.tickPrice) }}</span>
    <hr style="margin:0 30px;color:#fff;opacity:.7">
    <span class="text-muted" title="{{ product.quote }} Total">{{ (quotePosition + quoteHeldPosition).toFixed(product.tickPrice) }}</span></small></h4>
    </div>
    <h4 class="col-md-12 col-xs-2" style="margin-bottom: 0px!important;"><small>Value:</small><br><b title="{{ product.base }} Total">{{ baseValue.toFixed(8) }}</b><br/><b title="{{ product.quote }} Total">{{ quoteValue.toFixed(product.tickPrice) }}</b></h4>
    <h4 class="col-md-12 col-xs-2" style="margin-top: 0px!important;"><small style="font-size:69%"><span title="{{ product.base }} profit %" class="{{ profitBase>0 ? \'text-danger\' : \'text-muted\' }}">{{ profitBase>=0?'+':'' }}{{ profitBase.toFixed(2) }}%</span><span *ngIf="!product.margin">, </span><span *ngIf="!product.margin" title="{{ product.quote }} profit %" class="{{ profitQuote>0 ? \'text-danger\' : \'text-muted\' }}">{{ profitQuote>=0?'+':'' }}{{ profitQuote.toFixed(2) }}%</span></small></h4>
  </div>`
})
export class WalletPositionComponent {

  public basePosition: number = 0;
  public quotePosition: number = 0;
  public baseHeldPosition: number = 0;
  public quoteHeldPosition: number = 0;
  public baseValue: number = 0;
  public quoteValue: number = 0;
  private profitBase: number = 0;
  private profitQuote: number = 0;

  @Input() product: Models.ProductAdvertisement;

  @Input() set setPosition(o: Models.PositionReport) {
    if (o === null) {
      this.basePosition = 0;
      this.quotePosition = 0;
      this.baseHeldPosition = 0;
      this.quoteHeldPosition = 0;
      this.baseValue = 0;
      this.quoteValue = 0;
      this.profitBase = 0;
      this.profitQuote = 0;
    } else {
      this.basePosition = o.base.amount;
      this.quotePosition = o.quote.amount;
      this.baseHeldPosition = o.base.held;
      this.quoteHeldPosition = o.quote.held;
      this.baseValue = o.base.value;
      this.quoteValue = o.quote.value;
      this.profitBase = o.base.profit;
      this.profitQuote = o.quote.profit;
    }
  }
}
