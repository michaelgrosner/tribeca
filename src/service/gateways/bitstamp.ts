/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />
///<reference path="../interfaces.ts"/>

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
import Interfaces = require("../interfaces");
import moment = require("moment");
import _ = require("lodash");
import log from "../logging";
var shortId = require("shortid");
var Deque = require("collections/deque");

interface BitstampMessageIncomingMessage {
    event?: string;
    channel: string;
    time: string;
    data: any;
}

interface BitstampDepthMessage {
    asks: [string, string][];
    bids: [string, string][];
    timestamp: string;
}

interface BitstampTradeMessage {
    buy_order_id: number;
    amount_str: string;
    timestamp: string;
    microtimestamp: string;
    id: number;
    amount: number;
    sell_order_id: number;
    price_str: string;
    type: number;
    price: number;
}

interface SignedMessage {
    key?: string;
    signature?: string;
    nonce?: string;
}

interface CancelOrderRequest extends SignedMessage {
    id: string;
}

interface NewMarketOrderRequest extends SignedMessage {
    amount: string;
}

interface NewLimitOrderRequest extends SignedMessage {
    amount: string;
    price: string;
}
interface NewLimitFOKOrderRequest extends SignedMessage {
    amount: string;
    price: string;
    fok_order: string;
}
interface NewLimitIOCOrderRequest extends SignedMessage {
    amount: string;
    price: string;
    ioc_order: string;
}

interface UserTransactionsRequest extends SignedMessage {
    since_timestamp: number;
}

interface OrderStatusRequest extends SignedMessage {
    id: string;
}

interface SemaphoreInterface {
    acquire(): Promise<[number, SemaphoreInterface.Releaser]>;

    runExclusive<T>(callback: SemaphoreInterface.Worker<T>): Promise<T>;

    isLocked(): boolean;

    release(): void;
}

interface MutexInterface {
    acquire(): Promise<MutexInterface.Releaser>;

    runExclusive<T>(callback: MutexInterface.Worker<T>): Promise<T>;

    isLocked(): boolean;

    release(): void;
}

namespace MutexInterface {
    export interface Releaser {
        (): void;
    }

    export interface Worker<T> {
        (): Promise<T> | T;
    }
}

namespace SemaphoreInterface {
    export interface Releaser {
        (): void;
    }

    export interface Worker<T> {
        (value: number): Promise<T> | T;
    }
}

class Semaphore implements SemaphoreInterface {
    constructor(private _maxConcurrency: number) {
        if (_maxConcurrency <= 0) {
            throw new Error('semaphore must be initialized to a positive value');
        }

        this._value = _maxConcurrency;
    }

    acquire(): Promise<[number, SemaphoreInterface.Releaser]> {
        const locked = this.isLocked();
        const ticket = new Promise<[number, SemaphoreInterface.Releaser]>((r) => this._queue.push(r));

        if (!locked) this._dispatch();

        return ticket;
    }

    async runExclusive<T>(callback: SemaphoreInterface.Worker<T>): Promise<T> {
        const [value, release] = await this.acquire();

        try {
            return await callback(value);
        } finally {
            release();
        }
    }

    isLocked(): boolean {
        return this._value <= 0;
    }

    release(): void {
        if (this._maxConcurrency > 1) {
            throw new Error(
                'this method is unavailabel on semaphores with concurrency > 1; use the scoped release returned by acquire instead'
            );
        }

        if (this._currentReleaser) {
            this._currentReleaser();
            this._currentReleaser = undefined;
        }
    }

    private _dispatch(): void {
        const nextConsumer = this._queue.shift();

        if (!nextConsumer) return;

        let released = false;
        this._currentReleaser = () => {
            if (released) return;

            released = true;
            this._value++;

            this._dispatch();
        };

        nextConsumer([this._value--, this._currentReleaser]);
    }

    private _queue: Array<(lease: [number, SemaphoreInterface.Releaser]) => void> = [];
    private _currentReleaser: SemaphoreInterface.Releaser | undefined;
    private _value: number;
}

class Mutex implements MutexInterface {
    async acquire(): Promise<MutexInterface.Releaser> {
        const [, releaser] = await this._semaphore.acquire();

        return releaser;
    }

    runExclusive<T>(callback: MutexInterface.Worker<T>): Promise<T> {
        return this._semaphore.runExclusive(() => callback());
    }

