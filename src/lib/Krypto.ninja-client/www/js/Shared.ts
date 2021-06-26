import {NgModule, Component, enableProdMode} from '@angular/core';
import {platformBrowserDynamic}              from '@angular/platform-browser-dynamic';
import {BrowserModule}                       from '@angular/platform-browser';
import {FormsModule}                         from '@angular/forms';

import * as Highcharts         from 'highcharts';
require('highcharts/highcharts-more')(Highcharts);
export {Highcharts};
import {HighchartsChartModule} from 'highcharts-angular';

import {ModuleRegistry, ICellRendererParams}       from '@ag-grid-community/core';
import {ClientSideRowModelModule, GridApi, ColDef} from '@ag-grid-community/all-modules';
import {AgGridModule, AgRendererComponent}         from '@ag-grid-community/angular';

export function bootstrapModule(declarations: any[]) {
  ModuleRegistry.registerModules([ClientSideRowModelModule]);

  @NgModule({
    imports: [
      BrowserModule,
      FormsModule,
      HighchartsChartModule,
      AgGridModule
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

function currencyHeaderTemplate(symbol: string) {
  return {
    template:`<div class="ag-cell-label-container" role="presentation">
      <span ref="eMenu" class="ag-header-icon ag-header-cell-menu-button"></span>
      <div ref="eLabel" class="ag-header-cell-label" role="presentation">
          <span ref="eText" class="ag-header-cell-text" role="columnheader"></span>
          <i class="beacon-sym-` + symbol.toLowerCase() + `-s"></i>
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

    colDef.map((x: ColDef)  => {
      if (['price', 'value', 'Kprice', 'Kvalue'].indexOf(x.field) > -1)
        x.headerComponentParams = currencyHeaderTemplate(quote);
      if (['quantity', 'Kqty'].indexOf(x.field) > -1)
        x.headerComponentParams = currencyHeaderTemplate(base);
      return x;
    });

    api.setColumnDefs(colDef);
};