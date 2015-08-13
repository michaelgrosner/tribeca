/// <reference path="../../../typings/tsd.d.ts" />
/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />

import ws = require('ws');
import Q = require("q");
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import Config = require("../config");
import NullGateway = require("./nullgw");
import Models = require("../../common/models");
import Utils = require("../utils");
import util = require("util");
import Interfaces = require("../interfaces");
import moment = require("moment");
import _ = require("lodash");
var shortId = require("shortid");

interface BitfinexMarketTrade {
	tid: number;
	timestamp: number;
	price: string;
	amount: string;
	exchange: string;
	type: string;
}

interface BitfinexMarketLevel {
    price: string;
    amount: string;
    timestamp: string;
}

interface BitfinexOrderBook {
    bids: BitfinexMarketLevel[];
    asks: BitfinexMarketLevel[];
}

function decodeSide(side: string) {
    switch (side) {
        case "buy": return Models.Side.Bid;
        case "sell": return Models.Side.Ask;
        default: return  Models.Side.Unknown;
    }
}

function encodeSide(side: Models.Side) {
    switch (side) {
        case Models.Side.Bid: return "buy";
        case Models.Side.Ask: return "sell";
        default: return "";
    } 
}

function encodeTimeInForce(tif: Models.TimeInForce) {
    switch (tif) {
        case Models.TimeInForce.FOK: return "fill-or-kill";
        case Models.TimeInForce.GTC: return "limit";
        default: throw new Error("unsupported tif " + Models.TimeInForce[tif]);
    }
}

class BitfinexMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private _since : number = null;
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private onTrades = (trades : Models.Timestamped<BitfinexMarketTrade[]>) => {
        _.forEach(trades.data, trade => {
            var px = parseFloat(trade.price);
            var sz = parseFloat(trade.amount);
            var time = moment.unix(trade.timestamp);
            var side = decodeSide(trade.type);
            var mt = new Models.GatewayMarketTrade(px, sz, time, this._since === null, side);
            this.MarketTrade.trigger(mt);
        });
        
        this._since = moment().unix();
    };
    
    private downloadMarketTrades = () => {
        var qs = this._since === null ? undefined : {timestamp: this._since};
        this._http
            .get<BitfinexMarketTrade[]>("trades/"+this._symbolProvider.symbol, qs)
            .then(this.onTrades)
            .done();
    };
    
    private static ConvertToMarketSide(level: BitfinexMarketLevel) : Models.MarketSide {
        return new Models.MarketSide(parseFloat(level.price), parseFloat(level.amount));
    }
    
    private static ConvertToMarketSides(level: BitfinexMarketLevel[]) : Models.MarketSide[] {
        return _(level).first(3).map(BitfinexMarketDataGateway.ConvertToMarketSide).value();
    }

    MarketData = new Utils.Evt<Models.Market>();
    private onMarketData = (book: Models.Timestamped<BitfinexOrderBook>) => {
        var bids = BitfinexMarketDataGateway.ConvertToMarketSides(book.data.bids);
        var asks = BitfinexMarketDataGateway.ConvertToMarketSides(book.data.asks);
        this.MarketData.trigger(new Models.Market(bids, asks, book.time));
    };
    
    private downloadMarketData = () => {
        this._http
            .get<BitfinexOrderBook>("book/"+this._symbolProvider.symbol, {limit_bids: 3, limit_asks: 3})
            .then(this.onMarketData)
            .done();
    };

    constructor(
        timeProvider: Utils.ITimeProvider,
        private _http: BitfinexHttp,
        private _symbolProvider: BitfinexSymbolProvider) {
        
        timeProvider.setInterval(this.downloadMarketData, moment.duration(4, "seconds"));
        timeProvider.setInterval(this.downloadMarketTrades, moment.duration(10, "seconds"));
        
        this.downloadMarketData();
        this.downloadMarketTrades();
        
        _http.ConnectChanged.on(s => this.ConnectChanged.trigger(s));
    }
}

interface BitfinexNewOrderRequest {
    symbol: string;
    amount: number;
    price: number; //Price to buy or sell at. Must be positive. Use random number for market orders.
    exchange: string; //always "bitfinex"
    side: string; // buy or sell
    type: string; // "market" / "limit" / "stop" / "trailing-stop" / "fill-or-kill" / "exchange market" / "exchange limit" / "exchange stop" / "exchange trailing-stop" / "exchange fill-or-kill". (type starting by "exchange " are exchange orders, others are margin trading orders)
    is_hidden: boolean;
}

interface BitfinexNewOrderResponse {
    order_id: string;
}

interface BitfinexCancelOrderRequest {
    order_id: string;
}

interface BitfinexCancelReplaceOrderRequest extends BitfinexNewOrderRequest {
    order_id: string;
}

