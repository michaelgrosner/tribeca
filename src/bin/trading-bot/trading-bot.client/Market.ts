import {Component, Input, Output, EventEmitter} from '@angular/core';

import {Models} from 'lib/K';

@Component({
  selector: 'market',
  template: `<div class="col-md-6">
    <div class="tradeSafety2">
      <div>
        <div class="param-info"><span class="param-label">Market Width</span>:<span class="param-value {{ marketWidth ? \'text-danger\' : \'text-muted\' }}">{{ marketWidth.toFixed(product.tickPrice) }}</span></div>
        <div class="param-info"><span class="param-label">Quote Width</span>:<span class="param-value {{ ordersWidth ? \'text-danger\' : \'text-muted\' }}">{{ ordersWidth.toFixed(product.tickPrice) }}</span></div>
        <div class="param-info"><span class="param-label">Quotes</span>:<span class="param-value"><span title="Quotes in memory Waiting status update" class="{{ status.quotesInMemoryWaiting ? \'text-danger\' : \'text-muted\' }}">{{ status.quotesInMemoryWaiting }}</span>/<span title="Quotes in memory Working" class="{{ status.quotesInMemoryWorking ? \'text-danger\' : \'text-muted\' }}">{{ status.quotesInMemoryWorking }}</span>/<span title="Quotes in memory Zombie" class="{{ status.quotesInMemoryZombies ? \'text-danger\' : \'text-muted\' }}">{{ status.quotesInMemoryZombies }}</span></span></div>
        <div class="param-info"><span class="param-label">openOrders/60sec</span>:<span class="param-value {{ tradeFreq ? \'text-danger\' : \'text-muted\' }}">{{ tradeFreq }}</span></div>
        <div class="param-info"><span class="param-label">Wallet TBP</span>:<span class="param-value text-danger">{{ targetBasePosition.tbp.toFixed(8) }}</span></div>
        <div class="param-info"><span class="param-label">pDiv</span>:<span class="param-value text-danger">{{ targetBasePosition.pDiv.toFixed(8) }}</span></div>
        <div class="param-info"><span class="param-label">APR</span>:<span class="param-value {{ status.sideAPR ? \'text-danger\' : \'text-muted\' }}">{{ getAPR() }}</span></div>
      </div>
    </div>
  </div>
  <div style="padding-right:4px;padding-left:4px;padding-top:4px;line-height:1.3;">
    <table class="marketQuoting table table-hover table-responsive text-center">
      <tr class="info">
        <td>BID Size<i class="beacon sym-{{ product.base.toLowerCase() }}-s"></i></td>
        <td>BID Price<i class="beacon sym-{{ product.quote.toLowerCase() }}-s"></i></td>
        <td>ASK Price<i class="beacon sym-{{ product.quote.toLowerCase() }}-s"></i></td>
        <td>ASK Size<i class="beacon sym-{{ product.base.toLowerCase() }}-s"></i></td>
      </tr>
      <tr class="info">
        <th *ngIf="status.bidStatus == 1" class="text-danger">{{ qBidSz.toFixed(product.tickSize) }}<span *ngIf="!qBidSz">&nbsp;</span></th>
        <th *ngIf="status.bidStatus == 1" class="text-danger">{{ qBidPx.toFixed(product.tickPrice) }}</th>
        <th *ngIf="status.bidStatus != 1" colspan="2" class="text-danger" title="Bids Quote Status">{{ getStatus(status.bidStatus) }}</th>
        <th *ngIf="status.askStatus == 1" class="text-danger">{{ qAskPx.toFixed(product.tickPrice) }}</th>
        <th *ngIf="status.askStatus == 1" class="text-danger">{{ qAskSz.toFixed(product.tickSize) }}<span *ngIf="!qAskSz">&nbsp;</span></th>
        <th *ngIf="status.askStatus != 1" colspan="2" class="text-danger" title="Ask Quote Status">{{ getStatus(status.askStatus) }}</th>
      </tr>
    </table>
    <div *ngIf="levels != null" [ngClass]="(addr?'addr ':'')+'levels'">
      <table class="marketQuoting table table-hover table-responsive text-center" style="width:50%;float:left;">
        <tr [ngClass]="orderPriceBids.indexOf(lvl.price.toFixed(product.tickPrice))==-1?'active':'success buy'" *ngFor="let lvl of levels.bids; let i = index">
          <td>
            <div style="position:relative;" [ngClass]="'bids'+lvl.cssMod">
              <div class="bgSize" [ngStyle]="{'background': getBgSize(lvl, 'bids')}"></div>
              {{ getSizeLevel(lvl.size.toFixed(product.tickSize), true) }}<span class="truncated">{{ getSizeLevel(lvl.size.toFixed(product.tickSize), false) }}</span>
            </div>
          </td>
          <td>
            <div [ngClass]="'bids'+(lvl.cssMod==2?2:0)">
              {{ lvl.price.toFixed(product.tickPrice) }}
            </div>
          </td>
        </tr>
      </table>
      <table class="marketQuoting table table-hover table-responsive text-center" style="width:50%;">
        <tr [ngClass]="orderPriceAsks.indexOf(lvl.price.toFixed(product.tickPrice))==-1?'active':'success sell'" *ngFor="let lvl of levels.asks; let i = index">
          <td>
            <div [ngClass]="'asks'+(lvl.cssMod==2?2:0)">
              {{ lvl.price.toFixed(product.tickPrice) }}
            </div>
          </td>
          <td>
            <div style="position:relative;" [ngClass]="'asks'+lvl.cssMod">
              <div class="bgSize" [ngStyle]="{'background': getBgSize(lvl, 'asks')}"></div>
              {{ getSizeLevel(lvl.size.toFixed(product.tickSize), true) }}<span class="truncated">{{ getSizeLevel(lvl.size.toFixed(product.tickSize), false) }}</span>
            </div>
          </td>
        </tr>
      </table>
    </div>
    <table *ngIf="addr" class="table-responsive text-center" style="width:100%;"><tr><td><div class="text-danger text-center"><br /><br />To <a href="https://github.com/ctubio/Krypto-trading-bot/blob/master/README.md#unlock" target="_blank">unlock</a> realtime market data,<br />and to collaborate with the development..<br /><br />make an acceptable <code>Pull Request</code> on github,<br/><br/>or send <code>0.01210000 BTC</code> or more to:<br /><a href="https://live.blockcypher.com/btc/address/{{ addr }}" target="_blank">{{ addr }}</a><br /><br />Wait 0 confirmations and restart this bot.</div></td></tr></table>
    </div>`
})
export class MarketComponent {

