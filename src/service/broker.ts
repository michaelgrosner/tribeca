/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import Utils = require("./utils");
import _ = require("lodash");
import mongodb = require('mongodb');
import Q = require("q");
import momentjs = require('moment');
import Interfaces = require("./interfaces");

export class OrderStatusPersister {
     _log : Utils.Logger = Utils.log("tribeca:exchangebroker:persister");

    public getLatestStatuses = (last : number, exchange : Models.Exchange) : Q.Promise<Models.OrderStatusReport[]> => {
        var deferred = Q.defer<Models.OrderStatusReport[]>();
        this._db.then(db => {
            var selector : Models.OrderStatusReport = {exchange: exchange};
            db.collection('osr').find(selector, {}, {limit: last}, (err, docs) => {
                if (err) deferred.reject(err);
                else {
                    docs.toArray((err, arr) => {
                        if (err) {
                            deferred.reject(err);
                        }
                        else {
                            var cvtArr = _.map(arr, x => {
                                x.time = momentjs(x.time);
                                return x;
                            });
                            deferred.resolve(cvtArr);
                        }
                    });
                }
            });
        }).done();

        return deferred.promise;
    };

    public persist = (report : Models.OrderStatusReport) => {
        var rpt : any = report;
        rpt.time = rpt.time.toISOString();
        this._db.then(db => db.collection('osr').insert(rpt, (err, res) => {
            if (err)
                this._log("Unable to insert order %s; %o", report.orderId, err);
        })).done();
    };

    _db : Q.Promise<mongodb.Db>;
    constructor() {
        var deferred = Q.defer<mongodb.Db>();
        mongodb.MongoClient.connect('mongodb://localhost:27017/tribeca', (err, db) => {
            if (err) deferred.reject(err);
            else {
                deferred.resolve(db);
                this._log("Successfully connected to DB");
            }
        });
        this._db = deferred.promise;
    }
}

export class ExchangeBroker implements Interfaces.IBroker {
    _log : Utils.Logger;

    PositionUpdate = new Utils.Evt<Models.ExchangeCurrencyPosition>();
    private _currencies : { [currency : number] : Models.ExchangeCurrencyPosition } = {};
    public getPosition(currency : Models.Currency) : Models.ExchangeCurrencyPosition {
        return this._currencies[currency];
    }

    private onPositionUpdate = (rpt : Models.CurrencyPosition) => {
        if (typeof this._currencies[rpt.currency] === "undefined" || this._currencies[rpt.currency].amount != rpt.amount) {
            var newRpt = rpt.toExchangeReport(this.exchange());
            this._currencies[rpt.currency] = newRpt;
            this.PositionUpdate.trigger(newRpt);
            this._log("New currency report: %s", newRpt);
        }
    };

    cancelOpenOrders() : void {
        for (var k in this._allOrders) {
            if (!this._allOrders.hasOwnProperty(k)) continue;
            var e : Models.OrderStatusReport = _.last(this._allOrders[k]);

            switch (e.orderStatus) {
                case Models.OrderStatus.New:
                case Models.OrderStatus.Working:
                    this.cancelOrder(new Models.OrderCancel(e.orderId, e.exchange, Utils.date()));
                    break;
            }
        }
    }

    allOrderStates() : Array<Models.OrderStatusReport> {
        var os : Array<Models.OrderStatusReport> = [];
        for (var k in this._allOrders) {
            var e = this._allOrders[k];
            for (var i = 0; i < e.length; i++) {
                os.push(e[i]);
            }
        }
        return os;
    }

    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    _allOrders : { [orderId: string]: Models.OrderStatusReport[] } = {};
    _exchIdsToClientIds : { [exchId: string] : string} = {};

    private static generateOrderId = () => {
        // use moment.js?
        return new Date().getTime().toString(32)
    };

