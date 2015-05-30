/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("./config");
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import io = require("socket.io-client");
import moment = require("moment");
import WebSocket = require('ws');
import _ = require('lodash');

var uuid = require('node-uuid');
var SortedArrayMap = require("collections/sorted-array-map");

class BacktestMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.MarketSide>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    _log: Utils.Logger = Utils.log("tribeca:gateway:btMD");
    constructor() {
        
    }
}

class BacktestOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    generateClientOrderId = (): string => {
        return uuid.v1();
    }

    cancelOrder = (cancel: Models.BrokeredCancel): Models.OrderGatewayActionReport => {
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace: Models.BrokeredReplace): Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order: Models.BrokeredOrder): Models.OrderGatewayActionReport => {
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    public cancelsByClientOrderId = false;

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    _log: Utils.Logger = Utils.log("tribeca:gateway:btOE");
    constructor() {
    }
}


class BacktestPositionGateway implements Interfaces.IPositionGateway {
    _log: Utils.Logger = Utils.log("tribeca:gateway:btPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private onTick = () => {
        // raise positions
    };

    constructor() {}
}

class BacktestBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return true;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.Null;
    }

    makeFee(): number {
        return 0;
    }

    takeFee(): number {
        return 0;
    }

    name(): string {
        return "Backtest";
    }
}

export class Backtester extends Interfaces.CombinedGateway {
    constructor() {
        var orderGateway = new BacktestOrderEntryGateway();
        var positionGateway = new BacktestPositionGateway();
        var mdGateway = new BacktestMarketDataGateway();

        super(
            mdGateway,
            orderGateway,
            positionGateway,
            new BacktestBaseGateway());
    }
}