  private levels: Models.Market = null;
  private allBidsSize: number = 0;
  private allAsksSize: number = 0;
  private dirtyBids: number = 0;
  private dirtyAsks: number = 0;
  private qBidSz: number = 0;
  private qBidPx: number = 0;
  private qAskPx: number = 0;
  private qAskSz: number = 0;
  private orderBids: Models.OrderSide[];
  private orderAsks: Models.OrderSide[];
  private orderPriceBids: string[] = [];
  private orderPriceAsks: string[] = [];
  private marketWidth: number = 0;
  private ordersWidth: number = 0;
  private noBidReason: string;
  private noAskReason: string;

  @Input() product: Models.ProductAdvertisement;

  @Input() targetBasePosition: Models.TargetBasePositionValue;

  @Input() tradeFreq: number;

  @Input() status: Models.TwoSidedQuoteStatus;

  @Input() addr: string;

  @Input() set orders(o: Models.Order[]) {
    this.addOrders(o);
  };

  @Input() set market(o: Models.Market) {
    this.addMarket(o);
  };

  @Output() onBidsLength = new EventEmitter<number>();

  @Output() onAsksLength = new EventEmitter<number>();

  @Output() onMarketWidth = new EventEmitter<number>();

  private clearQuote = () => {
    this.orderBids = [];
    this.orderAsks = [];
    this.orderPriceBids = [];
    this.orderPriceAsks = [];
  };

  private getAPR = () => {
    return Models.SideAPR[this.status.sideAPR];
  };

  private getStatus = (o: Models.QuoteStatus) => {
    return Models.QuoteStatus[o].replace(/([A-Z])/g, ' $1').trim();
  };

  private getSizeLevel = (size: string, ret: boolean) => {
    var decimals = (''+size).indexOf(".")+1;
    if (!decimals) return ret?size:"";
    var tokens: string[] = size.split("");
    var token: string = tokens.pop();
    var zeros: number = 0;
    while(token == "0" || token == ".") {
      zeros++;
      if (token == ".") break;
      token = tokens.pop();
    }
    if (!zeros) return ret?size:"";
    return ret
      ? size.substr(0, size.length-zeros)
      : size.substr(size.length-zeros);
  };

  private getBgSize = (lvl: Models.MarketSide, side: string) => {
    var allSize: string = side=='bids'?'allBidsSize':'allAsksSize';
    var red: string = side=='bids'?'141':'255';
    var green: string = side=='bids'?'226':'142';
    var blue: string = side=='bids'?'255':'140';
    var dir: string = side=='bids'?'left':'right';
    return 'linear-gradient(to '+dir+', rgba('+red+', '+green+', '+blue+', 0.69) '
      + Math.ceil(lvl.size/this[allSize]*100)
      + '%, rgba('+red+', '+green+', '+blue+', 0) 0%)';
  };

