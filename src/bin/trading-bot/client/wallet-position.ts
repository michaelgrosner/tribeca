import {Component, Input} from '@angular/core';

import * as Models from './models';

@Component({
  selector: 'wallet-position',
  template: `<div class="positions" *ngIf="baseValue || quoteValue">
    <h4 class="col-md-12 col-xs-2"><small>{{ baseCurrency }}:<br><span title="{{ baseCurrency }} Available" class="text-danger">{{ basePosition | number:'1.8-8' }}</span><br/><span title="{{ baseCurrency }} Held" [ngClass]="baseHeldPosition ? 'sell' : 'text-muted'">{{ baseHeldPosition | number:'1.8-8' }}</span>
    <hr style="margin:0 30px;color:#fff;opacity:.7">
    <span class="text-muted" title="{{ baseCurrency }} Total">{{ basePosition + baseHeldPosition | number:'1.8-8' }}</span></small></h4>
    <h4 class="col-md-12 col-xs-2"><small>{{ quoteCurrency }}:<br><span title="{{ quoteCurrency }} Available" class="text-danger">{{ quotePosition | number:'1.'+product.fixed+'-'+product.fixed }}</span><br/><span title="{{ quoteCurrency }} Held" [ngClass]="quoteHeldPosition ? 'buy' : 'text-muted'">{{ quoteHeldPosition | number:'1.'+product.fixed+'-'+product.fixed }}</span>
    <hr style="margin:0 30px;color:#fff;opacity:.7">
    <span class="text-muted" title="{{ quoteCurrency }} Total">{{ quotePosition + quoteHeldPosition | number:'1.'+product.fixed+'-'+product.fixed }}</span></small></h4>
    <h4 class="col-md-12 col-xs-2" style="margin-bottom: 0px!important;"><small>Value:</small><br><b title="{{ baseCurrency }} Total">{{ baseValue | number:'1.8-8' }}</b><br/><b title="{{ quoteCurrency }} Total">{{ quoteValue | number:'1.'+product.fixed+'-'+product.fixed }}</b></h4>
    <h4 class="col-md-12 col-xs-2" style="margin-top: 0px!important;"><small style="font-size:69%"><span title="{{ baseCurrency }} profit %" class="{{ profitBase>0 ? \'text-danger\' : \'text-muted\' }}">{{ profitBase>=0?'+':'' }}{{ profitBase | number:'1.2-2' }}%</span>, <span title="{{ quoteCurrency }} profit %" class="{{ profitQuote>0 ? \'text-danger\' : \'text-muted\' }}">{{ profitQuote>=0?'+':'' }}{{ profitQuote | number:'1.2-2' }}%</span></small></h4>
  </div><div class="positions" *ngIf="!baseCurrency && !quoteCurrency"><br/><b>NO WALLET DATA</b><br/><br/>Do a manual order first using the website of this Market!<br/><br/></div>`
})
export class WalletPositionComponent {

  public basePosition: number;
  public quotePosition: number;
  public baseHeldPosition: number;
  public quoteHeldPosition: number;
  public baseValue: number;
  public quoteValue: number;
  private profitBase: number = 0;
  private profitQuote: number = 0;

  @Input() product: Models.ProductState;
  @Input() baseCurrency: string;
  @Input() quoteCurrency: string;

  @Input() set setPosition(o: Models.PositionReport) {
    if (o === null) {
      this.basePosition = null;
      this.quotePosition = null;
      this.baseHeldPosition = null;
      this.quoteHeldPosition = null;
      this.baseValue = null;
      this.quoteValue = null;
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
