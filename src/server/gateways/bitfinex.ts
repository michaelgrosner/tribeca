import ws = require('ws');
import Q = require("q");
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import querystring = require("querystring");
import Config = require("../config");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import moment = require("moment");
import _ = require("lodash");
import log from "../logging";
var shortId = require("shortid");
var Deque = require("collections/deque");

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
        default: return Models.Side.Unknown;
    }
}

function encodeSide(side: Models.Side) {
    switch (side) {
        case Models.Side.Bid: return "buy";
        case Models.Side.Ask: return "sell";
        default: return "";
    }
}

function encodeTimeInForce(tif: Models.TimeInForce, type: Models.OrderType) {
    if (type === Models.OrderType.Market) {
        return "exchange market";
    }
    else if (type === Models.OrderType.Limit) {
        if (tif === Models.TimeInForce.FOK) return "exchange fill-or-kill";
        if (tif === Models.TimeInForce.GTC) return "exchange limit";
    }
    throw new Error("unsupported tif " + Models.TimeInForce[tif] + " and order type " + Models.OrderType[type]);
}

function getJSON<T>(url: string, qs?: any) : Promise<T> {
    return new Promise((resolve, reject) => {
        request({url: url, qs: qs}, (err: Error, resp, body) => {
            if (err) {
                reject(err);
            }
            else {
                try {
                    resolve(JSON.parse(body));
                }
                catch (e) {
                    reject(e);
                }
            }
        });
    });
}

class BitfinexWebsocket {
	send = <T>(channel : string, parameters: any, cb?: () => void) => {
        var subsReq : any = {event: 'subscribe', channel: channel};

        if (parameters !== null)
            subsReq = Object.assign(subsReq, parameters);

        this._ws.send(JSON.stringify(subsReq), (e: Error) => {
            if (!e && cb) cb();
        });
    }

    setHandler = <T>(channel : string, handler: (newMsg : Models.Timestamped<T>) => void) => {
        this._handlers[channel] = handler;
    }

    private onMessage = (raw : string) => {
        var t = Utils.date();
        try {
            var msg:any = JSON.parse(raw);
            if (typeof msg === "undefined") throw new Error("Unkown message from Bitfinex socket: " + raw);

            if (typeof msg.event !== "undefined" && msg.event == "subscribed") {
                this._handlers[msg.chanId] = this._handlers[msg.channel];
                delete this._handlers[msg.channel];
                this._log.info("Successfully connected to %s", msg.channel);
                return;
            }
            if (typeof msg.event !== "undefined" && msg.event == "ping") {
                this._ws.send(this._serializedHeartbeat);
                return;
            }
            if (typeof msg.event !== "undefined" && msg.event == "error") {
                this._log.info("Error on Bitfinex API:", msg.code+':'+msg.msg);
                return;
            }
            if (msg[1] == 'hb' || (typeof msg.event !== "undefined" && msg.event == "pong")) {
                this._stillAlive = true;
                return;
            }

            var handler = this._handlers[msg[0]];

            if (typeof handler === "undefined") {
                this._log.warn("Got message on unknown topic", msg);
                return;
            }

            if (typeof msg[1][0] == 'object')
              handler(new Models.Timestamped(msg[1], t));
            else
              handler(new Models.Timestamped([msg[1] == 'tu' ? msg[2] : msg[1]], t));
        }
        catch (e) {
            this._log.error(e, "Error parsing msg", raw);
            throw e;
        }
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private _serializedHeartping = JSON.stringify({event: "ping"});
    private _serializedHeartbeat = JSON.stringify({event: "pong"});
    private _stillAlive: boolean = true;
    private _log = log("tribeca:gateway:BitfinexWebsocket");
    private _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
    private _ws : ws;
    constructor(config : Config.IConfigProvider) {
        this._ws = new ws(config.GetString("BitfinexWebsocketUrl"));
        this._ws.on("open", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("close", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
        setInterval(() => {
          if (!this._stillAlive) this._log.info('Bitfinex heartbeat lost.');
          this._stillAlive = false;
          this._ws.send(this._serializedHeartping);
        }, 7000);
    }
}

class BitfinexMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private onTrade = (trades: Models.Timestamped<any[]>) => {
        trades.data.forEach(trade => {
            if (trade=='te') return;
            var px = trade[3];
            var sz = Math.abs(trade[2]);
            var time = trades.time;
            var side = trade[2] > 0 ? Models.Side.Bid : Models.Side.Ask;
            var mt = new Models.GatewayMarketTrade(px, sz, time, false, side);
            this.MarketTrade.trigger(mt);
        });
    };

    MarketData = new Utils.Evt<Models.Market>();

    private mkt = new Models.Market([], [], null);

    private onDepth = (depth : Models.Timestamped<any>) => {
        depth.data.forEach(x => {
          var side = x[2] > 0 ? 'bids' : 'asks';
          this.mkt[side] = this.mkt[side].filter(a => a.price != x[0]);
          if (x[1]) this.mkt[side].push(new Models.MarketSide(x[0], Math.abs(x[2])));
        });
        this.mkt.bids = this.mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0,13);
        this.mkt.asks = this.mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0,13);
        this.mkt.time = depth.time;

        this.MarketData.trigger(this.mkt);
    };