    isLocked(): boolean {
        return this._semaphore.isLocked();
    }

    release(): void {
        this._semaphore.release();
    }

    private _semaphore = new Semaphore(1);
}

class BitstampWebsocket {
    sendSubscribe = <T>(channel: string, cb?: () => void) => {
        var msg = {
            "event": "bts:subscribe",
            "data": {
                "channel": channel
            }
        };

        this._ws.send(JSON.stringify(msg), (e: Error) => {
            if (!e && cb) cb();
        });
    }

    setHandler = <T>(channel: string, handler: (newMsg: Models.Timestamped<T>) => void) => {
        this._handlers[channel] = handler;
    }

    private onMessage = (raw: string) => {
        var t = Utils.date();
        try {
            this._log.debug("Received MSG: %s", raw);
            var msg: BitstampMessageIncomingMessage = JSON.parse(raw);

            if (msg.event == "bts:subscription_succeeded") {
                this._log.info("Successfully subscribed to %s", msg.channel);
                return;
            }

            var handler = this._handlers[msg.channel];

            if (typeof handler == "undefined") {
                this._log.warn("Got message on unknown topic", msg);
                return;
            }

            handler(new Models.Timestamped(msg.data, t));
        }
        catch (e) {
            this._log.error(e, "Error parsing msg %o", raw);
            throw e;
        }
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private _log = log("tribeca:gateway:BitstampWebsocket");
    private _handlers: { [channel: string]: (newMsg: Models.Timestamped<any>) => void } = {};
    private _ws: ws;
    constructor(config: Config.IConfigProvider) {
        this._ws = new ws(config.GetString("BitstampWsUrl"));

        this._ws.on("open", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("close", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
    }
}

class BitstampMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private static GetLevel = (n: [string, string]): Models.MarketSide => new Models.MarketSide(parseFloat(n[0]), parseFloat(n[1]));
    private _log = log("tribeca:gateway:BitstampMD");

    private onTrade = (trade: Models.Timestamped<BitstampTradeMessage>) => {
        let msg = trade.data;
        let time = new Date(trade.time);
        var mt = new Models.GatewayMarketTrade(msg.price, msg.amount, time, false, this.decodeSide(msg.type));
        this.MarketTrade.trigger(mt);
        this._log.debug(mt);
    };

    private readonly Depth: number = 25;
    private onDepth = (depth: Models.Timestamped<BitstampDepthMessage>) => {
        let msg = depth.data;

        let bids = _(msg.bids).take(this.Depth).map(BitstampMarketDataGateway.GetLevel).value();
        let asks = _(msg.asks).take(this.Depth).map(BitstampMarketDataGateway.GetLevel).value()
        var mkt = new Models.Market(bids, asks, depth.time);

        this.MarketData.trigger(mkt);
    };

    private decodeSide = (side: number): Models.Side => {
        switch (side) {
            case 0: return Models.Side.Bid;
            case 1: return Models.Side.Ask;
            default: return Models.Side.Unknown;
        }
    }
    
    constructor(socket: BitstampWebsocket, symbolProvider: BitstampSymbolProvider, http: BitstampHttp) {
        // fetch initial state from http
        http.get<BitstampDepthMessage>("v2/order_book/" + symbolProvider.symbolWithoutUnderscore + "/").then(resp => {
            this.onDepth(resp);
        });

        var depthChannel = "order_book_" + symbolProvider.symbolWithoutUnderscore;
        var tradesChannel = "live_trades_" + symbolProvider.symbolWithoutUnderscore;

        socket.setHandler(depthChannel, this.onDepth);
        socket.setHandler(tradesChannel, this.onTrade);

        socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);

            if (cs == Models.ConnectivityStatus.Connected) {
                socket.sendSubscribe(depthChannel);
                socket.sendSubscribe(tradesChannel);
            }
        });
    }
}

class BitstampOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    public cancelsByClientOrderId = false;
    private _since = moment.utc();
    private _log = log("tribeca:gateway:BitstampOE");

    supportsCancelAllOpenOrders = (): boolean => { return true; };

    generateClientOrderId = () => shortId.generate();    

    sendOrder = (order: Models.OrderStatusReport) => {
        var req = null;
        var url = "";
        if (order.type == Models.OrderType.Market) {
            url = "v2/" + this.encodeSide(order.side) + "/market/" + this._symbolProvider.symbolWithoutUnderscore + "/";
            req = <NewMarketOrderRequest>{
                amount: order.quantity.toFixed(this._amountDecimalPlaces)
            };
        } else if (order.type == Models.OrderType.Limit) {
            url = "v2/" + this.encodeSide(order.side) + "/" + this._symbolProvider.symbolWithoutUnderscore + "/";
            if (order.timeInForce == Models.TimeInForce.FOK) {
                req = <NewLimitFOKOrderRequest>{
                    amount: order.quantity.toFixed(this._amountDecimalPlaces),
                    price: order.price.toFixed(this._priceDecimalPlaces),
                    fok_order: "True",
                };
            } else if (order.timeInForce == Models.TimeInForce.IOC) {
                req = <NewLimitIOCOrderRequest>{
                    amount: order.quantity.toFixed(this._amountDecimalPlaces),
                    price: order.price.toFixed(this._priceDecimalPlaces),
                    ioc_order: "True",
                };
            } else {
                req = <NewLimitOrderRequest>{
                    amount: order.quantity.toFixed(this._amountDecimalPlaces),
                    price: order.price.toFixed(this._priceDecimalPlaces),
                };
            }
        }

        // send request
        this._http
            .post(url, req)
            .then(resp => {
                var data = <any>resp.data;
                var errMsg = null;
                if (data.status && data.status == "error") {
                    errMsg = data.reason;
                } else if (typeof data.id == "undefined") {
                    errMsg = "Invalid response type!";                    
                }
                if (errMsg != null) {
                    this._log.error("Error when submiting bitstamp order.", data, req)
                    this.OrderUpdate.trigger({
                        orderStatus: Models.OrderStatus.Rejected,
                        orderId: order.orderId,
                        rejectMessage: errMsg,
                        time: resp.time
                    });
                    return;
                }

                this.OrderUpdate.trigger({
                    orderId: order.orderId,
                    exchangeId: data.id,
                    time: resp.time,
                    orderStatus: Models.OrderStatus.Working
                });
            }).done();

        this.OrderUpdate.trigger({
            orderId: order.orderId,
            computationalLatency: Utils.fastDiff(new Date(), order.time)
        });
    };

    cancelOrder = (cancel: Models.OrderStatusReport) => {
        var req: CancelOrderRequest = { id: cancel.exchangeId };
        this._http
            .post("v2/cancel_order/", req)
            .then(resp => {
                var data = <any>resp.data;
                var errMsg = null;
                if (data.status && data.status == "error") {
                    errMsg = data.reason;
                } else if (data.error) {
                    errMsg = data.error;
                } else if (typeof data.id == "undefined") {
                    errMsg = "Invalid response type!";
                }
                // check if reason is that it's already filled
                if (errMsg != null && errMsg != "Order not found") {
                    this._log.error("Error when canceling bitstamp order.", data, req)
                    this.OrderUpdate.trigger({
                        orderStatus: Models.OrderStatus.Rejected,
                        cancelRejected: true,
                        orderId: cancel.orderId,
                        rejectMessage: errMsg,
                        time: resp.time
                    });
                    return
                }

                // if order not found, check if it was alread filled
                if (errMsg == "Order not found"){
                    this.fetchOrderStatus(cancel.exchangeId, undefined, undefined);
                } else {
                    this.OrderUpdate.trigger({
                        orderId: cancel.orderId,
                        time: resp.time,
                        orderStatus: Models.OrderStatus.Cancelled,
                        leavesQuantity: 0
                    });
                }                
            })
            .done();

        this.OrderUpdate.trigger({
            orderId: cancel.orderId,
            computationalLatency: Utils.fastDiff(new Date(), cancel.time)
        });
    };

    cancelAllOpenOrders = (): Q.Promise<number> => {
        var d = Q.defer<number>();
        this._http.post("v2/open_orders/" + this._symbolProvider.symbolWithoutUnderscore + "/", {}).then(openOrders => {
            const openOrdersData = <any>openOrders.data;
            if (openOrdersData.reason !== undefined) {
                d.reject(openOrdersData.reason);
            } else {
                this._http.post("cancel_all_orders/", {}).then(success => {
                    if (<any>success.data == true) {
                        // mark all open orders as canceled
                        var t = this._timeProvider.utcNow();
                        for (let oneOpenOrder of openOrdersData) {
                            this.OrderUpdate.trigger({
                                exchangeId: oneOpenOrder.id,
                                time: t,
                                orderStatus: Models.OrderStatus.Cancelled,
                                leavesQuantity: 0
                            });
                        }
                        d.resolve(openOrdersData.length);
                    } else {
                        d.reject("cancel_all_orders failed!");
                    }
                })
            }
        });
        return d.promise;
    };

    replaceOrder = (replace: Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private encodeSide(side: Models.Side) {
        switch (side) {
            case Models.Side.Bid: return "buy";
            case Models.Side.Ask: return "sell";
            default: return "";
        }
    }

    private parsePair(transaction) {
        for (let t in transaction) {
            if (t.indexOf("_") > 0 && t != "order_id") {
                return t;
            }
        }
        return null;
    }

    private parseAmount(transactions, currency) {
        var totalAmount = 0;
        for (let t of transactions) {
            totalAmount += Math.abs(parseFloat(t[currency]))
        }
        return totalAmount;
    }

    private parseAvgPrice(transactions) {
        var i = 0;
        var totalPrice = 0.0;
        for (let t of transactions) {
            totalPrice += parseFloat(t.price);
            i++;
        }
        return totalPrice / i;
    }

    private getOrderStatus(orderStatus) {
        if (orderStatus == "Finished") {
            return Models.OrderStatus.Complete;
        } else if (orderStatus == "Canceled"){
            return Models.OrderStatus.Cancelled;
        }
        return Models.OrderStatus.Working;
    }

    private async fetchOrderStatus(orderId, lastPrice, amountCurrency){
        let orderStatusRequest: OrderStatusRequest = { id: orderId }
        let r = await this._http.post("v2/order_status/", orderStatusRequest);
        let statusData = <any>r.data;
        let convertedStatus = {
            exchangeId: orderId,
            lastPrice: lastPrice,
            lastQuantity: amountCurrency ? this.parseAmount(statusData.transactions, amountCurrency) : undefined,
            orderStatus: this.getOrderStatus(statusData.status),
            averagePrice: this.parseAvgPrice(statusData.transactions),
        };
        this.OrderUpdate.trigger(convertedStatus);
    }

    private async processTransactionData(transactionsData) {
        // init
        var alreadyProccessedOrderIds = {};

        // check for error
        if (transactionsData.reason !== undefined) {
            this._log.error("Error while trying to fetch bitstamp transaction data", transactionsData)
            return;
        }

        // process each transaction
        for (const transaction of transactionsData) {
            // check if already processed
            if (alreadyProccessedOrderIds[transaction.order_id] == true) {
                return;
            }

            // parse pair
            let pair = this.parsePair(transaction);
            let amountCurrency = pair.split("_")[0];

            // fetch order status
            this.fetchOrderStatus(transaction.order_id, parseFloat(transaction[pair]), amountCurrency)
            alreadyProccessedOrderIds[transaction.order_id] = true;
        }
    }

    private async downloadUserOrdersData() {
        // download user transactions
        let tradesReq: UserTransactionsRequest = { since_timestamp: this._since.unix() };
        let transactionData = await this._http.post("v2/user_transactions/" + this._symbolProvider.symbolWithoutUnderscore + "/", tradesReq);
        this.processTransactionData(<any>transactionData.data);
        this._since = moment.utc();
    };

    constructor(
        private _timeProvider: Utils.ITimeProvider,
        private _http: BitstampHttp,
        private _symbolProvider: BitstampSymbolProvider,
        private _priceDecimalPlaces: number,
        private _amountDecimalPlaces: number) {

        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 10);
        this._timeProvider.setInterval(this.downloadUserOrdersData.bind(this), moment.duration(2, "seconds"));
    }
}

