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
var Deque = require("collections/deque");

function encodeTimeInForce(tif: Models.TimeInForce, type: Models.OrderType) {
    if (type === Models.OrderType.Market) {
        return "EXCHANGE MARKET";
    }
    else if (type === Models.OrderType.Limit) {
        if (tif === Models.TimeInForce.FOK) return "EXCHANGE FOK";
        if (tif === Models.TimeInForce.GTC) return "EXCHANGE LIMIT";
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
	send = <T>(channel: string, parameters: any, cb?: () => void) => {
        var subsReq: any;

        if (['on','oc'].indexOf(channel)>-1) {
          subsReq = [0, channel, null, parameters];
        }
        else {
          subsReq = channel == 'auth'
            ? {event: channel} : {event: 'subscribe', channel: channel};

          if (parameters !== null)
              subsReq = Object.assign(subsReq, parameters);
        }

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
            if (typeof msg === "undefined")
              throw new Error("Unkown message from Bitfinex socket: " + raw);
            else if (typeof msg.event !== "undefined" && msg.event == "subscribed") {
                this._handlers['c'+msg.chanId] = this._handlers[msg.channel];
                delete this._handlers[msg.channel];
                this._log.info("Successfully connected to %s", msg.channel);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "auth") {
                this._handlers['c'+msg.chanId] = this._handlers[msg.event];
                delete this._handlers[msg.channel];
                this._log.info("Bitfinex authentication status:", msg.status);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "info") {
                this._log.info("Bitfinex info:", msg);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "ping") {
                this._ws.send(this._serializedHeartbeat);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "error") {
                this._log.info("Error on Bitfinex API:", msg.code+':'+msg.msg);
                return;
            }
            if (msg[1] == 'hb' || (typeof msg.event !== "undefined" && msg.event == "pong")) {
                this._stillAlive = true;
                return;
            }
            else if (msg[1]=='n') {
              this._log.info("Bitfinex notice:", msg[2][6], msg[2][7]);
              return;
            }

            var handler = this._handlers['c'+msg[0]];

            if (typeof handler === "undefined") {
                this._log.warn("Got message on unknown topic", msg);
                return;
            }

            if (typeof msg[1][0] == 'object')
              handler(new Models.Timestamped(msg[1], t));
            else {
              if (['hos','hts'].indexOf(msg[1]) > -1) return;
              handler(new Models.Timestamped(
                ['os','ou','oc'].indexOf(msg[1]) > -1
                  ? [msg[1], msg[2]]
                  : [msg[1] == 'te' ? msg[2] : msg[1]],
                t
              ));
            }
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
        this._ws.on("error", (x) => this._log.info('Bitfinex WS ERROR', x));
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
            if (trade=='tu') return;
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

interface BitfinexCancelOrderRequest {
    order_id: string;
}

interface BitfinexCancelReplaceOrderResponse extends BitfinexCancelOrderRequest, RejectableResponse { }

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

    generateClientOrderId = () => parseInt((Math.random()+'').substr(-10) ,10).toString();

    public cancelsByClientOrderId = true;


    sendOrder = (order: Models.OrderStatusReport) => {
        this._socket.send<Order>("on", <Order>{
            gid: 0,
            cid: parseInt(order.orderId,10),
            amount: (order.quantity * (order.side == Models.Side.Bid ? 1 : -1)).toString(),
            price: order.price.toFixed(this._details.minTickIncrement).toString(),
            symbol: 't'+this._symbolProvider.symbol.toUpperCase(),
            type: encodeTimeInForce(order.timeInForce, order.type)
        }, () => {
            this.OrderUpdate.trigger({
                orderId: order.orderId,
                computationalLatency: Utils.date().valueOf() - order.time.valueOf()
            });
        });
    };

    private onOrderAck = (orders: any[], time: Date) => {
        orders.forEach(order => {
            this.OrderUpdate.trigger({
              orderId: order[2].toString(),
              time: time,
              exchangeId: order[0].toString(),
              orderStatus: BitfinexOrderEntryGateway.GetOrderStatus(order[13]),
              leavesQuantity: Math.abs(order[6]),
              lastPrice: order[16],
              lastQuantity: Math.abs(Math.abs(order[7]) - Math.abs(order[6])),
              averagePrice: order[17],
              side: order[7] > 0 ? Models.Side.Bid : Models.Side.Ask,
              cumQuantity: Math.abs(Math.abs(order[7]) - Math.abs(order[6])),
              quantity: Math.abs(order[7])
            });
        });
    };

    cancelOrder = (cancel: Models.OrderStatusReport) => {
        this._socket.send<Cancel>("oc", cancel.exchangeId?<Cancel>{
            id: parseInt(cancel.exchangeId, 10)
        }:<Cancel>{
            cid: parseInt(cancel.orderId, 10),
            cid_date: moment(cancel.time).format('YYYY-MM-DD')
        }, () => {
            this.OrderUpdate.trigger({
                orderId: cancel.orderId,
                leavesQuantity: 0,
                time: cancel.time,
                orderStatus: Models.OrderStatus.Cancelled,
                done: true
            });
        });
    };

    replaceOrder = (replace: Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private static GetOrderStatus(r: string) {
        switch(r.split(' ')[0]) {
          case 'ACTIVE': return Models.OrderStatus.Working;
          case 'EXECUTED': return Models.OrderStatus.Complete;
          case 'PARTIALLY': return Models.OrderStatus.Working;
          case 'CANCELED': return Models.OrderStatus.Cancelled;
          default: return Models.OrderStatus.Other;
        }
    }

    private _log = log("tribeca:gateway:BitfinexOE");
    constructor(
        timeProvider: Utils.ITimeProvider,
        private _details: BitfinexBaseGateway,
        private _http: BitfinexHttp,
        private _socket: BitfinexWebsocket,
        private _signer: BitfinexMessageSigner,
        private _symbolProvider: BitfinexSymbolProvider) {
        _http.ConnectChanged.on(s => this.ConnectChanged.trigger(s));

        _socket.setHandler("auth", (msg: Models.Timestamped<any>) => {
          if (typeof msg.data[1] == 'undefined' || !msg.data[1].length) return;
          if (['ou','oc'].indexOf(msg.data[0])>-1) this.onOrderAck([msg.data[1]], msg.time);
        });

        _socket.ConnectChanged.on(cs => {
            this.ConnectChanged.trigger(cs);

            if (cs === Models.ConnectivityStatus.Connected) {
                _socket.send("auth", _signer.signMessage({filter: ['trading']}));
            }
        });
    }
}

interface SignedMessage {
    apiKey?: string;
    authNonce?: number;
    authPayload?: string;
    authSig?: string;
    filter?: string[];
}

interface Order extends SignedMessage {
    id?: number;
    cid?: number;
    amount?: string;
    price?: string;
    symbol?: string;
    type?: string;
}

interface Cancel extends SignedMessage {
    id?: number;
    cid?: number;
    cid_date?: string;
}

class BitfinexMessageSigner {
    private _secretKey : string;
    private _api_key : string;

    public signMessage = (m : SignedMessage) : SignedMessage => {
        var els : string[] = [];

        if (!m.hasOwnProperty("api_key"))
            m.apiKey = this._api_key;

        var keys = [];
        for (var key in m) {
            if (m.hasOwnProperty(key))
                keys.push(key);
        }
        keys.sort();

        for (var i = 0; i < keys.length; i++) {
            const k = keys[i];
            if (m.hasOwnProperty(k))
                els.push(k + "=" + m[k]);
        }

        m.authNonce = Date.now() * 1000;
        m.authPayload = 'AUTH' + m.authNonce;
        m.authSig = crypto.createHmac("sha384", this._secretKey).update(m.authPayload).digest('hex')

        return m;
    };

    constructor(config : Config.IConfigProvider) {
        this._api_key = config.GetString("BitfinexKey");
        this._secretKey = config.GetString("BitfinexSecret");
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
        this._nonce = Date.now() * 1000;
        msg["nonce"] = this._nonce.toString();

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
                var amt = parseFloat(p.available);
                var cur = Models.toCurrency(p.currency);
                var held = parseFloat(p.amount) - amt;
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
        const signer = new BitfinexMessageSigner(config);
        const socket = new BitfinexWebsocket(config);
        const details = new BitfinexBaseGateway(pricePrecision, minSize);

        const orderGateway = config.GetString("BitfinexOrderDestination") == "Bitfinex"
            ? <Interfaces.IOrderEntryGateway>new BitfinexOrderEntryGateway(timeProvider, details, http, socket, signer, symbol)
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
