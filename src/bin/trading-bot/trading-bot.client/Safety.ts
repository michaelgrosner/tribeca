import {Component, Input} from '@angular/core';

import {Models} from 'lib/K';

@Component({
  selector: 'safety',
  template: `<div div class="tradeSafety">
    <div>
      <div class="param-info">
        <span class="param-label">Fair Value</span>:
          <span
            [ngClass]="getClass(fairValue.price) + ' fairvalue'"
          >{{ fairValue.price.toFixed(product.tickPrice) }}<i
          class="beacon sym-{{ product.quote.toLowerCase() }}-s"></i></span>
      </div>
      <div class="param-info">
        <span class="param-label">BuyPing</span>:
          <span
            [ngClass]="getClass(tradeSafety.buyPing)"
          >{{ tradeSafety.buyPing.toFixed(product.tickPrice) }}<i
          class="beacon sym-{{ product.quote.toLowerCase() }}-s"></i></span>
      </div>
      <div class="param-info">
        <span class="param-label">SellPing</span>:
          <span
            [ngClass]="getClass(tradeSafety.sellPing)"
          >{{ tradeSafety.sellPing.toFixed(product.tickPrice) }}<i
          class="beacon sym-{{ product.quote.toLowerCase() }}-s"></i></span>
      </div>
      <div class="param-info">
        <span class="param-label">BuyTS</span>:<span
          [ngClass]="getClass(tradeSafety.buy)"
        >{{ tradeSafety.buy.toFixed(2) }}</span>
      </div>
      <div class="param-info">
        <span class="param-label">SellTS</span>:<span
          [ngClass]="getClass(tradeSafety.sell)"
        >{{ tradeSafety.sell.toFixed(2) }}</span>
      </div>
      <div class="param-info">
        <span class="param-label">TotalTS</span>:<span
          [ngClass]="getClass(tradeSafety.combined)"
        >{{ tradeSafety.combined.toFixed(2) }}</span>
      </div>
    </div>
  </div>`
})
export class SafetyComponent {

  @Input() product: Models.ProductAdvertisement;

  @Input() fairValue: Models.FairValue;

  @Input() tradeSafety: Models.TradeSafety;

  private getClass = (o: number) => {
    if (o) return "param-value text-danger";
    else   return "param-value text-muted";
  };
};
