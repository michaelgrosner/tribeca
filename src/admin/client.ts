/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");

module Client {
    interface MainWindowScope extends ng.IScope {
        env : string;
        connected : boolean;
        active : boolean;
        orders_statuses : DisplayOrderStatusReport[];
        exchanges : { [exchange: number]: DisplayExchangeInformation };
        current_result : DisplayResult;
        order : Models.OrderRequestFromUI;
        cancel_replace_model : Models.ReplaceRequestFromUI;

        submitOrder : () => void;
        cancelReplaceOrder : (orig : Models.OrderStatusReport) => void;
        cancelOrder : (orig : Models.OrderStatusReport) => void;
        changeActive : () => void;
        getOrderStatus : (o : Models.OrderStatusReport) => string;
    }

    class DisplayOrder implements Models.OrderRequestFromUI {
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

    class DisplayExchangeInformation {
        connected : boolean;
        name : string;
        bidSize : number;
        bid : number;
        askSize : number;
        ask : number;

        usdPosition : number = null;
        btcPosition : number = null;
        ltcPosition : number = null;
        usdValue : number = null;

        constructor(exchange : Models.Exchange) {
            this.name = Models.Exchange[exchange];
        }

        public setConnectStatus = (cs : Models.ConnectivityStatus) => {
            this.connected = cs == Models.ConnectivityStatus.Connected;
        };

        public updateMarket = (book : Models.Market) => {
            this.bidSize = book.update.bid.size;
            this.bid = book.update.bid.price;
            this.ask = book.update.ask.price;
            this.askSize = book.update.ask.size;
        };

        public updatePosition = (position : Models.ExchangeCurrencyPosition) => {
            switch (position.currency) {
                case Models.Currency.BTC:
                    this.btcPosition = position.amount;
                    break;
                case Models.Currency.USD:
                    this.usdPosition = position.amount;
                    break;
                case Models.Currency.LTC:
                    this.ltcPosition = position.amount;
                    break;
            }

            if (this.btcPosition != null && this.usdPosition != null) {
                this.usdValue = this.usdPosition + this.btcPosition * (this.bid + this.ask)/2.0;
            }
        };
    }

    class DisplayOrderStatusReport {
        orderId : string;
        time : string;
        timeSortable : Date;
        exchange : string;
        orderStatus : string;
        price : number;
        quantity : number;
        side : string;
        orderType : string;
        tif : string;
        computationalLatency : number;
        lastQuantity : number;
        lastPrice : number;
        leavesQuantity : number;
        cumQuantity : number;
        averagePrice : number;
        liquidity : string;
        rejectMessage : string;
        version : number;

        constructor(public osr : Models.OrderStatusReport, private $scope : MainWindowScope) {
            this.orderId = osr.orderId;
            var parsedTime = (moment.isMoment(osr.time) ? osr.time : moment(osr.time));
            this.time = Models.toUtcFormattedTime(parsedTime);
            this.timeSortable = parsedTime.toDate();
            this.exchange = Models.Exchange[osr.exchange];
            this.orderStatus = DisplayOrderStatusReport.getOrderStatus(osr);
            this.price = osr.price;
            this.quantity = osr.quantity;
            this.side = Models.Side[osr.side];
            this.orderType = Models.OrderType[osr.type];
            this.tif = Models.TimeInForce[osr.timeInForce];
            this.computationalLatency = osr.computationalLatency;
            this.lastQuantity = osr.lastQuantity;
            this.lastPrice = osr.lastPrice;
            this.leavesQuantity = osr.leavesQuantity;
            this.cumQuantity = osr.cumQuantity;
            this.averagePrice = osr.averagePrice;
            this.liquidity = Models.Liquidity[osr.liquidity];
            this.rejectMessage = osr.rejectMessage;
            this.version = osr.version;
        }

        public cancel = () => {
            this.$scope.cancelOrder(this.osr);
        };

        public cancelReplace = () => {
            this.$scope.cancelReplaceOrder(this.osr);
        };

