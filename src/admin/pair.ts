/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

class FormViewModel<T> {
    master : T;
    display : T;
    pending : boolean = false;
    connected : boolean = false;

    constructor(defaultParameter : T,
                private _sub : Messaging.ISubscribe<T>,
                private _fire : Messaging.IFire<T>,
                private _submitConverter : (disp : T) => T = null) {
        if (this._submitConverter === null)
            this._submitConverter = d => d;

        _sub.registerConnectHandler(() => this.connected = true)
            .registerDisconnectedHandler(() => this.connected = false)
            .registerSubscriber(this.update, us => us.forEach(this.update));

        this.master = angular.copy(defaultParameter);
        this.display = angular.copy(defaultParameter);
    }

    public reset = () => {
        this.display = angular.copy(this.master);
    };

    public update = (p : T) => {
        console.log("updating parameters", p);
        this.master = angular.copy(p);
        this.display = angular.copy(p);
        this.pending = false;
    };

    public submit = () => {
        this.pending = true;
        this._fire.fire(this._submitConverter(this.display));
    };
}

class QuotingButtonViewModel extends FormViewModel<boolean> {
    constructor(sub : Messaging.ISubscribe<boolean>,
                fire : Messaging.IFire<boolean>) {
        super(false, sub, fire, d => !d);
    }

    public getClass = () => {
        if (this.pending) return "btn btn-warning";
        if (this.display) return "btn btn-success";
        return "btn btn-danger"
    }
}

class DisplayQuotingParameters extends FormViewModel<Models.QuotingParameters> {
    availableQuotingModes = [];
    availableFvModels = [];
    availableAutoPositionModes = [];

    constructor(sub : Messaging.ISubscribe<Models.QuotingParameters>,
                fire : Messaging.IFire<Models.QuotingParameters>) {
        super(new Models.QuotingParameters(null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null), sub, fire);

        this.availableQuotingModes = DisplayQuotingParameters.getMapping(Models.QuotingMode);
        this.availableFvModels = DisplayQuotingParameters.getMapping(Models.FairValueModel);
        this.availableAutoPositionModes = DisplayQuotingParameters.getMapping(Models.AutoPositionMode);
    }

    private static getMapping<T>(enumObject : T) {
        var names = [];
        for (var mem in enumObject) {
            if (!enumObject.hasOwnProperty(mem)) continue;
            var val = parseInt(mem, 10);
            if (val >= 0) {
                names.push({'str': enumObject[mem], 'val': val});
            }
        }
        return names;
    }
}

export class DisplayPair {
    name : string;
    connected : boolean;

    active : QuotingButtonViewModel;
    quotingParameters : DisplayQuotingParameters;

    private _subscribers : Messaging.ISubscribe<any>[] = [];

    constructor(public scope : ng.IScope,
                subscriberFactory : Shared.SubscriberFactory,
                fireFactory : Shared.FireFactory) {

        var setConnectStatus = (cs : Models.ConnectivityStatus) => {
            this.connected = cs == Models.ConnectivityStatus.Connected;
        };

        var connectivitySubscriber = subscriberFactory.getSubscriber(scope, Messaging.Topics.ExchangeConnectivity)
            .registerSubscriber(setConnectStatus, cs => cs.forEach(setConnectStatus));
        this._subscribers.push(connectivitySubscriber);

        var activeSub = subscriberFactory.getSubscriber(scope, Messaging.Topics.ActiveChange);
        this.active = new QuotingButtonViewModel(
            activeSub,
            fireFactory.getFire(Messaging.Topics.ActiveChange)
        );
        this._subscribers.push(activeSub);

        var qpSub = subscriberFactory.getSubscriber(scope, Messaging.Topics.QuotingParametersChange);
        this.quotingParameters = new DisplayQuotingParameters(
            qpSub,
            fireFactory.getFire(Messaging.Topics.QuotingParametersChange)
        );
        this._subscribers.push(qpSub);
    }

    public dispose = () => {
        this._subscribers.forEach(s => s.disconnect());
    };

    public updateParameters = (p : Models.QuotingParameters) => {
        this.quotingParameters.update(p);
    };
}

// ===============

class Level {
    bidPrice : number;
    bidSize : number;
    askPrice : number;
    askSize : number;

    bidClass : string;
    askClass : string;
}

interface MarketQuotingScope extends ng.IScope {
    levels : Level[];
    qBidSz : number;
    qBidPx : number;
    fairValue : number;
    qAskPx : number;
    qAskSz : number;
    extVal : number;

