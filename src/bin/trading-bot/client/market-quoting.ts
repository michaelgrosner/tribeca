import {Component, EventEmitter, Input, Output} from '@angular/core';

import * as Models from './models';

@Component({
  selector: 'market-quoting',
  template: `<div class="tradeSafety2" style="margin-top:-4px;padding-top:0px;padding-right:0px;"><div style="padding-top:0px;padding-right:0px;">
      Market Width: <span class="{{ marketWidth ? \'text-danger\' : \'text-muted\' }}">{{ marketWidth.toFixed(product.tickPrice) }}</span>,
      Quote Width: <span class="{{ ordersWidth ? \'text-danger\' : \'text-muted\' }}">{{ ordersWidth.toFixed(product.tickPrice) }}</span>, Quotes: <span title="Quotes in memory Waiting status update" class="{{ quotesInMemoryWaiting ? \'text-danger\' : \'text-muted\' }}">{{ quotesInMemoryWaiting }}</span>/<span title="Quotes in memory Working" class="{{ quotesInMemoryWorking ? \'text-danger\' : \'text-muted\' }}">{{ quotesInMemoryWorking }}</span>/<span title="Quotes in memory Zombie" class="{{ quotesInMemoryZombies ? \'text-danger\' : \'text-muted\' }}">{{ quotesInMemoryZombies }}</span>
      <div style="padding-left:0px;">Wallet TBP: <span class="text-danger">{{ targetBasePosition.toFixed(8) }}</span>, pDiv: <span class="text-danger">{{ positionDivergence.toFixed(8) }}</span>, APR: <span class="{{ sideAPRSafety!=\'Off\' ? \'text-danger\' : \'text-muted\' }}">{{ sideAPRSafety }}</span></div>
      </div></div><div style="padding-right:4px;padding-left:4px;padding-top:4px;line-height:1.3;">
      <table class="marketQuoting table table-hover table-responsive text-center">
        <tr class="info">
          <td>bidSize&nbsp;</td>
          <td>bidPrice</td>
          <td>askPrice</td>
          <td>askSize&nbsp;</td>
        </tr>
        <tr class="info">
          <th *ngIf="bidStatus == 'Live'" class="text-danger">{{ qBidSz.toFixed(product.tickSize) }}<span *ngIf="!qBidSz">&nbsp;</span></th>
          <th *ngIf="bidStatus == 'Live'" class="text-danger">{{ qBidPx.toFixed(product.tickPrice) }}</th>
          <th *ngIf="bidStatus != 'Live'" colspan="2" class="text-danger" title="Bids Quote Status">{{ bidStatus }}</th>
          <th *ngIf="askStatus == 'Live'" class="text-danger">{{ qAskPx.toFixed(product.tickPrice) }}</th>
          <th *ngIf="askStatus == 'Live'" class="text-danger">{{ qAskSz.toFixed(product.tickSize) }}<span *ngIf="!qAskSz">&nbsp;</span></th>
          <th *ngIf="askStatus != 'Live'" colspan="2" class="text-danger" title="Ask Quote Status">{{ askStatus }}</th>
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
    <table *ngIf="addr" class="table-responsive text-center" style="width:100%;"><tr><td><div class="text-danger text-center"><br /><br />To <a href="https://github.com/ctubio/Krypto-trading-bot/blob/master/README.md#unlock" target="_blank">unlock</a> realtime market levels,<br />and to collaborate with the development..<br /><br />make an acceptable <code>Pull Request</code> on github,<br/><br/>or send <code>0.01210000 BTC</code> or more to:<br /><a href="https://live.blockcypher.com/btc/address/{{ addr }}" target="_blank">{{ addr }}</a><br /><br />Wait 0 confirmations and restart this bot.</div></td></tr></table>
    </div>`
})
export class MarketQuotingComponent {