    sendOrder = (order : Models.SubmitNewOrder) : Models.SentOrder => {
        var orderId = ExchangeBroker.generateOrderId();
        var exch = this.exchange();
        var brokeredOrder = new Models.BrokeredOrder(orderId, order.side, order.quantity, order.type, order.price, order.timeInForce, exch);

        var sent = this._oeGateway.sendOrder(brokeredOrder);

        var rpt : Models.OrderStatusReport = {
            orderId: orderId,
            side: order.side,
            quantity: order.quantity,
            type: order.type,
            time: sent.sentTime,
            price: order.price,
            timeInForce: order.timeInForce,
            orderStatus: Models.OrderStatus.New,
            exchange: exch,
            computationalLatency: sent.sentTime.diff(order.generatedTime),
            rejectMessage: order.msg};
        this._allOrders[rpt.orderId] = [rpt];
        this.onOrderUpdate(rpt);

        return new Models.SentOrder(rpt.orderId);
    };

    replaceOrder = (replace : Models.CancelReplaceOrder) : Models.SentOrder => {
        var rpt = _.last(this._allOrders[replace.origOrderId]);
        var br = new Models.BrokeredReplace(replace.origOrderId, replace.origOrderId, rpt.side,
            replace.quantity, rpt.type, replace.price, rpt.timeInForce, rpt.exchange, rpt.exchangeId);

        var sent = this._oeGateway.replaceOrder(br);

        var rpt : Models.OrderStatusReport = {
            orderId: replace.origOrderId,
            orderStatus: Models.OrderStatus.Working,
            pendingReplace: true,
            price: replace.price,
            quantity: replace.quantity,
            time: sent.sentTime,
            computationalLatency: sent.sentTime.diff(replace.generatedTime)};
        this.onOrderUpdate(rpt);

        return new Models.SentOrder(rpt.orderId);
    };

    cancelOrder = (cancel : Models.OrderCancel) => {
        var rpt = _.last(this._allOrders[cancel.origOrderId]);
        var cxl = new Models.BrokeredCancel(cancel.origOrderId, ExchangeBroker.generateOrderId(), rpt.side, rpt.exchangeId);
        var sent = this._oeGateway.cancelOrder(cxl);

        var rpt : Models.OrderStatusReport = {
            orderId: cancel.origOrderId,
            orderStatus: Models.OrderStatus.Working,
            pendingCancel: true,
            time: sent.sentTime,
            computationalLatency: sent.sentTime.diff(cancel.generatedTime)};
        this.onOrderUpdate(rpt);
    };

    public onOrderUpdate = (osr : Models.OrderStatusReport) => {
        var orderChain = this._allOrders[osr.orderId];

        if (typeof orderChain === "undefined") {
            // this step and _exchIdsToClientIds is really BS, the exchanges should get their act together
            var secondChance = this._exchIdsToClientIds[osr.exchangeId];
            if (typeof secondChance !== "undefined") {
                osr.orderId = secondChance;
                orderChain = this._allOrders[secondChance];
            }
        }

        if (typeof orderChain === "undefined") {
            var keys = [];
            for (var k in this._allOrders)
                if (this._allOrders.hasOwnProperty(k))
                    keys.push(k);
            this._log("ERROR: cannot find orderId from %o, existing: %o", osr, keys);
        }

        var orig : Models.OrderStatusReport = _.last(orderChain);

        var cumQuantity = osr.cumQuantity || orig.cumQuantity;
        var quantity = osr.quantity || orig.quantity;
        var partiallyFilled = cumQuantity > 0 && cumQuantity !== quantity;

        var o = new Models.OrderStatusReportImpl(
            osr.side || orig.side,
            quantity,
            osr.type || orig.type,
            osr.price || orig.price,
            osr.timeInForce || orig.timeInForce,
            osr.orderId || orig.orderId,
            osr.exchangeId || orig.exchangeId,
            osr.orderStatus || orig.orderStatus,
            osr.rejectMessage,
            osr.time || Utils.date(),
            osr.lastQuantity,
            osr.lastPrice,
            cumQuantity > 0 ? osr.leavesQuantity || orig.leavesQuantity : undefined,
            cumQuantity,
            cumQuantity > 0 ? osr.averagePrice || orig.averagePrice : undefined,
            osr.liquidity,
            osr.exchange || orig.exchange,
            osr.computationalLatency,
            (typeof orig.version === "undefined") ? 0 : orig.version + 1,
            partiallyFilled,
            osr.pendingCancel,
            osr.pendingReplace,
            osr.cancelRejected
        );

        this._exchIdsToClientIds[osr.exchangeId] = osr.orderId;
        this._allOrders[osr.orderId].push(o);

        this.OrderUpdate.trigger(o);

        this._log("applied gw update -> %s", o);
        this._persister.persist(o);
    };

