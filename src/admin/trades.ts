/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

interface TradesScope extends ng.IScope {
    trade_statuses : DisplayTrade[];
    exch : Models.Exchange;
    pair : Models.CurrencyPair;
    gridOptions : any;
}

class DisplayTrade {
    tradeId : string;
    time : moment.Moment;
    price : number;
    quantity : number;
    side : string;
    value : number;

    constructor(public trade : Models.Trade) {
        this.tradeId = trade.tradeId;
        this.side = Models.Side[trade.side];
        this.time = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));
        this.price = trade.price;
        this.quantity = trade.quantity;
        this.value = trade.value;
    }
}

var TradesListController = ($scope : TradesScope, $log : ng.ILogService, subscriberFactory : Shared.SubscriberFactory) => {
    $scope.trade_statuses = [];
    $scope.gridOptions = {
        data: 'trade_statuses',
        showGroupPanel: false,
        primaryKey: 'tradeId',
        groupsCollapsedByDefault: true,
        enableColumnResize: true,
        sortInfo: {fields: ['time'], directions: ['desc']},
        rowHeight: 20,
        headerRowHeight: 20,
        columnDefs: [
            {width: 120, field:'time', displayName:'t', cellFilter: 'momentFullDate'},
            {width: 55, field:'price', displayName:'px', cellFilter: 'currency'},
            {width: 50, field:'quantity', displayName:'qty'},
            {width: 35, field:'side', displayName:'side'},
            {width: 50, field:'value', displayName:'val', cellFilter: 'currency:"$":3'}
        ]
    };

    var addTrade = t => $scope.trade_statuses.push(new DisplayTrade(t));

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.Trades)
        .registerConnectHandler(() => $scope.trade_statuses.length = 0)
        .registerDisconnectedHandler(() => $scope.trade_statuses.length = 0)
        .registerSubscriber(addTrade, trades => trades.forEach(addTrade));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        $log.info("destroy trades list");
    });

    $log.info("started trades list");
};

var tradeList = () : ng.IDirective => {
    var template = '<div><div ui-grid="gridOptions" class="table table-striped table-hover table-condensed" style="height: 180px" ></div></div>';

    return {
        template: template,
        restrict: "E",
        replace: true,
        transclude: false,
        controller: TradesListController,
        scope: {
          exch: '=',
          pair: '='
        }
    }
};

export var tradeListDirective = "tradeListDirective";

angular.module(tradeListDirective, ['ui.bootstrap', 'ui.grid', Shared.sharedDirectives])
       .directive("tradeList", tradeList);
