/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="utils.ts"/>
/// <reference path="interfaces.ts"/>
/// <reference path="persister.ts"/>
/// <reference path="messages.ts"/>

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import mongodb = require('mongodb');
import Q = require("q");
import Interfaces = require("./interfaces");
import Persister = require("./persister");
import util = require("util");
import Messages = require("./messages");

export class MarketDataBroker implements Interfaces.IMarketDataBroker {
    MarketData = new Utils.Evt<Models.Market>();
    public get currentBook() : Models.Market { return this._currentBook; }

    private _currentBook : Models.Market = null;
    private handleMarketData = (book : Models.Market) => {
        this._currentBook = book;
        this.MarketData.trigger(this.currentBook);
        this._marketPublisher.publish(this.currentBook);
        this._persister.persist(this.currentBook);
    };

    constructor(private _mdGateway : Interfaces.IMarketDataGateway,
                private _marketPublisher : Messaging.IPublish<Models.Market>,
                private _persister: Persister.IPersist<Models.Market>,
                private _messages : Messages.MessagesPubisher) {
        _marketPublisher.registerSnapshot(() => this.currentBook === null ? [] : [this.currentBook]);

        this._mdGateway.MarketData.on(this.handleMarketData);
        this._mdGateway.ConnectChanged.on(s => {
            if (s == Models.ConnectivityStatus.Disconnected) this._currentBook = null;
            _messages.publish("MD gw " + Models.ConnectivityStatus[s]);
        });
    }
}

export class OrderStateCache implements Interfaces.IOrderStateCache {
    public allOrders : { [orderId: string]: Models.OrderStatusReport[] } = {};
    public allOrdersFlat : Models.OrderStatusReport[] = [];
    public exchIdsToClientIds : { [exchId: string] : string} = {};
}

export class OrderBroker implements Interfaces.IOrderBroker {
    private _log = Utils.log("oe:broker");

    cancelOpenOrders() : Q.Promise<number> {
        if (this._oeGateway.supportsCancelAllOpenOrders()) {
            return this._oeGateway.cancelAllOpenOrders();
        }
        
        var deferred = Q.defer<number>();

        var lateCancels : {[id: string] : boolean} = {};
        for (var k in this._orderCache.allOrders) {
            if (!(k in this._orderCache.allOrders)) continue;
            var e : Models.OrderStatusReport = _.last(this._orderCache.allOrders[k]);

            if (e.pendingCancel) continue;

            switch (e.orderStatus) {
                case Models.OrderStatus.New:
                case Models.OrderStatus.Working:
                    this.cancelOrder(new Models.OrderCancel(e.orderId, e.exchange, this._timeProvider.utcNow()));
                    lateCancels[e.orderId] = false;
                    break;
            }
        }

        if (_.isEmpty(_.keys(lateCancels))) {
            deferred.resolve(0);
        }

        this.OrderUpdate.on(o => {
            if (o.orderStatus === Models.OrderStatus.New || o.orderStatus === Models.OrderStatus.Working)
                return;

            lateCancels[e.orderId] = true;
            if (_.every(_.values(lateCancels)))
                deferred.resolve(_.size(lateCancels));
        });

        return deferred.promise;
    }

    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    private _cancelsWaitingForExchangeOrderId : {[clId : string] : Models.OrderCancel} = {};

    Trade = new Utils.Evt<Models.Trade>();
    _trades : Models.Trade[] = [];

    sendOrder = (order : Models.SubmitNewOrder) : Models.SentOrder => {
        var orderId = this._oeGateway.generateClientOrderId();
        var exch = this._baseBroker.exchange();
        var brokeredOrder = new Models.BrokeredOrder(orderId, order.side, order.quantity, order.type, 
            order.price, order.timeInForce, exch, order.preferPostOnly);

        var sent = this._oeGateway.sendOrder(brokeredOrder);

        var rpt : Models.OrderStatusReport = {
            pair: this._baseBroker.pair,
            orderId: orderId,
            side: order.side,
            quantity: order.quantity,
            type: order.type,
            time: sent.sentTime,
            price: order.price,
            timeInForce: order.timeInForce,
            orderStatus: Models.OrderStatus.New,
            preferPostOnly: order.preferPostOnly,
            exchange: exch,
            computationalLatency: Utils.fastDiff(sent.sentTime, order.generatedTime),
            rejectMessage: order.msg};
        this.onOrderUpdate(rpt);

        return new Models.SentOrder(rpt.orderId);
    };

