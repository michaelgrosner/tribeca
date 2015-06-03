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
    
    private _openBidOrders : {[orderId: string]: Models.BrokeredOrder} = {};
    private _openAskOrders : {[orderId: string]: Models.BrokeredOrder} = {};

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        this.timeProvider.setTimeout(() => {
            if (order.side === Models.Side.Bid) {
                this._openBidOrders[order.orderId] = order;
                this._quoteHeld += order.price*order.quantity;
                this._quoteAmount -= order.price*order.quantity;
            }
            else {
                this._openAskOrders[order.orderId] = order;
                this._baseHeld += order.quantity;
                this._baseAmount -= order.quantity;
            }
            
            this.OrderUpdate.trigger({ orderId: order.orderId, orderStatus: Models.OrderStatus.Working });
            this.recomputePosition();
        }, moment.duration(3));
        
        return new Models.OrderGatewayActionReport(this.timeProvider.utcNow());
    };

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this.timeProvider.setTimeout(() => {
            if (cancel.side === Models.Side.Bid) {
                var existing = this._openBidOrders[cancel.clientOrderId];
                if (typeof existing === "undefined") {
                    this.OrderUpdate.trigger({orderId: cancel.clientOrderId, orderStatus: Models.OrderStatus.Rejected});
                    return;
                }
                this._quoteHeld -= existing.price * existing.quantity;
                this._quoteAmount += existing.price * existing.quantity;
                delete this._openBidOrders[cancel.clientOrderId];
            }
            else {
                var existing = this._openAskOrders[cancel.clientOrderId];
                if (typeof existing === "undefined") {
                    this.OrderUpdate.trigger({orderId: cancel.clientOrderId, orderStatus: Models.OrderStatus.Rejected});
                    return;
                }
                this._baseHeld -= existing.quantity;
                this._baseAmount += existing.quantity;
                delete this._openAskOrders[cancel.clientOrderId];
            }
            
            this.OrderUpdate.trigger({ orderId: cancel.clientOrderId, orderStatus: Models.OrderStatus.Cancelled });
            this.recomputePosition();
        }, moment.duration(3));
        
        return new Models.OrderGatewayActionReport(this.timeProvider.utcNow());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };
    
    private onMarketData = (market : Models.Market) => {
        this.timeProvider.scrollTimeTo(market.time);
        
        this._openAskOrders = this.tryToMatch(_.values(this._openAskOrders), market.bids, Models.Side.Ask);
        this._openBidOrders = this.tryToMatch(_.values(this._openBidOrders), market.asks, Models.Side.Bid);
        
        this.MarketData.trigger(market);
    };
    
    private tryToMatch = (orders: Models.BrokeredOrder[], marketSides: Models.MarketSide[], side: Models.Side) => {
        var cmp = side === Models.Side.Ask ? (m, o) => o < m : (m, o) => o > m;
        _.forEach(orders, order => {
            _.forEach(marketSides, mkt => {
                if (cmp(mkt.price, order.price) && order.quantity > 0) {
                    var update : Models.OrderStatusReport = { orderId: order.orderId, lastPrice: order.price };
                    if (mkt.size >= order.quantity) {
                        update.orderStatus = Models.OrderStatus.Complete;
                        update.lastQuantity = order.quantity;
                    }
                    else {
                        update.partiallyFilled = true;
                        update.orderStatus = Models.OrderStatus.Working;
                        update.lastQuantity = mkt.size;
                    }
                    this.OrderUpdate.trigger(update);
                    
                    if (side === Models.Side.Bid) {
                        this._baseAmount += update.lastQuantity;
                        this._quoteHeld -= (update.lastQuantity*order.price);
                    }
                    else {
                        this._baseHeld -= update.lastQuantity;
                        this._quoteAmount += (update.lastQuantity*order.price);
                    }
                    this.recomputePosition();
                    
                    order.quantity -= update.lastQuantity;
                };
            });
        });
        
        return _.indexBy(_.filter(orders, (o: Models.BrokeredOrder) => o.quantity > 0), k => k.orderId);
    };
    
    private onMarketTrade = (trade : Models.GatewayMarketTrade) => {
        this.timeProvider.scrollTimeTo(trade.time);
        this.MarketTrade.trigger(trade);
    };
    
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    recomputePosition = () => {
        this.PositionUpdate.trigger(new Models.CurrencyPosition(this._baseAmount, this._baseHeld, Models.Currency.BTC));
        this.PositionUpdate.trigger(new Models.CurrencyPosition(this._quoteAmount, this._quoteHeld, Models.Currency.USD));
    };
    
    private _baseHeld = 0;
    private _quoteHeld = 0;
    
    constructor(
            private _baseAmount : number,
            private _quoteAmount : number,
            private timeProvider: Utils.IBacktestingTimeProvider) {
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
    }
}