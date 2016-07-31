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
    gridApi : any;
    audioReady: boolean;
    audio: boolean;
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
        this.side = (trade.alloc >= trade.quantity) ? 'K' : (trade.side === Models.Side.Ask ? "S" : "B");
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
            {width: 75, field:'time', displayName:'t', cellFilter: 'momentShortDate',
                sortingAlgorithm: (a: moment.Moment, b: moment.Moment) => a.diff(b),
                sort: { direction: uiGridConstants.DESC, priority: 1} },
            {width: 50, field:'price', displayName:'px', cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price > row.entity.allocprice) ? "sell" : "buy"; else return "";
            }},
            {width: 50, field:'quantity', displayName:'qty', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price > row.entity.allocprice) ? "sell" : "buy"; else return "";
            }},
            {width: 20, field:'side', displayName:'side', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (grid.getCellValue(row, col) === 'B') {
                    return 'buy';
                }
                else if (grid.getCellValue(row, col) === 'S') {
                    return "sell";
                }
                else if (grid.getCellValue(row, col) === 'K') {
                    return "kira";
                }
                else {
                    return "unknown";
                }
            }},
            {width: 60, field:'value', displayName:'val', cellFilter: 'currency:"$":3', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return "kira"; else return "";
            }},
            {width: 50, field:'alloc', displayName:'Kqty', visible:false, cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price < row.entity.allocprice) ? "sell" : "buy"; else return "";
            }},
            {width: 55, field:'allocprice', displayName:'Kpx', cellFilter: 'currency', visible:false, cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price < row.entity.allocprice) ? "sell" : "buy"; else return "";
            }}
        ],
        onRegisterApi: function(gridApi) {
          $scope.gridApi = gridApi;
        }
    };

    var addTrade = t => {
      if (t.alloc<0) {
        for(var i = 0;i<$scope.trade_statuses.length;i++) {
          if ($scope.trade_statuses[i].tradeId==t.tradeId) {
            $scope.trade_statuses.splice(i, 1);
            break;
          }
        }
      } else {
        var exists = false;
        for(var i = 0;i<$scope.trade_statuses.length;i++) {
          if ($scope.trade_statuses[i].tradeId==t.tradeId) {
            exists = true;
            $scope.trade_statuses[i].time = t.time;
            var merged = ($scope.trade_statuses[i].quantity != t.quantity);
            $scope.trade_statuses[i].quantity = t.quantity;
            $scope.trade_statuses[i].value = t.value;
            $scope.trade_statuses[i].alloc = t.alloc;
            $scope.trade_statuses[i].allocprice = t.allocprice;
            if ($scope.trade_statuses[i].alloc >= $scope.trade_statuses[i].quantity)
              $scope.trade_statuses[i].side = 'K';
            if ($scope.audioReady && $scope.audio) {
                var audio = new Audio('/audio/'+(merged?'boom':'erang')+'.mp3');
                audio.volume = 0.5;
                audio.play();
            }
            break;
          }
        }
        if (!exists) {
          $scope.trade_statuses.push(new DisplayTrade($scope, t));
          if ($scope.audioReady && $scope.audio) {
              var audio = new Audio('/audio/boom.mp3');
              audio.volume = 0.5;
              audio.play();
          }
        }
      }
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.Trades)
        .registerConnectHandler(() => $scope.trade_statuses.length = 0)
        .registerDisconnectedHandler(() => $scope.trade_statuses.length = 0)
        .registerSubscriber(addTrade, trades => trades.forEach(addTrade));

    var updateQP = qp => {
      $scope.audio = qp.audio;
      $scope.gridOptions.columnDefs[$scope.gridOptions.columnDefs.map(function (e) { return e.field; }).indexOf('alloc')].visible = (qp.mode === Models.QuotingMode.Boomerang);
      $scope.gridOptions.columnDefs[$scope.gridOptions.columnDefs.map(function (e) { return e.field; }).indexOf('allocprice')].visible = (qp.mode === Models.QuotingMode.Boomerang);
      $scope.gridApi.grid.refresh();
    };

    var qpSub = subscriberFactory.getSubscriber($scope, Messaging.Topics.QuotingParametersChange)
        .registerConnectHandler(() => $scope.trade_statuses.length = 0)
        .registerDisconnectedHandler(() => $scope.trade_statuses.length = 0)
        .registerSubscriber(updateQP, qp => qp.forEach(updateQP));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        qpSub.disconnect();
        // $log.info("destroy trades list");
    });

    // $log.info("started trades list");
    setTimeout(function(){$scope.audioReady = true;},7000);
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
