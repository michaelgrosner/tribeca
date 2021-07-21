import {NgModule, Component, enableProdMode, OnInit} from '@angular/core';
import {platformBrowserDynamic}                      from '@angular/platform-browser-dynamic';
import {BrowserModule}                               from '@angular/platform-browser';
import {FormsModule}                                 from '@angular/forms';

import * as Highcharts         from 'highcharts';
require('highcharts/highcharts-more')(Highcharts);
export {Highcharts};
import {HighchartsChartModule} from 'highcharts-angular';

import {ModuleRegistry, ICellRendererParams}                from '@ag-grid-community/core';
import {ClientSideRowModelModule, GridApi, ColDef, RowNode} from '@ag-grid-community/all-modules';
import {AgGridModule}                                       from '@ag-grid-community/angular';

import {Socket, Models} from 'lib/K';

@Component({
  selector: 'K',
  template: `<div>
    <div [hidden]="state.online !== null"
      style="padding:42px;transform:rotate(-6deg);">
      <h4 class="text-danger text-center">
        <i class="beacon exc-{{ exchange_icon }}-s" style="font-size:30px;"></i>
        <br /><br />
        {{ product.environment ? product.environment+' is d' : 'D' }}isconnected
      </h4>
    </div>
    <div [hidden]="state.online === null">
      <div class="container-fluid">
          <div id="hud"
            [ngClass]="state.online ? 'bg-success' : 'bg-danger'">
            <client
              [state]="state"
              [product]="product"
              [addr]="addr"
              [tradeFreq]="tradeFreq"
              (onFooter)="onFooter($event)"></client>
          </div>
      </div>
    </div>
    <address>
      <small>
        <a rel="noreferrer" target="_blank"
          href="{{ homepage }}/blob/master/README.md">README</a> -
        <a rel="noreferrer" target="_blank"
          href="{{ homepage }}/blob/master/doc/MANUAL.md">MANUAL</a> -
        <a rel="noreferrer" target="_blank"
          href="{{ homepage }}">SOURCE</a> -
        <span [hidden]="state.online === null">
          <span [hidden]="!product.inet">
            <span title="non-default Network Interface for outgoing traffic">{{ product.inet }}</span> -
          </span>
          <span title="Server used RAM"
            style="margin-top: 6px;display: inline-block;">{{ server_memory }}</span> -
          <span title="Client used RAM"
            style="margin-top: 6px;display: inline-block;">{{ client_memory }}</span> -
          <span title="Database Size"
            style="margin-top: 6px;display: inline-block;">{{ db_size }}</span> -
          <span [innerHTML]="footer"></span>
          <a href="#"
            (click)="changeTheme()">{{ system_theme ? 'LIGHT' : 'DARK' }}</a> -
        </span>
        <a href="#"
          (click)="openMatryoshka()">MATRYOSHKA</a> -
        <a rel="noreferrer" target="_blank"
          href="{{ homepage }}/issues/new?title=%5Btopic%5D%20short%20and%20sweet%20description&body=description%0Aplease,%20consider%20to%20add%20all%20possible%20details%20%28if%20any%29%20about%20your%20new%20feature%20request%20or%20bug%20report%0A%0A%2D%2D%2D%0A%60%60%60%0Aapp%20exchange%3A%20{{ product.exchange }}/{{ product.base+'/'+product.quote }}%0Aapp%20version%3A%20undisclosed%0AOS%20distro%3A%20undisclosed%0A%60%60%60%0A![300px-spock_vulcan-salute3](https://cloud.githubusercontent.com/assets/1634027/22077151/4110e73e-ddb3-11e6-9d84-358e9f133d34.png)">CREATE ISSUE</a> -
        <a rel="noreferrer" target="_blank"
          href="https://github.com/ctubio/Krypto-trading-bot/discussions/new">HELP</a> -
        <a title="irc://irc.freenode.net:6697/#tradingBot"
          href="irc://irc.freenode.net:6697/#tradingBot"
        >IRC</a>|<a rel="noreferrer" target="_blank"
          href="https://kiwiirc.com/client/irc.freenode.net:6697/?theme=cli#tradingBot"
        >www</a>
      </small>
    </address>
    <iframe
      (window:resize)="resizeMatryoshka()"
      id="matryoshka"
      src="about:blank"></iframe>
  </div>`
})
export class KComponent implements OnInit {

