import {NgModule, Component, Injectable, Inject} from '@angular/core';
import {AgRendererComponent} from 'ag-grid-ng2/main';

import moment = require('moment');
import * as io from 'socket.io-client';

import Messaging = require("../common/messaging");
import Models = require("../common/models");

@Injectable()
export class FireFactory {
    constructor(@Inject('socket') private socket: SocketIOClient.Socket) {}

    public getFire = <T>(topic : string) : Messaging.IFire<T> => {
        return new Messaging.Fire<T>(topic, this.socket);
    }
}

@Injectable()
export class SubscriberFactory {
    constructor(@Inject('socket') private socket: SocketIOClient.Socket) {}

    public getSubscriber = <T>(scope: any, topic: string): Messaging.ISubscribe<T> => {
      return new EvalAsyncSubscriber<T>(scope, topic, this.socket);
    }
}

class EvalAsyncSubscriber<T> implements Messaging.ISubscribe<T> {
    private _wrapped: Messaging.ISubscribe<T>;

    constructor(private _scope: any, topic: string, io: any) {
      this._wrapped = new Messaging.Subscriber<T>(topic, io);
    }

    public registerSubscriber = (incrementalHandler: (msg: T) => void) => {
      return this._wrapped.registerSubscriber(x => this._scope.run(() => incrementalHandler(x)))
    };

    public registerDisconnectedHandler = (handler: () => void) => {
      return this._wrapped.registerDisconnectedHandler(() => this._scope.run(handler));
    };

    public registerConnectHandler = (handler: () => void) => {
      return this._wrapped.registerConnectHandler(() => this._scope.run(handler));
    };

    public disconnect = () => this._wrapped.disconnect();

    public get connected() { return this._wrapped.connected; }
}

@Component({
    selector: 'base-currency-cell',
    template: `{{ params.value | number:'1.3-3' }}`
})
export class BaseCurrencyCellComponent implements AgRendererComponent {
  private params:any;

  agInit(params:any):void {
    this.params = params;
  }
}

@Component({
    selector: 'quote-currency-cell',
    template: `{{ params.value | currency:quoteSymbol:true:'1.2-2' }}`
})
export class QuoteCurrencyCellComponent implements AgRendererComponent {
  private params:any;
  private quoteSymbol:string = 'USD';

  agInit(params:any):void {
    this.params = params;
    if ('quoteSymbol' in params.node.data)
      this.quoteSymbol = params.node.data.quoteSymbol;
  }
}

@NgModule({
  providers: [
    SubscriberFactory,
    FireFactory,
    {
      provide: 'socket',
      useValue: io()
    }
  ]
})
export class SharedModule {}