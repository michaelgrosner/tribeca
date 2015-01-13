/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import moment = require("moment");
import Exchange = require("./exchange");
import OrderList = require("./orderlist");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

module Client {
    interface MainWindowScope extends ng.IScope {
        env : string;
        connected : boolean;
        order : Models.OrderRequestFromUI;

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

        private _fire : Messaging.IFire<Models.OrderRequestFromUI>;
        constructor(socket : SocketIOClientStatic) {
            this.availableExchanges = DisplayOrder.getNames(Models.Exchange);
            this.availableSides = DisplayOrder.getNames(Models.Side);
            this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
            this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);

            this._fire = new Messaging.Fire<Models.OrderRequestFromUI>(Messaging.Topics.SubmitNewOrder, socket);
        }

        public submit = () => {
            this._fire.fire(new Models.OrderRequestFromUI(this.exchange,
                this.side, this.price, this.quantity, this.timeInForce, this.orderType));
        };
    }

    var uiCtrl = ($scope : MainWindowScope, $timeout : ng.ITimeoutService, $log : ng.ILogService,
                  socket : SocketIOClientStatic, productListings : Shared.ProductListingRegistrar) => {
        $scope.connected = false;
        $scope.exchanges = [];
        $scope.order = new DisplayOrder(socket);

        var onConnect = () => {
            $scope.connected = true;
            $scope.exchanges = [];
        };

        var onAdvert = (pa : Models.ProductAdvertisement) => {
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
        };

        productListings.registerOnConnect(onConnect);
        productListings.registerOnAdvert(onAdvert);
        productListings.registerOnDisconnect(onDisconnect);

        var refresh_timer = () => {
            $timeout(refresh_timer, 250);
        };
        $timeout(refresh_timer, 250);

        $log.info("started client");
    };

    angular.module('projectApp', ['ui.bootstrap', 'orderListDirective', 'sharedDirectives'])
           .controller('uiCtrl', uiCtrl)
}