class BitstampMessageSigner {
    private _secretKey: string;
    private _api_key: string;
    private _client_id: string;

    private generateNonce = () => {
        var hrTime = process.hrtime()
        return (hrTime[0] * 1000000000 + hrTime[1]).toString();
    }

    public signMessage = (m: SignedMessage): SignedMessage => {
        if (!m.hasOwnProperty("key"))
            m.key = this._api_key;

        // generate nonce and signature
        var nonce = this.generateNonce();
        var message = nonce + this._client_id + this._api_key;
        var signer = crypto.createHmac('sha256', new Buffer(this._secretKey, 'utf8'));
        m.signature = signer.update(message).digest('hex').toUpperCase();
        m.nonce = nonce;
        return m;
    };

    constructor(config: Config.IConfigProvider) {
        this._client_id = config.GetString("BitstampClientId");
        this._api_key = config.GetString("BitstampApiKey");
        this._secretKey = config.GetString("BitstampSecretKey");
    }
}

class RateLimitMonitor {
    private _log = log("tribeca:gateway:rlm");
    private _queue = Deque();
    private _durationMs: number;

    public add = () => {
        var now = moment.utc();

        while (now.diff(this._queue.peek()) > this._durationMs) {
            this._queue.shift();
        }

        this._queue.push(now);

        if (this._queue.length > this._number) {
            this._log.error("Exceeded rate limit", { nRequests: this._queue.length, max: this._number, durationMs: this._durationMs });
        }
    }

