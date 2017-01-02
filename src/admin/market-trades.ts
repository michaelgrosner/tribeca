/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="shared_directives.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

class MarketTradeViewModel {
    price: number;
    size: number;
    time: moment.Moment;
    recent: boolean;

    qA: number;
    qB: number;
    qAz: number;
    qBz: number;

    mA: number;
    mB: number;
    mAz: number;
    mBz: number;

    make_side: string;

    constructor(trade: Models.MarketTrade) {
        this.price = MarketTradeViewModel.round(trade.price);
        this.size = MarketTradeViewModel.round(trade.size);
        this.time = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));

        if (trade.quote != null) {
            if (trade.quote.ask !== null) {
                this.qA = MarketTradeViewModel.round(trade.quote.ask.price);
                this.qAz = MarketTradeViewModel.round(trade.quote.ask.size);
            }

            if (trade.quote.bid !== null) {
                this.qB = MarketTradeViewModel.round(trade.quote.bid.price);
                this.qBz = MarketTradeViewModel.round(trade.quote.bid.size);
            }
        }

        if (trade.ask != null) {
            this.mA = MarketTradeViewModel.round(trade.ask.price);
            this.mAz = MarketTradeViewModel.round(trade.ask.size);
        }

        if (trade.bid != null) {
            this.mB = MarketTradeViewModel.round(trade.bid.price);
            this.mBz = MarketTradeViewModel.round(trade.bid.size);
        }

        this.make_side = Models.Side[trade.make_side];

        this.recent = true;
    }

    private static round(num: number) {
        return Math.round(num * 100) / 100;
    }
}

interface MarketTradeScope extends ng.IScope {
    marketTrades: MarketTradeViewModel[];
    marketTradeOptions: Object;
}

var MarketTradeGrid = ($scope: MarketTradeScope,
                       $log: ng.ILogService,
                       subscriberFactory: Shared.SubscriberFactory,
                       uiGridConstants: any) => {
    $scope.marketTrades = [];
    $scope.marketTradeOptions = {
        data: 'marketTrades',
        showGroupPanel: false,
        rowHeight: 20,
        headerRowHeight: 20,
        groupsCollapsedByDefault: true,
        enableColumnResize: true,
        sortInfo: { fields: ['time'], directions: ['desc'] },
        columnDefs: [
            { width: 90, field: 'time', displayName: 'time', cellFilter: "momentShortDate",
                sortingAlgorithm: (a: moment.Moment, b: moment.Moment) => a.diff(b),
                sort: { direction: uiGridConstants.DESC, priority: 1}, cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return 'fs11px '+(!row.entity.recent ? "text-muted" : "");
            } },
            { width: 60, field: 'price', displayName: 'px', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return (row.entity.make_side === 'Ask') ? "sell" : "buy";
            } },
            { width: 50, field: 'size', displayName: 'sz', cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                return (row.entity.make_side === 'Ask') ? "sell" : "buy";
            } },
            { width: 40, field: 'make_side', displayName: 'ms' , cellClass: (grid, row, col, rowRenderIndex, colRenderIndex) => {
                if (grid.getCellValue(row, col) === 'Bid') {
                    return 'buy';
                }
                else if (grid.getCellValue(row, col) === 'Ask') {
                    return "sell";
                }
            }},
            { width: 40, field: 'qBz', displayName: 'qBz' },
            { width: 50, field: 'qB', displayName: 'qB' },
            { width: 50, field: 'qA', displayName: 'qA' },
            { width: 40, field: 'qAz', displayName: 'qAz' },
            { width: 40, field: 'mBz', displayName: 'mBz' },
            { width: 50, field: 'mB', displayName: 'mB' },
            { width: 50, field: 'mA', displayName: 'mA' },
            { width: 40, field: 'mAz', displayName: 'mAz' }
        ]
    };

    var addNewMarketTrade = (u: Models.MarketTrade) => {
        if (u != null)
            $scope.marketTrades.push(new MarketTradeViewModel(u));

        for(var i=$scope.marketTrades.length-1;i>-1;i--)
          if (Math.abs(moment.utc().valueOf() - $scope.marketTrades[i].time.valueOf()) > 3600000)
            $scope.marketTrades.splice(i,1);
          else if (Math.abs(moment.utc().valueOf() - $scope.marketTrades[i].time.valueOf()) > 7000)
            $scope.marketTrades[i].recent = false;
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.MarketTrade)
        .registerSubscriber(addNewMarketTrade, x => x.forEach(addNewMarketTrade))
        .registerDisconnectedHandler(() => $scope.marketTrades.length = 0);

    $scope.$on('$destroy', () => {
        sub.disconnect();
        // $log.info("destroy market trade grid");
    });

    // $log.info("started market trade grid");
};

export var marketTradeDirective = "marketTradeDirective";

angular
    .module(marketTradeDirective, ['ui.bootstrap', 'ui.grid', Shared.sharedDirectives])
    .directive("marketTradeGrid", () => {
        var template = '<div><div style="height: 180px" class="table table-striped table-hover table-condensed" ui-grid="marketTradeOptions"></div></div>';

        return {
            restrict: 'E',
            transclude: false,
            template: template,
            controller: MarketTradeGrid
        }
    });