interface BitfinexCancelReplaceOrderResponse extends BitfinexCancelOrderRequest {}

interface BitfinexOrderStatusRequest {
    order_id: string;
}

interface BitfinexMyTradesRequest {
    symbol: string;
    timestamp: number;
}

interface BitfinexMyTradesResponse {
    price: string;
    amount: string;
    timestamp: number;
    exchange: string;
    type: string;
    fee_currency: string;
    fee_amount: string;
    tid: number;
    order_id: string;
}

interface BitfinexOrderStatusResponse {
    symbol: string;
    exchange: string; // bitstamp or bitfinex
    price: number;
    avg_execution_price: string;
    side: string;
    type: string; // "market" / "limit" / "stop" / "trailing-stop".
    timestamp: number;
    is_live: boolean;
    is_cancelled: boolean;
    is_hidden: boolean;
    was_forced: boolean;
    executed_amount: string;
    remaining_amount: string;
    original_amount: string;
}

class BitfinexOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => shortId.generate();

    public cancelsByClientOrderId = false;
    
    private convertToOrderRequest = (order: Models.Order) : BitfinexNewOrderRequest => {
        return {
            amount: order.quantity,
            exchange: "bitfinex",
            is_hidden: false,
            price: order.price,
            side: encodeSide(order.side),
            symbol: this._symbolProvider.symbol,
            type: encodeTimeInForce(order.timeInForce)
        };
    }
    
    private _orderIdsToMonitor : { [orderId: string] : boolean} = {};
    
    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var req = this.convertToOrderRequest(order);
        
        this._http
            .post<BitfinexNewOrderRequest, BitfinexNewOrderResponse>("order/new", req)
            .then(resp => {
                this._orderIdsToMonitor[resp.data.order_id] = true;
                
                this.OrderUpdate.trigger({
                    orderId: order.orderId, 
                    exchangeId: resp.data.order_id,
                    time: resp.time,
                    orderStatus: Models.OrderStatus.Working
                });
            }).done();
            
        return new Models.OrderGatewayActionReport(Utils.date());
    };
    
    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        var req = {order_id: cancel.exchangeId};
        this._http
            .post<BitfinexCancelOrderRequest, any>("order/cancel", req)
            .then(resp => {
                delete this._orderIdsToMonitor[cancel.exchangeId];
                
                this.OrderUpdate.trigger({
                    exchangeId: cancel.clientOrderId,
                    time: resp.time,
                    orderStatus: Models.OrderStatus.Cancelled
                });
            })
            .done();
        
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };
    
    private downloadOrderStatuses = () => {
        var tradesReq = { timestamp: this._since.unix(), symbol: this._symbolProvider.symbol };
        this._http
            .post<BitfinexMyTradesRequest, BitfinexMyTradesResponse[]>("mytrades", tradesReq)
            .then(resps => {
                _.forEach(resps.data, t => {
                    
                    this._http
                        .post<BitfinexOrderStatusRequest, BitfinexOrderStatusResponse>("order/status", {order_id: t.order_id})
                        .then(r => {
                            
                            this.OrderUpdate.trigger({
                               exchangeId: t.order_id,
                               lastPrice: parseFloat(t.price),
                               lastQuantity: parseFloat(t.amount),
                               orderStatus: BitfinexOrderEntryGateway.GetOrderStatus(r.data),
                               averagePrice: parseFloat(r.data.avg_execution_price),
                               leavesQuantity: parseFloat(r.data.remaining_amount),
                               cumQuantity: parseFloat(r.data.executed_amount),
                               quantity: parseFloat(r.data.original_amount)
                            });
                            
                        })
                        .done();
                });
            }).done();
            
        this._since = moment.utc();
    };
    
    private static GetOrderStatus(r: BitfinexOrderStatusResponse) {
        if (r.is_cancelled) return Models.OrderStatus.Cancelled;
        if (r.is_live) return Models.OrderStatus.Working;
        return Models.OrderStatus.Other;
    }
    
    private _since = moment.utc();
    private _log : Utils.Logger = Utils.log("tribeca:gateway:BitfinexOE");
    constructor(timeProvider: Utils.ITimeProvider,
        private _http: BitfinexHttp,
        private _symbolProvider: BitfinexSymbolProvider) {
            
        _http.ConnectChanged.on(s => this.ConnectChanged.trigger(s));
        timeProvider.setInterval(this.downloadOrderStatuses, moment.duration(7, "seconds"));
    }
}

class BitfinexHttp {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    get = <T>(actionUrl: string, qs: any = undefined) : Q.Promise<Models.Timestamped<T>> => { 
        var opts = {
            timeout: 5000,
            url: url.resolve(this._baseUrl, actionUrl),
            qs: qs,
            method: "GET"
        };
        
