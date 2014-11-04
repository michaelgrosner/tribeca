/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />
/// <reference path="null.ts" />

module OkCoin {
    var ws = require('ws');
    var crypto = require("crypto");
    var request = require("request");
    var url = require("url");
    var querystring = require("querystring");

    interface OkCoinMessageIncomingMessage {
        channel : string;
        success : string;
        data : any;
        event? : string;
    }

    interface OkCoinDepthMessage {
        asks : Array<Array<number>>;
        bids : Array<Array<number>>;
        timestamp : string;
    }

    interface OkCoinExecutionReport {
        createdDate : string;
        userid : string;
        tradeType : string;
        tradeAmount : number;
        tradeUnitPrice : number;
        completedTradeAmount : number;
        tradePrice : number;
        averagePrice : number;
        status : number;
    }

    interface OkCoinOrderAck {
        result : boolean;
        order_id : string;
    }

    interface OkCoinOutgoingMessage {
        sign : string;
        partner : string;
    }

    class OkCoinSocket {
        subscribe<T>(channel : string, handler: (newMsg : Timestamped<T>) => void) {
            var subsReq = {event: 'addChannel',
                           channel: channel,
                           parameters: {partner: this._partner,
                                        secretkey: this._secretKey}};
            this._ws.send(JSON.stringify(subsReq));
            this._handlers[channel] = handler;
        }

        private onMessage = (raw : string) => {
            var t = date();
            try {
                var msg : OkCoinMessageIncomingMessage = JSON.parse(raw)[0];

                if (typeof msg.event !== "undefined" && msg.event == "ping") {
                    this._ws.send(this._serializedHeartbeat);
                    return;
                }

                if (typeof msg.success !== "undefined") {
                    if (msg.success !== "true")
                        this._log("Unsuccessful message %o", msg);
                    else
                        this._log("Successfully connected to %s", msg.channel);
                    return;
                }

                var handler = this._handlers[msg.channel];

                if (typeof handler === "undefined") {
                    this._log("Got message on unknown topic %o", msg);
                    return;
                }

                handler(new Timestamped(msg.data, t));
            }
            catch (e) {
                this._log("Error parsing msg %o", raw);
                throw e;
            }
        };

        ConnectChanged = new Evt<ConnectivityStatus>();
        _serializedHeartbeat = JSON.stringify({event: "pong"});
        _log : Logger = log("tribeca:gateway:OkCoinSocket");
        _secretKey : string;
        _partner : string;
        _handlers : { [channel : string] : (newMsg : Timestamped<any>) => void} = {};
        _ws : any;
        constructor(config : IConfigProvider) {
            this._partner = config.GetString("OkCoinPartner");
            this._secretKey = config.GetString("OkCoinSecretKey");
            this._ws = new ws(config.GetString("OkCoinWsUrl"));

            this._ws.on("open", () => this.ConnectChanged.trigger(ConnectivityStatus.Connected));
            this._ws.on("message", this.onMessage);
            this._ws.on("close", () => this.ConnectChanged.trigger(ConnectivityStatus.Disconnected));
        }
    }

    class OkCoinMarketDataGateway implements IMarketDataGateway {
        MarketData = new Evt<MarketUpdate>();
        ConnectChanged = new Evt<ConnectivityStatus>();

        private onDepth = (tsMsg : Timestamped<OkCoinDepthMessage>) => {
            var msg = tsMsg.data;
            var getLevel = n => {
                return new MarketUpdate(
                    new MarketSide(msg.bids[n][0], msg.bids[n][1]),
                    new MarketSide(msg.asks[n][0], msg.asks[n][1]),
                    tsMsg.time);
            };

            this.MarketData.trigger(getLevel(0));
        };

        constructor(socket : OkCoinSocket) {
            socket.ConnectChanged.on(cs => {
                if (cs == ConnectivityStatus.Connected)
                    socket.subscribe("ok_btcusd_depth", this.onDepth);
                this.ConnectChanged.trigger(cs);
            });
        }
    }

    class OkCoinHttp {
        private signMsg = (m : Map<string, string>) => {
            var els : string[] = [];

            var keys = [];
            for (var key in m) {
                if (m.hasOwnProperty(key))
                    keys.push(key);
            }
            keys.sort();

            for (var i = 0; i < keys.length; i++) {
                var key = keys[i];
                if (m.hasOwnProperty(key))
                    els.push(key + "=" + m[key]);
            }

            var sig = els.join("&") + "&secret_key=" + this._secretKey;
            return crypto.createHash('md5', this._secretKey).update(sig).digest("hex").toString().toUpperCase();
        };

