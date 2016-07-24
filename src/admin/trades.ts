/// <reference path="../common/models.ts" />
/// <reference path="shared_directives.ts"/>

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
    sound: boolean;
}

class DisplayTrade {
    tradeId : string;
    time : moment.Moment;
    price : number;
    quantity : number;
    side : string;
    value : number;
    liquidity : string;
    alloc : number;
    allocprice : number;

    constructor($scope : TradesScope, public trade : Models.Trade) {
        this.tradeId = trade.tradeId;
        this.side = trade.side === Models.Side.Ask ? "S" : "B";
        this.time = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));
        this.price = trade.price;
        this.quantity = trade.quantity;
        this.alloc = trade.alloc;
        this.value = trade.value;
        this.allocprice = trade.allocprice;

        if (trade.liquidity === 0 || trade.liquidity === 1) {
            this.liquidity = Models.Liquidity[trade.liquidity].charAt(0);
        }
        else {
            this.liquidity = "?";
        }
    }
}

var TradesListController = ($scope : TradesScope, $log : ng.ILogService, subscriberFactory : Shared.SubscriberFactory, uiGridConstants: any) => {
    $scope.trade_statuses = [];
    $scope.gridOptions = {
        data: 'trade_statuses',
        treeRowHeaderAlwaysVisible: false,
        primaryKey: 'tradeId',
        groupsCollapsedByDefault: true,
        enableColumnResize: true,
        sortInfo: {fields: ['time'], directions: ['desc']},
        rowHeight: 20,
        headerRowHeight: 20,
        columnDefs: [
            {width: 80, field:'time', displayName:'t', cellFilter: 'momentShortDate',
                sortingAlgorithm: (a: moment.Moment, b: moment.Moment) => a.diff(b),
                sort: { direction: uiGridConstants.DESC, priority: 1} },
            {width: 55, field:'price', displayName:'px', cellFilter: 'currency'},
            {width: 50, field:'quantity', displayName:'qty'},
            {width: 30, field:'side', displayName:'side', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (grid.getCellValue(row, col) === 'B') {
                    return 'buy';
                }
                else if (grid.getCellValue(row, col) === 'S') {
                    return "sell";
                }
                else {
                    return "unknown";
                }
            }},
            {width: 30, field:'liquidity', displayName:'liq'},
            {width: 60, field:'value', displayName:'val', cellFilter: 'currency:"$":3'},
            {width: 50, field:'alloc', displayName:'alloc'},
            {width: 55, field:'allocprice', displayName:'px', cellFilter: 'currency'}
        ]
    };

    var addTrade = t => {
      var exists = 0;
      for(var i = 0;i<$scope.trade_statuses.length;i++) {
        if ($scope.trade_statuses[i].tradeId==t.tradeId) {
          exists = 1;
          $scope.trade_statuses[i].alloc = t.alloc;
          $scope.trade_statuses[i].allocprice = t.allocprice;
        }
      }
      if (!exists) $scope.trade_statuses.push(new DisplayTrade($scope, t));
      if ($scope.sound) {
          var audio = new Audio('http://antminer/a.mp3');
          audio.volume = 0.5;
          audio.play();
      }
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.Trades)
        .registerConnectHandler(() => $scope.trade_statuses.length = 0)
        .registerDisconnectedHandler(() => $scope.trade_statuses.length = 0)
        .registerSubscriber(addTrade, trades => trades.forEach(addTrade));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        $log.info("destroy trades list");
    });

    $log.info("started trades list");
    setTimeout(function(){$scope.sound = true;},7000);
};

var tradeList = () : ng.IDirective => {
    var template = '<div><div ui-grid="gridOptions" ui-grid-grouping class="table table-striped table-hover table-condensed" style="height: 180px" ></div></div>';

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

angular.module(tradeListDirective, ['ui.bootstrap', 'ui.grid', "ui.grid.grouping", Shared.sharedDirectives])
       .directive("tradeList", tradeList);