        return this.doRequest<T>(actionUrl, opts);
    };
    
    post = <TRequest, TResponse>(actionUrl: string, msg : TRequest) : Q.Promise<Models.Timestamped<TResponse>> => {        
        msg["request"] = "/v1/" + actionUrl;
        msg["nonce"] = this._nonce;
        this._nonce += 1;
        
        var payload = new Buffer(JSON.stringify(msg)).toString("base64");
        var signature = crypto.createHmac("sha384", this._secret).update(payload).digest('hex');
        
        var opts : request.Options = {
            timeout: 5000,
            url: url.resolve(this._baseUrl, actionUrl),
            headers: {
                "X-BFX-APIKEY": this._apiKey,
                "X-BFX-PAYLOAD": payload,
                "X-BFX-SIGNATURE": signature
            },
            method: "POST"
        };

        return this.doRequest<TResponse>(actionUrl, opts);
    };
    
    private doRequest = <TResponse>(actionUrl: string, msg : request.Options) : Q.Promise<Models.Timestamped<TResponse>> => {
        var d = Q.defer<Models.Timestamped<TResponse>>();
        
        request(msg, (err, resp, body) => {
            if (err) d.reject(err);
            else {
                try {
                    var t = Utils.date();
                    var data = JSON.parse(body);
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    this._log("url: %s, err: %o, body: %o", actionUrl, err, body);
                    d.reject(e);
                }
            }
        });
        
        return d.promise;
    };
    
    private _log : Utils.Logger = Utils.log("tribeca:gateway:BitfinexHTTP");
    private _baseUrl : string;
    private _apiKey: string;
    private _secret: string;
    private _nonce: number;
    
    
    constructor(config : Config.IConfigProvider) {
        this._baseUrl = config.GetString("BitfinexHttpUrl")
        this._apiKey = config.GetString("BitfinexKey");
        this._secret = config.GetString("BitfinexSecret");
        
        this._nonce = new Date().valueOf();
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 10);
    }
}

class BitfinexPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private static convertCurrency(name : string) : Models.Currency {
        switch (name.toLowerCase()) {
            case "usd": return Models.Currency.USD;
            case "ltc": return Models.Currency.LTC;
            case "btc": return Models.Currency.BTC;
            default: throw new Error("Unsupported currency " + name);
        }
    }

    private _log : Utils.Logger = Utils.log("tribeca:gateway:BitfinexPG");
    constructor(private _http: BitfinexHttp) {
        this._http.post("positions", {}).then(r => console.log(util.inspect(r)));
    }
}

class BitfinexBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    name() : string {
        return "Bitfinex";
    }

    makeFee() : number {
        return 0.001;
    }

    takeFee() : number {
        return 0.002;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.Bitfinex;
    }
    
    private static AllPairs = [
        new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD),
        //new Models.CurrencyPair(Models.Currency.LTC, Models.Currency.USD),
    ];
    public get supportedCurrencyPairs() {
        return BitfinexBaseGateway.AllPairs;
    }
}

function GetCurrencyEnum(c: string) : Models.Currency {
    switch (name.toLowerCase()) {
        case "usd": return Models.Currency.USD;
        case "ltc": return Models.Currency.LTC;
        case "btc": return Models.Currency.BTC;
        default: throw new Error("Unsupported currency " + name);
    }
}

function GetCurrencySymbol(c: Models.Currency) : string {
    switch (c) {
        case Models.Currency.USD: return "usd";
        case Models.Currency.LTC: return "ltc";
        case Models.Currency.BTC: return "btc";
        default: throw new Error("Unsupported currency " + Models.Currency[c]);
    }
}

class BitfinexSymbolProvider {
    public symbol : string;
    
    constructor(pair: Models.CurrencyPair) {
        this.symbol = GetCurrencySymbol(pair.base) + GetCurrencySymbol(pair.quote);
    }
}

export class Bitfinex extends Interfaces.CombinedGateway {
    constructor(timeProvider: Utils.ITimeProvider, config : Config.IConfigProvider, pair: Models.CurrencyPair) {
        var symbol = new BitfinexSymbolProvider(pair);
        var http = new BitfinexHttp(config);

        var orderGateway = config.GetString("BitfinexOrderDestination") == "Bitfinex"
            ? <Interfaces.IOrderEntryGateway>new BitfinexOrderEntryGateway(timeProvider, http, symbol)
            : new NullGateway.NullOrderGateway();

        super(
            new BitfinexMarketDataGateway(timeProvider, http, symbol),
            orderGateway,
            new BitfinexPositionGateway(http),
            new BitfinexBaseGateway());
        }
}