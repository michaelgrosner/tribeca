/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");

interface ExchangesScope extends ng.IScope {
    exchanges : { [exch : number] : DisplayExchangeInformation};
    sendUpdatedParameters : (p : Models.ExchangePairMessage<Models.QuotingParameters>) => void;
    sendUpdatedSafetySettingsParameters : (p : Models.ExchangePairMessage<Models.SafetySettings>) => void;
    changeActive : (p : Models.ExchangePairMessage<boolean>) => void;
}

class FormViewModel<T> {
    master : T;
    display : T;
    pending : boolean = false;

    constructor(defaultParameter : T,
                private _submitAction : (disp : T) => void) {
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
        this._submitAction(this.display);
    };
}

class QuotingButtonViewModel extends FormViewModel<boolean> {
    constructor($scope : ExchangesScope, pair : Models.CurrencyPair, exch : Models.Exchange) {
        super(false,
            d => $scope.changeActive(new Models.ExchangePairMessage(exch, pair, !d)));
    }

    public getClass = () => {
        if (this.pending) return "btn btn-warning";
        if (this.display) return "btn btn-success";
        return "btn btn-danger"
    }
}

class DisplayQuotingParameters extends FormViewModel<Models.QuotingParameters> {
    availableQuotingModes = [];

    constructor($scope : ExchangesScope, pair : Models.CurrencyPair, exch : Models.Exchange) {
        super(new Models.QuotingParameters(null, null, null),
              d => $scope.sendUpdatedParameters(new Models.ExchangePairMessage(exch, pair, d)));

        var modes = [Models.QuotingMode.Mid, Models.QuotingMode.Top];
        this.availableQuotingModes = modes.map(m => {
            return {'str': Models.QuotingMode[m], 'val': m};
        })
    }
}

class DisplaySafetySettingsParameters extends FormViewModel<Models.SafetySettings> {
    constructor($scope : ExchangesScope, pair : Models.CurrencyPair, exch : Models.Exchange) {
        super(new Models.SafetySettings(null),
              d => $scope.sendUpdatedSafetySettingsParameters(new Models.ExchangePairMessage(exch, pair, d)));
    }
}

class DisplayPair {
    name : string;
    base : string;
    quote : string;

    bidSize : number;
    bid : number;
    askSize : number;
    ask : number;

    bidAction : string;
    askAction : string;

    qBidPx : number;
    qBidSz : number;
    qAskPx : number;
    qAskSz : number;
    fairValue : number;

    active : QuotingButtonViewModel;
    quotingParameters : DisplayQuotingParameters;
    safetySettings : DisplaySafetySettingsParameters;

    constructor(private $scope : ExchangesScope,
                private exch : Models.Exchange,
                public pair : Models.CurrencyPair,
                log : ng.ILogService,
                io : any) {

        var makeSubscriber = <T>(topic : string) =>
            new Messaging.ExchangePairPubSub.ExchangePairSubscriber<T>(exch, pair, topic, io, log.info);

        makeSubscriber<Models.Market>(Messaging.Topics.MarketData)
            .registerSubscriber(this.updateMarket, ms => ms.forEach(this.updateMarket))
            .registerDisconnectedHandler(this.clearMarket);

        this.quote = Models.Currency[pair.quote];
        this.base = Models.Currency[pair.base];
        this.name = this.base + "/" + this.quote;

        this.active = new QuotingButtonViewModel($scope, this.pair, this.exch);
        this.quotingParameters = new DisplayQuotingParameters($scope, this.pair, this.exch);
        this.safetySettings = new DisplaySafetySettingsParameters($scope, this.pair, this.exch);
    }

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

    public updateDecision = (decision : Models.TradingDecision) => {
        this.bidAction = Models.QuoteSent[decision.bidAction];
        this.askAction = Models.QuoteSent[decision.askAction];
    };

    public updateQuote = (quote : Models.TwoSidedQuote) => {
        this.qBidPx = quote.bid.price;
        this.qBidSz = quote.bid.size;
        this.qAskPx = quote.ask.price;
        this.qAskSz = quote.ask.size;
    };

    public updateFairValue = (fv : Models.FairValue) => {
        this.fairValue = fv.price;
    };

    public updateParameters = (p : Models.QuotingParameters) => {
        this.quotingParameters.update(p);
    };
}

class DisplayExchangeInformation {
    connected : boolean;
    name : string;

