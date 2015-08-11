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

class BitfinexMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private _since : number = null;
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private onTrades = (trades : Models.Timestamped<BitfinexMarketTrade[]>) => {
        _.forEach(trades.data, trade => {
            var px = parseFloat(trade.price);
            var sz = parseFloat(trade.amount);
            var time = moment.unix(trade.timestamp);
            var side = trade.type === "buy" ? Models.Side.Bid : Models.Side.Ask;
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

interface BitfinexOrderStatusResponse {
    symbol: string;
    exchange: string; // bitstamp or bitfinex
    price: number;
    avg_execution_price: number;
    side: string;
    type: string; // "market" / "limit" / "stop" / "trailing-stop".
    timestamp: number;
    is_live: boolean;
    is_cancelled: boolean;
    is_hidden: boolean;
    was_forced: boolean;
    executed_amount: number;
    remaining_amount: number;
    original_amount: number;
}

class BitfinexOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => shortId.generate();

    public cancelsByClientOrderId = false;
    
    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        return new Models.OrderGatewayActionReport(Utils.date());
    };
    
    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };
    
    private _log : Utils.Logger = Utils.log("tribeca:gateway:BitfinexOE");
    constructor(private _symbolProvider: BitfinexSymbolProvider) {
    }
}

class BitfinexHttp {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    get = <T>(actionUrl: string, qs: any = undefined) : Q.Promise<Models.Timestamped<T>> => { 
        var d = Q.defer<Models.Timestamped<T>>();

        request({
            url: url.resolve(this._baseUrl, actionUrl),
            qs: qs,
            method: "GET"
        }, (err, resp, body) => {
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
    
    post = <T>(actionUrl: string, msg : any) : Q.Promise<Models.Timestamped<T>> => {
        var d = Q.defer<Models.Timestamped<T>>();

        request({
            url: url.resolve(this._baseUrl, actionUrl),
            body: querystring.stringify({}),
            headers: {"Content-Type": "application/x-www-form-urlencoded"},
            method: "POST"
        }, (err, resp, body) => {
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
    constructor(config : Config.IConfigProvider) {
        this._baseUrl = config.GetString("BitfinexHttpUrl")
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
    constructor() {
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
    constructor(config : Config.IConfigProvider, pair: Models.CurrencyPair) {
        var symbol = new BitfinexSymbolProvider(pair);
        var http = new BitfinexHttp(config);

        var orderGateway = config.GetString("BitfinexOrderDestination") == "Bitfinex"
            ? <Interfaces.IOrderEntryGateway>new BitfinexOrderEntryGateway(symbol)
            : new NullGateway.NullOrderGateway();

        super(
            new BitfinexMarketDataGateway(symbol),
            orderGateway,
            new BitfinexPositionGateway(),
            new BitfinexBaseGateway());
        }
}