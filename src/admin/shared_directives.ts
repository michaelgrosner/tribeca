/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Messaging = require("../common/messaging");
import Models = require("../common/models");
import io = require("socket.io-client");

var mypopover = ($compile : ng.ICompileService, $templateCache : ng.ITemplateCacheService) => {
    var getTemplate = (contentType, template_url) => {
        var template = '';
        switch (contentType) {
            case 'user':
                template = $templateCache.get(template_url);
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
            $(element).popover(options).click((e) => {
                e.preventDefault();
            });
        }
    };
};

var bindOnce = () => {
    return {
        scope: true,
        link: ($scope) => {
            setTimeout(() => {
                $scope.$destroy();
            }, 0);
        }
    }
};

export class EvalAsyncSubscriber<T> implements Messaging.ISubscribe<T> {
    private _wrapped : Messaging.ISubscribe<T>;

    constructor(private _scope : ng.IScope, topic : string, io : any, log : (...args: any[]) => void) {
        this._wrapped = new Messaging.Subscriber<T>(topic, io, log);
    }

    public registerSubscriber = (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => {
        return this._wrapped.registerSubscriber(
            x => this._scope.$evalAsync(() => incrementalHandler(x)), 
            xs => this._scope.$evalAsync(() => snapshotHandler(xs)))
    }

    public registerDisconnectedHandler = (handler : () => void) => {
        return this._wrapped.registerDisconnectedHandler(() => this._scope.$evalAsync(handler));
    }

    public registerConnectHandler = (handler : () => void) => { 
        return this._wrapped.registerConnectHandler(() => this._scope.$evalAsync(handler));
    }

    public disconnect = () => this._wrapped.disconnect();
}

angular.module('sharedDirectives', ['ui.bootstrap'])
       .directive('mypopover', mypopover)
       .directive('bindOnce', bindOnce)
       .factory("socket", () => io)
       .filter("momentFullDate", () => Models.toUtcFormattedTime)
       .filter("momentShortDate", () => Models.toShortTimeString);
