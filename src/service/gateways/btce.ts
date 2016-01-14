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

interface TradeHistory {
    pair: string;
    type: string;
    amount: number;
    rate: number;
    order_id: number;
    is_your_order: number;
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
            
        var setTimer = (func) => {
            _timeProvider.setTimeout(func, moment.duration(5));
            _timeProvider.setInterval(func, moment.duration(5, "seconds"));
        };
        
        setTimer(this.onRefreshMarketData);
        setTimer(this.onRefreshMarketTrades);
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

    public postToEndpoint = <TRequest, TResponse>(method: string, params: TRequest): Q.Promise<Models.Timestamped<Response<TResponse>>> => {
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

        var d = Q.defer<Models.Timestamped<Response<TResponse>>>();
        request(options, (err, resp, body) => {
            var t = moment.utc();
            if (err) d.reject(err);
            else {
                this._log("IN", body);
                d.resolve(new Models.Timestamped(JSON.parse(body), t));
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

interface IHaveFunds {
    funds: { [currencyName: string]: number };
}

// aka order ack
interface TradeResponse extends IHaveFunds {
    received: number;
    remains: number;
    order_id: number;
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
            if (r.data.success === 1) {
                this.raiseOrderUpdate({
                    time: r.time,
                    exchangeId: cancel.exchangeId,
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Cancelled,
                    leavesQuantity: 0
                });
            }
            else {
                this.raiseOrderUpdate({
                    time: r.time,
                    exchangeId: cancel.exchangeId,
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Rejected,
                    rejectMessage: r.data.error
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
            if (r.data.success === 1) {
                var exchangeOrderId = r.data.return.order_id;
                
                var rpt: Models.OrderStatusReport = {
                    time: r.time,
                    orderId: order.orderId,
                    leavesQuantity: r.data.return.remains,
                    partiallyFilled: r.data.return.received > 1e-6 && exchangeOrderId !== 0
                };

                if (exchangeOrderId === 0) {
                    rpt.orderStatus = Models.OrderStatus.Complete;
                }
                else {
                    rpt.orderStatus = Models.OrderStatus.Working;
                    rpt.exchangeId = exchangeOrderId.toString();
                }

                this.raiseOrderUpdate(rpt);
                this._positions.handlePositionUpdate(r.data.return);
            }
            else {
                this.raiseOrderUpdate({
                    time: r.time,
                    orderId: order.orderId,
                    orderStatus: Models.OrderStatus.Rejected,
                    rejectMessage: r.data.error
                });
            }
        });

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private refreshOpenOrders = () => {
        _.forIn(this._openOrderExchangeIds, (unused, id) => {
            var req = { order_id: parseInt(id) };
            this._client.postToEndpoint<ActiveOrderRequest, ActiveOrder>("OrderInfo", req).then(r => {
                if (r.data.success === 1) {
                    this.raiseOrderUpdate({
                        time: r.time,
                        exchangeId: id,
                        leavesQuantity: r.data.return.amount,
                        partiallyFilled: Math.abs(r.data.return.start_amount - r.data.return.amount) > 1e-4
                    });
                }
                else {
                    this._log("OrderInfo request for order " + id + " did not return successfully: " + r.data.error);
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
                    this._log("add", id, "to _openOrderExchangeIds");
                    break;
                case Models.OrderStatus.Cancelled:
                case Models.OrderStatus.Complete:
                case Models.OrderStatus.Rejected:
                case Models.OrderStatus.Other:
                    delete this._openOrderExchangeIds[id];
                    this._log("removed", id, "from _openOrderExchangeIds");
                    break;
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
    
    private _tradeSince = moment.utc();
    private monitorTrades = () => {
        var params = { count: 5 };
        //var params = { since: Math.round(new Date().getTime() / 1000) };
        this._client.postToEndpoint<{}, {[trade_id: string]: TradeHistory[]}>("TradeHistory", params).then(r => {
            _.forIn(r.data.return, (trade : TradeHistory, trade_id) => {
                this.raiseOrderUpdate({
                    lastQuantity: trade.amount,
                    lastPrice: trade.rate,
                    exchangeId: trade.order_id.toString(),
                    time: moment.unix(trade.timestamp),
                    exchangeTradeId: trade_id.toString()
                });
            });
        });
        this._tradeSince = moment.utc();
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => shortId.generate();

    private _log: Utils.Logger = Utils.log("tribeca:gateway:BtcEOE");
    private _openOrderExchangeIds: { [id: number]: boolean } = {};
    constructor(
        private _pairKey: string,
        private _timeProvider: Utils.ITimeProvider,
        private _client: BtcEAuthenticatedApiClient,
        private _positions: BtcEPositionGateway) {
            
        this._timeProvider.setImmediate(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._timeProvider.setInterval(this.refreshOpenOrders, moment.duration(5, "seconds"));
        
        this._timeProvider.setTimeout(() => {
            this._timeProvider.setInterval(this.monitorTrades, moment.duration(5, "seconds"));
        }, moment.duration(2, "seconds"));
    }
}

class BtcEPositionGateway implements Interfaces.IPositionGateway {
    _log: Utils.Logger = Utils.log("tribeca:gateway:BtcEPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    
    private downloadPositions = () => {
        this._client.postToEndpoint<{}, IHaveFunds>("getInfo", {}).then(r => {
            if (r.data.success === 1) {
                this.handlePositionUpdate(r.data.return);
            }
        });
    };
    
    public handlePositionUpdate = (msg : IHaveFunds) => {
        _.forIn(msg.funds, (val, curName) => {
            var cur = BtcEPositionGateway.GetCurrency(curName);
            
            if (cur !== null) {
                this.PositionUpdate.trigger(new Models.CurrencyPosition(val, 0, cur));
            }
        });
    };
    
    private static GetCurrency(name: string) : Models.Currency {
        switch (name) {
            case "usd": return Models.Currency.USD;
            case "btc": return Models.Currency.BTC;
            case "ltc": return Models.Currency.LTC;
        }
        return null;
    }

    constructor(
        private _pairKey: string,
        private _timeProvider: Utils.ITimeProvider,
        private _client: BtcEAuthenticatedApiClient) {
            this._timeProvider.setTimeout(this.downloadPositions, moment.duration(5));
            this._timeProvider.setInterval(this.downloadPositions, moment.duration(30, "seconds"));
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
        var positions = new BtcEPositionGateway(pairKey, timeProvider, authClient);
        super(
            new BtcEMarketDataGateway(pairKey, timeProvider, publicClient),
            new BtcEOrderEntryGateway(pairKey, timeProvider, authClient, positions),
            positions,
            new BtcEBaseGateway());
    }
}
