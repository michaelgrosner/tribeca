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

interface IProductsService {
    advertisements : Models.ProductAdvertisement[];
    addCallback : any;
}

var productListingService = ($rootScope : ng.IRootScopeService, socket : SocketIOClient.Socket, $log : ng.ILogService) : IProductsService => {
    var cbs = [];
    var data = {advertisements: [], addCallback: a => cbs.push(a)};

    var addAdvert = (a : Models.ProductAdvertisement) => {
        $log.info("Adding product advertisement", Models.Exchange[a.exchange], Models.Currency[a.pair.base] + "/" + Models.Currency[a.pair.quote]);

        data.advertisements.push(a);
        cbs.forEach(cb => cb(a));
    };

    new Messaging.Subscriber<Models.ProductAdvertisement>(Messaging.Topics.ProductAdvertisement, socket, $log.info)
        .registerSubscriber(addAdvert, as => as.forEach(addAdvert));

    return data;
};

angular.module('sharedDirectives', ['ui.bootstrap'])
       .directive('mypopover', mypopover)
       .directive('bindOnce', bindOnce)
       .factory("socket", () => io.connect())
       .factory("productListings", productListingService);
