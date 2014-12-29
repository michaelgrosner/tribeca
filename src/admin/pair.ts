/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

interface PairScope extends ng.IScope {
    pairs : { [name : string] : DisplayPair }
}

class DisplayPair {
    name : string;
    base : string;
    quote : string;

    bidSize : number;
    bid : number;
    askSize : number;
    ask : number;

    constructor(quote : Models.Currency, base : Models.Currency) {
        this.quote = Models.Currency[quote];
        this.base = Models.Currency[base];
    }

    public updateMarket = (book : Models.Market) => {
        this.bidSize = book.update.bid.size;
        this.bid = book.update.bid.price;
        this.ask = book.update.ask.price;
        this.askSize = book.update.ask.size;
    };
}

var PairsController = ($scope : PairScope, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
    $log.info("started pairs");
};

var pairDirective = () : ng.IDirective => {
    return {
        restrict: "E",
        templateUrl: "pair.html"
    }
};

angular.module('pairsDirective', ['ui.bootstrap', 'sharedDirectives'])
       .controller('PairsController', PairsController)
       .directive("pairs", pairDirective);