    bidIsLive : boolean;
    askIsLive : boolean;
}

var MarketQuotingController = ($scope : MarketQuotingScope,
                               $log : ng.ILogService,
                               subscriberFactory : Shared.SubscriberFactory) => {
    var clearMarket = () => {
        $scope.levels = [];
    };
    clearMarket();
    
    var clearBid = () => {
        $scope.qBidPx = null;
        $scope.qBidSz = null;
    };
    
    var clearAsk = () => {
        $scope.qAskPx = null;
        $scope.qAskSz = null;
    };

    var clearQuote = () => {
        clearBid();
        clearAsk();
    };

    var clearFairValue = () => {
        $scope.fairValue = null;
    };

    var clearQuoteStatus = () => {
        $scope.bidIsLive = false;
        $scope.askIsLive = false;
    };

    var clearExtVal = () => {
        $scope.extVal = null;
    };

    var updateMarket = (update : Models.Market) => {
        if (update == null) {
            clearMarket();
            return;
        }

        for (var i = 0; i < update.asks.length; i++) {
            if (angular.isUndefined($scope.levels[i]))
                $scope.levels[i] = new Level();
            $scope.levels[i].askPrice = update.asks[i].price;
            $scope.levels[i].askSize = update.asks[i].size;
        }

        for (var i = 0; i < update.bids.length; i++) {
            if (angular.isUndefined($scope.levels[i]))
                $scope.levels[i] = new Level();
            $scope.levels[i].bidPrice = update.bids[i].price;
            $scope.levels[i].bidSize = update.bids[i].size;
        }

        updateQuoteClass();
    };

    var updateQuote = (quote : Models.TwoSidedQuote) => {
        if (quote !== null) {
            if (quote.bid !== null) {
                $scope.qBidPx = quote.bid.price;
                $scope.qBidSz = quote.bid.size;
            }
            else {
                clearBid();
            }
            
            if (quote.ask !== null) {
                $scope.qAskPx = quote.ask.price;
                $scope.qAskSz = quote.ask.size;
            }
            else {
                clearAsk();
            }
        }
        else {
            clearQuote();
        }
        
        updateQuoteClass();
    };

    var updateQuoteStatus = (status : Models.TwoSidedQuoteStatus) => {
        if (status == null) {
            clearQuoteStatus();
            return;
        }

        $scope.bidIsLive = (status.bidStatus === Models.QuoteStatus.Live);
        $scope.askIsLive = (status.askStatus === Models.QuoteStatus.Live);
        updateQuoteClass();
    };

    var updateQuoteClass = () => {
        if (!angular.isUndefined($scope.levels) && $scope.levels.length > 0) {
            var tol = .005;
            for (var i = 0; i < $scope.levels.length; i++) {
                var level = $scope.levels[i];

                if (Math.abs($scope.qBidPx - level.bidPrice) < tol && $scope.bidIsLive) {
                    level.bidClass = 'success';
                }
                else {
                    level.bidClass = 'active';
                }

                if (Math.abs($scope.qAskPx - level.askPrice) < tol && $scope.askIsLive) {
                    level.askClass = 'success';
                }
                else {
                    level.askClass = 'active';
                }
            }
        }
    };

    var updateFairValue = (fv : Models.FairValue) => {
        if (fv == null) {
            clearFairValue();
            return;
        }

        $scope.fairValue = fv.price;
    };

    var subscribers = [];

    var makeSubscriber = <T>(topic : string, updateFn, clearFn) => {
        var sub = subscriberFactory.getSubscriber<T>($scope, topic)
            .registerSubscriber(updateFn, ms => ms.forEach(updateFn))
            .registerDisconnectedHandler(clearFn);
        subscribers.push(sub);
    };

    makeSubscriber<Models.Market>(Messaging.Topics.MarketData, updateMarket, clearMarket);
    makeSubscriber<Models.TwoSidedQuote>(Messaging.Topics.Quote, updateQuote, clearQuote);
    makeSubscriber<Models.TwoSidedQuoteStatus>(Messaging.Topics.QuoteStatus, updateQuoteStatus, clearQuoteStatus);
    makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue, updateFairValue, clearFairValue);

    $scope.$on('$destroy', () => {
        subscribers.forEach(d => d.disconnect());
        $log.info("destroy market quoting grid");
    });

    $log.info("started market quoting grid");
};

angular
    .module("marketQuotingDirective", ['ui.bootstrap', 'ngGrid', 'sharedDirectives'])
    .directive("marketQuotingGrid", () => {

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            templateUrl: "market_display.html",
            controller: MarketQuotingController
          }
    });

