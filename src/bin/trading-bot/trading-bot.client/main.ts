import 'zone.js';

import {NgModule, Component, OnInit, enableProdMode} from '@angular/core';
import {platformBrowserDynamic} from '@angular/platform-browser-dynamic';
import {FormsModule} from '@angular/forms';
import {BrowserModule} from '@angular/platform-browser';
import {AgGridModule} from '@ag-grid-community/angular';
import {HighchartsChartModule} from 'highcharts-angular';

import * as Models from 'lib/models';
import * as Socket from 'lib/socket';
import * as Shared from 'lib/shared';

import {SettingsComponent} from './settings';
import {MarketComponent} from './market';
import {TakersComponent} from './takers';
import {SafetyComponent} from './safety';
import {WalletComponent} from './wallet';
import {OrdersComponent} from './orders';
import {TradesComponent} from './trades';
import {SubmitComponent} from './submit';
import {StateComponent} from './state';
import {StatsComponent} from './stats';

@Component({
  selector: 'ui',
  template: `<div>
    <div [hidden]="ready" style="padding:42px;transform:rotate(-6deg);">
        <h4 class="text-danger text-center">{{ product.environment ? product.environment+' is d' : 'D' }}isconnected.</h4>
    </div>
    <div [hidden]="!ready">
        <div class="container-fluid">
            <div id="hud" [ngClass]="connected ? 'bg-success' : 'bg-danger'">
                <div *ngIf="ready" class="row" [hidden]="!showSettings">
                    <div class="col-md-12 col-xs-12 parameters-inputs">
                        <div class="row">
                          <settings [product]="product" [setSettings]="quotingParameters"></settings>
                        </div>
                    </div>
                </div>
                <div class="row">
                    <div class="col-md-1 col-xs-12 text-center" style="padding-right:0px;">
                        <div class="row exchange">
                            <state-button [product]="product" (onConnected)="onConnected($event)" (onAgree)="onAgree($event)" [setExchangeStatus]="state" [setServerStatus]="connected"></state-button>
                            <wallet [product]="product" [setPosition]="Position"></wallet>
                            <div>
                              <a [hidden]="!product.webMarket" rel="noreferrer" href="{{ product.webMarket }}" target="_blank">Market</a><span [hidden]="!(product.webMarket && product.webOrders)">, </span><a [hidden]="!product.webOrders" rel="noreferrer" href="{{ product.webOrders }}" target="_blank">Orders</a>
                              <br/><br/><div>
                                  <button type="button"
                                          class="btn btn-default"
                                          id="order_form"
                                          (click)="showSubmitOrder = !showSubmitOrder">Submit Order
                                  </button>
                              </div>
                              <div style="padding-top: 2px;padding-bottom: 2px;">
                                  <button type="button"
                                          class="btn btn-danger"
                                          style="margin:5px 0px;"
                                          (click)="cancelAllOrders()"
                                          data-placement="bottom">Cancel Orders
                                  </button>
                              </div>
                              <div style="padding-bottom: 2px;">
                                  <button type="button"
                                          class="btn btn-info"
                                          (click)="cleanAllClosedOrders()"
                                          *ngIf="quotingParameters.safety"
                                          data-placement="bottom">Clean Pongs
                                  </button>
                              </div>
                              <div>
                                  <button type="button"
                                          class="btn btn-danger"
                                          style="margin-top:5px;"
                                          (click)="cleanAllOrders()"
                                          data-placement="bottom">{{ quotingParameters.safety ? 'Clean Pings' : 'Clean Trades' }}
                                  </button>
                              </div>
                              <br [hidden]="product.exchange=='HITBTC'" /><a [hidden]="product.exchange=='HITBTC'" href="#" (click)="toggleWatch(product.exchange.toLowerCase(), (product.base+'-'+product.quote).toLowerCase())">Watch</a><br [hidden]="product.exchange=='HITBTC'" />
                              <br/><a href="#" (click)="toggleTakers()">Takers</a>, <a href="#" (click)="toggleStats()">Stats</a>
                              <br/><button type="button"
                                          class="btn btn-default"
                                          style="margin:14px 0px;"
                                          id="order_form"
                                          (click)="toggleSettings(showSettings = !showSettings)">Settings
                                   </button>
                              <br/><a href="#" (click)="changeTheme()">{{ system_theme ? 'Light' : 'Dark' }}</a>
                            </div>
                        </div>
                    </div>
                    <div [hidden]="!showSubmitOrder" class="col-md-5 col-xs-12">
                      <submit-order [product]="product"></submit-order>
                    </div>

                    <div [hidden]="!showStats" [ngClass]="showStats == 2 ? 'col-md-11 col-xs-12 absolute-charts' : 'col-md-11 col-xs-12 relative-charts'">
                      <stats ondblclick="this.style.opacity=this.style.opacity<1?1:0.4" [setMarketWidth]="marketWidth" [setShowStats]="!!showStats" [product]="product" [setQuotingParameters]="quotingParameters" [setTargetBasePosition]="TargetBasePosition" [setMarketChartData]="MarketChartData" [setTradesChartData]="TradesChartData" [setPosition]="Position" [setFairValue]="FairValue"></stats>
                    </div>
                    <div [hidden]="showStats === 1" class="col-md-{{ showTakers ? '9' : '11' }} col-xs-12" style="padding-left:0px;padding-bottom:0px;">
                      <div class="row" style="padding-top:0px;">
                        <div class="markets-params-left col-md-4 col-xs-12" style="padding-left:0px;padding-top:0px;padding-right:0px;">
                            <div class="row">
                              <div class="col-md-6">
                                <safety [product]="product" [setFairValue]="FairValue" [setTradeSafety]="TradeSafety"></safety>
                              </div>
                              <market [tradeFreq]="tradeFreq" (onBidsLength)="onBidsLength($event)" (onAsksLength)="onAsksLength($event)" (onMarketWidth)="onMarketWidth($event)" [agree]="!!state.agree" [product]="product" [addr]="addr" [setQuoteStatus]="QuoteStatus" [setMarketData]="MarketData" [setOrderList]="orderList" [setTargetBasePosition]="TargetBasePosition"></market>
                            </div>
                        </div>
                        <div class="orders-table-right col-md-8 col-xs-12" style="padding-left:0px;padding-right:0px;padding-top:0px;">
                          <div class="row">
                            <div class="notepad col-md-2 col-xs-12 text-center">
                              <textarea [(ngModel)]="notepad" (ngModelChange)="changeNotepad(notepad)" placeholder="ephemeral notepad" class="ephemeralnotepad" style="height:131px;width: 100%;max-width: 100%;"></textarea>
                            </div>
                            <div class="col-md-10 col-xs-12" style="padding-right:0px;padding-top:0px;">
                              <order-list [agree]="!!state.agree" [product]="product" [setOrderList]="orderList"></order-list>
                            </div>
                          </div>
                          <div class="row">
                            <trade-list (onTradesChartData)="onTradesChartData($event)" (onTradesMatchedLength)="onTradesMatchedLength($event)" (onTradesLength)="onTradesLength($event)" [product]="product" [setQuotingParameters]="quotingParameters" [setTrade]="Trade"></trade-list>
                          </div>
                        </div>
                      </div>
                    </div>
                    <div [hidden]="!showTakers || showStats === 1" class="col-md-2 col-xs-12" style="padding-left:0px;">
                      <takers [product]="product" [setMarketTradeData]="MarketTradeData"></takers>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <address class="text-center">
      <small>
        <a rel="noreferrer" href="{{ homepage }}/blob/master/README.md" target="_blank">README</a> - <a rel="noreferrer" href="{{ homepage }}/blob/master/doc/MANUAL.md" target="_blank">MANUAL</a> - <a rel="noreferrer" href="{{ homepage }}" target="_blank">SOURCE</a> - <span [hidden]="!ready"><span [hidden]="!product.inet"><span title="non-default Network Interface for outgoing traffic">{{ product.inet }}</span> - </span><span title="Server used RAM" style="margin-top: 6px;display: inline-block;">{{ server_memory }}</span> - <span title="Client used RAM" style="margin-top: 6px;display: inline-block;">{{ client_memory }}</span> - <span title="Database Size" style="margin-top: 6px;display: inline-block;">{{ db_size }}</span> - <span style="margin-top: 6px;display: inline-block;"><span title="{{ tradesMatchedLength===-1 ? 'Trades' : 'Pings' }} in memory">{{ tradesLength }}</span><span [hidden]="tradesMatchedLength < 0">/</span><span [hidden]="tradesMatchedLength < 0" title="Pongs in memory">{{ tradesMatchedLength }}</span></span> - <span title="Market Levels in memory (bids|asks)" style="margin-top: 6px;display: inline-block;">{{ bidsLength }}|{{ asksLength }}</span> - </span><a href="#" (click)="openMatryoshka()">MATRYOSHKA</a> - <a rel="noreferrer" href="{{ homepage }}/issues/new?title=%5Btopic%5D%20short%20and%20sweet%20description&body=description%0Aplease,%20consider%20to%20add%20all%20possible%20details%20%28if%20any%29%20about%20your%20new%20feature%20request%20or%20bug%20report%0A%0A%2D%2D%2D%0A%60%60%60%0Aapp%20exchange%3A%20{{ product.exchange }}/{{ product.base+'/'+product.quote }}%0Aapp%20version%3A%20undisclosed%0AOS%20distro%3A%20undisclosed%0A%60%60%60%0A![300px-spock_vulcan-salute3](https://cloud.githubusercontent.com/assets/1634027/22077151/4110e73e-ddb3-11e6-9d84-358e9f133d34.png)" target="_blank">CREATE ISSUE</a> - <a rel="noreferrer" href="https://github.com/ctubio/Krypto-trading-bot/discussions/new" target="_blank">HELP</a> - <a title="irc://irc.freenode.net:6697/#tradingBot" href="irc://irc.freenode.net:6697/#tradingBot">IRC</a>|<a target="_blank" rel="noreferrer" href="https://kiwiirc.com/client/irc.freenode.net:6697/?theme=cli#tradingBot" rel="nofollow">www</a>
      </small>
    </address>
    <iframe id="matryoshka" style="margin:0px;padding:0px;border:0px;width:100%;height:0px;" src="about:blank"></iframe>
  </div>`
})
class ClientComponent implements OnInit {