    constructor(private _number: number, duration: moment.Duration) {
        this._durationMs = duration.asMilliseconds();
    }
}

class BitstampHttp {
    private _timeout = 15000;
    private mutex = new Mutex();
    private _log = log("tribeca:gateway:BitstampHTTP");
    private _baseUrl: string;

    get = <T>(actionUrl: string, qs?: any): Q.Promise<Models.Timestamped<T>> => {
        var d = Q.defer<Models.Timestamped<T>>();

        // for rate limiting
        this._monitor.add();

        request({
            url: url.resolve(this._baseUrl, actionUrl),
            method: "GET",
            qs: qs || undefined,
            timeout: this._timeout
        }, (err, resp, body) => {
            if (err) d.reject(err);
            else {
                try {
                    var t = Utils.date();
                    var data = JSON.parse(body);
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (e) {
                    this._log.error(err, "url: %s, err: %o, body: %o", actionUrl, err, body);
                    d.reject(e);
                }
            }
        });

        return d.promise;
    };

    post = <T>(actionUrl: string, msg: SignedMessage): Q.Promise<Models.Timestamped<T>> => {
        var d = Q.defer<Models.Timestamped<T>>();

        this.mutex
            .acquire()
            .then(function (release) {
                // for rate limiting
                this._monitor.add();                

                // send request
                let signedMsg = this._signer.signMessage(msg);
                request({
                    url: url.resolve(this._baseUrl, actionUrl),
                    body: querystring.stringify(signedMsg),
                    headers: { "Content-Type": "application/x-www-form-urlencoded" },
                    method: "POST",
                    timeout: this._timeout
                }, (err, resp, body) => {
                    if (err) d.reject(err);
                    else {
                        try {
                            var t = Utils.date();
                            var data = JSON.parse(body);
                            d.resolve(new Models.Timestamped(data, t));
                            release();
                        }
                        catch (e) {
                            this._log.error(err, "url: %s, err: %o, body: %o", actionUrl, err, body);
                            d.reject(e);
                            release();
                        }
                    }
                });
            }.bind(this));

        return d.promise;
    };
    
    constructor(config: Config.IConfigProvider, private _signer: BitstampMessageSigner, private _monitor: RateLimitMonitor) {
        this._baseUrl = config.GetString("BitstampRestUrl")
    }
}

class BitstampPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    private _log = log("tribeca:gateway:BitstampPG");

    private static convertCurrency(name: string): Models.Currency {
        switch (name.toLowerCase()) {
            case "usd": return Models.Currency.USD;
            case "eur": return Models.Currency.EUR;
            case "gbp": return Models.Currency.GBP;
            case "ltc": return Models.Currency.LTC;
            case "btc": return Models.Currency.BTC;
            case "bch": return Models.Currency.BCH;
            case "eth": return Models.Currency.ETH;
            case "pax": return Models.Currency.PAX;
            case "xlm": return Models.Currency.XLM;
            case "xrp": return Models.Currency.XMR;
            default: throw new Error("Unsupported currency " + name);
        }
    }

    private trigger = () => {
        this._http.post("v2/balance/", {}).then(msg => {
            var data = <any>msg.data;
            const GetCurrencySymbol = (s: Models.Currency): string => Models.fromCurrency(s).toLowerCase();
            let baseSymbol = GetCurrencySymbol(this._pair.base);
            let quoteSymbol = GetCurrencySymbol(this._pair.quote);
            try {
                if (data.status && data.status == "error") {
                    this._log.error(data.reason, "error while downloading Bitstamp positions", data)
                    return;
                }

                for (let key in data) {
                    if (key.includes("_available")) {
                        let currencyName = key.split("_")[0];
                        if (currencyName == baseSymbol || currencyName == quoteSymbol) {
                            var freeAmount = parseFloat(data[key]);
                            var held = parseFloat(data[currencyName + "_reserved"]);
                            var pos = new Models.CurrencyPosition(freeAmount, held, BitstampPositionGateway.convertCurrency(currencyName));
                            this.PositionUpdate.trigger(pos);
                        }
                    }
                }
            } catch (error) {
                this._log.error(error, "Exception while downloading Bitstamp positions", data)
            }
        }).done();
    };
    
