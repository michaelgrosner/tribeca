import {NgModule, Component, enableProdMode} from '@angular/core';
import {platformBrowserDynamic}              from '@angular/platform-browser-dynamic';
import {BrowserModule}                       from '@angular/platform-browser';
import {FormsModule}                         from '@angular/forms';

import {HighchartsChartModule} from 'highcharts-angular';

import {ModuleRegistry, ICellRendererParams} from '@ag-grid-community/core';
import {ClientSideRowModelModule}            from '@ag-grid-community/all-modules';
import {AgGridModule, AgRendererComponent}   from '@ag-grid-community/angular';

export function bootstrapModule(declarations: any[]) {
  ModuleRegistry.registerModules([ClientSideRowModelModule]);
  declarations.push(BaseCurrencyCellComponent);
  declarations.push(QuoteCurrencyCellComponent);

  @NgModule({
    imports: [
      BrowserModule,
      FormsModule,
      HighchartsChartModule,
      AgGridModule.withComponents([
        BaseCurrencyCellComponent,
        QuoteCurrencyCellComponent
      ])
    ],
    declarations: declarations,
    bootstrap: [declarations[0]]
  })
  class ClientModule {};

  enableProdMode();
  platformBrowserDynamic().bootstrapModule(ClientModule);
};

export function bytesToSize(input: number, precision: number) {
  if (!input) return '0B';
  let unit: string[] = ['', 'K', 'M', 'G', 'T', 'P'];
  let index: number = Math.floor(Math.log(input) / Math.log(1024));
  if (index >= unit.length) return input + 'B';
  return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B';
};

export function playAudio(basename: string) {
  let audio = new Audio('audio/' + basename + '.mp3');
  audio.volume = 0.5;
  audio.play();
};

@Component({
    selector: 'base-currency-cell',
    template: `{{ (params.value||0).toFixed(productFixedSize) }}`
})
export class BaseCurrencyCellComponent implements AgRendererComponent {
  private params: ICellRendererParams;
  private productFixedSize: number = 8;

  agInit(params: ICellRendererParams): void {
    this.params = params;
    if ('productFixedSize' in params.node.data)
      this.productFixedSize = params.node.data.productFixedSize;
  }

  refresh(): boolean { return false; }
};

@Component({
    selector: 'quote-currency-cell',
    template: `{{ (params.value||0).toFixed(productFixedPrice) }} {{ quoteSymbol }}`
})
export class QuoteCurrencyCellComponent implements AgRendererComponent {
  private params: ICellRendererParams;
  private quoteSymbol: string = 'USD';
  private productFixedPrice: number = 8;

  agInit(params: ICellRendererParams): void {
    this.params = params;
    if ('quoteSymbol' in params.node.data)
      this.quoteSymbol = params.node.data.quoteSymbol.substr(0,3);
    if ('productFixedPrice' in params.node.data)
      this.productFixedPrice = params.node.data.productFixedPrice;
  }

  refresh(): boolean { return false; }
};