    makeFee() : number {
        return this._baseGateway.makeFee();
    }

    takeFee() : number {
        return this._baseGateway.takeFee();
    }


    name() : string {
        return this._baseGateway.name();
    }

    exchange() : Models.Exchange {
        return this._baseGateway.exchange();
    }

    private static _pair = new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD);
    public get pair() {
        return ExchangeBroker._pair;
    }

    MarketData = new Utils.Evt<Models.Market>();
    _currentBook : Models.Market = null;

    public get currentBook() : Models.Market {
        return this._currentBook;
    }

    private handleMarketData = (book : Models.Market) => {
        var bids : Models.MarketSide[] = [];
        var asks : Models.MarketSide[] = [];
        var isDupe = true;
        for (var i = 0; i < 5; i++) {
            var bid = book.bids[i];
            var ask = book.asks[i];

            bids.push(bid);
            asks.push(ask);

            if (isDupe) {
                if (this.currentBook !== null) {
                    if (!Models.marketSideEquals(bid, this.currentBook.bids[i]) ||
                        !Models.marketSideEquals(ask, this.currentBook.asks[i])) {
                        isDupe = false;
                    }
                }
                else {
                    isDupe = false;
                }
            }
        }

        if (isDupe) return;

        var truncatedMkt = new Models.Market(bids, asks, book.time);

        this._currentBook = book;
        this.MarketData.trigger(this.currentBook);
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private mdConnected = Models.ConnectivityStatus.Disconnected;
    private oeConnected = Models.ConnectivityStatus.Disconnected;
    private _connectStatus = Models.ConnectivityStatus.Disconnected;
    public onConnect = (gwType : Models.GatewayType, cs : Models.ConnectivityStatus) => {
        if (gwType == Models.GatewayType.MarketData) this.mdConnected = cs;
        if (gwType == Models.GatewayType.OrderEntry) this.oeConnected = cs;

        var newStatus = this.mdConnected == Models.ConnectivityStatus.Connected && this.oeConnected == Models.ConnectivityStatus.Connected
            ? Models.ConnectivityStatus.Connected
            : Models.ConnectivityStatus.Disconnected;

        if (newStatus != this._connectStatus)
            this.ConnectChanged.trigger(newStatus);
        else
            return;

        this._connectStatus = newStatus;
        this._log(Models.GatewayType[gwType], "Connection status changed ", Models.ConnectivityStatus[cs]);
    };

    public get connectStatus() : Models.ConnectivityStatus {
        return this._connectStatus;
    }

    constructor(private _mdGateway : Interfaces.IMarketDataGateway,
                private _baseGateway : Interfaces.IExchangeDetailsGateway,
                private _oeGateway : Interfaces.IOrderEntryGateway,
                private _posGateway : Interfaces.IPositionGateway,
                private _persister : OrderStatusPersister) {
        this._log = Utils.log("tribeca:exchangebroker:" + this._baseGateway.name());

        this._mdGateway.MarketData.on(this.handleMarketData);
        this._mdGateway.ConnectChanged.on(s => {
            if (s == Models.ConnectivityStatus.Disconnected) this._currentBook = null;
            this.onConnect(Models.GatewayType.MarketData, s);
        });

        this._oeGateway.OrderUpdate.on(this.onOrderUpdate);
        this._oeGateway.ConnectChanged.on(s => {
            this.onConnect(Models.GatewayType.OrderEntry, s)
        });

        this._posGateway.PositionUpdate.on(this.onPositionUpdate);

        this._persister.getLatestStatuses(1000, this.exchange()).then(osrs => {
            _.each(osrs, osr => {
                this._exchIdsToClientIds[osr.exchangeId] = osr.orderId;

                if (!this._allOrders.hasOwnProperty(osr.orderId))
                    this._allOrders[osr.orderId] = [osr];
                else
                    this._allOrders[osr.orderId].push(osr);
            });
        });
    }
}