    constructor(private _http: BitstampHttp, private _pair: Models.CurrencyPair) {
        setInterval(this.trigger, 15000);
        setTimeout(this.trigger, 100);
    }
}

class BitstampBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return true;
    }

    name(): string {
        return "Bitstamp";
    }

    makeFee(): number {
        return this._feeAmount;
    }

    takeFee(): number {
        return this._feeAmount;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.Bitstamp;
    }

    constructor(public minTickIncrement: number, private _feeAmount: number) { }
}

class BitstampSymbolProvider {
    public symbol: string;
    public symbolWithoutUnderscore: string;

    constructor(pair: Models.CurrencyPair) {
        const GetCurrencySymbol = (s: Models.Currency): string => Models.fromCurrency(s).toLowerCase();
        this.symbol = GetCurrencySymbol(pair.base) + "_" + GetCurrencySymbol(pair.quote);
        this.symbolWithoutUnderscore = GetCurrencySymbol(pair.base) + GetCurrencySymbol(pair.quote);
    }
}

class Bitstamp extends Interfaces.CombinedGateway {
    constructor(timeProvider: Utils.ITimeProvider, config: Config.IConfigProvider, pair: Models.CurrencyPair, http: BitstampHttp, minTickIncrement: number, feeAmount: number, priceDecimalPlaces: number, amountDecimalPlaces: number) {
        const symbol = new BitstampSymbolProvider(pair);
        const socket = new BitstampWebsocket(config);

        var orderGateway = config.GetString("BitstampOrderDestination") == "Bitstamp"
            ? <Interfaces.IOrderEntryGateway>new BitstampOrderEntryGateway(timeProvider, http, symbol, priceDecimalPlaces, amountDecimalPlaces)
            : new NullGateway.NullOrderGateway();

        super(
            new BitstampMarketDataGateway(socket, symbol, http),
            orderGateway,
            new BitstampPositionGateway(http, pair),
            new BitstampBaseGateway(minTickIncrement, feeAmount));
    }
}

export async function createBitstamp(timeProvider: Utils.ITimeProvider, config: Config.IConfigProvider, pair: Models.CurrencyPair): Promise<Interfaces.CombinedGateway> {
    const monitor = new RateLimitMonitor(800, moment.duration(1, "minutes"));
    const signer = new BitstampMessageSigner(config);
    const http = new BitstampHttp(config, signer, monitor);
    const symbol = new BitstampSymbolProvider(pair);

    // fetch product details, to get the correct minimal tick increment
    const productDetails = await http.get("v2/trading-pairs-info/");
    let minTickIncrement = -1;
    let priceDecimalPlaces = undefined;
    let amountDecimalPlaces = undefined;
    for (const pairDetails of <any>productDetails.data) {
        if (pairDetails.url_symbol == symbol.symbolWithoutUnderscore) {
            minTickIncrement = 10 ** (-1 * parseInt(pairDetails.counter_decimals));
            priceDecimalPlaces = parseInt(pairDetails.counter_decimals);
            amountDecimalPlaces = parseInt(pairDetails.base_decimals);
            break;
        }
    }
    if (minTickIncrement < 0) {
        throw new Error("unable to match pair to a bitstamp symbol " + pair.toString());
    }

    // fetch account balance, to get the user's fee amount
    let feeAmount = 0.005;
    const feeResponse = await http.post("v2/balance/", {});
    const pairFee = <any>feeResponse.data[symbol.symbolWithoutUnderscore + "_fee"];
    if (pairFee) {
        feeAmount = parseFloat(pairFee) / 100;
    } else {
        throw new Error("unable to fetch fee data for bitstamp symbol " + pair.toString());
    }

    return new Bitstamp(timeProvider, config, pair, http, minTickIncrement, feeAmount, priceDecimalPlaces, amountDecimalPlaces);
}