  private homepage: string = 'https://github.com/ctubio/Krypto-trading-bot';
  private exchange_icon: string;
  private tradeFreq: number = 0;
  private addr: string;
  private footer: string;

  private server_memory: string = '0KB';
  private client_memory: string = '0KB';
  private db_size: string = '0KB';

  private user_theme: string = null;
  private system_theme: string = null;

  private state: Models.ExchangeState = new Models.ExchangeState();
  private product: Models.ProductAdvertisement = new Models.ProductAdvertisement();

  ngOnInit() {
    new Socket.Client();

    new Socket.Subscriber(Models.Topics.ProductAdvertisement)
      .registerSubscriber(this.onProduct);

    new Socket.Subscriber(Models.Topics.Connectivity)
      .registerSubscriber((o: Models.ExchangeState) => { this.state = o; })
      .registerDisconnectedHandler(() => { this.state.online = null; });

    new Socket.Subscriber(Models.Topics.ApplicationState)
      .registerSubscriber(this.onAppState);

    window.addEventListener("message", e => {
      if (!e.data.indexOf) return;

      if (e.data.indexOf('height=') === 0) {
        document.getElementById('matryoshka').style.height = e.data.replace('height=', '');
        this.resizeMatryoshka();
      }
    }, false);
  };

  private onFooter(o: string) {
    this.footer = o;
  };

  private openMatryoshka = () => {
    const url = window.prompt('Enter the URL of another instance:', this.product.matryoshka || 'https://');
    document.getElementById('matryoshka').setAttribute('src', url || 'about:blank');
    document.getElementById('matryoshka').style.height = (url && url != 'https://') ? '589px' : '0px';
  };

  private resizeMatryoshka = () => {
    if (window.parent === window) return;
    window.parent.postMessage('height=' + document.getElementsByTagName('body')[0].getBoundingClientRect().height + 'px', '*');
  };

  private setTheme = () => {
    if (document.getElementById('daynight').getAttribute('href') != '/css/bootstrap-theme' + this.system_theme + '.min.css')
      document.getElementById('daynight').setAttribute('href', '/css/bootstrap-theme' + this.system_theme + '.min.css');
  };

  private changeTheme = () => {
    this.user_theme = this.user_theme!==null
                  ? (this.user_theme  == '' ? '-dark' : '')
                  : (this.system_theme== '' ? '-dark' : '');
    this.system_theme = this.user_theme;
    this.setTheme();
  };

  private getTheme = (hour: number) => {
    return this.user_theme!==null
         ? this.user_theme
         : ((hour<9 || hour>=21)?'-dark':'');
  };

  private onAppState = (o : Models.ApplicationState) => {
    this.server_memory = bytesToSize(o.memory, 0);
    this.client_memory = bytesToSize((<any>window.performance).memory ? (<any>window.performance).memory.usedJSHeapSize : 1, 0);
    this.db_size = bytesToSize(o.dbsize, 0);
    this.tradeFreq = (o.freq);
    this.user_theme = this.user_theme!==null ? this.user_theme : (o.theme==1 ? '' : (o.theme==2 ? '-dark' : this.user_theme));
    this.system_theme = this.getTheme((new Date).getHours());
    this.setTheme();
    this.addr = o.addr;
  }