    replaceOrder = (replace : Models.CancelReplaceOrder) : Models.SentOrder => {
        var rpt = _.last(this._orderCache.allOrders[replace.origOrderId]);
        var br = new Models.BrokeredReplace(replace.origOrderId, replace.origOrderId, rpt.side, replace.quantity, 
            rpt.type, replace.price, rpt.timeInForce, rpt.exchange, rpt.exchangeId, rpt.preferPostOnly);

        var sent = this._oeGateway.replaceOrder(br);

        var rpt : Models.OrderStatusReport = {
            orderId: replace.origOrderId,
            orderStatus: Models.OrderStatus.Working,
            pendingReplace: true,
            price: replace.price,
            quantity: replace.quantity,
            time: sent.sentTime,
            computationalLatency: Utils.fastDiff(sent.sentTime, replace.generatedTime)};
        this.onOrderUpdate(rpt);

        return new Models.SentOrder(rpt.orderId);
    };

    cancelOrder = (cancel : Models.OrderCancel) => {
        var rpt = _.last(this._orderCache.allOrders[cancel.origOrderId]);

        if (!this._oeGateway.cancelsByClientOrderId) {
            // race condition! i cannot cancel an order before I get the exchangeId (oid); register it for deletion on the ack
            if (typeof rpt.exchangeId === "undefined") {
                this._cancelsWaitingForExchangeOrderId[rpt.orderId] = cancel;
                this._log.info("Registered %s for late deletion", rpt.orderId);
                return;
            }
        }

        var cxl = new Models.BrokeredCancel(cancel.origOrderId, this._oeGateway.generateClientOrderId(), rpt.side, rpt.exchangeId);
        var sent = this._oeGateway.cancelOrder(cxl);

        var rpt : Models.OrderStatusReport = {
            orderId: cancel.origOrderId,
            orderStatus: Models.OrderStatus.Working,
            pendingCancel: true,
            time: sent.sentTime,
            computationalLatency: Utils.fastDiff(sent.sentTime, cancel.generatedTime)};
        this.onOrderUpdate(rpt);
    };