        post = <T>(actionUrl: string, msg : any, cb : (m : Timestamped<T>) => void) => {
            msg.partner = this._partner;
            msg.sign = this.signMsg(msg);

            var t_start = date();
            request({
                url: url.resolve(this._baseUrl, actionUrl),
                body: querystring.stringify(msg),
                headers: {"Content-Type": "application/x-www-form-urlencoded"},
                method: "POST"
            }, (err, resp, body) => {
                try {
                    var t = date();
                    cb(new Timestamped(JSON.parse(body), t));
                    this._log("POST to OKCoin took %s ms", t.diff(t_start));
                }
                catch (e) {
                    this._log("url: %s, err: %o, body: %o", actionUrl, err, body);
                    throw e;
                }
            });
        };

        _log : Logger = log("tribeca:gateway:OkCoinHTTP");
        _secretKey : string;
        _partner : string;
        _baseUrl : string;
        constructor(config : IConfigProvider) {
            this._partner = config.GetString("OkCoinPartner");
            this._secretKey = config.GetString("OkCoinSecretKey");
            this._baseUrl = config.GetString("OkCoinHttpUrl")
        }
    }

    class OkCoinOrderEntryGateway implements IOrderEntryGateway {
        OrderUpdate = new Evt<OrderStatusReport>();
        ConnectChanged = new Evt<ConnectivityStatus>();

        sendOrder = (order : BrokeredOrder) : OrderGatewayActionReport => {
            var o = {
                symbol: "btc_usd",
                type: order.side == Side.Bid ? "buy" : "sell",
                price: order.price,
                amount: order.quantity};

            this._http.post("trade.do", o, (tsMsg : Timestamped<OkCoinOrderAck>) => {
                this._log("trade.do %o", tsMsg);
                this.OrderUpdate.trigger({
                    orderId: order.orderId,
                    exchangeId: tsMsg.data.order_id,
                    orderStatus: tsMsg.data.result == true ? OrderStatus.Working : OrderStatus.Rejected
                })
            });

            return new OrderGatewayActionReport(date());
        };

        cancelOrder = (cancel : BrokeredCancel) : OrderGatewayActionReport => {
            var c = {symbol: "btc_usd", order_id: cancel.exchangeId};
            this._http.post("cancel_order.do", c, (tsMsg : Timestamped<OkCoinOrderAck>) => {
                this._log("cancel_order.do %o", tsMsg);
                this.OrderUpdate.trigger({
                    orderId: cancel.clientOrderId,
                    orderStatus: tsMsg.data.result == true ? OrderStatus.Cancelled : OrderStatus.Rejected,
                    cancelRejected: tsMsg.data.result != true
                })
            });

            return new OrderGatewayActionReport(date());
        };

        replaceOrder = (replace : BrokeredReplace) : OrderGatewayActionReport => {
            this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
            return this.sendOrder(replace);
        };

        private static getStatus(code: number) : OrderStatus {
            switch (code) {
                case 0: return OrderStatus.Working;
                case 1: return OrderStatus.Working;
                case 2: return OrderStatus.Complete;
                case -1: return OrderStatus.Cancelled;
                default: return OrderStatus.Other;
            }
        }

        private onMessage = (tsMsg : Timestamped<OkCoinExecutionReport>) => {
            this._log("%o status %s", tsMsg, OrderStatus[OkCoinOrderEntryGateway.getStatus(tsMsg.data.status)]);
        };

        _log : Logger = log("tribeca:gateway:OkCoinOE");
        constructor(private _socket : OkCoinSocket,
                    private _http : OkCoinHttp) {
            this._socket.ConnectChanged.on(cs => {
                if (cs == ConnectivityStatus.Connected)
                    this._socket.subscribe("ok_usd_realtrades", this.onMessage);
                this.ConnectChanged.trigger(cs)
            });
        }
    }

    class OkCoinPositionGateway implements IPositionGateway {
        PositionUpdate = new Evt<CurrencyPosition>();

        constructor(config : IConfigProvider) {}
    }

    class OkCoinBaseGateway implements IExchangeDetailsGateway {
        name() : string {
            return "OkCoin";
        }

        makeFee() : number {
            return 0.0005;
        }

        takeFee() : number {
            return 0.002;
        }

        exchange() : Exchange {
            return Exchange.OkCoin;
        }
    }

    export class OkCoin extends CombinedGateway {
        constructor(config : IConfigProvider) {
            var http = new OkCoinHttp(config);
            var socket = new OkCoinSocket(config);
            super(
                new OkCoinMarketDataGateway(socket),
                config.GetString("OkCoinOrderDestination") == "OkCoin" ? <IOrderEntryGateway>new OkCoinOrderEntryGateway(socket, http) : new NullOrderGateway(),
                new NullPositionGateway(), //new OkCoinPositionGateway(config),
                new OkCoinBaseGateway());
            }
    }
}