  private incrementMarketData = (diff: Models.MarketSide[], side: string) => {
    var allSize: string = side=='bids'?'allBidsSize':'allAsksSize';
    var dirtySize: string = side=='bids'?'dirtyBids':'dirtyAsks';
    for (var i: number = 0; i < diff.length; i++) {
      if (typeof diff[i].size != 'number') diff[i].size = 0;
      var found = false;
      for (var j: number = 0; j < this.levels[side].length; j++)
        if (diff[i].price === this.levels[side][j].price) {
          found = true;
          this[allSize] -= this.levels[side][j].size;
          if (diff[i].size) {
            this.levels[side][j].size = diff[i].size;
            this.levels[side][j].cssMod = 1;
            this[allSize] += this.levels[side][j].size;
          } else {
            this.levels[side][j].cssMod = 2;
            this[dirtySize]++;
          }
          break;
        }
      if (!found && diff[i].size) {
        for (var j: number = 0; j < this.levels[side].length; j++)
          if (this.levels[side][j].cssMod != 2 && (side == 'bids'
            ? diff[i].price > this.levels[side][j].price
            : diff[i].price < this.levels[side][j].price)
          ) {
            found = true;
            this[allSize] += diff[i].size;
            this.levels[side].splice(j, 0, diff[i]);
            this.levels[side][j].cssMod = 1;
            break;
          }
        if (!found) {
          this[allSize] += diff[i].size;
          this.levels[side].push(diff[i]);
          this.levels[side][this.levels[side].length - 1].cssMod = 1;
        }
      }
    }
  };

  private addOrders = (o: Models.Order[]) => {
    this.clearQuote();
    o.forEach(o => {
      const orderSide = o.side === Models.Side.Bid ? 'orderBids' : 'orderAsks';
      const orderPrice = o.side === Models.Side.Bid ? 'orderPriceBids' : 'orderPriceAsks';
      if (o.status == Models.OrderStatus.Terminated)
        this[orderSide] = this[orderSide].filter(x => x.orderId !== o.orderId);
      else if (!this[orderSide].filter(x => x.orderId === o.orderId).length)
        this[orderSide].push({
          orderId: o.orderId,
          side: o.side,
          price: o.price,
          quantity: o.quantity,
        });
      this[orderPrice] = this[orderSide].map((a)=>a.price.toFixed(this.product.tickPrice));

      if (this.orderBids.length) {
        var bid = this.orderBids.reduce((a,b)=>a.price>b.price?a:b);
        this.qBidPx = bid.price;
        this.qBidSz = bid.quantity;
      } else {
        this.qBidPx = 0;
        this.qBidSz = 0;
      }
      if (this.orderAsks.length) {
        var ask = this.orderAsks.reduce((a,b)=>a.price<b.price?a:b);
        this.qAskPx = ask.price;
        this.qAskSz = ask.quantity;
      } else {
        this.qAskPx = 0;
        this.qAskSz = 0;
      }

      this.ordersWidth = Math.max((this.qAskPx && this.qBidPx) ? this.qAskPx - this.qBidPx : 0, 0);
    });
  };

  private addMarket = (o: Models.Market) => {
    if (o == null || typeof o.diff != 'boolean') {
      this.allBidsSize = 0;
      this.allAsksSize = 0;
      if (o != null) {
        for (var i: number = 0; i < o.bids.length; i++)
          this.allBidsSize += o.bids[i].size;
        for (var i: number = 0; i < o.asks.length; i++)
          this.allAsksSize += o.asks[i].size;
      }
      this.levels = o;
    } else {
      if (this.levels == null) return;
      for (var i = this.levels.bids.length - 1; i >= 0; i--)
        if (this.levels.bids[i].cssMod)
          if (this.levels.bids[i].cssMod==2)
            this.levels.bids.splice(i, 1);
          else this.levels.bids[i].cssMod = 0;
      for (var i = this.levels.asks.length - 1; i >= 0; i--)
        if (this.levels.asks[i].cssMod)
          if (this.levels.asks[i].cssMod==2)
            this.levels.asks.splice(i, 1);
          else this.levels.asks[i].cssMod = 0;
      this.dirtyBids = 0;
      this.dirtyAsks = 0;
      this.incrementMarketData(o.bids, 'bids');
      this.incrementMarketData(o.asks, 'asks');
      if (this.levels == null) {
        this.onBidsLength.emit(0);
        this.onAsksLength.emit(0);
        this.marketWidth = 0;
      } else {
        this.onBidsLength.emit(this.levels.bids.length - this.dirtyBids);
        this.onAsksLength.emit(this.levels.asks.length - this.dirtyAsks);
        var topBid: number = 0;
        var topAsk: number = 0;
        for (var i: number = 0; i < this.levels.bids.length; i++)
          if (this.levels.bids[i].cssMod != 2) {
            topBid = this.levels.bids[i].price;
            break;
          }
        for (var i: number = 0; i < this.levels.asks.length; i++)
          if (this.levels.asks[i].cssMod != 2) {
            topAsk = this.levels.asks[i].price;
            break;
          }
        this.marketWidth = (topBid && topAsk) ? topAsk - topBid : 0;
      }
      this.onMarketWidth.emit(this.marketWidth / 2);
    }
  };
};
