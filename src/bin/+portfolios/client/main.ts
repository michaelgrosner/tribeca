import 'zone.js';

import {NgModule, NgZone, Component, Inject, OnInit, enableProdMode} from '@angular/core';
import {platformBrowserDynamic} from '@angular/platform-browser-dynamic';
import {FormsModule} from '@angular/forms';
import {BrowserModule} from '@angular/platform-browser';
import {AgGridModule} from '@ag-grid-community/angular';

import * as Models from '../../../www/ts/models';
import * as Socket from '../../../www/ts/socket';
import * as Shared from '../../../www/ts/shared';

import {AssetsComponent} from './assets';

@Component({
  selector: 'ui',
  template: `<div>
    <div [hidden]="ready" style="padding:42px;transform:rotate(-6deg);">
        <h4 class="text-danger text-center">{{ product.environment ? product.environment+' is d' : 'D' }}isconnected.</h4>
    </div>
    <div [hidden]="!ready">
        <div class="container-fluid">
            <div id="hud">
                <div *ngIf="ready" class="row">
                    <div class="col-md-12 col-xs-12">
                        <div class="row">
                          <assets [setAsset]="Asset"></assets>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <address class="text-center">
      <small>
        <a rel="noreferrer" href="{{ homepage }}/blob/master/README.md" target="_blank">README</a> - <a rel="noreferrer" href="{{ homepage }}" target="_blank">SOURCE</a> - <span title="Server used RAM" style="margin-top: 6px;display: inline-block;">{{ server_memory }}</span> - <span title="Client used RAM" style="margin-top: 6px;display: inline-block;">{{ client_memory }}</span> - <a href="#" (click)="changeTheme()">{{ system_theme ? 'Light' : 'Dark' }}</a> - <a href="#" (click)="openMatryoshka()">MATRYOSHKA</a> - <a rel="noreferrer" href="{{ homepage }}/issues/new?title=%5Btopic%5D%20short%20and%20sweet%20description&body=description%0Aplease,%20consider%20to%20add%20all%20possible%20details%20%28if%20any%29%20about%20your%20new%20feature%20request%20or%20bug%20report%0A%0A%2D%2D%2D%0A%60%60%60%0Aapp%20exchange%3A%20{{ product.exchange }}%0Aapp%20version%3A%20undisclosed%0AOS%20distro%3A%20undisclosed%0A%60%60%60%0A![300px-spock_vulcan-salute3](https://cloud.githubusercontent.com/assets/1634027/22077151/4110e73e-ddb3-11e6-9d84-358e9f133d34.png)" target="_blank">CREATE ISSUE</a> - <a rel="noreferrer" href="https://github.com/ctubio/Krypto-trading-bot/discussions/new" target="_blank">HELP</a> - <a title="irc://irc.freenode.net:6697/#tradingBot" href="irc://irc.freenode.net:6697/#tradingBot">IRC</a>|<a target="_blank" rel="noreferrer" href="https://kiwiirc.com/client/irc.freenode.net:6697/?theme=cli#tradingBot" rel="nofollow">www</a>
      </small>
    </address>
    <iframe id="matryoshka" style="margin:0px;padding:0px;border:0px;width:100%;height:0px;" src="about:blank"></iframe>
  </div>`
})
class ClientComponent implements OnInit {

  public homepage: string = "https://github.com/ctubio/Krypto-trading-bot";
  public server_memory: string;
  public client_memory: string;
  public ready: boolean;
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

  private user_theme: string = null;
  private system_theme: string = null;

  public Asset: any = null;

  constructor(
    @Inject(NgZone) private zone: NgZone,
    @Inject(Socket.SubscriberFactory) private subscriberFactory: Socket.SubscriberFactory,
    @Inject(Socket.FireFactory) private fireFactory: Socket.FireFactory
  ) {}

  ngOnInit() {
    new Socket.Client();

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.ProductAdvertisement)
      .registerSubscriber(this.onAdvert)
      .registerDisconnectedHandler(() => { this.ready = false; });

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.Position)
      .registerSubscriber((o: any[]) => { this.Asset = o; })
      .registerDisconnectedHandler(() => { this.Asset = null; });

    window.addEventListener("message", e => {
      if (!e.data.indexOf) return;

      if (e.data.indexOf('height=')===0) {
        document.getElementById('matryoshka').style.height = e.data.replace('height=','');
        this.resizeMatryoshka();
      }
    }, false);

    this.ready = true;
    // this.ready = false;

    this.subscriberFactory
      .getSubscriber(this.zone, Models.Topics.ApplicationState)
      .registerSubscriber(this.onAppState);
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
    this.user_theme = this.user_theme!==null ? this.user_theme : (o.theme==1 ? '' : (o.theme==2 ? '-dark' : this.user_theme));
    this.system_theme = this.getTheme((new Date).getHours());
    this.setTheme();
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
    Socket.DataModule,
    BrowserModule,
    FormsModule,
    AgGridModule.withComponents([
      Shared.BaseCurrencyCellComponent,
      Shared.QuoteCurrencyCellComponent
    ])
  ],
  declarations: [
    ClientComponent,
    AssetsComponent,
    Shared.BaseCurrencyCellComponent,
    Shared.QuoteCurrencyCellComponent
  ],
  bootstrap: [ClientComponent]
})
class ClientModule {}

enableProdMode();
platformBrowserDynamic().bootstrapModule(ClientModule);
