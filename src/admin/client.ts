/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import moment = require("moment");
import Exchange = require("./exchange");
import OrderList = require("./orderlist");
import Messaging = require("../common/messaging");

module Client {
    interface MainWindowScope extends ng.IScope {
        env : string;
        connected : boolean;
        order : Models.OrderRequestFromUI;
        submitOrder : () => void;

        exchanges : Exchange.DisplayExchangeInformation[];
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
        //$scope.orderlist = new OrderList.OrderList($log, socket);
        $scope.exchanges = [];

        var onConnect = () => {
            $log.debug("onConnect");
            $scope.connected = true;
            $scope.exchanges = [];
            //$scope.orderlist = new OrderList.OrderList($log, socket);
        };

        var onAdvert = (pa : Models.ProductAdvertisement) => {
            $log.debug("onAdvert", pa);
            //$scope.orderlist.subscribe(pa);

            var getExchInfo = () : Exchange.DisplayExchangeInformation => {
                for (var i = 0; i < $scope.exchanges.length; i++) {
                    if ($scope.exchanges[i].exchange === pa.exchange)
                        return $scope.exchanges[i];
                }

                $log.info("adding new exchange", Models.Exchange[pa.exchange]);
                var exch = new Exchange.DisplayExchangeInformation($log, pa.exchange, socket);
                $scope.exchanges.push(exch);
                return exch;
            };

            getExchInfo().getOrAddDisplayPair(pa.pair);
        };

        var onDisconnect = () => {
            $scope.connected = false;
            $scope.exchanges.forEach(x => x.dispose());
            $scope.exchanges = [];
            //$scope.orderlist.dispose();
            //$scope.orderlist = null;
        };

        new Messaging.Subscriber<Models.ProductAdvertisement>(Messaging.Topics.ProductAdvertisement, socket, $log.info)
                .registerConnectHandler(onConnect)
                .registerSubscriber(onAdvert, a => a.forEach(onAdvert))
                .registerDisconnectedHandler(onDisconnect);

        var refresh_timer = () => {
            $timeout(refresh_timer, 250);
        };
        $timeout(refresh_timer, 250);

        $scope.order = new DisplayOrder();
        $scope.submitOrder = () => {
            var o = $scope.order;
            socket.emit("submit-order",
                new Models.OrderRequestFromUI(o.exchange, o.side, o.price, o.quantity, o.timeInForce, o.orderType));
        };

        $log.info("started client");
    };

    angular.module('projectApp', ['ui.bootstrap', 'orderListDirective', 'exchangesDirective', 'sharedDirectives'])
           .controller('uiCtrl', uiCtrl)
}