    public onOrderUpdate = (osr : Models.OrderStatusReport) => {
        var orig : Models.OrderStatusReport;
        if (osr.orderStatus === Models.OrderStatus.New) {
            orig = osr;
        }
        else {
            var orderChain = this._orderCache.allOrders[osr.orderId];

            if (typeof orderChain === "undefined") {
                // this step and _exchIdsToClientIds is really BS, the exchanges should get their act together
                var secondChance = this._orderCache.exchIdsToClientIds[osr.exchangeId];
                if (typeof secondChance !== "undefined") {
                    osr.orderId = secondChance;
                    orderChain = this._orderCache.allOrders[secondChance];
                }
            }

            if (typeof orderChain === "undefined") {
                this._log.error("cannot find orderId from", osr);
                return;
            }

            orig = _.last(orderChain);
        }

        var getOrFallback = (n, o) => typeof n !== "undefined" ? n : o;

        var quantity = getOrFallback(osr.quantity, orig.quantity);
        var leavesQuantity = getOrFallback(osr.leavesQuantity, orig.leavesQuantity);

        var cumQuantity : number = undefined;
        if (typeof osr.cumQuantity !== "undefined") {
            cumQuantity = getOrFallback(osr.cumQuantity, orig.cumQuantity);
        }
        else {
            cumQuantity = getOrFallback(orig.cumQuantity, 0) + getOrFallback(osr.lastQuantity, 0);
        }

        var partiallyFilled = cumQuantity > 0 && cumQuantity !== quantity;

        var o = new Models.OrderStatusReportImpl(
            getOrFallback(osr.pair, orig.pair),
            getOrFallback(osr.side, orig.side),
            quantity,
            getOrFallback(osr.type, orig.type),
            getOrFallback(osr.price, orig.price),
            getOrFallback(osr.timeInForce, orig.timeInForce),
            getOrFallback(osr.orderId, orig.orderId),
            getOrFallback(osr.exchangeId, orig.exchangeId),
            getOrFallback(osr.orderStatus, orig.orderStatus),
            osr.rejectMessage,
            getOrFallback(osr.time, this._timeProvider.utcNow()),
            osr.lastQuantity,
            osr.lastPrice,
            leavesQuantity,
            cumQuantity,
            cumQuantity > 0 ? osr.averagePrice || orig.averagePrice : undefined,
            getOrFallback(osr.liquidity, orig.liquidity),
            getOrFallback(osr.exchange, orig.exchange),
            getOrFallback(osr.computationalLatency, 0) + getOrFallback(orig.computationalLatency, 0),
            (typeof orig.version === "undefined") ? 0 : orig.version + 1,
            partiallyFilled,
            osr.pendingCancel,
            osr.pendingReplace,
            osr.cancelRejected,
            getOrFallback(osr.preferPostOnly, orig.preferPostOnly)
        );

        this.addOrderStatusToMemory(o);

        // cancel any open orders waiting for oid
        if (!this._oeGateway.cancelsByClientOrderId
                && typeof o.exchangeId !== "undefined"
                && o.orderId in this._cancelsWaitingForExchangeOrderId) {
            this._log.info("Deleting %s late, oid: %s", o.exchangeId, o.orderId);
            var cancel = this._cancelsWaitingForExchangeOrderId[o.orderId];
            delete this._cancelsWaitingForExchangeOrderId[o.orderId];
            this.cancelOrder(cancel);
        }

        this.OrderUpdate.trigger(o);

        this._orderPersister.persist(o);
        this._orderStatusPublisher.publish(o);

        if (osr.lastQuantity > 0) {
            let value = Math.abs(o.lastPrice * o.lastQuantity);

            const liq = o.liquidity;
            let feeCharged = null;
            if (typeof liq !== "undefined") {
                // negative fee is a rebate, positive fee is a fee
                feeCharged = (liq === Models.Liquidity.Make ? this._baseBroker.makeFee() : this._baseBroker.takeFee());
                const sign = (o.side === Models.Side.Bid ? 1 : -1);
                value = value * (1 + sign * feeCharged);
            }

            const trade = new Models.Trade(o.orderId+"."+o.version, o.time, o.exchange, o.pair, 
                o.lastPrice, o.lastQuantity, o.side, value, o.liquidity, feeCharged);
            this.Trade.trigger(trade);
            this._tradePublisher.publish(trade);
            this._tradePersister.persist(trade);
            this._trades.push(trade);
        }
    };

    private addOrderStatusToMemory = (osr : Models.OrderStatusReport) => {
        this._orderCache.exchIdsToClientIds[osr.exchangeId] = osr.orderId;

        if (!(osr.orderId in this._orderCache.allOrders))
            this._orderCache.allOrders[osr.orderId] = [osr];
        else
            this._orderCache.allOrders[osr.orderId].push(osr);

        this._orderCache.allOrdersFlat.push(osr);
    };