    usdPosition : number = null;
    btcPosition : number = null;
    ltcPosition : number = null;

    constructor(private $scope : ExchangesScope,
                private _log : ng.ILogService,
                public exchange : Models.Exchange,
                private _io : any) {
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

    pairs : DisplayPair[] = [];

    public getOrAddDisplayPair = (pair : Models.CurrencyPair) : DisplayPair => {
        for (var i = 0; i < this.pairs.length; i++) {
            var p = this.pairs[i];
            if (pair.base === p.pair.base && pair.quote === p.pair.quote) {
                return p;
            }
        }

        this._log.info("adding new pair, base:", Models.Currency[pair.base], "quote:",
            Models.Currency[pair.quote], "to exchange", Models.Exchange[this.exchange]);

        var newPair = new DisplayPair(this.$scope, this.exchange, pair, this._log, this._io);
        this.pairs.push(newPair);
        return newPair;
    };
}

var ExchangesController = ($scope : ExchangesScope, $log : ng.ILogService, socket : SocketIOClient.Socket) => {
    var getOrAddDisplayExchange = (exch : Models.Exchange) : DisplayExchangeInformation => {
        var disp = $scope.exchanges[exch];

        if (angular.isUndefined(disp)) {
            $log.info("adding new exchange", Models.Exchange[exch]);
            $scope.exchanges[exch] = new DisplayExchangeInformation($scope, $log, exch, socket);
            disp = $scope.exchanges[exch];
        }

        return disp;
    };

    $scope.sendUpdatedParameters = (p : Models.ExchangePairMessage<Models.QuotingParameters>) => {
        socket.emit("parameters-update-request", p);
    };

    $scope.sendUpdatedSafetySettingsParameters = (p : Models.ExchangePairMessage<Models.SafetySettings>) => {
        socket.emit("ss-parameters-update-request", p);
    };

    $scope.changeActive = (p : Models.ExchangePairMessage<boolean>) => {
        socket.emit("active-change-request", p);
    };

    // ugh
    var subscriber = () => {
        $scope.exchanges = {};
        socket.emit("subscribe-position-report");
        socket.emit("subscribe-connection-status");
        socket.emit("subscribe-new-trading-decision");
        socket.emit("subscribe-fair-value");
        socket.emit("subscribe-quote");
        socket.emit("subscribe-parameter-updates");
    };

    socket.on("hello", subscriber);
    subscriber();

    socket.on("position-report", (rpt : Models.ExchangeCurrencyPosition) =>
        getOrAddDisplayExchange(rpt.exchange).updatePosition(rpt));

    socket.on("connection-status", (exch : Models.Exchange, cs : Models.ConnectivityStatus) =>
        getOrAddDisplayExchange(exch).setConnectStatus(cs) );

    socket.on("new-trading-decision", (d : Models.ExchangePairMessage<Models.TradingDecision>) =>
        getOrAddDisplayExchange(d.exchange).getOrAddDisplayPair(d.pair).updateDecision(d.data));

    socket.on('fair-value', (fv : Models.ExchangePairMessage<Models.FairValue>) =>
        getOrAddDisplayExchange(fv.exchange).getOrAddDisplayPair(fv.pair).updateFairValue(fv.data));

    socket.on('quote', (q : Models.ExchangePairMessage<Models.TwoSidedQuote>) =>
        getOrAddDisplayExchange(q.exchange).getOrAddDisplayPair(q.pair).updateQuote(q.data));

    socket.on("parameter-updates", (p : Models.ExchangePairMessage<Models.QuotingParameters>) =>
        getOrAddDisplayExchange(p.exchange).getOrAddDisplayPair(p.pair).updateParameters(p.data));

    socket.on('active-changed', (b : Models.ExchangePairMessage<boolean>) => {
        getOrAddDisplayExchange(b.exchange).getOrAddDisplayPair(b.pair).active.update(b.data);
    });


    socket.on("disconnect", () => {
        $scope.exchanges = {};
    });

    $log.info("started exchanges");
};

var exchangeDirective = () : ng.IDirective => {
    return {
        restrict: "E",
        templateUrl: "exchange.html"
    }
};

angular.module('exchangesDirective', ['ui.bootstrap', 'sharedDirectives'])
       .controller('ExchangesController', ExchangesController)
       .directive("exchanges", exchangeDirective);