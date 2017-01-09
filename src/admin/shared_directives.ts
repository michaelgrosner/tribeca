/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import {NgModule, Inject, Injectable, Component, ElementRef, Pipe, PipeTransform} from '@angular/core';
import moment = require('moment');
import * as io from 'socket.io-client';

import Messaging = require("../common/messaging");
import Models = require("../common/models");

// @Component({
    // selector: '[mypopover]',
    // template: `<form style="margin: 20px" class="form-horizontal" novalidate role="form">
        // <div class="form-group">
            // <label>Side</label>
            // <select class="form-control input-sm" ng-model="order.side" ng-options="x for x in order.availableSides"></select>
        // </div>
        // <div class="form-group">
            // <label>Price</label>
            // <input class="form-control input-sm" type="number" ng-model="order.price" />
        // </div>
        // <div class="form-group">
            // <label>Size</label>
            // <input class="form-control input-sm" type="number" ng-model="order.quantity" />
        // </div>
        // <div class="form-group">
            // <label>TIF</label>
            // <select class="form-control input-sm" ng-model="order.timeInForce" ng-options="x for x in order.availableTifs"></select>
        // </div>
        // <div class="form-group">
            // <label>Type</label>
            // <select class="form-control input-sm" ng-model="order.orderType" ng-options="x for x in order.availableOrderTypes"></select>
        // </div>
        // <button type="button"
                // class="btn btn-success"
                // onclick="jQuery('#order_form').popover('hide');"
                // ng-click="order.submit()">Submit</button>
    // </form>`
// })
// export class MypopoverComponent {
  // $compile : ng.ICompileService;
  // $templateCache : ng.ITemplateCacheService;
  // constructor() {
    // var getTemplate = (contentType, template_url) => {
        // var template = '';
        // switch (contentType) {
            // case 'user':
                // template = <any>this.$templateCache.get(template_url);
                // break;
        // }
        // return template;
    // };
    // return {
        // restrict: "A",
        // link: (scope, element, attrs) => {
            // var popOverContent = this.$compile("<div>" + getTemplate("user", attrs.popoverTemplate) + "</div>")(scope);
            // var options = {
                // content: popOverContent,
                // placement: attrs.dataPlacement,
                // html: true,
                // date: scope.date
            // };
            // (<any>jQuery(element)).popover(options).click((e) => {
                // e.preventDefault();
            // });
        // }
    // };
  // }
// }

@Pipe({
    name: 'bindOnce',
})
export class BindOncePipe {
  constructor(/*el:ElementRef*/) {
    return {
      scope: true,
      link: ($scope) => {
        setTimeout(() => {
          $scope.$destroy();
        }, 0);
      }
    }
  }
}

@Injectable()
export class FireFactory {
    constructor(private socket: SocketIOClient.Socket) {}

    public getFire = <T>(topic : string) : Messaging.IFire<T> => {
        return new Messaging.Fire<T>(topic, this.socket);
    }
}

@Injectable()
export class SubscriberFactory {
    socket: any;
    constructor(@Inject('socket') socket: any) {
      this.socket = socket;
    }

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
            x => this._scope.$evalAsync(() => incrementalHandler(x)),
            xs => this._scope.$evalAsync(() => snapshotHandler(xs)))
    };

    public registerDisconnectedHandler = (handler : () => void) => {
        return this._wrapped.registerDisconnectedHandler(() => this._scope.$evalAsync(handler));
    };

    public registerConnectHandler = (handler : () => void) => {
        return this._wrapped.registerConnectHandler(() => this._scope.$evalAsync(handler));
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

// export var sharedDirectives = "sharedDirectives";

// angular.module(sharedDirectives, ['ui.bootstrap'])
       // .directive('mypopover', ['$compile', '$templateCache', mypopover])
       // .directive('bindOnce', bindOnce)
       // .factory("socket", () : SocketIOClient.Socket => io())
       // .service("subscriberFactory", ['socket', '$log', SubscriberFactory])
       // .service("fireFactory", ['socket', '$log', FireFactory])
       // .filter("momentFullDatePipe", () => Models.toUtcFormattedTime)
       // .filter("momentShortDate", () => Models.toShortTimeString);
@NgModule({
    declarations: [
      // MypopoverComponent,
      BindOncePipe,
      MomentFullDatePipe,
      MomentShortDatePipe
    ],
    providers: [
      /*'ui.grid',*/
      // FireFactory,
      SubscriberFactory,
      ,{
        provide: 'socket',
        useValue: io()
      }
    ],
    exports: [
      // MypopoverComponent,
      BindOncePipe,
      MomentFullDatePipe,
      MomentShortDatePipe
    ]
})
export class SharedModule {}