    constructor(
        timeProvider: Utils.ITimeProvider,
        private _socket: BitfinexWebsocket,
        private _symbolProvider: BitfinexSymbolProvider) {
        var depthChannel = "book";
        var tradesChannel = "trades";
        _socket.setHandler(depthChannel, this.onDepth);
        _socket.setHandler(tradesChannel, this.onTrade);

        _socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);

            if (cs == Models.ConnectivityStatus.Connected) {
                _socket.send(depthChannel, {
                  'symbol': 't'+_symbolProvider.symbol.toUpperCase(),
                  'prec': 'P0',
                  'freq': 'F0'
                });
                _socket.send(tradesChannel, {
                  'symbol': 't'+_symbolProvider.symbol.toUpperCase()
                });
            }
        });
    }
}

interface RejectableResponse {
    message: string;
}

interface BitfinexNewOrderRequest {
    symbol: string;
    amount: string;
    price: string; //Price to buy or sell at. Must be positive. Use random number for market orders.
    exchange: string; //always "bitfinex"
    side: string; // buy or sell
    type: string; // "market" / "limit" / "stop" / "trailing-stop" / "fill-or-kill" / "exchange market" / "exchange limit" / "exchange stop" / "exchange trailing-stop" / "exchange fill-or-kill". (type starting by "exchange " are exchange orders, others are margin trading orders)
    is_hidden?: boolean;
}

interface BitfinexNewOrderResponse extends RejectableResponse {
    order_id: string;
    remaining_amount?: string;
}

interface BitfinexCancelOrderRequest {
    order_id: string;
}

interface BitfinexCancelReplaceOrderRequest extends BitfinexNewOrderRequest {
    order_id: string;
}

interface BitfinexCancelReplaceOrderResponse extends BitfinexCancelOrderRequest, RejectableResponse { }

interface BitfinexOrderStatusRequest {
    order_id: string;
}

interface BitfinexMyTradesRequest {
    symbol: string;
    timestamp: number;
}

interface BitfinexMyTradesResponse extends RejectableResponse {
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

interface BitfinexOrderStatusResponse extends RejectableResponse {
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
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Q.Promise<number> => {
        var d = Q.defer<number>();
        this._http
            .post<any, any[]>("orders", {})
            .then(resps => {
                _.forEach(resps.data, t => {
                    var req = { order_id: t.id };
                    this._http
                        .post<BitfinexCancelOrderRequest, any>("order/cancel", req)
                        .then(resp => {
                            if (typeof resp.data.message !== "undefined")
                                return;

                            this.OrderUpdate.trigger({
                                exchangeId: t.id,
                                leavesQuantity: 0,
                                time: resp.time,
                                orderStatus: Models.OrderStatus.Cancelled,
                                done: true
                            });
                        })
                        .done();
                });
                d.resolve(resps.data.length);
            }).done();
        return d.promise;
    };

    generateClientOrderId = () => shortId.generate();

    public cancelsByClientOrderId = false;

    private convertToOrderRequest = (order: Models.OrderStatusReport): BitfinexNewOrderRequest => {
        return {
            amount: order.quantity.toString(),
            exchange: "bitfinex",
            price: order.price.toString(),
            side: encodeSide(order.side),
            symbol: this._symbolProvider.symbol,
            type: encodeTimeInForce(order.timeInForce, order.type)
        };
    }

    sendOrder = (order: Models.OrderStatusReport) => {
        var req = this.convertToOrderRequest(order);

        this._http
            .post<BitfinexNewOrderRequest, BitfinexNewOrderResponse>("order/new", req)
            .then(resp => {
                if (typeof resp.data.message !== "undefined") {
                    this.OrderUpdate.trigger({
                        orderStatus: Models.OrderStatus.Rejected,
                        orderId: order.orderId,
                        rejectMessage: resp.data.message,
                        time: resp.time
                    });
                    return;
                }

                this.OrderUpdate.trigger({
                    orderId: order.orderId,
                    exchangeId: resp.data.order_id,
                    leavesQuantity: parseFloat(resp.data.remaining_amount),
                    time: resp.time,
                    orderStatus: Models.OrderStatus.Working
                });
            }).done();

        this.OrderUpdate.trigger({
            orderId: order.orderId,
            computationalLatency: (new Date()).getTime() - order.time.getTime()
        });
    };

