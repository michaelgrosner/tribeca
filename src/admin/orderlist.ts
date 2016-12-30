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
    value: number;
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
        this.value = Math.round(osr.price * osr.quantity * 100) / 100;
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
        sortInfo: {fields: ['price'], directions: ['desc']},
        columnDefs: [
            { width: 140, field: 'time', displayName: 'time', cellFilter: "momentFullDate",
                sortingAlgorithm: (a: moment.Moment, b: moment.Moment) => a.diff(b),
                sort: { direction: uiGridConstants.DESC, priority: 2}, cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return Math.abs(moment.utc().valueOf() - row.entity.time.valueOf()) > 7000 ? "text-muted" : "";
            }  },
            { width: 90, field: 'orderId', displayName: 'id' },
            { width: 35, field: 'computationalLatency', displayName: 'lat'},
            { width: 35, field: 'version', displayName: 'v' },
            { width: 120, field: 'orderStatus', displayName: 'status', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return row.entity.orderStatus == "New" ? "text-muted" : "";
            }  },
            { width: 50, field: 'side', displayName: 'side' , cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (grid.getCellValue(row, col) === 'Bid') {
                    return 'buy';
                }
                else if (grid.getCellValue(row, col) === 'Ask') {
                    return "sell";
                }
            }},
            { width: 60, field: 'leavesQuantity', displayName: 'lvQty', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return (row.entity.side === 'Ask') ? "sell" : "buy";
            } },
            { width: 65, field: 'price', displayName: 'px',
            sort: { direction: uiGridConstants.DESC, priority: 1}, cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return (row.entity.side === 'Ask') ? "sell" : "buy";
            } },
            { width: 60, field: 'quantity', displayName: 'qty', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return (row.entity.side === 'Ask') ? "sell" : "buy";
            }},
            { width: 60, field: 'value', displayName: 'value', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return (row.entity.side === 'Ask') ? "sell" : "buy";
            }},
            { width: 50, field: 'orderType', displayName: 'type' },
            { width: 50, field: 'tif', displayName: 'tif' },
            { width: 60, field: 'lastQuantity', displayName: 'lQty' },
            { width: 65, field: 'lastPrice', displayName: 'lPx', cellFilter: 'currency' },
            { width: 60, field: 'cumQuantity', displayName: 'cum' },
            { width: 65, field: 'averagePrice', displayName: 'avg', cellFilter: 'currency' },
            { width: 40, field: 'liquidity', displayName: 'liq' },
            { width: "*", field: 'rejectMessage', displayName: 'msg' },
            { width: 40, name: "cancel", displayName: 'cxl', cellTemplate: '<button type="button" class="btn btn-danger btn-xs" ng-click="row.entity.cancel()"><span class="glyphicon glyphicon-remove"></span></button>' },
        ]
    };

    var addOrderRpt = (o: Models.OrderStatusReport) => {
        var idx = -1;
        for(var i=0;i<$scope.order_statuses.length;i++)
          if ($scope.order_statuses[i].orderId==o.orderId) {idx=i; break;}
        if (idx!=-1) {
            if (o.leavesQuantity) {
              var existing = $scope.order_statuses[idx];
              if (existing.version < o.version)
                  existing.updateWith(o);
            } else $scope.order_statuses.splice(idx,1);
        } else if (o.leavesQuantity || o.orderStatus == Models.OrderStatus.New)
          $scope.order_statuses.push(new DisplayOrderStatusReport(o, fireCxl));
    };

    var clear = () => {
        $scope.order_statuses.length = 0;
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.OrderStatusReports)
        .registerConnectHandler(clear)
        .registerDisconnectedHandler(clear)
        .registerSubscriber(addOrderRpt, os => os.forEach(addOrderRpt));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        // $log.info("destroy order list");
    });

    // $log.info("started order list");
};

var orderList = (): ng.IDirective => {
    var template = '<div><div ui-grid="gridOptions" ui-grid-grouping class="table table-striped table-hover table-condensed" style="height: 400px"></div></div>';

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
