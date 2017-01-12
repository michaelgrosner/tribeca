/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import {NgModule, Inject, Injectable, Pipe, PipeTransform} from '@angular/core';
import moment = require('moment');
import * as io from 'socket.io-client';

import Messaging = require("../common/messaging");
import Models = require("../common/models");

// @Pipe({
    // name: 'bindOnce',
// })
// export class BindOncePipe {
  // constructor(/*el:ElementRef*/) {
    // return {
      // scope: true,
      // link: ($scope) => {
        // setTimeout(() => {
          // $scope.$destroy();
        // }, 0);
      // }
    // }
  // }
// }

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

    public getSubscriber = <T>(scope : any, topic : string) : Messaging.ISubscribe<T> => {
        return new EvalAsyncSubscriber<T>(scope, topic, this.socket);
    }
}

class EvalAsyncSubscriber<T> implements Messaging.ISubscribe<T> {
    private _wrapped : Messaging.ISubscribe<T>;

    constructor(private _scope : any, topic : string, io : any) {
        this._wrapped = new Messaging.Subscriber<T>(topic, io);
    }

    public registerSubscriber = (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => {
        return this._wrapped.registerSubscriber(
            x => this._scope.run(() => incrementalHandler(x)),
            xs => this._scope.run(() => snapshotHandler(xs)))
    };

    public registerDisconnectedHandler = (handler : () => void) => {
        return this._wrapped.registerDisconnectedHandler(() => this._scope.run(handler));
    };

    public registerConnectHandler = (handler : () => void) => {
        return this._wrapped.registerConnectHandler(() => this._scope.run(handler));
    };

    public disconnect = () => this._wrapped.disconnect();

    public get connected() { return this._wrapped.connected; }
}

@Pipe({
    name: 'momentFullDate',
})
export class MomentFullDatePipe implements PipeTransform {
  constructor() {}

  transform(value: moment.Moment, args: any[]): any {
    if (!value) return;
    return Models.toUtcFormattedTime(value);
  }
}

@Pipe({
    name: 'momentShortDate',
})
export class MomentShortDatePipe implements PipeTransform {
  constructor() {}

  transform(value: moment.Moment, args: any[]): any {
    if (!value) return;
    return Models.toShortTimeString(value);
  }
}

       // .directive('bindOnce', bindOnce)
       // .filter("momentFullDatePipe", () => Models.toUtcFormattedTime)
       // .filter("momentShortDate", () => Models.toShortTimeString);
@NgModule({
    declarations: [
      // BindOncePipe,
      MomentFullDatePipe,
      MomentShortDatePipe
    ],
    providers: [
      {
        provide: 'socket',
        useValue: io()
      },
      SubscriberFactory,
      FireFactory,
    ],
    exports: [
      // BindOncePipe,
      MomentFullDatePipe,
      MomentShortDatePipe
    ]
})
export class SharedModule {}