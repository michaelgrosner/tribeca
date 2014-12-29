/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

interface OrderListScope extends ng.IScope {
    order_statuses : DisplayOrderStatusReport[];
    cancel_replace_model : Models.ReplaceRequestFromUI;
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
    trackable : string;

    constructor(public osr : Models.OrderStatusReport,
                private _io : SocketIOClient.Socket,
                private _cxrRplModel : Models.ReplaceRequestFromUI) {
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
        this.trackable = osr.orderId + ":" + osr.version;
    }

    public cancel = () => {
        this._io.emit("cancel-order", this.osr);
    };

    public cancelReplace = () => {
        this._io.emit("cancel-replace", this.osr, this._cxrRplModel);
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

var OrderListController = ($scope : OrderListScope, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
    $scope.cancel_replace_model = {price: null, quantity: null};
    $scope.order_statuses = [];

    var addOrderRpt = (o : Models.OrderStatusReport) => {
        $scope.order_statuses.push(new DisplayOrderStatusReport(o, socket, $scope.cancel_replace_model));
    };

    socket.on("hello", () => {
        socket.emit("subscribe-order-status-report");
    });
    socket.emit("subscribe-order-status-report");

    socket.on('order-status-report', (o : Models.OrderStatusReport) =>
        addOrderRpt(o));

    socket.on('order-status-report-snapshot', (os : Models.OrderStatusReport[]) =>
        os.forEach(addOrderRpt));

    socket.on("disconnect", () => {
        $scope.order_statuses.length = 0;
    });

    $log.info("started orderlist");
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

var orderListDirective = () : ng.IDirective => {
    return {
        restrict: "E",
        templateUrl: "orderlist.html"
    }
};

angular.module('orderListDirective', ['ui.bootstrap'])
       .controller('OrderListController', OrderListController)
       .directive("orderList", orderListDirective)
       .directive('bindOnce', bindOnce);