  public levels: Models.Market = null;
  public allBidsSize: number = 0;
  public allAsksSize: number = 0;
  public dirtyBids: number = 0;
  public dirtyAsks: number = 0;
  public qBidSz: number = 0;
  public qBidPx: number = 0;
  public qAskPx: number = 0;
  public qAskSz: number = 0;
  public orderBids: any[];
  public orderAsks: any[];
  public orderPriceBids: number[] = [];
  public orderPriceAsks: number[] = [];
  public bidStatus: string;
  public askStatus: string;
  public quotesInMemoryWaiting: number;
  public quotesInMemoryWorking: number;
  public quotesInMemoryZombies: number;
  public marketWidth: number = 0;
  public ordersWidth: number = 0;
  public noBidReason: string;
  public noAskReason: string;
  private targetBasePosition: number = 0;
  private positionDivergence: number = 0;
  private sideAPRSafety: string;

  @Input() product: Models.ProductAdvertisement;

  @Input() addr: string;

  @Input() set agree(agree: boolean) {
    if (agree) return;
    this.clearQuote();
  }

  @Output() onBidsLength = new EventEmitter<number>();
  @Output() onAsksLength = new EventEmitter<number>();
  @Output() onMarketWidth = new EventEmitter<number>();

  @Input() set setOrderList(o: any[]) {
    this.updateQuote(o);
  }

  @Input() set setTargetBasePosition(o: Models.TargetBasePositionValue) {
    if (o == null) {
      this.targetBasePosition = 0;
      this.positionDivergence = 0;
    } else {
      this.targetBasePosition = o.tbp;
      this.positionDivergence = o.pDiv;
    }
  }

  @Input() set setQuoteStatus(o: Models.TwoSidedQuoteStatus) {
    if (o == null) {
      this.bidStatus = Models.QuoteStatus[0];
      this.askStatus = Models.QuoteStatus[0];
      this.sideAPRSafety = null;
      this.quotesInMemoryWaiting = 0;
      this.quotesInMemoryWorking = 0;
      this.quotesInMemoryZombies = 0;
    } else {
      this.bidStatus = Models.QuoteStatus[o.bidStatus];
      this.askStatus = Models.QuoteStatus[o.askStatus];
      this.sideAPRSafety = Models.SideAPR[o.sideAPR];
      this.quotesInMemoryWaiting = o.quotesInMemoryWaiting;
      this.quotesInMemoryWorking = o.quotesInMemoryWorking;
      this.quotesInMemoryZombies = o.quotesInMemoryZombies;
    }
  }

  private clearQuote = () => {
    this.orderBids = [];
    this.orderAsks = [];
  }

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
  }

  private getBgSize = (lvl: Models.MarketSide, side: string) => {
    var allSize: string = side=='bids'?'allBidsSize':'allAsksSize';
    var red: string = side=='bids'?'141':'255';
    var green: string = side=='bids'?'226':'142';
    var blue: string = side=='bids'?'255':'140';
    var dir: string = side=='bids'?'left':'right';
    return 'linear-gradient(to '+dir+', rgba('+red+', '+green+', '+blue+', 0.69) '
      + Math.ceil(lvl.size/this[allSize]*100)
      + '%, rgba('+red+', '+green+', '+blue+', 0) 0%)';
  }

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

  @Input() set setMarketData(update: Models.Market) {
    if (update == null || typeof (<any>update).diff != 'boolean') {
      this.allBidsSize = 0;
      this.allAsksSize = 0;
      if (update != null) {
        for (var i: number = 0; i < update.bids.length; i++)
          this.allBidsSize += update.bids[i].size;
        for (var i: number = 0; i < update.asks.length; i++)
          this.allAsksSize += update.asks[i].size;
      }
      this.levels = update;
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
      this.incrementMarketData(update.bids, 'bids');
      this.incrementMarketData(update.asks, 'asks');
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
      this.onMarketWidth.emit(this.marketWidth);
    }
  }

  private updateQuote = (o) => {
    if (!o || (typeof o.length == 'number' && !o.length)) {
      this.clearQuote();
      return;
    } else if (typeof o.length == 'number' && typeof o[0] == 'object') {
      this.clearQuote();
      return o.forEach(x => setTimeout(this.updateQuote(x), 0));
    }

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
  }
}
