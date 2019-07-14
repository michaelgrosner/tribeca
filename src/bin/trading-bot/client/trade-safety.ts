import {Component, Input} from '@angular/core';

import * as Models from './models';

@Component({
  selector: 'trade-safety',
  template: `<div div class="tradeSafety img-rounded"><div>
      Fair Value: <span class="{{ fairValue ? \'text-danger fairvalue\' : \'text-muted\' }}" style="font-size:121%;">{{ fairValue.toFixed(product.tickPrice) }}</span>,
      BuyPing: <span class="{{ buySizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ buySizeSafety.toFixed(product.tickPrice) }}</span>,
      SellPing: <span class="{{ sellSizeSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSizeSafety.toFixed(product.tickPrice) }}</span>,
      BuyTS: <span class="{{ buySafety ? \'text-danger\' : \'text-muted\' }}">{{ buySafety.toFixed(2) }}</span>,
      SellTS: <span class="{{ sellSafety ? \'text-danger\' : \'text-muted\' }}">{{ sellSafety.toFixed(2) }}</span>,
      TotalTS: <span class="{{ tradeSafetyValue ? \'text-danger\' : \'text-muted\' }}">{{ tradeSafetyValue.toFixed(2) }}</span>,
      openOrders/60sec: <span class="{{ tradeFreq ? \'text-danger\' : \'text-muted\' }}">{{ tradeFreq }}</span>
    </div>
  </div>`
})
export class TradeSafetyComponent {

  public fairValue: number = 0;
  private buySafety: number = 0;
  private sellSafety: number = 0;
  private buySizeSafety: number = 0;
  private sellSizeSafety: number = 0;
  private tradeSafetyValue: number = 0;

  @Input() tradeFreq: number;

  @Input() product: Models.ProductAdvertisement;

  @Input() set setFairValue(o: Models.FairValue) {
    if (o === null)
      this.fairValue = 0;
    else
      this.fairValue = o.price;
  }

  @Input() set setTradeSafety(o: Models.TradeSafety) {
    if (o === null) {
      this.tradeSafetyValue = 0;
      this.buySafety = 0;
      this.sellSafety = 0;
      this.buySizeSafety = 0;
      this.sellSizeSafety = 0;
    } else {
      this.tradeSafetyValue = o.combined;
      this.buySafety = o.buy;
      this.sellSafety = o.sell;
      this.buySizeSafety = o.buyPing;
      this.sellSizeSafety = o.sellPing;
    }
  }
}
