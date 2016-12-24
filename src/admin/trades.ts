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
    Kqty : number;
    Kprice : number;
    Kvalue : number;
    Kdiff : number;

    constructor($scope : TradesScope, public trade : Models.Trade) {
        this.tradeId = trade.tradeId;
        this.side = (trade.Kqty >= trade.quantity) ? 'K' : (trade.side === Models.Side.Ask ? "Sell" : "Buy");
        this.time = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));
        this.price = trade.price;
        this.quantity = trade.quantity;
        this.value = trade.value;
        this.Kqty = trade.Kqty ? trade.Kqty : null;
        this.Kprice = trade.Kprice ? trade.Kprice : null;
        this.Kvalue = trade.Kvalue ? trade.Kvalue : null;
        this.Kdiff = trade.Kdiff ? trade.Kdiff : null;

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
            {width: 134, field:'time', displayName:'t', cellFilter: 'momentFullDate',
                sortingAlgorithm: (a: moment.Moment, b: moment.Moment) => a.diff(b),
                sort: { direction: uiGridConstants.DESC, priority: 1} },
            {width: 32, field:'side', displayName:'side', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (grid.getCellValue(row, col) === 'Buy') {
                    return 'buy';
                }
                else if (grid.getCellValue(row, col) === 'Sell') {
                    return "sell";
                }
                else if (grid.getCellValue(row, col) === 'K') {
                    return "kira";
                }
                else {
                    return "unknown";
                }
            }},
            {width: 65, field:'price', displayName:'px', cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price > row.entity.Kprice) ? "sell" : "buy"; else return row.entity.side === 'Sell' ? "sell" : "buy";
            }},
            {width: 65, field:'quantity', displayName:'qty', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price > row.entity.Kprice) ? "sell" : "buy"; else return row.entity.side === 'Sell' ? "sell" : "buy";
            }},
            {width: 65, field:'value', displayName:'val', cellFilter: 'currency:"$":3', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price > row.entity.Kprice) ? "sell" : "buy"; else return row.entity.side === 'Sell' ? "sell" : "buy";
            }},
            {width: 65, field:'Kvalue', displayName:'valPong', visible:false, cellFilter: 'currency:"$":3', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price < row.entity.Kprice) ? "sell" : "buy"; else return row.entity.Kqty ? ((row.entity.price < row.entity.Kprice) ? "sell" : "buy") : "";
            }},
            {width: 65, field:'Kqty', displayName:'qtyPong', visible:false, cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price < row.entity.Kprice) ? "sell" : "buy"; else return row.entity.Kqty ? ((row.entity.price < row.entity.Kprice) ? "sell" : "buy") : "";
            }},
            {width: 65, field:'Kprice', displayName:'pxPong', visible:false, cellFilter: 'currency', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return (row.entity.price < row.entity.Kprice) ? "sell" : "buy"; else return row.entity.Kqty ? ((row.entity.price < row.entity.Kprice) ? "sell" : "buy") : "";
            }},
            {width: 65, field:'Kdiff', displayName:'diff', visible:false, cellFilter: 'currency:"$":3', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (row.entity.side === 'K') return "kira"; else return "";
            }}
        ],
        onRegisterApi: function(gridApi) {
          $scope.gridApi = gridApi;
        }
    };

    var addTrade = t => {
      if (t.Kqty<0) {
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
            $scope.trade_statuses[i].time = (moment.isMoment(t.time) ? t.time : moment(t.time));
            var merged = ($scope.trade_statuses[i].quantity != t.quantity);
            $scope.trade_statuses[i].quantity = t.quantity;
            $scope.trade_statuses[i].value = t.value;
            $scope.trade_statuses[i].Kqty = t.Kqty;
            $scope.trade_statuses[i].Kprice = t.Kprice;
            $scope.trade_statuses[i].Kvalue = t.Kvalue;
            $scope.trade_statuses[i].Kdiff = t.Kdiff;
            if ($scope.trade_statuses[i].Kqty >= $scope.trade_statuses[i].quantity)
              $scope.trade_statuses[i].side = 'K';
            $scope.gridApi.grid.notifyDataChange(uiGridConstants.dataChange.ALL);
            if (t.loadedFromDB === false && $scope.audio) {
                var audio = new Audio('/audio/'+(merged?'boom':'erang')+'.mp3');
                audio.volume = 0.5;
                audio.play();
            }
            break;
          }
        }
        if (!exists) {
          $scope.trade_statuses.push(new DisplayTrade($scope, t));
          if (t.loadedFromDB === false && $scope.audio) {
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
      var visibility = (qp.mode === Models.QuotingMode.Boomerang || qp.mode === Models.QuotingMode.AK47);
      $scope.gridOptions.columnDefs[$scope.gridOptions.columnDefs.map(function (e) { return e.field; }).indexOf('Kqty')].visible = visibility;
      $scope.gridOptions.columnDefs[$scope.gridOptions.columnDefs.map(function (e) { return e.field; }).indexOf('Kprice')].visible = visibility;
      $scope.gridOptions.columnDefs[$scope.gridOptions.columnDefs.map(function (e) { return e.field; }).indexOf('Kvalue')].visible = visibility;
      $scope.gridOptions.columnDefs[$scope.gridOptions.columnDefs.map(function (e) { return e.field; }).indexOf('Kdiff')].visible = visibility;
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
