/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import {Injectable, Directive, ElementRef, Pipe, PipeTransform} from '@angular/core';
import moment = require('moment');

import Messaging = require("../common/messaging");
import Models = require("../common/models");

@Directive({})
export class Mypopover {
  constructor($compile : ng.ICompileService, $templateCache : ng.ITemplateCacheService) {
    var getTemplate = (contentType, template_url) => {
        var template = '';
        switch (contentType) {
            case 'user':
                template = <any>$templateCache.get(template_url);
                break;
        }
        return template;
    };
    return {
        restrict: "A",
        link: (scope, element, attrs) => {
            var popOverContent = $compile("<div>" + getTemplate("user", attrs.popoverTemplate) + "</div>")(scope);
            var options = {
                content: popOverContent,
                placement: attrs.dataPlacement,
                html: true,
                date: scope.date
            };
            (<any>jQuery(element)).popover(options).click((e) => {
                e.preventDefault();
            });
        }
    };
  }
}

@Directive({})
export class BindOnceDirective {
  constructor(el: ElementRef) {
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
    constructor(private socket : any, private $log : ng.ILogService) {}

    public getFire = <T>(topic : string) : Messaging.IFire<T> => {
        return new Messaging.Fire<T>(topic, this.socket, this.$log.info);
    }
}

@Injectable()
export class SubscriberFactory {
    constructor(private socket : any, private $log : ng.ILogService) {}

    public getSubscriber = <T>(scope : ng.IScope, topic : string) : Messaging.ISubscribe<T> => {
        return new EvalAsyncSubscriber<T>(scope, topic, this.socket, this.$log.info);
    }
}

class EvalAsyncSubscriber<T> implements Messaging.ISubscribe<T> {
    private _wrapped : Messaging.ISubscribe<T>;

    constructor(private _scope : ng.IScope, topic : string, io : any, log : (...args: any[]) => void) {
        this._wrapped = new Messaging.Subscriber<T>(topic, io, log);
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
  constructor() { }

  transform(value: moment.Moment, args: any[]): any {
    if (!value) return;
    return Models.toUtcFormattedTime(value);
  }
}

@Pipe({
    name: 'momentShortDate',
})
export class MomentShortDatePipe implements PipeTransform {
  constructor() { }

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
