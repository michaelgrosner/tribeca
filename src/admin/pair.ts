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

    constructor(sub : Messaging.ISubscribe<Models.QuotingParameters>,
                fire : Messaging.IFire<Models.QuotingParameters>) {
        super(new Models.QuotingParameters(null, null, null), sub, fire);

        var modes = [Models.QuotingMode.Mid, Models.QuotingMode.Top];
        this.availableQuotingModes = modes.map(m => {
            return {'str': Models.QuotingMode[m], 'val': m};
        })
    }
}

class DisplaySafetySettingsParameters extends FormViewModel<Models.SafetySettings> {
    constructor(sub : Messaging.ISubscribe<Models.SafetySettings>,
                fire : Messaging.IFire<Models.SafetySettings>) {
        super(new Models.SafetySettings(null), sub, fire);
    }
}

export class DisplayPair {
    name : string;
    base : string;
    quote : string;

    bidSize : number;
    bid : number;
    askSize : number;
    ask : number;

    qBidPx : number;
    qBidSz : number;
    qAskPx : number;
    qAskSz : number;
    fairValue : number;

    active : QuotingButtonViewModel;
    quotingParameters : DisplayQuotingParameters;
    safetySettings : DisplaySafetySettingsParameters;

    private _subscribers : Messaging.ISubscribe<any>[] = [];

    constructor(public exch : Models.Exchange,
                public pair : Models.CurrencyPair,
                log : ng.ILogService,
                io : any) {

        var makeSubscriber = <T>(topic : string) => {
            var wrappedTopic = Messaging.ExchangePairMessaging.wrapExchangePairTopic(exch, pair, topic);
            var subscriber = new Messaging.Subscriber<T>(wrappedTopic, io, log.info);
            this._subscribers.push(subscriber);
            return  subscriber;
        };

        var makeFire = <T>(topic : string) => {
            var wrappedTopic = Messaging.ExchangePairMessaging.wrapExchangePairTopic(exch, pair, topic);
            return new Messaging.Fire<T>(wrappedTopic, io);
        };

        makeSubscriber<Models.Market>(Messaging.Topics.MarketData)
            .registerSubscriber(this.updateMarket, ms => ms.forEach(this.updateMarket))
            .registerDisconnectedHandler(this.clearMarket);

        makeSubscriber<Models.TwoSidedQuote>(Messaging.Topics.Quote)
            .registerSubscriber(this.updateQuote, qs => qs.forEach(this.updateQuote))
            .registerDisconnectedHandler(this.clearQuote);

        makeSubscriber<Models.FairValue>(Messaging.Topics.FairValue)
            .registerSubscriber(this.updateFairValue, qs => qs.forEach(this.updateFairValue))
            .registerDisconnectedHandler(this.clearFairValue);

        this.quote = Models.Currency[pair.quote];
        this.base = Models.Currency[pair.base];
        this.name = this.base + "/" + this.quote;

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

    private clearMarket = () => {
        this.bidSize = null;
        this.bid = null;
        this.ask = null;
        this.askSize = null;
    };

    public updateMarket = (update : Models.Market) => {
        this.bidSize = update.bids[0].size;
        this.bid = update.bids[0].price;
        this.ask = update.asks[0].price;
        this.askSize = update.asks[0].size;
    };

    public updateQuote = (quote : Models.TwoSidedQuote) => {
        this.qBidPx = quote.bid.price;
        this.qBidSz = quote.bid.size;
        this.qAskPx = quote.ask.price;
        this.qAskSz = quote.ask.size;
    };

    public clearQuote = () => {
        this.qBidPx = null;
        this.qBidSz = null;
        this.qAskPx = null;
        this.qAskSz = null;
    };

    public updateFairValue = (fv : Models.FairValue) => {
        this.fairValue = fv.price;
    };

    public clearFairValue = () => {
        this.fairValue = null;
    };

    public updateParameters = (p : Models.QuotingParameters) => {
        this.quotingParameters.update(p);
    };
}

// ===============

class MarketTradeViewModel {
    price : number;
    size : number;
    time : Date;
    displayTime : string;
    qA : number;
    qB : number;
    qAz : number;
    qBz : number;

    constructor(trade : Models.MarketTrade) {
        this.price = MarketTradeViewModel.round(trade.price);
        this.size = MarketTradeViewModel.round(trade.size);

        var parsedTime = (moment.isMoment(trade.time) ? trade.time : moment(trade.time));
        this.time = parsedTime.toDate();
        this.displayTime = parsedTime.format('HH:mm:ss,SSS');

        if (trade.quote != null) {
            this.qA = MarketTradeViewModel.round(trade.quote.ask.price);
            this.qAz = MarketTradeViewModel.round(trade.quote.ask.size);
            this.qB = MarketTradeViewModel.round(trade.quote.bid.price);
            this.qBz = MarketTradeViewModel.round(trade.quote.bid.size);
        }
    }

    private static round(num : number) {
        return Math.round(num * 100) / 100;
    }
}

interface MarketTradeScope extends ng.IScope {
    marketTrades : MarketTradeViewModel[];
    marketTradeOptions : Object;
    exch : Models.Exchange;
    pair : Models.CurrencyPair;
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
            {width: 75, field:'displayTime', displayName:'t'},
            {width: 50, field:'price', displayName:'px'},
            {width: 40, field:'size', displayName:'sz'},
            {width: 40, field:'qBz', displayName:'qBz'},
            {width: 50, field:'qB', displayName:'qB'},
            {width: 50, field:'qA', displayName:'qA'},
            {width: 40, field:'qAz', displayName:'qAz'}
        ]
    };

    var addNewMarketTrade = (u : Models.MarketTrade) => {
        $log.info(u);
        $scope.marketTrades.push(new MarketTradeViewModel(u));
    };

    var topic = Messaging.ExchangePairMessaging.wrapExchangePairTopic($scope.exch, $scope.pair, Messaging.Topics.MarketTrade);
    new Messaging.Subscriber<Models.MarketTrade>(topic, socket, $log.info)
            .registerSubscriber(addNewMarketTrade, x => x.forEach(addNewMarketTrade))
            .registerDisconnectedHandler(() => $scope.marketTrades.length = 0);

    $log.info("starting market trade grid for", $scope.pair, Models.Exchange[$scope.exch]);
};

angular
    .module("marketTradeDirective", ['ui.bootstrap', 'ngGrid', 'sharedDirectives'])
    .directive("marketTradeGrid", () => {
        var template = '<div><div style="height: 150px" class="table table-striped table-hover table-condensed" ng-grid="marketTradeOptions"></div></div>';

        return {
            restrict: 'E',
            replace: true,
            transclude: false,
            template: template,
            controller: MarketTradeGrid,
            scope: {
              exch: '=',
              pair: '='
            }
          }
    });
