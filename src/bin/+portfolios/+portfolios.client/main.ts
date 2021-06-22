import 'zone.js';

import {Component, OnInit} from '@angular/core';

import * as Models from 'lib/models';
import * as Socket from 'lib/socket';
import * as Shared from 'lib/shared';

import {WalletComponent} from './wallet';

@Component({
  selector: 'ui',
  template: `<div>
    <div [hidden]="state" style="padding:42px;transform:rotate(-6deg);">
        <h4 class="text-danger text-center">{{ product.environment ? product.environment+' is d' : 'D' }}isconnected.</h4>
    </div>
    <div [hidden]="!state">
        <div class="container-fluid">
            <div id="hud" [ngClass]="state.online ? 'bg-success' : 'bg-danger'">
                <div class="row">
                    <div class="col-md-12 col-xs-12">
                        <div class="row">
                          <wallet [asset]="asset"></wallet>
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

  private homepage: string = 'https://github.com/ctubio/Krypto-trading-bot';
  private server_memory: string;
  private client_memory: string;
  private user_theme: string = null;
  private system_theme: string = null;
  private state: Models.ConnectionStatus = <Models.ConnectionStatus>{};

  private openMatryoshka = () => {
    const url = window.prompt('Enter the URL of another instance:',this.product.matryoshka||'https://');
    document.getElementById('matryoshka').setAttribute('src', url||'about:blank');
    document.getElementById('matryoshka').style.height = (url&&url!='https://')?'589px':'0px';
  };

  private resizeMatryoshka = () => {
    if (window.parent === window) return;
    window.parent.postMessage('height='+document.getElementsByTagName('body')[0].getBoundingClientRect().height+'px', '*');
  };

  private product: Models.ProductAdvertisement = new Models.ProductAdvertisement(
    "", "", "", "", "", 0, "", "", "", "", 8, 8, 1e-8, 1e-8, 1e-8
  );

  private asset: any = null;

  ngOnInit() {
    new Socket.Client();

    new Socket.Subscriber(Models.Topics.Connectivity)
      .registerSubscriber((o: Models.ExchangeState) => { this.state = o; })
      .registerDisconnectedHandler(() => { this.state = null; });

    new Socket.Subscriber(Models.Topics.ProductAdvertisement)
      .registerSubscriber(this.onAdvert);

    new Socket.Subscriber(Models.Topics.Position)
      .registerSubscriber((o: any[]) => { this.asset = o; })
      .registerDisconnectedHandler(() => { this.asset = null; });

    new Socket.Subscriber(Models.Topics.ApplicationState)
      .registerSubscriber(this.onAppState);

    window.addEventListener("message", e => {
      if (!e.data.indexOf) return;

      if (e.data.indexOf('height=')===0) {
        document.getElementById('matryoshka').style.height = e.data.replace('height=','');
        this.resizeMatryoshka();
      }
    }, false);
  };

  private onAppState = (o : Models.ApplicationState) => {
    this.server_memory = Shared.bytesToSize(o.memory, 0);
    this.client_memory = Shared.bytesToSize((<any>window.performance).memory ? (<any>window.performance).memory.usedJSHeapSize : 1, 0);
    this.user_theme = this.user_theme!==null ? this.user_theme : (o.theme==1 ? '' : (o.theme==2 ? '-dark' : this.user_theme));
    this.system_theme = this.getTheme((new Date).getHours());
    this.setTheme();
  };

  private setTheme = () => {
    if (document.getElementById('daynight').getAttribute('href') != '/css/bootstrap-theme' + this.system_theme + '.min.css')
      document.getElementById('daynight').setAttribute('href', '/css/bootstrap-theme' + this.system_theme + '.min.css');
  };

  private changeTheme = () => {
    this.user_theme = this.user_theme!==null
                  ? (this.user_theme  ==''?'-dark':'')
                  : (this.system_theme==''?'-dark':'');
    this.system_theme = this.user_theme;
    this.setTheme();
  };

  private getTheme = (hour: number) => {
    return this.user_theme!==null
         ? this.user_theme
         : ((hour<9 || hour>=21)?'-dark':'');
  };

  private onAdvert = (o : Models.ProductAdvertisement) => {
    window.document.title = '[' + o.environment + ']';
    this.product = o;
    setTimeout(this.resizeMatryoshka, 5e+3);
    console.log(
      "%cK started " + (new Date().toISOString().slice(11, -1))+"  %c" + this.homepage,
      "color:green;font-size:32px;",
      "color:red;font-size:16px;"
    );
  };
};

Shared.bootstrapModule([
  ClientComponent,
  WalletComponent
]);
