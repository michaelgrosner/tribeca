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

var SortedArray = require("collections/sorted-array");
var uuid = require('node-uuid');

enum TimedType {
    Interval,
    Timeout
}

class Timed {
    constructor(
        public action : () => void, 
        public time : Moment, 
        public type : TimedType,
        public interval : Duration) {}
}

export class BacktestTimeProvider implements Utils.ITimeProvider {
    constructor(private _internalTime : Moment) { }
    
    utcNow = () => this._internalTime;
    
    private _immediates = new Array<() => void>();
    setImmediate = (action: () => void) => this._immediates.push(action);
    
    private _timeouts = new SortedArray(null, null, (a : Timed, b : Timed) => a.time.diff(b.time));
    setTimeout = (action: () => void, time: Duration) => {
        this.setAction(action, time, TimedType.Timeout);
    };
    
    setInterval = (action: () => void, time: Duration) => {
        this.setAction(action, time, TimedType.Interval);
    };
    
    private setAction  = (action: () => void, time: Duration, type : TimedType) => {
        this._timeouts.push(new Timed(action, this._internalTime.clone().add(time), type, time));
    };
    
    scrollTimeTo = (time : Moment) => {
        while (this._immediates.length > 0) {
            this._immediates.pop()();
        }
        
        while (this._timeouts.length > 0 && this._timeouts.min().time.diff(time) > 0) {
            var evt : Timed = this._timeouts.shift();
            this._internalTime = evt.time;
            evt.action();
            if (evt.type === TimedType.Interval) {
                this.setAction(evt.action, evt.interval, evt.type);
            }
        }
        
        this._internalTime = time;
    };
}

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