    cancelOrder = (cancel: Models.OrderStatusReport) => {
        var req = { order_id: cancel.exchangeId };
        this._http
            .post<BitfinexCancelOrderRequest, any>("order/cancel", req)
            .then(resp => {
                if (typeof resp.data.message !== "undefined") {
                    this.OrderUpdate.trigger({
                        orderStatus: Models.OrderStatus.Rejected,
                        cancelRejected: true,
                        orderId: cancel.orderId,
                        rejectMessage: resp.data.message,
                        time: resp.time
                    });
                    return;
                }

                this.OrderUpdate.trigger({
                    orderId: cancel.orderId,
                    time: resp.time,
                    orderStatus: Models.OrderStatus.Cancelled
                });
            })
            .done();

        this.OrderUpdate.trigger({
            orderId: cancel.orderId,
            computationalLatency: (new Date()).getTime() - cancel.time.getTime()

        });
    };

    replaceOrder = (replace: Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private downloadOrderStatuses = () => {
        var tradesReq = { timestamp: this._since.unix(), symbol: this._symbolProvider.symbol };
        this._http
            .post<BitfinexMyTradesRequest, BitfinexMyTradesResponse[]>("mytrades", tradesReq)
            .then(resps => {
                _.forEach(resps.data, t => {
                    this._http
                        .post<BitfinexOrderStatusRequest, BitfinexOrderStatusResponse>("order/status", { order_id: t.order_id })
                        .then(r => {
                            this.OrderUpdate.trigger({
                                exchangeId: t.order_id,
                                lastPrice: parseFloat(t.price),
                                time: moment.utc().toDate(),
                                lastQuantity: parseFloat(r.data.remaining_amount),
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
        if (r.executed_amount === r.original_amount) return Models.OrderStatus.Complete;
        return Models.OrderStatus.Other;
    }

    private _since = moment.utc();
    private _log = log("tribeca:gateway:BitfinexOE");
    constructor(
        timeProvider: Utils.ITimeProvider,
        private _details: BitfinexBaseGateway,
        private _http: BitfinexHttp,
        private _symbolProvider: BitfinexSymbolProvider) {

        _http.ConnectChanged.on(s => this.ConnectChanged.trigger(s));
        timeProvider.setInterval(this.downloadOrderStatuses, moment.duration(8, "seconds"));
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

class BitfinexHttp {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private _timeout = 15000;

    get = <T>(actionUrl: string, qs?: any): Q.Promise<Models.Timestamped<T>> => {
        const url = this._baseUrl + "/" + actionUrl;
        var opts = {
            timeout: this._timeout,
            url: url,
            qs: qs || undefined,
            method: "GET"
        };

        return this.doRequest<T>(opts, url);
    };

    // Bitfinex seems to have a race condition where nonces are processed out of order when rapidly placing orders
    // Retry here - look to mitigate in the future by batching orders?
    post = <TRequest, TResponse>(actionUrl: string, msg: TRequest): Q.Promise<Models.Timestamped<TResponse>> => {
        return this.postOnce<TRequest, TResponse>(actionUrl, _.clone(msg)).then(resp => {
            var rejectMsg: string = (<any>(resp.data)).message;
            if (typeof rejectMsg !== "undefined" && rejectMsg.indexOf("Nonce is too small") > -1)
                return this.post<TRequest, TResponse>(actionUrl, _.clone(msg));
            else
                return resp;
        });
    }

    private postOnce = <TRequest, TResponse>(actionUrl: string, msg: TRequest): Q.Promise<Models.Timestamped<TResponse>> => {
        msg["request"] = "/v1/" + actionUrl;
        msg["nonce"] = this._nonce.toString();
        this._nonce += 1;

        var payload = new Buffer(JSON.stringify(msg)).toString("base64");
        var signature = crypto.createHmac("sha384", this._secret).update(payload).digest('hex');

        const url = this._baseUrl + "/" + actionUrl;
        var opts: request.Options = {
            timeout: this._timeout,
            url: url,
            headers: {
                "X-BFX-APIKEY": this._apiKey,
                "X-BFX-PAYLOAD": payload,
                "X-BFX-SIGNATURE": signature
            },
            method: "POST"
        };

        return this.doRequest<TResponse>(opts, url);
    };

    private doRequest = <TResponse>(msg: request.Options, url: string): Q.Promise<Models.Timestamped<TResponse>> => {
        var d = Q.defer<Models.Timestamped<TResponse>>();

        this._monitor.add();
        request(msg, (err, resp, body) => {
            if (err) {
                this._log.error(err, "Error returned: url=", url, "err=", err);
                d.reject(err);
            }
            else {
                try {
                    var t = new Date();
                    var data = JSON.parse(body);
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (err) {
                    this._log.error(err, "Error parsing JSON url=", url, "err=", err, ", body=", body);
                    d.reject(err);
                }
            }
        });

        return d.promise;
    };

    private _log = log("tribeca:gateway:BitfinexHTTP");
    private _baseUrl: string;
    private _apiKey: string;
    private _secret: string;
    private _nonce: number;

    constructor(config: Config.IConfigProvider, private _monitor: RateLimitMonitor) {
        this._baseUrl = config.GetString("BitfinexHttpUrl");
        this._apiKey = config.GetString("BitfinexKey");
        this._secret = config.GetString("BitfinexSecret");

        this._nonce = new Date().valueOf();
        this._log.info("Starting nonce: ", this._nonce);
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 10);
    }
}

interface BitfinexPositionResponseItem {
    type: string;
    currency: string;
    amount: string;
    available: string;
}

class BitfinexPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private onRefreshPositions = () => {
        this._http.post<{}, BitfinexPositionResponseItem[]>("balances", {}).then(res => {
            _.forEach(_.filter(res.data, x => x.type === "exchange"), p => {
                var amt = parseFloat(p.amount);
                var cur = Models.toCurrency(p.currency);
                var held = amt - parseFloat(p.available);
                var rpt = new Models.CurrencyPosition(amt, held, cur);
                this.PositionUpdate.trigger(rpt);
            });
        }).done();
    }

    private _log = log("tribeca:gateway:BitfinexPG");
    constructor(timeProvider: Utils.ITimeProvider, private _http: BitfinexHttp) {
        timeProvider.setInterval(this.onRefreshPositions, moment.duration(15, "seconds"));
        this.onRefreshPositions();
    }
}

class BitfinexBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    name(): string {
        return "Bitfinex";
    }

    makeFee(): number {
        return 0.001;
    }

    takeFee(): number {
        return 0.002;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.Bitfinex;
    }

    constructor(public minTickIncrement: number, public minSize: number) {}
}

class BitfinexSymbolProvider {
    public symbol: string;

    constructor(pair: Models.CurrencyPair) {
        this.symbol = Models.fromCurrency(pair.base).toLowerCase() + Models.fromCurrency(pair.quote).toLowerCase();
    }
}

class Bitfinex extends Interfaces.CombinedGateway {
    constructor(timeProvider: Utils.ITimeProvider, config: Config.IConfigProvider, symbol: BitfinexSymbolProvider, pricePrecision: number, minSize: number) {
        const monitor = new RateLimitMonitor(60, moment.duration(1, "minutes"));
        const http = new BitfinexHttp(config, monitor);
        var socket = new BitfinexWebsocket(config);
        const details = new BitfinexBaseGateway(pricePrecision, minSize);

        const orderGateway = config.GetString("BitfinexOrderDestination") == "Bitfinex"
            ? <Interfaces.IOrderEntryGateway>new BitfinexOrderEntryGateway(timeProvider, details, http, symbol)
            : new NullGateway.NullOrderGateway();

        super(
            new BitfinexMarketDataGateway(timeProvider, socket, symbol),
            orderGateway,
            new BitfinexPositionGateway(timeProvider, http),
            details);
    }
}

interface SymbolDetails {
    pair: string,
    price_precision: number,
    initial_margin:string,
    minimum_margin:string,
    maximum_order_size:string,
    minimum_order_size:string,
    expiration:string
}

export async function createBitfinex(timeProvider: Utils.ITimeProvider, config: Config.IConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    const detailsUrl = config.GetString("BitfinexHttpUrl")+"/symbols_details";
    const symbolDetails = await getJSON<SymbolDetails[]>(detailsUrl);
    const symbol = new BitfinexSymbolProvider(pair);

    for (let s of symbolDetails) {
        if (s.pair === symbol.symbol)
            return new Bitfinex(timeProvider, config, symbol, parseFloat(s.minimum_order_size), parseFloat(s.minimum_order_size));
    }
}
