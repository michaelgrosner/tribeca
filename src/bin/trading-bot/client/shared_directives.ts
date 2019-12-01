import {NgModule, Component, Injectable} from '@angular/core';
import {AgRendererComponent} from '@ag-grid-community/angular';

import * as Subscribe from './subscribe';
import * as Models from './models';

@Injectable()
export class FireFactory {
    constructor() {}

    public getFire = <T>(topic : string) : Subscribe.IFire<T> => {
        return new Subscribe.Fire<T>(topic);
    }
}

@Injectable()
export class SubscriberFactory {
    constructor() {}

    public getSubscriber = <T>(scope: any, topic: string): Subscribe.ISubscribe<T> => {
      return new EvalAsyncSubscriber<T>(scope, topic);
    }
}

class EvalAsyncSubscriber<T> implements Subscribe.ISubscribe<T> {
    private _wrapped: Subscribe.ISubscribe<T>;

    constructor(private _scope: any, topic: string) {
      this._wrapped = new Subscribe.Subscriber<T>(topic);
    }

    public registerSubscriber = (incrementalHandler: (msg: T) => void) => {
      return this._wrapped.registerSubscriber(x => this._scope.run(() => incrementalHandler(x)))
    };

    public registerConnectHandler = (handler : () => void) => {
        return this._wrapped.registerConnectHandler(() => this._scope.run(handler));
    };

    public registerDisconnectedHandler = (handler: () => void) => {
      return this._wrapped.registerDisconnectedHandler(() => this._scope.run(handler));
    };

    public get connected() { return this._wrapped.connected; }
}

@Component({
    selector: 'base-currency-cell',
    template: `{{ (params.value||0).toFixed(productFixedSize) }}`
})
export class BaseCurrencyCellComponent implements AgRendererComponent {
  private params:any;
  private productFixedSize:number = 8;

  agInit(params:any):void {
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
  private params:any;
  private quoteSymbol:string = 'USD';
  private productFixedPrice:number = 8;

  agInit(params:any):void {
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

@NgModule({
  providers: [
    SubscriberFactory,
    FireFactory
  ]
})
export class SharedModule {}
