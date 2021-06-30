import {Component, OnInit, Input, Output, EventEmitter} from '@angular/core';

import {Socket, Shared, Models} from 'lib/K';

@Component({
  selector: 'client',
  template: `<div class="row" [hidden]="!showSettings">
      <div class="col-md-12 col-xs-12 parameters-inputs">
          <div class="row">
            <settings
              [product]="product"
              [quotingParameters]="quotingParameters"></settings>
          </div>
      </div>
  </div>
  <div class="row">
    <div class="col-md-1 col-xs-12 text-center" style="padding-right:0px;">
      <div class="row exchange">
        <state-button
          [product]="product"
          [state]="state"
          (onConnected)="onConnected($event)"
          (onAgree)="onAgree($event)"></state-button>
        <wallet
          [product]="product"
          [position]="position"></wallet>
        <div>
          <a [hidden]="!product.webMarket"
            rel="noreferrer" target="_blank"
            href="{{ product.webMarket }}">Market</a>
          <span [hidden]="!(product.webMarket && product.webOrders)">, </span>
          <a [hidden]="!product.webOrders"
            rel="noreferrer" target="_blank"
            href="{{ product.webOrders }}">Orders</a>
          <br/><br/>
          <div>
            <button type="button"
              class="btn btn-default"
              id="order_form"
              (click)="showSubmitOrder = !showSubmitOrder">Submit Order</button>
          </div>
          <div style="padding-top: 2px;padding-bottom: 2px;">
            <button type="button"
              class="btn btn-danger"
              style="margin:5px 0px;"
              (click)="cancelAllOrders()"
              data-placement="bottom">Cancel Orders</button>
          </div>
          <div style="padding-bottom: 2px;">
            <button type="button"
              class="btn btn-info"
              (click)="cleanAllClosedOrders()"
              *ngIf="quotingParameters.safety"
              data-placement="bottom">Clean Pongs</button>
          </div>
          <div>
            <button type="button"
              class="btn btn-danger"
              style="margin-top:5px;"
              (click)="cleanAllOrders()"
              data-placement="bottom">{{ quotingParameters.safety ? 'Clean Pings' : 'Clean Trades' }}</button>
          </div>
          <br [hidden]="product.exchange=='HITBTC'" />
          <a href="#"
            [hidden]="product.exchange=='HITBTC'"
            (click)="toggleWatch(product.exchange.toLowerCase(), (product.base+'-'+product.quote).toLowerCase())">Watch</a>
          <br [hidden]="product.exchange=='HITBTC'" />
          <br/>
          <a href="#" (click)="toggleTakers()">Takers</a>, <a href="#" (click)="toggleStats()">Stats</a>
          <br/>
            <button type="button"
              class="btn btn-default"
              style="margin:14px 0px;"
              id="order_form"
              (click)="toggleSettings()">Settings
            </button>
          </div>
      </div>
    </div>
    <div [hidden]="!showSubmitOrder"
      class="col-md-5 col-xs-12">
      <submit-order
        [product]="product"></submit-order>
    </div>
    <div [hidden]="!showStats"
      [ngClass]="'col-md-11 col-xs-12  ' + (showStats == 2 ? 'absolute-charts' : 'relative-charts')">
      <stats
        ondblclick="this.style.opacity=this.style.opacity<1?1:0.4"
        [marketWidth]="marketWidth"
        [_showStats]="!!showStats"
        [product]="product"
        [quotingParameters]="quotingParameters"
        [targetBasePosition]="targetBasePosition"
        [marketChart]="marketChart"
        [tradesChart]="tradesChart"
        [position]="position"
        [fairValue]="fairValue"></stats>
    </div>
    <div [hidden]="showStats === 1"
      class="col-md-{{ showTakers ? '9' : '11' }} col-xs-12"
      style="padding-left:0px;padding-bottom:0px;">
      <div class="row" style="padding-top:0px;">
        <div class="col-md-4 col-xs-12" style="padding-left:0px;padding-top:0px;padding-right:0px;">
          <div class="row">
            <div class="col-md-6">
              <safety
                [product]="product"
                [fairValue]="fairValue"
                [tradeSafety]="tradeSafety"></safety>
            </div>
            <market
              [tradeFreq]="tradeFreq"
              (onBidsLength)="onBidsLength($event)"
              (onAsksLength)="onAsksLength($event)"
              (onMarketWidth)="onMarketWidth($event)"
              [product]="product"
              [addr]="addr"
              [status]="status"
              [market]="market"
              [orders]="orders"
              [targetBasePosition]="targetBasePosition"></market>
          </div>
        </div>
        <div class="col-md-8 col-xs-12" style="padding-left:0px;padding-right:0px;padding-top:0px;">
          <div class="row">
            <div class="notepad col-md-2 col-xs-12 text-center">
              <textarea
                [(ngModel)]="notepad"
                (ngModelChange)="changeNotepad(notepad)"
                placeholder="ephemeral notepad"
                class="ephemeralnotepad"
                style="height:131px;width: 100%;max-width: 100%;"></textarea>
            </div>
            <div class="col-md-10 col-xs-12" style="padding-right:0px;padding-top:0px;">
              <orders
                [product]="product"
                [orders]="orders"></orders>
            </div>
          </div>
          <div class="row">
            <trades
              (onTradesChartData)="onTradesChartData($event)"
              (onTradesMatchedLength)="onTradesMatchedLength($event)"
              (onTradesLength)="onTradesLength($event)"
              [product]="product"
              [quotingParameters]="quotingParameters"
              [trade]="trade"></trades>
          </div>
        </div>
      </div>
    </div>
    <div *ngIf="showTakers && showStats !== 1"
      class="col-md-2 col-xs-12" style="padding-left:0px;">
      <takers
        [product]="product"
        [taker]="taker"></takers>
    </div>
  </div>`
})
export class ClientComponent implements OnInit {

