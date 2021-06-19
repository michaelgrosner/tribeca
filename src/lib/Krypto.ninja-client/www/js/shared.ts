import {Component} from '@angular/core';
import {ICellRendererParams} from '@ag-grid-community/core';
import {AgRendererComponent} from '@ag-grid-community/angular';

@Component({
    selector: 'base-currency-cell',
    template: `{{ (params.value||0).toFixed(productFixedSize) }}`
})
export class BaseCurrencyCellComponent implements AgRendererComponent {
  private params:ICellRendererParams;
  private productFixedSize:number = 8;

  agInit(params:ICellRendererParams):void {
    this.params = params;
    if ('productFixedSize' in params.node.data)
      this.productFixedSize = params.node.data.productFixedSize;
  }

  refresh(): boolean {
      return false;
  }
}

@Component({
    selector: 'quote-currency-cell',
    template: `{{ (params.value||0).toFixed(productFixedPrice) }} {{ quoteSymbol }}`
})
export class QuoteCurrencyCellComponent implements AgRendererComponent {
  private params:ICellRendererParams;
  private quoteSymbol:string = 'USD';
  private productFixedPrice:number = 8;

  agInit(params:ICellRendererParams):void {
    this.params = params;
    if ('quoteSymbol' in params.node.data)
      this.quoteSymbol = params.node.data.quoteSymbol.substr(0,3);
    if ('productFixedPrice' in params.node.data)
      this.productFixedPrice = params.node.data.productFixedPrice;
  }

  refresh(): boolean {
      return false;
  }
}
