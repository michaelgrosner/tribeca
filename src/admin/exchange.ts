/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

interface ExchangesScope extends ng.IScope {
    exchanges : { [exch : number] : DisplayExchangeInformation};
}

class DisplayExchangeInformation {
    connected : boolean;
    name : string;

    usdPosition : number = null;
    btcPosition : number = null;
    ltcPosition : number = null;

    constructor(exchange : Models.Exchange) {
        this.name = Models.Exchange[exchange];
    }

    public setConnectStatus = (cs : Models.ConnectivityStatus) => {
        this.connected = cs == Models.ConnectivityStatus.Connected;
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
    };
}

var ExchangesController = ($scope : ExchangesScope, $timeout : ng.ITimeoutService, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
    var getOrAddDisplayExchange = (exch : Models.Exchange) : DisplayExchangeInformation => {
        var disp = $scope.exchanges[exch];

        if (angular.isUndefined(disp)) {
            $scope.exchanges[exch] = new DisplayExchangeInformation(exch);
            disp = $scope.exchanges[exch];
        }

        return disp;
    };

    // ugh
    var subscriber = () => {
        $scope.exchanges = {};
        socket.emit("subscribe-position-report");
        socket.emit("subscribe-connection-status");
    };

    socket.on("hello", subscriber);
    subscriber();

    socket.on("position-report", (rpt : Models.ExchangeCurrencyPosition) =>
        getOrAddDisplayExchange(rpt.exchange).updatePosition(rpt));

    socket.on("connection-status", (exch : Models.Exchange, cs : Models.ConnectivityStatus) =>
        getOrAddDisplayExchange(exch).setConnectStatus(cs) );

    socket.on("disconnect", () => {
        $scope.exchanges = {};
    });

    // i hate this
    var refresh_timer = () => {
        $timeout(refresh_timer, 250);
    };
    $timeout(refresh_timer, 250);

    $log.info("started exchanges");
};

var exchangeDirective = () : ng.IDirective => {
    return {
        restrict: "E",
        templateUrl: "exchange.html"
    }
};

angular.module('exchangesDirective', ['ui.bootstrap'])
       .controller('ExchangesController', ExchangesController)
       .directive("exchanges", exchangeDirective);