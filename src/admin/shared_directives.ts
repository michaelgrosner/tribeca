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

export class ProductListingRegistrar {
    private products = [];
    private onConnectHandlers = [];
    private onAdvertHandlers = [];
    private onDisconnectHandlers = [];

    public onConnect = () => {
        this.products.length = 0;
        this.onConnectHandlers.forEach(h => h());
    };

    public onAdvert = (pa : Models.ProductAdvertisement) => {
        for (var i = 0; i < this.products.length; i++) {
            if (Models.productAdvertisementsEqual(this.products[i], pa)) return;
        }

        this.products.push(pa);
        this.onAdvertHandlers.forEach(h => h(pa));
    };

    public onDisconnect = () => {
        this.products.length = 0;
        this.onDisconnectHandlers.forEach(h => h());
    };

    public registerOnAdvert = (handler) => {
        this.products.forEach(handler);
        this.onAdvertHandlers.push(handler);
    };

    public registerOnConnect = (handler) => this.onConnectHandlers.push(handler);
    public registerOnDisconnect = (handler) => this.onDisconnectHandlers.push(handler);
}

var productListings = (socket : any, $log : ng.ILogService) : ProductListingRegistrar => {
    var handlers = new ProductListingRegistrar();

    new Messaging.Subscriber<Models.ProductAdvertisement>(Messaging.Topics.ProductAdvertisement, socket, $log.info)
        .registerConnectHandler(handlers.onConnect)
        .registerSubscriber(handlers.onAdvert, a => a.forEach(handlers.onAdvert))
        .registerDisconnectedHandler(handlers.onDisconnect);

    return handlers;
};

angular.module('sharedDirectives', ['ui.bootstrap'])
       .directive('mypopover', mypopover)
       .directive('bindOnce', bindOnce)
       .factory("socket", () => io)
       .factory("productListings", productListings);
