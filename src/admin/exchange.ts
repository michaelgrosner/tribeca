/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Pair = require("./pair");

interface PositionScope extends ng.IScope {
    exch : Models.Exchange;

    baseCurrency : string;
    basePosition : number;
    quoteCurrency : string;
    quotePosition : number;
    baseHeldPosition : number;
    quoteHeldPosition : number;
    value : number;
}

var PositionController = ($scope : PositionScope, $log : ng.ILogService, socket : any) => {
    var clearPosition = () => {
        $scope.baseCurrency = null;
        $scope.quoteCurrency = null;
        $scope.basePosition = null;
        $scope.quotePosition = null;
        $scope.baseHeldPosition = null;
        $scope.quoteHeldPosition = null;
        $scope.value = null;
    };

    var updatePosition = (position : Models.PositionReport) => {
        $scope.baseCurrency = Models.Currency[position.pair.base];
        $scope.quoteCurrency = Models.Currency[position.pair.quote];
        $scope.basePosition = position.baseAmount;
        $scope.quotePosition = position.quoteAmount;
        $scope.baseHeldPosition = position.baseHeldAmount;
        $scope.quoteHeldPosition = position.quoteHeldAmount;
        $scope.value = position.value;
    };

    var makeSubscriber = <T>(topic : string) => {
        var wrappedTopic = Messaging.ExchangePairMessaging.wrapExchangeTopic($scope.exch, topic);
        return new Messaging.Subscriber<T>(wrappedTopic, socket, $log.info);
    };

    var positionSubscriber = makeSubscriber(Messaging.Topics.Position)
        .registerDisconnectedHandler(clearPosition)
        .registerSubscriber(updatePosition, us => us.forEach(updatePosition));

    $log.info("starting position grid for", Models.Exchange[$scope.exch]);
};

angular
    .module("positionDirective", ['ui.bootstrap', 'sharedDirectives'])
    .directive("positionGrid", () => {
        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            templateUrl: "positions.html",
            controller: PositionController,
            scope: {
              exch: '=',
            }
          }
    });

// ===============================

export class DisplayExchangeInformation {
    connected : boolean;
    name : string;
    pairs : Pair.DisplayPair[] = [];

    private _connectivitySubscriber : Messaging.ISubscribe<Models.ConnectivityStatus>;

    constructor(private _log : ng.ILogService,
                public exchange : Models.Exchange,
                private _io : any) {

        var makeSubscriber = <T>(topic : string) => {
            var wrappedTopic = Messaging.ExchangePairMessaging.wrapExchangeTopic(exchange, topic);
            return new Messaging.Subscriber<T>(wrappedTopic, _io, _log.info);
        };

        this._connectivitySubscriber = makeSubscriber(Messaging.Topics.ExchangeConnectivity)
            .registerSubscriber(this.setConnectStatus, cs => cs.forEach(this.setConnectStatus));

        this.name = Models.Exchange[exchange];
    }

    public dispose = () => {
        this._connectivitySubscriber.disconnect();
        this.pairs.forEach(p => p.dispose());
        this.pairs.length = 0;
    };

    private setConnectStatus = (cs : Models.ConnectivityStatus) => {
        this.connected = cs == Models.ConnectivityStatus.Connected;
    };

    public getOrAddDisplayPair = (pair : Models.CurrencyPair) : Pair.DisplayPair => {
        for (var i = 0; i < this.pairs.length; i++) {
            var p = this.pairs[i];
            if (pair.base === p.pair.base && pair.quote === p.pair.quote) {
                return p;
            }
        }

        this._log.info("adding new pair, base:", Models.Currency[pair.base], "quote:",
            Models.Currency[pair.quote], "to exchange", Models.Exchange[this.exchange]);

        var newPair = new Pair.DisplayPair(this.exchange, pair, this._log, this._io);
        this.pairs.push(newPair);
        return newPair;
    };
}