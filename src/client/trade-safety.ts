import {Component, Input} from '@angular/core';

import * as Models from './models';

@Component({
  selector: 'trade-safety',
  template: `<div div class="tradeSafety img-rounded"><div>
      Fair Value: <span class="{{ fairValue ? \'text-danger fairvalue\' : \'text-muted\' }}" style="font-size:121%;">{{ fairValue | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety | number:'1.'+product.fixed+'-'+product.fixed }}</span>,
      BuyTS: <span class="{{ buySafety ? \'text-danger\' : \'text-muted\' }}">{{ buySafety | number:'1.2-2' }}</span>,
      SellTS: <span class="{{ sellSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSafety | number:'1.2-2' }}</span>,
      TotalTS: <span class="{{ tradeSafetyValue ? \'text-danger\' : \'text-muted\' }}">{{ tradeSafetyValue | number:'1.2-2' }}</span>,
      openOrders/60sec: <span class="{{ tradeFreq ? \'text-danger\' : \'text-muted\' }}">{{ tradeFreq | number:'1.0-0' }}</span>
    </div>
  </div>`
})
export class TradeSafetyComponent {

  public fairValue: number;
  private buySafety: number;
  private sellSafety: number;
  private buySizeSafety: number;
  private sellSizeSafety: number;
  private tradeSafetyValue: number;

  @Input() tradeFreq: number;

  @Input() product: Models.ProductState;

  @Input() set setFairValue(o: Models.FairValue) {
    if (o === null)
      this.fairValue = null;
    else
      this.fairValue = o.price;
  }

  @Input() set setTradeSafety(o: Models.TradeSafety) {
    if (o === null) {
      this.tradeSafetyValue = null;
      this.buySafety = null;
      this.sellSafety = null;
      this.buySizeSafety = null;
      this.sellSizeSafety = null;
    } else {
      this.tradeSafetyValue = o.combined;
      this.buySafety = o.buy;
      this.sellSafety = o.sell;
      this.buySizeSafety = o.buyPing;
      this.sellSizeSafety = o.sellPing;
    }
  }
}