  private tradesLength: number = 0;
  private tradesMatchedLength: number = 0;
  private bidsLength: number = 0;
  private asksLength: number = 0;
  private notepad: string;
  private showSettings: boolean = true;
  private showTakers: boolean = false;
  private showStats: number = 0;
  private showSubmitOrder: boolean = false;
  private quotingParameters: Models.QuotingParameters = <Models.QuotingParameters>{};
  private marketWidth: number = 0;
  private orders: Models.Order[] = [];
  private market: Models.Market = null;
  private trade: Models.Trade = null;
  private taker: Models.MarketTrade = null;
  private tradesChart: Models.TradeChart = new Models.TradeChart();
  private marketChart: Models.MarketChart = new Models.MarketChart();
  private status: Models.TwoSidedQuoteStatus = new Models.TwoSidedQuoteStatus();
  private fairValue: Models.FairValue = new Models.FairValue();
  private position: Models.PositionReport = new Models.PositionReport();
  private tradeSafety: Models.TradeSafety = new Models.TradeSafety();
  private targetBasePosition: Models.TargetBasePositionValue = new Models.TargetBasePositionValue();

  private cancelAllOrders = () => new Socket.Fire(Models.Topics.CancelAllOrders).fire();

  private cleanAllClosedOrders = () => new Socket.Fire(Models.Topics.CleanAllClosedTrades).fire();

  private cleanAllOrders = () => new Socket.Fire(Models.Topics.CleanAllTrades).fire();

  private changeNotepad = (content:string) => new Socket.Fire(Models.Topics.Notepad).fire([content]);

  @Input() addr: string;

  @Input() tradeFreq: number;

  @Input() state: Models.ExchangeState;

  @Input() product: Models.ProductAdvertisement;

  @Output() onFooter = new EventEmitter<string>();