    constructor(private _timeProvider: Utils.ITimeProvider,
                private _baseBroker : Interfaces.IBroker,
                private _oeGateway : Interfaces.IOrderEntryGateway,
                private _orderPersister : Persister.IPersist<Models.OrderStatusReport>,
                private _tradePersister : Persister.IPersist<Models.Trade>,
                private _orderStatusPublisher : Messaging.IPublish<Models.OrderStatusReport>,
                private _tradePublisher : Messaging.IPublish<Models.Trade>,
                private _submittedOrderReciever : Messaging.IReceive<Models.OrderRequestFromUI>,
                private _cancelOrderReciever : Messaging.IReceive<Models.OrderStatusReport>,
                private _cancelAllOrdersReciever : Messaging.IReceive<Models.CancelAllOrdersRequest>,
                private _messages : Messages.MessagesPubisher,
                private _orderCache : OrderStateCache,
                initOrders : Models.OrderStatusReport[],
                initTrades : Models.Trade[]) {
        _orderStatusPublisher.registerSnapshot(() => _.takeRight(this._orderCache.allOrdersFlat, 1000));
        _tradePublisher.registerSnapshot(() => _.takeRight(this._trades, 100));

        _submittedOrderReciever.registerReceiver((o : Models.OrderRequestFromUI) => {
            this._log.info("got new order req", o);
            try {
                var order = new Models.SubmitNewOrder(Models.Side[o.side], o.quantity, Models.OrderType[o.orderType],
                    o.price, Models.TimeInForce[o.timeInForce], this._baseBroker.exchange(), _timeProvider.utcNow(), false);
                this.sendOrder(order);
            }
            catch (e) {
                this._log.error(e, "unhandled exception while submitting order", o);
            }
        });
        
        _cancelOrderReciever.registerReceiver(o => {
            this._log.info("got new cancel req", o);
            try {
                this.cancelOrder(new Models.OrderCancel(o.orderId, o.exchange, _timeProvider.utcNow()));    
            } catch (e) {
                this._log.error(e, "unhandled exception while submitting order", o);
            }
        });
        
        _cancelAllOrdersReciever.registerReceiver(o => {
            this._log.info("handling cancel all orders request");
            this.cancelOpenOrders()
                .then(x => this._log.info("cancelled all ", x, " open orders"), 
                      e => this._log.error(e, "error when cancelling all orders!"));
        });

        this._oeGateway.OrderUpdate.on(this.onOrderUpdate);

        _.each(initOrders, this.addOrderStatusToMemory);
        this._log.info("loaded %d osrs from %d orders", this._orderCache.allOrdersFlat.length, Object.keys(this._orderCache.allOrders).length);

        _.each(initTrades, t => this._trades.push(t));
        this._log.info("loaded %d trades", this._trades.length);

        this._oeGateway.ConnectChanged.on(s => {
            _messages.publish("OE gw " + Models.ConnectivityStatus[s]);
        });
    }
}

export class PositionBroker implements Interfaces.IPositionBroker {
    private _log = Utils.log("pos:broker");

    public NewReport = new Utils.Evt<Models.PositionReport>();

    private _report : Models.PositionReport = null;
    public get latestReport() : Models.PositionReport {
        return this._report;
    }

    private _currencies : { [currency : number] : Models.CurrencyPosition } = {};
    public getPosition(currency : Models.Currency) : Models.CurrencyPosition {
        return this._currencies[currency];
    }

    private onPositionUpdate = (rpt : Models.CurrencyPosition) => {
        this._currencies[rpt.currency] = rpt;
        var basePosition = this.getPosition(this._base.pair.base);
        var quotePosition = this.getPosition(this._base.pair.quote);

        if (typeof basePosition === "undefined"
            || typeof quotePosition === "undefined"
            || this._mdBroker.currentBook === null
            || this._mdBroker.currentBook.bids.length === 0
            || this._mdBroker.currentBook.asks.length === 0)
            return;

        var baseAmount = basePosition.amount;
        var quoteAmount = quotePosition.amount;
        var mid = (this._mdBroker.currentBook.bids[0].price + this._mdBroker.currentBook.asks[0].price) / 2.0;
        var baseValue = baseAmount + quoteAmount / mid + basePosition.heldAmount + quotePosition.heldAmount / mid;
        var quoteValue = baseAmount * mid + quoteAmount + basePosition.heldAmount * mid + quotePosition.heldAmount;
        var positionReport = new Models.PositionReport(baseAmount, quoteAmount, basePosition.heldAmount,
            quotePosition.heldAmount, baseValue, quoteValue, this._base.pair, this._base.exchange(), this._timeProvider.utcNow());

        if (this._report !== null && 
                Math.abs(positionReport.value - this._report.value) < 2e-2 && 
                Math.abs(baseAmount - this._report.baseAmount) < 2e-2 &&
                Math.abs(positionReport.baseHeldAmount - this._report.baseHeldAmount) < 2e-2 &&
                Math.abs(positionReport.quoteHeldAmount - this._report.quoteHeldAmount) < 2e-2)
            return;

        this._report = positionReport;
        this.NewReport.trigger(positionReport);
        this._positionPublisher.publish(positionReport);
        this._positionPersister.persist(positionReport);
    };

