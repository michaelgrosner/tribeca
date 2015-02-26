/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import moment = require("moment");
import Exchange = require("./exchange");
import OrderList = require("./orderlist");
import Trades = require("./trades");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

interface MainWindowScope extends ng.IScope {
    env : string;
    connected : boolean;
    order : DisplayOrder;

    exchanges : Exchange.DisplayExchangeInformation[];
}

class DisplayPair {
    public displayName : string;
    constructor(public pair : Models.CurrencyPair) {
        this.displayName = Models.Currency[pair.base] + "/" + Models.Currency[pair.quote];
    }
}

class DisplayOrder {
    exchange : string;
    side : string;
    price : number;
    quantity : number;
    timeInForce : string;
    orderType : string;
    pair : DisplayPair;

    availableExchanges : string[];
    availableSides : string[];
    availableTifs : string[];
    availableOrderTypes : string[];
    availablePairs : DisplayPair[] = [];

    private static getNames<T>(enumObject : T) {
        var names : string[] = [];
        for (var mem in enumObject) {
            if (!enumObject.hasOwnProperty(mem)) continue;
            if (parseInt(mem, 10) >= 0) {
              names.push(enumObject[mem]);
            }
        }
        return names;
    }

    private _fire : Messaging.IFire<Models.OrderRequestFromUI>;
    constructor(socket : SocketIOClientStatic, private _log : ng.ILogService) {
        this.availableExchanges = DisplayOrder.getNames(Models.Exchange);
        this.availableSides = DisplayOrder.getNames(Models.Side);
        this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
        this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);

        this._fire = new Messaging.Fire<Models.OrderRequestFromUI>(Messaging.Topics.SubmitNewOrder, socket, _log.info);
    }

    public submit = () => {
        var msg = new Models.OrderRequestFromUI(this.exchange,
            this.side, this.price, this.quantity, this.timeInForce, this.orderType, this.pair.pair);
        this._log.info("submitting order", msg);
        this._fire.fire(msg);
    };

    public addNewPair = (p : Models.CurrencyPair) => {
        for (var i = 0; i < this.availablePairs.length; i++) {
            if (Models.currencyPairEqual(this.availablePairs[i].pair, p))
                return;
        }

        this.availablePairs.push(new DisplayPair(p));
    };
}

var uiCtrl = ($scope : MainWindowScope,
              $timeout : ng.ITimeoutService,
              $log : ng.ILogService,
              socket : SocketIOClientStatic) => {
    $scope.connected = false;
    $scope.exchanges = [];
    $scope.order = new DisplayOrder(socket, $log);

    var onConnect = () => {
        $scope.connected = true;
        $scope.exchanges = [];
    };

    var onAdvert = (pa : Models.ProductAdvertisement) => {
        $scope.env = pa.environment;
        $scope.order.addNewPair(pa.pair);

        var getExchInfo = () : Exchange.DisplayExchangeInformation => {
            for (var i = 0; i < $scope.exchanges.length; i++) {
                if ($scope.exchanges[i].exchange === pa.exchange)
                    return $scope.exchanges[i];
            }

            $log.info("adding new exchange", Models.Exchange[pa.exchange]);
            var exch = new Exchange.DisplayExchangeInformation($log, pa.exchange, socket);
            $scope.exchanges.push(exch);
            return exch;
        };

        getExchInfo().getOrAddDisplayPair(pa.pair);
    };

    var onDisconnect = () => {
        $scope.connected = false;
        $scope.exchanges.forEach(x => x.dispose());
        $scope.exchanges = [];
    };

    new Messaging.Subscriber<Models.ProductAdvertisement>(Messaging.Topics.ProductAdvertisement, socket, $log.info)
        .registerConnectHandler(onConnect)
        .registerSubscriber(onAdvert, a => a.forEach(onAdvert))
        .registerDisconnectedHandler(onDisconnect);

    var refresh_timer = () => {
        $timeout(refresh_timer, 250);
    };
    $timeout(refresh_timer, 250);

    $log.info("started client");
};

var requires = ['ui.bootstrap',
                'ngGrid',
                'orderListDirective',
                'tradeListDirective',
                'marketQuotingDirective',
                'marketTradeDirective',
                'messagesDirective', 
                'positionDirective',
                'sharedDirectives'];

angular.module('projectApp', requires)
       .controller('uiCtrl', uiCtrl);