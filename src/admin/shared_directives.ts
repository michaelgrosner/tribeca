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

angular.module('sharedDirectives', ['ui.bootstrap'])
       .directive('mypopover', mypopover)
       .directive('bindOnce', bindOnce)
       .factory("socket", () => io.connect());