// ===============

class MarketTradeViewModel {
    price : number;
    size : number;
    time : moment.Moment;

    qA : number;
    qB : number;
    qAz : number;
    qBz : number;

    mA : number;
    mB : number;
    mAz : number;
    mBz : number;
    
    make_side : string;

    constructor(trade : Models.MarketTrade) {
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
    }

    private static round(num : number) {
        return Math.round(num * 100) / 100;
    }
}

interface MarketTradeScope extends ng.IScope {
    marketTrades : MarketTradeViewModel[];
    marketTradeOptions : Object;
}

var MarketTradeGrid = ($scope : MarketTradeScope,
                       $log : ng.ILogService,
                       subscriberFactory : Shared.SubscriberFactory) => {
    $scope.marketTrades = [];
    $scope.marketTradeOptions  = {
        data: 'marketTrades',
        showGroupPanel: false,
        rowHeight: 20,
        headerRowHeight: 20,
        groupsCollapsedByDefault: true,
        enableColumnResize: true,
        sortInfo: {fields: ['time'], directions: ['desc']},
        columnDefs: [
            {width: 80, field:'time', displayName:'t', cellFilter: "momentShortDate"},
            {width: 50, field:'price', displayName:'px'},
            {width: 40, field:'size', displayName:'sz'},
            {width: 40, field:'make_side', displayName:'ms'},
            {width: 40, field:'qBz', displayName:'qBz'},
            {width: 50, field:'qB', displayName:'qB'},
            {width: 50, field:'qA', displayName:'qA'},
            {width: 40, field:'qAz', displayName:'qAz'},
            {width: 40, field:'mBz', displayName:'mBz'},
            {width: 50, field:'mB', displayName:'mB'},
            {width: 50, field:'mA', displayName:'mA'},
            {width: 40, field:'mAz', displayName:'mAz'}
        ]
    };

    var addNewMarketTrade = (u : Models.MarketTrade) => {
        $scope.marketTrades.push(new MarketTradeViewModel(u));
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.MarketTrade)
            .registerSubscriber(addNewMarketTrade, x => x.forEach(addNewMarketTrade))
            .registerDisconnectedHandler(() => $scope.marketTrades.length = 0);

    $scope.$on('$destroy', () => {
        sub.disconnect();
        $log.info("destroy market trade grid");
    });

    $log.info("started market trade grid");
};

angular
    .module("marketTradeDirective", ['ui.bootstrap', 'ngGrid', 'sharedDirectives'])
    .directive("marketTradeGrid", () => {
        var template = '<div><div style="height: 180px" class="table table-striped table-hover table-condensed" ng-grid="marketTradeOptions"></div></div>';

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            template: template,
            controller: MarketTradeGrid
          }
    });

// ==================

class MessageViewModel {
    text : string;
    time : moment.Moment;

    constructor(message : Models.Message) {
        this.time = (moment.isMoment(message.time) ? message.time : moment(message.time));
        this.text = message.text;
    }
}

interface MessageLoggerScope extends ng.IScope {
    messages : MessageViewModel[];
    messageOptions : Object;
}

var MessagesController = ($scope : MessageLoggerScope, $log : ng.ILogService, subscriberFactory : Shared.SubscriberFactory) => {
    $scope.messages = [];
    $scope.messageOptions  = {
        data: 'messages',
        showGroupPanel: false,
        rowHeight: 20,
        headerRowHeight: 0,
        groupsCollapsedByDefault: true,
        enableColumnResize: true,
        sortInfo: {fields: ['time'], directions: ['desc']},
        columnDefs: [
            {width: 120, field:'time', displayName:'t', cellFilter: 'momentFullDate'},
            {width: "*", field:'text', displayName:'text'}
        ]
    };

    var addNewMessage = (u : Models.Message) => {
        $scope.messages.push(new MessageViewModel(u));
    };

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.Message)
            .registerSubscriber(addNewMessage, x => x.forEach(addNewMessage))
            .registerDisconnectedHandler(() => $scope.messages.length = 0);

    $scope.$on('$destroy', () => {
        sub.disconnect();
        $log.info("destroy message grid");
    });

    $log.info("started message grid");
};

angular
    .module("messagesDirective", ['ui.bootstrap', 'ngGrid', 'sharedDirectives'])
    .directive("messagesGrid", () => {
        var template = '<div><div class="table table-striped table-hover table-condensed" ng-grid="messageOptions"></div></div>';

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            template: template,
            controller: MessagesController
          }
    });