    constructor(private _timeProvider: Utils.ITimeProvider,
                private _base : Interfaces.IBroker,
                private _posGateway : Interfaces.IPositionGateway,
                private _positionPublisher : Messaging.IPublish<Models.PositionReport>,
                private _positionPersister : Persister.IPersist<Models.PositionReport>,
                private _mdBroker : Interfaces.IMarketDataBroker) {
        this._posGateway.PositionUpdate.on(this.onPositionUpdate);

        this._positionPublisher.registerSnapshot(() => (this._report === null ? [] : [this._report]));
    }
}

export class ExchangeBroker implements Interfaces.IBroker {
    private _log = Utils.log("ex:broker");

    public get hasSelfTradePrevention() {
        return this._baseGateway.hasSelfTradePrevention;
    }

    makeFee() : number {
        return this._baseGateway.makeFee();
    }

    takeFee() : number {
        return this._baseGateway.takeFee();
    }

    exchange() : Models.Exchange {
        return this._baseGateway.exchange();
    }

    public get pair() {
        return this._pair;
    }
    
    public get supportedCurrencyPairs() : Models.CurrencyPair[] {
        return this._baseGateway.supportedCurrencyPairs;
    }

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private mdConnected = Models.ConnectivityStatus.Disconnected;
    private oeConnected = Models.ConnectivityStatus.Disconnected;
    private _connectStatus = Models.ConnectivityStatus.Disconnected;
    public onConnect = (gwType : Models.GatewayType, cs : Models.ConnectivityStatus) => {
        if (gwType === Models.GatewayType.MarketData) {
            if (this.mdConnected === cs) return;
            this.mdConnected = cs;
        }

        if (gwType === Models.GatewayType.OrderEntry) {
            if (this.oeConnected === cs) return;
            this.oeConnected = cs;
        }

        var newStatus = this.mdConnected === Models.ConnectivityStatus.Connected && this.oeConnected === Models.ConnectivityStatus.Connected
            ? Models.ConnectivityStatus.Connected
            : Models.ConnectivityStatus.Disconnected;

        this._connectStatus = newStatus;
        this.ConnectChanged.trigger(newStatus);

        this._log.info("Connection status changed :: %s :: (md: %s) (oe: %s)", Models.ConnectivityStatus[this._connectStatus],
            Models.ConnectivityStatus[this.mdConnected], Models.ConnectivityStatus[this.oeConnected]);
        this._connectivityPublisher.publish(this.connectStatus);
    };

    public get connectStatus() : Models.ConnectivityStatus {
        return this._connectStatus;
    }

    constructor(private _pair : Models.CurrencyPair,
                private _mdGateway : Interfaces.IMarketDataGateway,
                private _baseGateway : Interfaces.IExchangeDetailsGateway,
                private _oeGateway : Interfaces.IOrderEntryGateway,
                private _connectivityPublisher : Messaging.IPublish<Models.ConnectivityStatus>) {
        this._mdGateway.ConnectChanged.on(s => {
            this.onConnect(Models.GatewayType.MarketData, s);
        });

        this._oeGateway.ConnectChanged.on(s => {
            this.onConnect(Models.GatewayType.OrderEntry, s)
        });

        this._connectivityPublisher.registerSnapshot(() => [this.connectStatus]);
    }
}
