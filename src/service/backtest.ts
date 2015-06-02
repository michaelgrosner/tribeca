/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />

import Config = require("./config");
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import io = require("socket.io-client");
import moment = require("moment");
import WebSocket = require('ws');
import _ = require('lodash');
import fs = require("fs");

var shortId = require("shortid");
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

export class BacktestTimeProvider implements Utils.IBacktestingTimeProvider {
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

export class BacktestExchange implements Interfaces.IPositionGateway, Interfaces.IOrderEntryGateway, Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    
    generateClientOrderId = () => {
        return "BACKTEST-" + shortId.generate();
    }

    public cancelsByClientOrderId = true;
    
    private _openBidOrders : Models.BrokeredOrder[] = [];
    private _openAskOrders : Models.BrokeredOrder[] = [];

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        this.timeProvider.setTimeout(() => {
            var collection = order.side === Models.Side.Bid ? this._openBidOrders : this._openAskOrders;
            collection.push(order);
            this.OrderUpdate.trigger({ orderId: order.orderId, orderStatus: Models.OrderStatus.Working });
        }, moment.duration(3));
        
        return new Models.OrderGatewayActionReport(this.timeProvider.utcNow());
    };

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this.timeProvider.setTimeout(() => {
            var collection = cancel.side === Models.Side.Bid ? this._openBidOrders : this._openAskOrders;
            collection = _.remove(collection, (b : Models.BrokeredOrder) => b.orderId === cancel.clientOrderId);
            this.OrderUpdate.trigger({ orderId: cancel.clientOrderId, orderStatus: Models.OrderStatus.Cancelled });
        }, moment.duration(3));
        
        return new Models.OrderGatewayActionReport(this.timeProvider.utcNow());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };
    
    private onMarketData = (market : Models.Market) => {
        this.timeProvider.scrollTimeTo(market.time);
        
        this._openAskOrders = this.tryToMatch(this._openAskOrders, market.bids, (a,b) => a<b);
        this._openBidOrders = this.tryToMatch(this._openBidOrders, market.asks, (b,a) => a<b);
        
        this.MarketData.trigger(market);
    };
    
    private tryToMatch = (orders: Models.BrokeredOrder[], 
                          marketSides: Models.MarketSide[], 
                          cmp: (a: number, b: number) => boolean) => {
        _.forEach(orders, order => {
            _.forEach(marketSides, mkt => {
                if (cmp(mkt.price, order.price) && order.quantity > 0) {
                    if (mkt.size >= order.quantity) {
                        this.OrderUpdate.trigger({ 
                            orderId: order.orderId, 
                            orderStatus: Models.OrderStatus.Complete, 
                            lastPrice: mkt.price, 
                            lastQuantity: order.quantity 
                        });
                    }
                    else {
                        this.OrderUpdate.trigger({ 
                            orderId: order.orderId, 
                            orderStatus: Models.OrderStatus.Working, 
                            partiallyFilled: true,
                            lastPrice: mkt.price,
                            lastQuantity: mkt.size
                        });
                    }
                    
                    order.quantity -= mkt.size;
                };
            });
        });
        
        return _.filter(orders, (o: Models.BrokeredOrder) => o.quantity > 0);
    };
    
    private onMarketTrade = (trade : Models.GatewayMarketTrade) => {
        this.timeProvider.scrollTimeTo(trade.time);
        this.MarketTrade.trigger(trade);
    };
    
    private _baseHeld = 0;
    private _quoteHeld = 0;
    private _baseAmount = 0;
    private _quoteAmount = 0;
    
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    recomputePosition = () => {
        this.PositionUpdate.trigger(new Models.CurrencyPosition(this._baseAmount, this._baseHeld, Models.Currency.BTC));
        this.PositionUpdate.trigger(new Models.CurrencyPosition(this._quoteAmount, this._quoteHeld, Models.Currency.USD));
    };
    
    constructor(private timeProvider: Utils.IBacktestingTimeProvider) {
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
    }
}