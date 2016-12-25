/// <reference path="../common/models.ts" />
/// <reference path="orderlist.ts"/>
/// <reference path="trades.ts"/>
/// <reference path="../common/messaging.ts"/>
/// <reference path="shared_directives.ts"/>
/// <reference path="pair.ts"/>
/// <reference path="market-quoting.ts"/>
/// <reference path="market-trades.ts"/>
/// <reference path="position.ts"/>
/// <reference path="target-base-position.ts"/>
/// <reference path="trade-safety.ts"/>

(<any>global).jQuery = require("jquery");
import angular = require("angular");

var ui_bootstrap = require("angular-ui-bootstrap");
var ngGrid = require("../ui-grid.min");
var bootstrap = require("../bootstrap.min");

import Models = require("../common/models");
import moment = require("moment");
import OrderList = require("./orderlist");
import Trades = require("./trades");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");
import Pair = require("./pair");
import MarketQuoting = require("./market-quoting");
import MarketTrades = require("./market-trades");
import Messages = require("./messages");
import Position = require("./position");
import Tbp = require("./target-base-position");
import TradeSafety = require("./trade-safety");

interface MainWindowScope extends ng.IScope {
    env : string;
    theme : string;
    memory : string;
    connected : boolean;
    order : DisplayOrder;
    pair : Pair.DisplayPair;
    exch_name : string;
    pair_name : string;
    cancelAllOrders();
    cleanAllClosedOrders();
    cleanAllOrders();
    changeTheme();
}

class DisplayOrder {
    side : string;
    price : number;
    quantity : number;
    timeInForce : string;
    orderType : string;

    availableSides : string[];
    availableTifs : string[];
    availableOrderTypes : string[];

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
    constructor(fireFactory : Shared.FireFactory, private _log : ng.ILogService) {
        this.availableSides = DisplayOrder.getNames(Models.Side);
        this.availableTifs = DisplayOrder.getNames(Models.TimeInForce);
        this.availableOrderTypes = DisplayOrder.getNames(Models.OrderType);

        this._fire = fireFactory.getFire(Messaging.Topics.SubmitNewOrder);
    }

    public submit = () => {
        var msg = new Models.OrderRequestFromUI(this.side, this.price, this.quantity, this.timeInForce, this.orderType);
        // this._log.info("submitting order", msg);
        this._fire.fire(msg);
    };
}

var uiCtrl = ($scope : MainWindowScope,
              $timeout : ng.ITimeoutService,
              $log : ng.ILogService,
              subscriberFactory : Shared.SubscriberFactory,
              fireFactory : Shared.FireFactory) => {

    var cancelAllFirer = fireFactory.getFire(Messaging.Topics.CancelAllOrders);
    $scope.cancelAllOrders = () => cancelAllFirer.fire(new Models.CancelAllOrdersRequest());

    var cleanAllClosedFirer = fireFactory.getFire(Messaging.Topics.CleanAllClosedOrders);
    $scope.cleanAllClosedOrders = () => cleanAllClosedFirer.fire(new Models.CleanAllClosedOrdersRequest());

    var cleanAllFirer = fireFactory.getFire(Messaging.Topics.CleanAllOrders);
    $scope.cleanAllOrders = () => cleanAllFirer.fire(new Models.CleanAllOrdersRequest());


    $scope.order = new DisplayOrder(fireFactory, $log);
    $scope.pair = null;

    var unit = ['', 'K', 'M', 'G', 'T', 'P'];

    var bytesToSize = (input:number, precision:number) => {
        var index = Math.floor(Math.log(input) / Math.log(1024));
        if (index >= unit.length) return input + 'B';
        return (input / Math.pow(1024, index)).toFixed(precision) + unit[index] + 'B'
    };

    var user_theme = null;
    $scope.changeTheme = () => {
      user_theme = user_theme!==null?(user_theme==''?'-dark':''):($scope.theme==''?'-dark':'');
      $scope.theme = user_theme;
      window.setTimeout(function(){window.dispatchEvent(new Event('resize'));}, 1000);
    };

    var getTheme = (hour: number) => {
      return user_theme!==null?user_theme:((hour<9 || hour>=21)?'-dark':'');
    };

    var onAppState = (as : Models.ApplicationState) => {
      $scope.memory = bytesToSize(as.memory, 3);
      $scope.theme = getTheme(as.hour);
    };

    var onAdvert = (pa : Models.ProductAdvertisement) => {
        // $log.info("advert", pa);
        $scope.connected = true;
        $scope.env = pa.environment;
        $scope.theme = getTheme(moment.utc().hours());
        $scope.pair_name = Models.Currency[pa.pair.base] + "/" + Models.Currency[pa.pair.quote];
        $scope.exch_name = Models.Exchange[pa.exchange];
        $scope.pair = new Pair.DisplayPair($scope, subscriberFactory, fireFactory);
        window.setTimeout(function(){window.dispatchEvent(new Event('resize'));}, 1000);
    };

    var reset = (reason : string) => {
        // $log.info("reset", reason);
        $scope.connected = false;
        $scope.pair_name = null;
        $scope.exch_name = null;

        if ($scope.pair !== null)
            $scope.pair.dispose();
        $scope.pair = null;
    };
    reset("startup");

    var sub = subscriberFactory.getSubscriber($scope, Messaging.Topics.ProductAdvertisement)
        .registerSubscriber(onAdvert, a => a.forEach(onAdvert))
        .registerDisconnectedHandler(() => reset("disconnect"));

    var ASsub = subscriberFactory.getSubscriber($scope, Messaging.Topics.ApplicationState)
        .registerSubscriber(onAppState, a => a.forEach(onAppState))
        .registerDisconnectedHandler(() => reset("disconnect"));

    $scope.$on('$destroy', () => {
        sub.disconnect();
        ASsub.disconnect();
        // $log.info("destroy client");
    });

    // $log.info("started client");
};

var requires = ['ui.bootstrap',
                'ui.grid',
                OrderList.orderListDirective,
                Trades.tradeListDirective,
                MarketQuoting.marketQuotingDirective,
                MarketTrades.marketTradeDirective,
                Messages.messagesDirective,
                Position.positionDirective,
                Tbp.targetBasePositionDirective,
                TradeSafety.tradeSafetyDirective,
                Shared.sharedDirectives];

angular.module('tribeca', requires)
       .controller('uiCtrl', uiCtrl);
