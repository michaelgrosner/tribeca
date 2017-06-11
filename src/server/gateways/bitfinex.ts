import ws = require('uws');
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
import * as Promises from '../promises';

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
        var t = new Date();
        try {
            var msg:any = JSON.parse(raw);
            if (typeof msg === "undefined")
              throw new Error("Unkown message from Bitfinex socket: " + raw);
            else if (typeof msg.event !== "undefined" && msg.event == "subscribed") {
                this._handlers['c'+msg.chanId] = this._handlers[msg.channel];
                delete this._handlers[msg.channel];
                console.info(new Date().toISOString().slice(11, -1), 'bitfinex', 'Successfully connected to', msg.channel);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "auth") {
                this._handlers['c'+msg.chanId] = this._handlers[msg.event];
                delete this._handlers[msg.channel];
                console.info(new Date().toISOString().slice(11, -1), 'bitfinex', 'Authentication status:', msg.status);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "info") {
                console.info(new Date().toISOString().slice(11, -1), 'bitfinex', 'Bitfinex info:', msg);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "ping") {
                this._ws.send(this._serializedHeartbeat);
                return;
            }
            else if (typeof msg.event !== "undefined" && msg.event == "error") {
                console.info(new Date().toISOString().slice(11, -1), 'bitfinex', 'Error on Bitfinex API:', msg.code+':'+msg.msg);
                return;
            }
            if (msg[1] == 'hb' || (typeof msg.event !== "undefined" && msg.event == "pong")) {
                this._stillAlive = true;
                return;
            }
            else if (msg[1]=='n') {
              console.info(new Date().toISOString().slice(11, -1), 'bitfinex', 'Bitfinex notice:', msg[2][6], msg[2][7]);
              return;
            }

            var handler = this._handlers['c'+msg[0]];

            if (typeof handler === "undefined") {
                console.warn(new Date().toISOString().slice(11, -1), 'bitfinex', 'Got message on unknown topic', msg);
                return;
            }

            if (typeof msg[1][0] == 'object')
              handler(new Models.Timestamped(msg[1], t));
            else {
              if (['hos','hts'].indexOf(msg[1]) > -1) return;
              handler(new Models.Timestamped(
                ['on','os','ou','oc'].indexOf(msg[1]) > -1
                  ? [msg[1], msg[2]]
                  : [msg[1] == 'te' ? msg[2] : msg[1]],
                t
              ));
            }
        }
        catch (e) {
            console.error(new Date().toISOString().slice(11, -1), 'bitfinex', e, 'Error parsing msg', raw);
            throw e;
        }
    };

    private connectWS = (config: Config.ConfigProvider) => {
        this._ws = new ws(config.GetString("BitfinexWebsocketUrl"));
        this._ws.on("open", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        this._ws.on("message", this.onMessage);
        this._ws.on("error", (x) => console.error(new Date().toISOString().slice(11, -1), 'bitfinex', 'WS ERROR', x));
        this._ws.on("close", () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private _serializedHeartping = JSON.stringify({event: "ping"});
    private _serializedHeartbeat = JSON.stringify({event: "pong"});
    private _stillAlive: boolean = true;
    private _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
    private _ws : ws;
    constructor(config : Config.ConfigProvider) {
        this.connectWS(config);
        setInterval(() => {
          if (!this._stillAlive) {
            console.warn(new Date().toISOString().slice(11, -1), 'bitfinex', 'Heartbeat lost, reconnecting...');
            this._stillAlive = true;
            this.connectWS(config);
          } else this._stillAlive = false;
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
        this.mkt.bids = this.mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0, 21);
        this.mkt.asks = this.mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0, 21);

        let _bids = this.mkt.bids.slice(0, 13);
        let _asks = this.mkt.asks.slice(0, 13);
        if (_bids.length && _asks.length)
          this.MarketData.trigger(new Models.Market(_bids, _asks, depth.time));
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

class BitfinexOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => {
        var d = Promises.defer<number>();
        this._http
            .post<any, any[]>("orders", {})
            .then(resps => {
                _.forEach(resps.data, t => {
                    this._http
                        .post<any, any>("order/cancel", { order_id: t.id })
                        .then(resp => {
                            if (typeof resp.data.message !== "undefined")
                                return;
                            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
                                exchangeId: t.id,
                                leavesQuantity: 0,
                                time: resp.time,
                                orderStatus: Models.OrderStatus.Cancelled
                            });
                        });
                });
                d.resolve(resps.data.length);
            });
        return d.promise;
    };

    generateClientOrderId = (): number => parseInt(new Date().valueOf().toString().substr(-9), 10);

    public cancelsByClientOrderId = false;

    sendOrder = (order: Models.OrderStatusReport) => {
        this._socket.send("on", {
            gid: 0,
            cid: order.orderId,
            amount: (order.quantity * (order.side == Models.Side.Bid ? 1 : -1)).toString(),
            price: order.price.toString(),
            symbol: 't'+this._symbolProvider.symbol.toUpperCase(),
            type: encodeTimeInForce(order.timeInForce, order.type),
            postonly: +order.preferPostOnly
        }, () => {
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
                orderId: order.orderId,
                computationalLatency: new Date().valueOf() - order.time.valueOf()
            });
        });
    };

    private onOrderAck = (orders: any[], time: Date) => {
        orders.forEach(order => {
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              orderId: order[2],
              time: time,
              exchangeId: order[0],
              orderStatus: BitfinexOrderEntryGateway.GetOrderStatus(order[13]),
              leavesQuantity: Math.abs(order[6]),
              lastPrice: order[16],
              lastQuantity: Math.abs(Math.abs(order[7]) - Math.abs(order[6])),
              averagePrice: order[17],
              side: order[7] > 0 ? Models.Side.Bid : Models.Side.Ask,
              cumQuantity: Math.abs(Math.abs(order[7]) - Math.abs(order[6]))
            });
        });
    };

    cancelOrder = (cancel: Models.OrderStatusReport) => {
        this._socket.send("oc", {
            id: cancel.exchangeId
        }, () => {
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
                orderId: cancel.orderId,
                leavesQuantity: 0,
                time: cancel.time,
                orderStatus: Models.OrderStatus.Cancelled
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
          if (['on','ou','oc'].indexOf(msg.data[0])>-1) this.onOrderAck([msg.data[1]], timeProvider.utcNow());
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

    constructor(config : Config.ConfigProvider) {
        this._api_key = config.GetString("BitfinexKey");
        this._secretKey = config.GetString("BitfinexSecret");
    }
}

class BitfinexHttp {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private _timeout = 15000;

    get = <T>(actionUrl: string, qs?: any): Promise<Models.Timestamped<T>> => {
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
    post = <TRequest, TResponse>(actionUrl: string, msg: TRequest): Promise<Models.Timestamped<TResponse>> => {
        return this.postOnce<TRequest, TResponse>(actionUrl, _.clone(msg)).then(resp => {
            var rejectMsg: string = (<any>(resp.data)).message;
            if (typeof rejectMsg !== "undefined" && rejectMsg.indexOf("Nonce is too small") > -1)
                return this.post<TRequest, TResponse>(actionUrl, _.clone(msg));
            else
                return resp;
        });
    }

    private postOnce = <TRequest, TResponse>(actionUrl: string, msg: TRequest): Promise<Models.Timestamped<TResponse>> => {
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

    private doRequest = <TResponse>(msg: request.Options, url: string): Promise<Models.Timestamped<TResponse>> => {
        var d = Promises.defer<Models.Timestamped<TResponse>>();

        request(msg, (err, resp, body) => {
            if (err) {
                console.error(new Date().toISOString().slice(11, -1), 'bitfinex', err, 'Error returned: url=', url, 'err=', err);
                d.reject(err);
            }
            else {
                try {
                    var t = new Date();
                    var data = JSON.parse(body);
                    d.resolve(new Models.Timestamped(data, t));
                }
                catch (err) {
                    console.error(new Date().toISOString().slice(11, -1), 'bitfinex', err, 'Error parsing JSON url=', url, 'err=', err, ', body=', body);
                    d.reject(err);
                }
            }
        });

        return d.promise;
    };

    private _baseUrl: string;
    private _apiKey: string;
    private _secret: string;
    private _nonce: number;

    constructor(config: Config.ConfigProvider) {
        this._baseUrl = config.GetString("BitfinexHttpUrl");
        this._apiKey = config.GetString("BitfinexKey");
        this._secret = config.GetString("BitfinexSecret");

        this._nonce = new Date().valueOf();
        console.info(new Date().toISOString().slice(11, -1), 'bitfinex', 'Starting nonce:', this._nonce);
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
        });
    }

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
    constructor(timeProvider: Utils.ITimeProvider, config: Config.ConfigProvider, symbol: BitfinexSymbolProvider, pricePrecision: number, minSize: number) {
        const http = new BitfinexHttp(config);
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

interface SymbolTicker {
  mid: string,
  bid: string,
  ask: string,
  last_price: string,
  low: string,
  high: string,
  volume: string
}

export async function createBitfinex(timeProvider: Utils.ITimeProvider, config: Config.ConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    const detailsUrl = config.GetString("BitfinexHttpUrl")+"/symbols_details";
    const symbolDetails = await getJSON<SymbolDetails[]>(detailsUrl);
    const symbol = new BitfinexSymbolProvider(pair);

    for (let s of symbolDetails) {
        if (s.pair === symbol.symbol) {
            const tickerUrl = config.GetString("BitfinexHttpUrl")+"/pubticker/"+s.pair;
            const symbolTicker = await getJSON<SymbolTicker>(tickerUrl);
            const precisePrice = parseFloat(symbolTicker.last_price).toPrecision(s.price_precision).toString();
            return new Bitfinex(timeProvider, config, symbol, parseFloat('1e-'+precisePrice.substr(0, precisePrice.length-1).concat('1').replace(/^-?\d*\.?|0+$/g, '').length), parseFloat(s.minimum_order_size));
        }
    }
}