        private static getOrderStatus(o : Models.OrderStatusReport) : string {
            var endingModifier = (o : Models.OrderStatusReport) => {
                if (o.pendingCancel)
                    return ", PndCxl";
                else if (o.pendingReplace)
                    return ", PndRpl";
                else if (o.partiallyFilled)
                    return ", PartFill";
                else if (o.cancelRejected)
                    return ", CxlRj";
                return "";
            };
            return Models.OrderStatus[o.orderStatus] + endingModifier(o);
        }
    }

    class DisplayResult {
        side : string;
        restExchange : string;
        hideExchange : string;
        restMarket : number;
        hideMarket : number;
        profit : number;
        size : number;
        time : string;
        hasResult : boolean;

        update = (wrappedMsg : Models.InactableResultMessage) => {
            if (wrappedMsg.isActive) {
                var res = wrappedMsg.msg;
                this.side = Models.Side[res.restSide];
                this.restExchange = Models.Exchange[res.restExchange];
                this.hideExchange = Models.Exchange[res.hideExchange];
                this.hideMarket = res.hideMkt.price;
                this.restMarket = res.restMkt.price;
                this.profit = res.profit;
                this.size = res.size;

                var parsedTime = (moment.isMoment(res.time) ? res.time : moment(res.time));
                this.time = Models.toUtcFormattedTime(parsedTime);
            }
            this.hasResult = wrappedMsg.isActive;
        }
    }

    var uiCtrl = ($scope : MainWindowScope, $timeout : ng.ITimeoutService, $log : ng.ILogService) => {
        $scope.connected = false;
        $scope.active = false;
        $scope.exchanges = {};
        $scope.orders_statuses = [];
        $scope.current_result = new DisplayResult();

        var refresh_timer = () => {
            $timeout(refresh_timer, 250);
        };
        $timeout(refresh_timer, 250);

        var getOrAddDisplayExchange = (exch : Models.Exchange) : DisplayExchangeInformation => {
            var disp = $scope.exchanges[exch];

            if (angular.isUndefined(disp)) {
                $scope.exchanges[exch] = new DisplayExchangeInformation(exch);
                disp = $scope.exchanges[exch];
            }

            return disp;
        };

        var socket = io();
        socket.on("hello", (env) => {
            $scope.env = env;
            $scope.connected = true;
            $log.info("connected");
        });

        socket.on("disconnect", () => {
            $scope.connected = false;
            $scope.exchanges = {};
            $scope.orders_statuses.length = 0;
            $scope.current_result.update(new Models.InactableResultMessage(false));
            $log.warn("disconnected");
        });

        socket.on("market-book", (b : Models.Market) =>
            getOrAddDisplayExchange(b.exchange).updateMarket(b));

        socket.on('order-status-report', (o : Models.OrderStatusReport) =>
            addOrderRpt(o));

        socket.on('order-status-report-snapshot', (os : Models.OrderStatusReport[]) =>
            os.forEach(addOrderRpt));

        socket.on('active-changed', b =>
            $scope.active = b );

        socket.on("result-change", (r : Models.InactableResultMessage) =>
            $scope.current_result.update(r));

        socket.on("position-report", (rpt : Models.ExchangeCurrencyPosition) =>
            getOrAddDisplayExchange(rpt.exchange).updatePosition(rpt));

        socket.on("connection-status", (exch : Models.Exchange, cs : Models.ConnectivityStatus) =>
            getOrAddDisplayExchange(exch).setConnectStatus(cs) );

        var addOrderRpt = (o : Models.OrderStatusReport) => {
            $scope.orders_statuses.push(new DisplayOrderStatusReport(o, $scope));
        };

        $scope.order = new DisplayOrder();
        $scope.submitOrder = () => {
            socket.emit("submit-order", $scope.order);
        };

        $scope.cancel_replace_model = {price: null, quantity: null};
        $scope.cancelReplaceOrder = (orig) => {
            socket.emit("cancel-replace", orig, $scope.cancel_replace_model);
        };

        $scope.cancelOrder = (order : Models.OrderStatusReport) => {
            $log.info("cancel-order", order);
            socket.emit("cancel-order", order);
        };

        $scope.changeActive = () => {
            socket.emit("active-change-request", !$scope.active);
        };

        $log.info("started");
    };

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

    angular.module('projectApp', ['ui.bootstrap'])
           .controller('uiCtrl', uiCtrl)
           .directive('mypopover', mypopover)
           .directive('bindOnce', bindOnce);
}