  private onProduct = (o : Models.ProductAdvertisement) => {
    if (this.product.source && this.product.source != o.source)
      window.location.reload();
    window.document.title = '[' + o.environment + ']';
    this.exchange_icon = o.exchange.toLowerCase().replace('coinbase', 'coinbase-pro');
    this.product = o;
    setTimeout(() => {window.dispatchEvent(new Event('resize'))}, 0);
    console.log(
      "%c" + this.product.source + " started " + (new Date().toISOString().slice(11, -1)) + "  %c" + this.homepage,
      "color:green;font-size:32px;",
      "color:red;font-size:16px;"
    );
  };
};

export function bootstrapModule(declarations: any[]) {
  ModuleRegistry.registerModules([ClientSideRowModelModule]);

  @NgModule({
    imports: [
      BrowserModule,
      FormsModule,
      HighchartsChartModule,
      AgGridModule
    ],
    declarations: [KComponent].concat(declarations),
    bootstrap: [KComponent]
  })
  class KModule {};

  enableProdMode();
  platformBrowserDynamic().bootstrapModule(KModule);
};

export function bytesToSize(input: number, precision: number) {
  if (!input) return '0KB';
  let unit: string[] = ['', 'K', 'M', 'G', 'T', 'P'];
  let index: number = Math.floor(Math.log(input) / Math.log(1024));
  return index >= unit.length
    ? input + 'B'
    : (input / Math.pow(1024, index))
        .toFixed(precision) + unit[index] + 'B';
};

export function playAudio(basename: string) {
  let audio = new Audio('audio/' + basename + '.mp3');
  audio.volume = 0.5;
  audio.play();
};

function currencyHeaderTemplate(symbol: string) {
  return {
    template:`<div class="ag-cell-label-container" role="presentation">
      <span ref="eMenu" class="ag-header-icon ag-header-cell-menu-button"></span>
      <div ref="eLabel" class="ag-header-cell-label" role="presentation">
          <span>
            <span ref="eText" class="ag-header-cell-text" role="columnheader"></span>
            <i class="beacon sym-` + symbol.toLowerCase() + `-s"></i>
          </span>
          <span ref="eFilter" class="ag-header-icon ag-filter-icon"></span>
          <span ref="eSortOrder" class="ag-header-icon ag-sort-order"></span>
          <span ref="eSortAsc" class="ag-header-icon ag-sort-ascending-icon"></span>
          <span ref="eSortDesc" class="ag-header-icon ag-sort-descending-icon"></span>
          <span ref="eSortNone" class="ag-header-icon ag-sort-none-icon"></span>
      </div>
    </div>`
  };
};

export function currencyHeaders(api: GridApi, base: string, quote: string) {
    if (!api) return;

    let colDef: ColDef[] = api.getColumnDefs();

    colDef.map((o: ColDef)  => {
      if (['price', 'value',   'Kprice', 'Kvalue',
           'delta', 'balance', 'spread', 'volume'].indexOf(o.field) > -1)
        o.headerComponentParams = currencyHeaderTemplate(quote);
      else if (['quantity', 'Kqty'].indexOf(o.field) > -1)
        o.headerComponentParams = currencyHeaderTemplate(base);
      return o;
    });

    api.setColumnDefs(colDef);
};

export function resetRowData(name: string, val: string, node: RowNode): string[] {
  var dir   = '';
  var _val  = num(val);
  var _data = num(node.data[name]);
  if      (_val > _data) dir = 'up-data';
  else if (_val < _data) dir = 'down-data';
  if (dir != '') {
    node.data['dir_' + name] = dir;
    node.setDataValue(name, val);
  }
  return dir != '' ? [name] : [];
};

export function str(val: number, decimals: number): string {
  return val.toLocaleString([].concat(navigator.languages), {
    minimumFractionDigits: decimals,
    maximumFractionDigits: decimals
  });
};

var exp = null;
export function num(val: string): number {
  if (exp) return parseFloat(val.replace(exp, ''));
  exp = new RegExp('[^\\d-' + str(0, 1).replace(/0/g, '') + ']', 'g');
  return num(val);
};

export function comparator(valueA, valueB, nodeA, nodeB, isInverted) {
  return num(valueA) - num(valueB);
};
