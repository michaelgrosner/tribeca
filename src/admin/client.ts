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
        current_result : string;
        order : Models.OrderRequestFromUI;
        cancel_replace_model : Models.ReplaceRequestFromUI;

        submitOrder : () => void;
        cancelReplaceOrder : (orig : Models.OrderStatusReport) => void;
        cancelOrder : (orig : Models.OrderStatusReport) => void;
        changeActive : () => void;
        getOrderStatus : (o : Models.OrderStatusReport) => string;
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
            if (position.currency == Models.Currency.BTC) {
                this.btcPosition = position.amount;
            }
            else if (position.currency == Models.Currency.USD) {
                this.usdPosition = position.amount;
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
            this.time = parsedTime.format('M/d/yy h:mm:ss,sss');
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

    var uiCtrl = ($scope : MainWindowScope, $timeout : ng.ITimeoutService, $log : ng.ILogService) => {
        $scope.connected = false;
        $scope.active = false;

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
            $scope.exchanges = {};
            $scope.orders_statuses = [];
            $log.info("connected");
        }).on("disconnect", () => {
            $scope.connected = false;
            $log.warn("disconnected");
        }).on("market-book", (b : Models.Market) => getOrAddDisplayExchange(b.exchange).updateMarket(b))
          .on('order-status-report', (o : Models.OrderStatusReport) => addOrderRpt(o))
          .on('order-status-report-snapshot', (os : Models.OrderStatusReport[]) => os.forEach(addOrderRpt))
          .on('active-changed', (b) => $scope.active = b )
          .on("result-change", (r : string) => $scope.current_result = r)
          .on("position-report", (rpt : Models.ExchangeCurrencyPosition) => getOrAddDisplayExchange(rpt.exchange).updatePosition(rpt))
          .on("connection-status", (exch : Models.Exchange, cs : Models.ConnectivityStatus) => getOrAddDisplayExchange(exch).setConnectStatus(cs) );

        var addOrderRpt = (o : Models.OrderStatusReport) => {
            $scope.orders_statuses.push(new DisplayOrderStatusReport(o, $scope));
        };

        $scope.order = {exchange: null, side: null, price: null, quantity: null, timeInForce: null, orderType: null};
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