  public addr: string;
  public homepage: string = "https://github.com/ctubio/Krypto-trading-bot";
  public server_memory: string;
  public client_memory: string;
  public db_size: string;
  public notepad: string;
  public ready: boolean;
  public state: any = {agree: 0};
  public connected: boolean = false;
  public showSettings: boolean = true;
  public showTakers: boolean = false;
  public showStats: number = 0;
  public showSubmitOrder: boolean = false;
  private user_theme: string = null;
  private system_theme: string = null;
  private quotingParameters: Models.QuotingParameters = <Models.QuotingParameters>{};
  public tradeFreq: number = 0;
  public tradesChart: Models.TradeChart = null;
  public tradesLength: number = 0;
  public tradesMatchedLength: number = 0;
  public bidsLength: number = 0;
  public asksLength: number = 0;
  public marketWidth: number = 0;
  public orderList: any[] = [];
  public FairValue: Models.FairValue = null;
  public Trade: Models.Trade = null;
  public Position: Models.PositionReport = null;
  public TradeSafety: Models.TradeSafety = null;
  public TargetBasePosition: Models.TargetBasePositionValue = null;
  public MarketData: Models.Market = null;
  public MarketTradeData: Models.MarketTrade = null;
  public QuoteStatus: Models.TwoSidedQuoteStatus = null;
  public MarketChartData: Models.MarketChart = null;
  public TradesChartData: Models.TradeChart = null;
  public cancelAllOrders = () => {};
  public cleanAllClosedOrders = () => {};
  public cleanAllOrders = () => {};

