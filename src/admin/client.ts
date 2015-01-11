/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

module Client {
    interface MainWindowScope extends ng.IScope {
        env : string;
        connected : boolean;
        order : Models.OrderRequestFromUI;
        submitOrder : () => void;
    }

    class DisplayOrder {
        exchange : string;
        side : string;
        price : number;
        quantity : number;
        timeInForce : string;
        orderType : string;

        availableExchanges : string[];
        availableSides : string[];
        availableTifs : string[];
        availableOrderTypes : string[];

        private static getNames<T>(enumObject : T) {
            var names : string[] = [];
            for (var mem in enumObject) {
                if (!enumObject.hasOwnProperty(mem)) continue;
                if (parseInt(mem, 10) >= 0) {
                  names.push(enumObject[mem]);
                }
            }
            return names;
        }

        constructor() {
            this.availableExchanges = DisplayOrder.getNames(Models.Exchange);
            this.availableSides = DisplayOrder.getNames(Models.Side);
            this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
            this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);
        }
    }

    var uiCtrl = ($scope : MainWindowScope, $timeout : ng.ITimeoutService, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
        $scope.connected = false;

        var refresh_timer = () => {
            $timeout(refresh_timer, 250);
        };
        $timeout(refresh_timer, 250);

        socket.on("hello", (env) => {
            $scope.env = env;
            $scope.connected = true;
            $log.info("connected");
        });

        socket.on("disconnect", () => {
            $scope.connected = false;
            $log.warn("disconnected");
        });

        $scope.order = new DisplayOrder();
        $scope.submitOrder = () => {
            var o = $scope.order;
            socket.emit("submit-order",
                new Models.OrderRequestFromUI(o.exchange, o.side, o.price, o.quantity, o.timeInForce, o.orderType));
        };

        $log.info("started client");
    };

    angular.module('projectApp', ['ui.bootstrap', 'orderListDirective', 'exchangesDirective', 'sharedDirectives'])
           .factory("socket", () => io.connect())
           .controller('uiCtrl', uiCtrl)
}