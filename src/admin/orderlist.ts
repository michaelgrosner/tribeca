/// <reference path="../common/models.ts" />
/// <reference path="shared_directives.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

interface OrderListScope extends ng.IScope {
    order_statuses: DisplayOrderStatusReport[];
    gridOptions: any;
}

class DisplayOrderStatusReport {
    orderId: string;
    time: moment.Moment;
    orderStatus: string;
    price: number;
    quantity: number;
    side: string;
    orderType: string;
    tif: string;
    computationalLatency: number;
    lastQuantity: number;
    lastPrice: number;
    leavesQuantity: number;
    cumQuantity: number;
    averagePrice: number;
    liquidity: string;
    rejectMessage: string;
    version: number;
    trackable: string;

    constructor(public osr: Models.OrderStatusReport,
        private _fireCxl: Messaging.IFire<Models.OrderStatusReport>) {
        this.orderId = osr.orderId;
        this.side = Models.Side[osr.side];
        this.updateWith(osr);
    }

    public updateWith = (osr: Models.OrderStatusReport) => {
        this.time = (moment.isMoment(osr.time) ? osr.time : moment(osr.time));
        this.orderStatus = DisplayOrderStatusReport.getOrderStatus(osr);
        this.price = osr.price;
        this.quantity = osr.quantity;
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
    };

    private static getOrderStatus(o: Models.OrderStatusReport): string {
        var endingModifier = (o: Models.OrderStatusReport) => {
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

    public cancel = () => {
        this._fireCxl.fire(this.osr);
    };
}

var OrderListController = ($scope: OrderListScope,
                           $log: ng.ILogService,
                           subscriberFactory: Shared.SubscriberFactory,
                           fireFactory: Shared.FireFactory,
                           uiGridConstants: any) => {
    var fireCxl = fireFactory.getFire(Messaging.Topics.CancelOrder);

    $scope.order_statuses = [];
    $scope.gridOptions = {
        data: 'order_statuses',
        primaryKey: 'orderId',
        groupsCollapsedByDefault: true,
        treeRowHeaderAlwaysVisible: false,
        enableColumnResize: true,
        rowHeight: 20,
        headerRowHeight: 20,
        columnDefs: [
            { width: 120, field: 'time', displayName: 'time', cellFilter: "momentFullDate", 
                sortingAlgorithm: Shared.fastDiff,
                sort: { direction: uiGridConstants.DESC, priority: 1} },
            { width: 90, field: 'orderId', displayName: 'id' },
            { width: 35, field: 'version', displayName: 'v' },
            { width: 120, field: 'orderStatus', displayName: 'status' },
            { width: 65, field: 'price', displayName: 'px' },
            { width: 60, field: 'quantity', displayName: 'qty' },
            { width: 50, field: 'side', displayName: 'side' },
            { width: 50, field: 'orderType', displayName: 'type' },
            { width: 50, field: 'tif', displayName: 'tif' },
            { width: 35, field: 'computationalLatency', displayName: 'lat' },
            { width: 60, field: 'lastQuantity', displayName: 'lQty' },
            { width: 65, field: 'lastPrice', displayName: 'lPx' },
            { width: 60, field: 'leavesQuantity', displayName: 'lvQty' },
            { width: 60, field: 'cumQuantity', displayName: 'cum' },
            { width: 65, field: 'averagePrice', displayName: 'avg' },
            { width: 40, field: 'liquidity', displayName: 'liq' },
            { width: "*", field: 'rejectMessage', displayName: 'msg' },
            { width: 40, name: "cancel", displayName: 'cxl', cellTemplate: '<button type="button" class="btn btn-danger btn-xs" ng-click="row.entity.cancel()"><span class="glyphicon glyphicon-remove"></span></button>' },
        ]
    };

    var idsToIndex = {};
    var addOrderRpt = (o: Models.OrderStatusReport) => {
        var idx = idsToIndex[o.orderId];
        if (typeof idx === "undefined") {
            idsToIndex[o.orderId] = $scope.order_statuses.length;
            $scope.order_statuses.push(new DisplayOrderStatusReport(o, fireCxl));
        }
        else {
            var existing = $scope.order_statuses[idx];
            if (existing.version < o.version) {
                existing.updateWith(o);
            }
        }
    };

    var clear = () => {
        $scope.order_statuses.length = 0;
        idsToIndex = {};
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.OrderStatusReports)
        .registerConnectHandler(clear)
        .registerSubscriber(addOrderRpt, os => os.forEach(addOrderRpt));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        $log.info("destroy order list");
    });

    $log.info("started order list");
};

var orderList = (): ng.IDirective => {
    var template = '<div><div ui-grid="gridOptions" ui-grid-grouping class="table table-striped table-hover table-condensed" style="height: 150px"></div></div>';

    return {
        template: template,
        restrict: "E",
        replace: true,
        transclude: false,
        controller: OrderListController
    }
};

export var orderListDirective = "orderListDirective";

angular.module(orderListDirective, ['ui.bootstrap', 'ui.grid', "ui.grid.grouping", Shared.sharedDirectives])
    .controller('OrderListController', OrderListController)
    .directive("orderList", orderList);