  ngOnInit() {
    new Socket.Client();

    this.buildDialogs(window, document);

    new Socket.Subscriber(Models.Topics.Connectivity)
      .registerSubscriber((o: any) => { this.state = o; })
      .registerConnectHandler(() => { this.connected = true; })
      .registerDisconnectedHandler(() => { this.connected = false; });

    new Socket.Subscriber(Models.Topics.QuotingParametersChange)
      .registerSubscriber((o: Models.QuotingParameters) => { this.quotingParameters = o; });

    new Socket.Subscriber(Models.Topics.ProductAdvertisement)
      .registerSubscriber(this.onAdvert)
      .registerDisconnectedHandler(() => { this.ready = false; });

    new Socket.Subscriber(Models.Topics.OrderStatusReports)
      .registerSubscriber((o: any[]) => { this.orderList = o; })
      .registerDisconnectedHandler(() => { this.orderList = []; });

    new Socket.Subscriber(Models.Topics.Position)
      .registerSubscriber((o: Models.PositionReport) => { this.Position = o; });

    new Socket.Subscriber(Models.Topics.FairValue)
      .registerSubscriber((o: Models.FairValue) => { this.FairValue = o; });

    new Socket.Subscriber(Models.Topics.TradeSafetyValue)
      .registerSubscriber((o: Models.TradeSafety) => { this.TradeSafety = o; });

    new Socket.Subscriber(Models.Topics.Trades)
      .registerSubscriber((o: Models.Trade) => { this.Trade = o; })
      .registerDisconnectedHandler(() => { this.Trade = null; });

    new Socket.Subscriber(Models.Topics.MarketData)
      .registerSubscriber((o: Models.Market) => { this.MarketData = o; })
      .registerDisconnectedHandler(() => { this.MarketData = null; });

    new Socket.Subscriber(Models.Topics.MarketTrade)
      .registerSubscriber((o: Models.MarketTrade) => { this.MarketTradeData = o; })
      .registerDisconnectedHandler(() => { this.MarketTradeData = null; });

    new Socket.Subscriber(Models.Topics.QuoteStatus)
      .registerSubscriber((o: Models.TwoSidedQuoteStatus) => { this.QuoteStatus = o; })
      .registerDisconnectedHandler(() => { this.QuoteStatus = null; });

    new Socket.Subscriber(Models.Topics.TargetBasePosition)
      .registerSubscriber((o: Models.TargetBasePositionValue) => { this.TargetBasePosition = o; });

    new Socket.Subscriber(Models.Topics.MarketChart)
      .registerSubscriber((o: Models.MarketChart) => { this.MarketChartData = o; });

    new Socket.Subscriber(Models.Topics.ApplicationState)
      .registerSubscriber(this.onAppState);

    new Socket.Subscriber(Models.Topics.Notepad)
      .registerSubscriber((notepad : string) => { this.notepad = notepad; });

    this.cancelAllOrders = () => new Socket.Fire(Models.Topics.CancelAllOrders).fire();

    this.cleanAllClosedOrders = () => new Socket.Fire(Models.Topics.CleanAllClosedTrades).fire();

    this.cleanAllOrders = () => new Socket.Fire(Models.Topics.CleanAllTrades).fire();

    this.changeNotepad = (content:string) => new Socket.Fire(Models.Topics.Notepad).fire([content]);

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

    this.ready = false;
  }

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
      if(watchExchange=='coinbase') watchExchange = 'coinbase-pro';
      (<any>window).setDialog('cryptoWatch'+watchExchange+watchPair, 'open', {title: watchExchange.toUpperCase()+' '+watchPair.toUpperCase().replace('-','/'),width: 800,height: 400,content: `<div id="container`+watchExchange+watchPair+`" style="width:100%;height:100%;"></div>`});
      (new (<any>window).cryptowatch.Embed(watchExchange, watchPair.replace('-',''), {timePeriod: '1d',customColorScheme: {bg:"000000",text:"b2b2b2",textStrong:"e5e5e5",textWeak:"7f7f7f",short:"FD4600",shortFill:"FF672C",long:"6290FF",longFill:"002782",cta:"363D52",ctaHighlight:"414A67",alert:"FFD506"}})).mount('#container'+watchExchange+watchPair);
    } else (<any>window).setDialog('cryptoWatch'+watchExchange+watchPair, 'close', {content:''});
  };

  public openMatryoshka = () => {
    const url = window.prompt('Enter the URL of another instance:',this.product.matryoshka||'https://');
    (<any>document.getElementById('matryoshka').attributes).src.value = url||'about:blank';
    document.getElementById('matryoshka').style.height = (url&&url!='https://')?'589px':'0px';
  };
  public resizeMatryoshka = () => {
    if (window.parent === window) return;
    window.parent.postMessage('height='+document.getElementsByTagName('body')[0].getBoundingClientRect().height+'px', '*');
  };
  public product: Models.ProductAdvertisement = new Models.ProductAdvertisement(
    "", "", "", "", "", 0, "", "", "", "", 8, 8, 1e-8, 1e-8, 1e-8
  );

  public onTradesChartData(tradesChart: Models.TradeChart) {
    this.TradesChartData = tradesChart;
  }
  public onTradesLength(tradesLength: number) {
    this.tradesLength = tradesLength;
  }
  public onTradesMatchedLength(tradesMatchedLength: number) {
    this.tradesMatchedLength = tradesMatchedLength;
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

  private bytesToSize = (input:number, precision:number) => {
    if (!input) return '0B';
    let unit = ['', 'K', 'M', 'G', 'T', 'P'];
    let index = Math.floor(Math.log(input) / Math.log(1024));
    if (index >= unit.length) return input + 'B';
    return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B'
  }

  private onAppState = (o : Models.ApplicationState) => {
    this.server_memory = this.bytesToSize(o.memory, 0);
    this.client_memory = this.bytesToSize((<any>window.performance).memory ? (<any>window.performance).memory.usedJSHeapSize : 1, 0);
    this.db_size = this.bytesToSize(o.dbsize, 0);
    this.tradeFreq = (o.freq);
    this.user_theme = this.user_theme!==null ? this.user_theme : (o.theme==1 ? '' : (o.theme==2 ? '-dark' : this.user_theme));
    this.system_theme = this.getTheme((new Date).getHours());
    this.setTheme();
    this.addr = o.addr;
  }

  private setTheme = () => {
    if ((<any>document.getElementById('daynight').attributes).href.value!='/css/bootstrap-theme'+this.system_theme+'.min.css')
      (<any>document.getElementById('daynight').attributes).href.value = '/css/bootstrap-theme'+this.system_theme+'.min.css';
  }

  public changeTheme = () => {
    this.user_theme = this.user_theme!==null
                  ? (this.user_theme  ==''?'-dark':'')
                  : (this.system_theme==''?'-dark':'');
    this.system_theme = this.user_theme;
    this.setTheme();
  }

  private getTheme = (hour: number) => {
    return this.user_theme!==null
         ? this.user_theme
         : ((hour<9 || hour>=21)?'-dark':'');
  }

  private buildDialogs = (a : any, b : any) => {
    a.setDialog = (uniqueId, set, config) => {
      if (set === "open") {
        var div = b.createElement('div');
          div.className = 'dialog-box';
          div.id = uniqueId;
          div.innerHTML = '<div class="dialog-content">&nbsp;</div><h3 class="dialog-title"><a href="javascript:;" class="dialog-close" title="Close">&times;</a></h3>';
        b.body.appendChild(div);
      }

      var dialog = b.getElementById(uniqueId), selected = null, defaults = {
        title: '',
        content: '',
        width: 300,
        height: 150,
        top: false,
        left: false
      };

      for (var i in config) { defaults[i] = (typeof config[i] !== 'undefined') ? config[i] : defaults[i]; }

      function _drag_init(e, el) {
        for (var i=0;i<b.getElementsByClassName('dialog-box').length;i++) b.getElementsByClassName('dialog-box')[i].style.zIndex = 9999;
        el.style.zIndex = 10000;
        var posX = e.clientX,
            posY = e.clientY,
            divTop = parseFloat(el.style.top.indexOf('%')>-1?el.offsetTop + el.offsetHeight/2:el.style.top),
            divLeft = parseFloat(el.style.left.indexOf('%')>-1?el.offsetLeft + el.offsetWidth/2:el.style.left)
        var diffX = posX - divLeft,
            diffY = posY - divTop;
        b.onmousemove = function(e){
            e = e || a.event;
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
      dialog.style.opacity = (set === "open") ? 1 : 0;
      dialog.style.width = defaults.width + 'px';
      dialog.style.height = defaults.height + 'px';
      dialog.style.top = (!defaults.top) ? "50%" : '0px';
      dialog.style.left = (!defaults.left) ? "50%" : '0px';
      dialog.style.marginTop = (!defaults.top) ? '-' + defaults.height/2 + 'px' : defaults.top + 'px';
      dialog.style.marginLeft = (!defaults.left) ? '-' + defaults.width/2 + 'px' : defaults.left + 'px';
      dialog.children[1].innerHTML = defaults.title+dialog.children[1].innerHTML;
      dialog.children[0].innerHTML = defaults.content;
      dialog.children[1].onmousedown = function(e) { if ((e.target || e.srcElement)===this) _drag_init(e, this.parentNode); };
      dialog.children[1].children[0].onclick = function() { a.setDialog(uniqueId, "close", {content:""}); };
      b.onmouseup = function() { b.onmousemove = function() {}; };
      if (set === "close") dialog.remove();
    };
  }

  private onAdvert = (p : Models.ProductAdvertisement) => {
    this.ready = true;
    window.document.title = '['+p.environment+']';
    this.product = p;
    setTimeout(this.resizeMatryoshka, 5000);
    console.log("%cK started "+(new Date().toISOString().slice(11, -1))+"  %c"+this.homepage, "color:green;font-size:32px;", "color:red;font-size:16px;");
  }
}

@NgModule({
  imports: [
    BrowserModule,
    FormsModule,
    AgGridModule.withComponents([
      Shared.BaseCurrencyCellComponent,
      Shared.QuoteCurrencyCellComponent
    ]),
    HighchartsChartModule
  ],
  declarations: [
    ClientComponent,
    SubmitComponent,
    OrdersComponent,
    TradesComponent,
    WalletComponent,
    MarketComponent,
    SafetyComponent,
    TakersComponent,
    StateComponent,
    StatsComponent,
    SettingsComponent,
    Shared.BaseCurrencyCellComponent,
    Shared.QuoteCurrencyCellComponent
  ],
  bootstrap: [ClientComponent]
})
class ClientModule {}

enableProdMode();
platformBrowserDynamic().bootstrapModule(ClientModule);