  ngOnInit() {
    new Socket.Subscriber(Models.Topics.QuotingParametersChange)
      .registerSubscriber((o: Models.QuotingParameters) => { this.quotingParameters = o; });

    new Socket.Subscriber(Models.Topics.OrderStatusReports)
      .registerSubscriber((o: Models.Order[]) => { this.orders = o; })
      .registerDisconnectedHandler(() => { this.orders = []; });

    new Socket.Subscriber(Models.Topics.Position)
      .registerSubscriber((o: Models.PositionReport) => { this.position = o; });

    new Socket.Subscriber(Models.Topics.FairValue)
      .registerSubscriber((o: Models.FairValue) => { this.fairValue = o; });

    new Socket.Subscriber(Models.Topics.TradeSafetyValue)
      .registerSubscriber((o: Models.TradeSafety) => { this.tradeSafety = o; });

    new Socket.Subscriber(Models.Topics.MarketData)
      .registerSubscriber((o: Models.Market) => { this.market = o; })
      .registerDisconnectedHandler(() => { this.market = null; });

    new Socket.Subscriber(Models.Topics.Trades)
      .registerSubscriber((o: Models.Trade) => { this.trade = o; })
      .registerDisconnectedHandler(() => { this.trade = null; });

    new Socket.Subscriber(Models.Topics.MarketTrade)
      .registerSubscriber((o: Models.MarketTrade) => { this.taker = o; })
      .registerDisconnectedHandler(() => { this.taker = null; });

    new Socket.Subscriber(Models.Topics.QuoteStatus)
      .registerSubscriber((o: Models.TwoSidedQuoteStatus) => { this.status = o; })
      .registerDisconnectedHandler(() => { this.status = new Models.TwoSidedQuoteStatus(); });

    new Socket.Subscriber(Models.Topics.TargetBasePosition)
      .registerSubscriber((o: Models.TargetBasePositionValue) => { this.targetBasePosition = o; });

    new Socket.Subscriber(Models.Topics.MarketChart)
      .registerSubscriber((o: Models.MarketChart) => { this.marketChart = o; });

    new Socket.Subscriber(Models.Topics.Notepad)
      .registerSubscriber((notepad : string) => { this.notepad = notepad; });

    window.addEventListener("message", e => {
      if (!e.data.indexOf) return;

      if (e.data.indexOf('cryptoWatch=') === 0) {
        if (window.parent !== window) window.parent.postMessage(e.data, '*');
        else {
          var data: string[] = e.data.replace('cryptoWatch=', '').split(',');
          this._toggleWatch(data[0], data[1]);
        }
      }
    }, false);
  };

  private toggleSettings = () => {
    this.showSettings = !this.showSettings;
    setTimeout(() => {window.dispatchEvent(new Event('resize'))}, 0);
  };

  private toggleTakers = () => {
    this.showTakers = !this.showTakers;
    setTimeout(() => {window.dispatchEvent(new Event('resize'))}, 0);
  };

  private toggleStats = () => {
    if (++this.showStats>=3) this.showStats = 0;
  };

  private toggleWatch = (watchExchange: string, watchPair: string) => {
    if (window.parent !== window) {
      window.parent.postMessage('cryptoWatch='+watchExchange+','+watchPair, '*');
      return;
    }
    var self = this;
    var toggleWatch = function() {
      self._toggleWatch(watchExchange, watchPair);
    };
    if (!window.hasOwnProperty("cryptowatch"))
      (function(d, script) {
        script = d.createElement('script');
        script.type = 'text/javascript';
        script.async = true;
        script.onload = toggleWatch;
        script.src = 'https://static.cryptowat.ch/assets/scripts/embed.bundle.js';
        d.getElementsByTagName('head')[0].appendChild(script);
      }(document));
    else toggleWatch();
  };

  private _toggleWatch = (watchExchange: string, watchPair: string) => {
    if (!document.getElementById('cryptoWatch'+watchExchange+watchPair)) {
      if(watchExchange=='coinbase') watchExchange = 'coinbase-pro';
      this.setDialog('cryptoWatch'+watchExchange+watchPair, 'open', {title: watchExchange.toUpperCase()+' '+watchPair.toUpperCase().replace('-','/'),width: 800,height: 400,content: `<div id="container`+watchExchange+watchPair+`" style="width:100%;height:100%;"></div>`});
      (new (<any>window).cryptowatch.Embed(watchExchange, watchPair.replace('-',''), {timePeriod: '1d',customColorScheme: {bg:"000000",text:"b2b2b2",textStrong:"e5e5e5",textWeak:"7f7f7f",short:"FD4600",shortFill:"FF672C",long:"6290FF",longFill:"002782",cta:"363D52",ctaHighlight:"414A67",alert:"FFD506"}})).mount('#container'+watchExchange+watchPair);
    } else this.setDialog('cryptoWatch'+watchExchange+watchPair, 'close', {content:''});
  };

