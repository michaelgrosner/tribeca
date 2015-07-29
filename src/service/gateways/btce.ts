/// <reference path="../../../typings/tsd.d.ts" />
/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("../config");
import crypto = require('crypto');
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import moment = require("moment");
import util = require("util");
import Q = require("q");
import _ = require('lodash');

var shortId = require("shortid");

class BtcEPublicApiClient {
    private _baseUrl: string;
    constructor(config: Config.IConfigProvider) {
        this._baseUrl = config.GetString("BtcEPublicAPIRestUrl") + "/";
    }

    public getFromEndpoint = <TResponse>(endpoint: string, params: any = null): Q.Promise<TResponse> => {
        var options: request.Options = {
            url: this._baseUrl + endpoint,
            method: "GET"
        };

        if (params !== null)
            options.qs = params;

        var d = Q.defer<TResponse>();
        request(options, (err, resp, body) => {
            if (err) d.reject(err);
            else d.resolve(JSON.parse(body));
        });
        return d.promise;
    };
}

interface OrderBook {
    bids: [number, number][];
    asks: [number, number][];
}

interface MarketTrade {
    type: string;
    price: number;
    amount: number;
    tid: number;
    timestamp: number;
}

class BtcEMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();

    private static ConvertToMarketSide = (input: [number, number]) =>
        new Models.MarketSide(input[0], input[1]);

    private static ConvertToMarketSideList = (input: [number, number][]) =>
        input.map(BtcEMarketDataGateway.ConvertToMarketSide);

    private onRefreshMarketData = () => {
        this._client.getFromEndpoint("depth/" + this._pairKey, { limit: 5 }).then(p => {
            var orderBook: OrderBook = p[this._pairKey];
            var bids = BtcEMarketDataGateway.ConvertToMarketSideList(orderBook.bids);
            var asks = BtcEMarketDataGateway.ConvertToMarketSideList(orderBook.asks);
            var market = new Models.Market(bids, asks, moment.utc());
            this.MarketData.trigger(market);
        }).done();
    };

    MarketTrade = new Utils.Evt<Models.MarketSide>();

    private static ConvertToMarketTrade = (trd: MarketTrade): Models.GatewayMarketTrade =>
        new Models.GatewayMarketTrade(trd.price, trd.amount, moment.unix(trd.timestamp), false, null);

    private _seenMarketTradeIds: { [tid: number]: boolean } = {};
    private onRefreshMarketTrades = () => {
        this._client.getFromEndpoint("trades/" + this._pairKey, { limit: 5 }).then(p => {
            var tradesList: MarketTrade[] = p[this._pairKey];
            tradesList.forEach(t => {
                if (this._seenMarketTradeIds[t.tid] === true) return;
                this._seenMarketTradeIds[t.tid] = true;
                this.MarketTrade.trigger(BtcEMarketDataGateway.ConvertToMarketTrade(t));
            });
        });
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private _log: Utils.Logger = Utils.log("tribeca:gateway:BtcEMD");
    constructor(
        private _pairKey: string,
        private _timeProvider: Utils.ITimeProvider,
        private _client: BtcEPublicApiClient) {
        _timeProvider.setInterval(this.onRefreshMarketData, moment.duration(200, "seconds"));
        _timeProvider.setInterval(this.onRefreshMarketTrades, moment.duration(200, "seconds"));

        _timeProvider.setImmediate(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
    }
}

interface Response<T> {
    success: number;
    return?: T;
    error?: string;
}

class BtcEAuthenticatedApiClient {
    private _log: Utils.Logger = Utils.log("tribeca:gateway:BtcAuthClient");
    private _baseUrl: string;
    private _key: string;
    private _secret: string;

    constructor(config: Config.IConfigProvider) {
        this._baseUrl = config.GetString("BtcETradeAPIRestUrl") + "/";
        this._key = config.GetString("BtcEKey");
        this._secret = config.GetString("BtcESecret");
    }

    private _lastTimeMs: number = 1;
    private getNonce = () => {
        var t = Math.round(new Date().getTime() / 1000);
        if (t <= this._lastTimeMs) {
            this._lastTimeMs++;
        }
        else {
            this._lastTimeMs = t;
        }

        return this._lastTimeMs;
    };

    public postToEndpoint = <TRequest, TResponse>(method: string, params: TRequest): Q.Promise<Response<TResponse>> => {
        var formData: any = {};
        for (var key in params) {
            formData[key] = params[key];
        }
        formData.method = method;
        formData.nonce = this.getNonce();

        var form = querystring.stringify(formData);
        var sign = crypto.createHmac('sha512', this._secret).update(new Buffer(form)).digest('hex').toString();

        var options: request.Options = {
            url: this._baseUrl + method,
            method: "POST",
            form: form,
            headers: {
                Sign: sign,
                Key: this._key
            }
        };

        this._log("OUT", util.inspect(options));

        var d = Q.defer<Response<TResponse>>();
        request(options, (err, resp, body) => {
            if (err) d.reject(err);
            else {
                this._log("IN", body);
                d.resolve(JSON.parse(body));
            }
        });
        return d.promise;
    };
}

// aka new order
interface Trade {
    pair: string;
    type: string; // buy or sell
    rate: number;
    amount: number;
}

// aka order ack
interface TradeResponse {
    received: number;
    remains: number;
    order_id: number;
    funds: Object;
}

interface CancelOrder {
    order_id: number;
}

interface ActiveOrderRequest {
    order_id: number;
}

interface ActiveOrder {
    pair: string;
    type: string;   // buy or sell
    start_amount: number;
    amount: number;
    rate: number;
    timestamp_created: number;
    status: number;
}

interface CancelOrderAck {
    order_id: number;
    funds: { [currency: string]: number };
}

class BtcEOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    public cancelsByClientOrderId = false;

    cancelOrder = (cancel: Models.BrokeredCancel): Models.OrderGatewayActionReport => {
        var req = { order_id: parseInt(cancel.exchangeId) };
        this._client.postToEndpoint<CancelOrder, CancelOrderAck>("CancelOrder", req).then(r => {
            if (r.success === 1) {
                this.raiseOrderUpdate({
                    exchangeId: cancel.exchangeId,
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Cancelled
                });
            }
            else {
                this.raiseOrderUpdate({
                    exchangeId: cancel.exchangeId,
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Rejected,
                    rejectMessage: r.error
                });
            }
        }).done();
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace: Models.BrokeredReplace): Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order: Models.BrokeredOrder): Models.OrderGatewayActionReport => {
        var t: Trade = {
            pair: this._pairKey,
            type: order.side === Models.Side.Bid ? "buy" : "sell",
            rate: order.price,
            amount: order.quantity
        };

        this._client.postToEndpoint<Trade, TradeResponse>("Trade", t).then(r => {
            if (r.success === 1) {
                var rpt: Models.OrderStatusReport = {
                    orderId: order.orderId,
                    leavesQuantity: r.return.remains,
                    partiallyFilled: Math.abs(r.return.received - r.return.remains) > 1e-4
                };

                var exchangeOrderId = r.return.order_id;
                if (exchangeOrderId === 0) {
                    rpt.orderStatus = Models.OrderStatus.Complete;
                }
                else {
                    rpt.orderStatus = Models.OrderStatus.Working;
                    rpt.exchangeId = exchangeOrderId.toString();
                }

                this.raiseOrderUpdate(rpt);
            }
            else {
                this.raiseOrderUpdate({
                    orderId: order.orderId,
                    orderStatus: Models.OrderStatus.Rejected,
                    rejectMessage: r.error
                });
            }
        });

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private refreshOpenOrders = () => {
        _.forIn(this._openOrderExchangeIds, (unused, id) => {
            var req = { order_id: parseInt(id) };
            this._client.postToEndpoint<ActiveOrderRequest, ActiveOrder>("OrderInfo", req).then(r => {
                if (r.success === 1) {
                    this.raiseOrderUpdate({
                        exchangeId: id,
                        leavesQuantity: r.return.amount,
                        partiallyFilled: Math.abs(r.return.start_amount - r.return.amount) > 1e-4
                    });
                }
                else {
                    this._log("OrderInfo request for order " + id + " did not return successfully: " + r.error);
                }
            });
        });
    };

    private raiseOrderUpdate = (rpt: Models.OrderStatusReport) => {
        if (typeof rpt.exchangeId !== "undefined") {
            var id = parseInt(rpt.exchangeId);
            switch (rpt.orderStatus) {
                case Models.OrderStatus.New:
                case Models.OrderStatus.Working:
                    this._openOrderExchangeIds[id] = true;
                case Models.OrderStatus.Cancelled:
                case Models.OrderStatus.Complete:
                case Models.OrderStatus.Rejected:
                case Models.OrderStatus.Other:
                    delete this._openOrderExchangeIds[id];
            }
        }

        this.OrderUpdate.trigger(rpt);
    };
    
    private static ConvertOrderStatus(st: number) : Models.OrderStatus {
        switch (st) {
            case 0: return Models.OrderStatus.Working;
            case 1: return Models.OrderStatus.Complete;
            case 2: return Models.OrderStatus.Cancelled;
            case 3: return Models.OrderStatus.Cancelled;
            default: return Models.OrderStatus.Other;
        }
    }

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => shortId.generate();

    private _log: Utils.Logger = Utils.log("tribeca:gateway:BtcEOE");
    private _openOrderExchangeIds: { [id: number]: boolean } = {};
    constructor(
        private _pairKey: string,
        private _timeProvider: Utils.ITimeProvider,
        private _client: BtcEAuthenticatedApiClient) {
        this._timeProvider.setImmediate(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._timeProvider.setInterval(this.refreshOpenOrders, moment.duration(5, "seconds"));
    }
}

class BtcEPositionGateway implements Interfaces.IPositionGateway {
    _log: Utils.Logger = Utils.log("tribeca:gateway:BtcEPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    constructor(
        private _pairKey: string,
        private _timeProvider: Utils.ITimeProvider,
        private _client: BtcEAuthenticatedApiClient) {
    }
}

class BtcEBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.BtcE;
    }

    makeFee(): number {
        return -0.0001;
    }

    takeFee(): number {
        return 0.001;
    }

    name(): string {
        return "BtcE";
    }
}

export class BtcE extends Interfaces.CombinedGateway {
    constructor(pair: Models.CurrencyPair, timeProvider: Utils.ITimeProvider, config: Config.IConfigProvider) {
        var publicClient = new BtcEPublicApiClient(config);
        var authClient = new BtcEAuthenticatedApiClient(config);
        var pairKey = Models.Currency[pair.base].toLowerCase() + "_" + Models.Currency[pair.quote].toLowerCase();
        super(
            new BtcEMarketDataGateway(pairKey, timeProvider, publicClient),
            new BtcEOrderEntryGateway(pairKey, timeProvider, authClient),
            new BtcEPositionGateway(pairKey, timeProvider, authClient),
            new BtcEBaseGateway());
    }
}
