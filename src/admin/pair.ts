/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

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

    constructor(sub : Messaging.ISubscribe<Models.QuotingParameters>,
                fire : Messaging.IFire<Models.QuotingParameters>) {
        super(new Models.QuotingParameters(null, null, null, null), sub, fire);

        this.availableQuotingModes = DisplayQuotingParameters.getMapping(Models.QuotingMode);
        this.availableFvModels = DisplayQuotingParameters.getMapping(Models.FairValueModel);
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

class DisplaySafetySettingsParameters extends FormViewModel<Models.SafetySettings> {
    constructor(sub : Messaging.ISubscribe<Models.SafetySettings>,
                fire : Messaging.IFire<Models.SafetySettings>) {
        super(new Models.SafetySettings(null, null, null), sub, fire);
    }
}

export class DisplayPair {
    name : string;
    exch_name : string;
    connected : boolean;

    active : QuotingButtonViewModel;
    quotingParameters : DisplayQuotingParameters;
    safetySettings : DisplaySafetySettingsParameters;

    private _subscribers : Messaging.ISubscribe<any>[] = [];

    constructor(public exch : Models.Exchange,
                public pair : Models.CurrencyPair,
                log : ng.ILogService,
                io : any) {

        var makeSubscriber = <T>(topic : string) => {
            var subscriber = new Messaging.Subscriber<T>(topic, io, log.info);
            this._subscribers.push(subscriber);
            return subscriber;
        };

        var makeFire = <T>(topic : string) => {
            return new Messaging.Fire<T>(topic, io, log.info);
        };

        var makeSubscriber2 = <T>(topic : string) => {
            var subscriber = new Messaging.Subscriber<T>(topic, io, log.info);
            this._subscribers.push(subscriber);
            return subscriber;
        };

        var setConnectStatus = (cs : Models.ConnectivityStatus) => {
            this.connected = cs == Models.ConnectivityStatus.Connected;
        };

        var connectivitySubscriber = makeSubscriber2(Messaging.Topics.ExchangeConnectivity)
            .registerSubscriber(setConnectStatus, cs => cs.forEach(setConnectStatus));

        this.active = new QuotingButtonViewModel(
            makeSubscriber(Messaging.Topics.ActiveChange),
            makeFire(Messaging.Topics.ActiveChange)
        );

        this.quotingParameters = new DisplayQuotingParameters(
            makeSubscriber(Messaging.Topics.QuotingParametersChange),
            makeFire(Messaging.Topics.QuotingParametersChange)
        );

        this.safetySettings = new DisplaySafetySettingsParameters(
            makeSubscriber(Messaging.Topics.SafetySettings),
            makeFire(Messaging.Topics.SafetySettings)
        );
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

    bidIsLive : boolean;
    askIsLive : boolean;
}

var MarketQuotingController = ($scope : MarketQuotingScope,
                               $log : ng.ILogService,
                               socket : any) => {
    var clearMarket = () => {
        $scope.levels = [];
    };
    clearMarket();

    var clearQuote = () => {
        $scope.qBidPx = null;
        $scope.qBidSz = null;
        $scope.qAskPx = null;
        $scope.qAskSz = null;
    };

    var clearFairValue = () => {
        $scope.fairValue = null;
    };

    var clearQuoteStatus = () => {
        $scope.bidIsLive = false;
        $scope.askIsLive = false;
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
        if (quote == null) {
            clearQuote();
            return;
        }

        $scope.qBidPx = quote.bid.price;
        $scope.qBidSz = quote.bid.size;
        $scope.qAskPx = quote.ask.price;
        $scope.qAskSz = quote.ask.size;
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

    var _subscribers = [];
    var makeSubscriber = <T>(topic : string) => {
        var subscriber = new Messaging.Subscriber<T>(topic, socket, $log.info);
        _subscribers.push(subscriber);
        return subscriber;
    };

    makeSubscriber<Models.Market>(Messaging.Topics.MarketData)
        .registerSubscriber(updateMarket, ms => ms.forEach(updateMarket))
        .registerDisconnectedHandler(clearMarket);

    makeSubscriber<Models.TwoSidedQuote>(Messaging.Topics.Quote)
        .registerSubscriber(updateQuote, qs => qs.forEach(updateQuote))
        .registerDisconnectedHandler(clearQuote);

    makeSubscriber<Models.TwoSidedQuoteStatus>(Messaging.Topics.QuoteStatus)
        .registerSubscriber(updateQuoteStatus, qs => qs.forEach(updateQuoteStatus))
        .registerDisconnectedHandler(clearQuoteStatus);

    makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue)
        .registerSubscriber(updateFairValue, qs => qs.forEach(updateFairValue))
        .registerDisconnectedHandler(clearFairValue);

    $log.info("starting market quoting grid");
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
    time : Moment;

    qA : number;
    qB : number;
    qAz : number;
    qBz : number;

    mA : number;
    mB : number;
    mAz : number;
    mBz : number;

    constructor(trade : Models.MarketTrade) {
        this.price = MarketTradeViewModel.round(trade.price);
        this.size = MarketTradeViewModel.round(trade.size);
        this.time = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));

        if (trade.quote != null) {
            this.qA = MarketTradeViewModel.round(trade.quote.ask.price);
            this.qAz = MarketTradeViewModel.round(trade.quote.ask.size);
            this.qB = MarketTradeViewModel.round(trade.quote.bid.price);
            this.qBz = MarketTradeViewModel.round(trade.quote.bid.size);
        }

        if (trade.ask != null) {
            this.mA = MarketTradeViewModel.round(trade.ask.price);
            this.mAz = MarketTradeViewModel.round(trade.ask.size);
        }

        if (trade.bid != null) {
            this.mB = MarketTradeViewModel.round(trade.bid.price);
            this.mBz = MarketTradeViewModel.round(trade.bid.size);
        }
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
                       socket : any) => {
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

    new Messaging.Subscriber<Models.MarketTrade>(Messaging.Topics.MarketTrade, socket, $log.info)
            .registerSubscriber(addNewMarketTrade, x => x.forEach(addNewMarketTrade))
            .registerDisconnectedHandler(() => $scope.marketTrades.length = 0);

    $log.info("starting market trade grid");
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
    time : Moment;

    constructor(message : Models.Message) {
        this.time = (moment.isMoment(message.time) ? message.time : moment(message.time));
        this.text = message.text;
    }
}

interface MessageLoggerScope extends ng.IScope {
    messages : MessageViewModel[];
    messageOptions : Object;
}

var MessagesController = ($scope : MessageLoggerScope, $log : ng.ILogService, socket : any) => {
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

    new Messaging.Subscriber<Models.Message>(Messaging.Topics.Message, socket, $log.info)
            .registerSubscriber(addNewMessage, x => x.forEach(addNewMessage))
            .registerDisconnectedHandler(() => $scope.messages.length = 0);

    $log.info("starting message grid");
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