  private onTradesChartData(o: Models.TradeChart) {
    this.tradesChart = o;
  };

  private onTradesLength(o: number) {
    this.tradesLength = o;
    this.setFooter();
  };

  private onTradesMatchedLength(o: number) {
    this.tradesMatchedLength = o;
    this.setFooter();
  };

  private onBidsLength(o: number) {
    this.bidsLength = o;
    this.setFooter();
  };

  private onAsksLength(o: number) {
    this.asksLength = o;
    this.setFooter();
  };

  private setFooter() {
    this.onFooter.emit(`<span style="margin-top: 6px;display: inline-block;">
      <span
        title="` + ( this.tradesMatchedLength===-1 ? 'Orders filled' : 'Pings (open positions)') + ` in memory"
      >` + this.tradesLength + `</span>` + (
        this.tradesMatchedLength < 0
          ? ``
          : `/<span title="Pongs (closed positions) in memory">` + this.tradesMatchedLength + `</span>`
      ) + `</span> -
    <span
      title="Market Levels in memory (bids|asks)"
      style="margin-top: 6px;display: inline-block;"
    >` + this.bidsLength + `|` + this.asksLength + `</span> - `);
  };

  private onMarketWidth(o: number) {
    this.marketWidth = o;
  };

  private setDialog = (uniqueId: string, set: string, config: object) => {
    if (set === "open") {
      var div = document.createElement('div');
        div.className = 'dialog-box';
        div.id = uniqueId;
        div.innerHTML = '<div class="dialog-content">&nbsp;</div><h3 class="dialog-title"><a href="javascript:;" class="dialog-close" title="Close">&times;</a></h3>';
      document.body.appendChild(div);
    }

    var dialog = document.getElementById(uniqueId), selected = null, defaults = {
      title: '',
      content: '',
      width: 300,
      height: 150,
      top: false,
      left: false
    };

    for (var i in config) { defaults[i] = (typeof config[i] !== 'undefined') ? config[i] : defaults[i]; }

    function _drag_init(e, el) {
      for (var i=0;i<document.getElementsByClassName('dialog-box').length;i++)
        (<HTMLElement>document.getElementsByClassName('dialog-box')[i]).style.zIndex = "9999";
      el.style.zIndex = "10000";
      var posX = e.clientX,
          posY = e.clientY,
          divTop = parseFloat(el.style.top.indexOf('%')>-1?el.offsetTop + el.offsetHeight/2:el.style.top),
          divLeft = parseFloat(el.style.left.indexOf('%')>-1?el.offsetLeft + el.offsetWidth/2:el.style.left)
      var diffX = posX - divLeft,
          diffY = posY - divTop;
      document.onmousemove = function(e){
          var posX = e.clientX,
              posY = e.clientY,
              aX = posX - diffX,
              aY = posY - diffY;
          el.style.left = aX + 'px';
          el.style.top = aY + 'px';
      }
    }

    dialog.className =  'dialog-box fixed-dialog-box';
    dialog.style.visibility = (set === "open") ? "visible" : "hidden";
    dialog.style.opacity = (set === "open") ? "1" : "0";
    dialog.style.width = defaults.width + 'px';
    dialog.style.height = defaults.height + 'px';
    dialog.style.top = (!defaults.top) ? "50%" : '0px';
    dialog.style.left = (!defaults.left) ? "50%" : '0px';
    dialog.style.marginTop = (!defaults.top) ? '-' + defaults.height/2 + 'px' : defaults.top + 'px';
    dialog.style.marginLeft = (!defaults.left) ? '-' + defaults.width/2 + 'px' : defaults.left + 'px';
    dialog.children[1].innerHTML = defaults.title + dialog.children[1].innerHTML;
    dialog.children[0].innerHTML = defaults.content;
    (<HTMLElement>dialog.children[1]).onmousedown = function(e) {
      if ((e.target || e.srcElement)===<HTMLElement>this) _drag_init(e, (<HTMLElement>this).parentNode);
    };
    (<HTMLElement>dialog.children[1].children[0]).onclick = function() {
      dialog.remove();
    };
    document.onmouseup = function() { document.onmousemove = function() {}; };
    if (set === "close") dialog.remove();
  };
};
