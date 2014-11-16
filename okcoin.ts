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
    var zeromq = require("zmq");

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
        exchangeId : string;
        orderId : string;
        orderStatus : string;
        rejectMessage : string;
        lastQuantity: number;
        lastPrice : number;
        leavesQuantity : number;
        cumQuantity : number;
        averagePrice : number;
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

        private onDepth = (prettyMegan : Timestamped<OkCoinDepthMessage>) => {
            var msg = prettyMegan.data;
            var getLevel = n => {
                return new MarketUpdate(
                    new MarketSide(msg.bids[n][0], msg.bids[n][1]),
                    new MarketSide(msg.asks[n][0], msg.asks[n][1]),
                    prettyMegan.time);
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
                    var data = JSON.parse(body);
                    cb(new Timestamped(data, t));
                    this._log("POST to OKCoin took %s ms : %o", t.diff(t_start), data);
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

    class OkCoinFixBridge {
        sendOrder = (order : BrokeredOrder) => {
            var o = {
                id: order.orderId,
                symbol: "BTC/USD",
                side: order.side == Side.Bid ? "buy" : "sell",
                price: order.price,
                tif: "GTC",
                amount: order.quantity};
            this._sock.send(JSON.stringify({evt: "New", obj: o}));
        };

        cancelOrder = (exchangeId : string, cancel : BrokeredCancel) => {
            var c = {
                symbol: "BTC/USD",
                origOrderId: cancel.clientOrderId,
                origExchOrderId: exchangeId,
                side: cancel.side == Side.Bid ? "buy" : "sell"
            };
            this._sock.send(JSON.stringify({evt: "Cxl", obj: c}));
        };

        _log : Logger = log("tribeca:gateway:OkCoinFIX");
        _sock : any;
        constructor() {
            this._sock = new zeromq.socket("pair");
            this._sock.connect("tcp://localhost:5556");
            this._sock.on("message", rawMsg => {
                var msg = JSON.parse(rawMsg);
                this._log("got new message %o", msg);

                if (this._handlers.hasOwnProperty(msg.evt)) {
                    this._handlers[msg.evt](new Timestamped(msg.obj, new Date(msg.ts)));
                }
                else {
                    this._log("no handler registered for inbound FIX message: %o", msg);
                }
            });
        }

        _handlers : { [channel : string] : (newMsg : Timestamped<any>) => void} = {};

        subscribe<T>(channel : string, handler: (newMsg : Timestamped<T>) => void) {
            this._handlers[channel] = handler;
        }
    }

    class OkCoinOrderEntryGateway implements IOrderEntryGateway {
        OrderUpdate = new Evt<OrderStatusReport>();
        ConnectChanged = new Evt<ConnectivityStatus>();

        sendOrder = (order : BrokeredOrder) : OrderGatewayActionReport => {
            this._socket.sendOrder(order);
            return new OrderGatewayActionReport(date());
        };

        private _cancelsWaitingForExchangeOrderId : {[clId : string] : BrokeredCancel} = {};
        cancelOrder = (cancel : BrokeredCancel) : OrderGatewayActionReport => {
            // race condition! i cannot cancel an order before I get the exchangeId (oid); register it for deletion on the ack
            if (typeof cancel.exchangeId !== "undefined") {
                this.sendCancel(cancel.exchangeId, cancel);
            }
            else {
                this._cancelsWaitingForExchangeOrderId[cancel.clientOrderId] = cancel;
                this._log("Registered %s for late deletion", cancel.clientOrderId);
            }

            return new OrderGatewayActionReport(date());
        };

        private sendCancel = (exchangeId : string, cancel : BrokeredCancel) => {
            this._socket.cancelOrder(exchangeId, cancel);
        };

        replaceOrder = (replace : BrokeredReplace) : OrderGatewayActionReport => {
            this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
            return this.sendOrder(replace);
        };

        private static getStatus(status: string) : OrderStatus {
            // these are the quickfix tags
            switch (status) {
                case '0':
                case '1':
                case '6': // pending cxl, repl, and new are all working
                case 'A':
                case 'E':
                case '5':
                    return OrderStatus.Working;
                case '2':
                    return OrderStatus.Complete;
                case '3':
                case '4':
                    return OrderStatus.Cancelled;
                case '8':
                    return OrderStatus.Rejected;
            }
            return OrderStatus.Other;
        }

        private onMessage = (tsMsg : Timestamped<OkCoinExecutionReport>) => {
            var t = tsMsg.time;
            var msg : OkCoinExecutionReport = tsMsg.data;

            // cancel any open orders waiting for oid
            if (this._cancelsWaitingForExchangeOrderId.hasOwnProperty(msg.orderId)) {
                var cancel = this._cancelsWaitingForExchangeOrderId[msg.orderId];
                this.sendCancel(msg.exchangeId, cancel);
                this._log("Deleting %s late, oid: %s", cancel.clientOrderId, msg.orderId);
                delete this._cancelsWaitingForExchangeOrderId[msg.orderId];
            }

            var orderStatus = OkCoinOrderEntryGateway.getStatus(msg.orderStatus);
            var status : OrderStatusReport = {
                exchangeId: msg.exchangeId,
                orderId: msg.orderId,
                orderStatus: orderStatus,
                time: t,
                lastQuantity: msg.lastQuantity > 0 ? msg.lastQuantity : undefined,
                lastPrice: msg.lastPrice > 0 ? msg.lastPrice : undefined,
                leavesQuantity: orderStatus == OrderStatus.Working ? msg.leavesQuantity : undefined,
                cumQuantity: msg.cumQuantity > 0 ? msg.cumQuantity : undefined,
                averagePrice: msg.averagePrice > 0 ? msg.averagePrice : undefined,
                pendingCancel: msg.orderStatus == "6",
                pendingReplace: msg.orderStatus == "E"
            };

            this.OrderUpdate.trigger(status);

            this._log("got new exec rpt %o", tsMsg);
        };

        private onConnectionStatus = (tsMsg : Timestamped<string>) => {
            if (tsMsg.data == "Logon") {
                this.ConnectChanged.trigger(ConnectivityStatus.Connected);
            }
            else if (tsMsg.data == "Logout") {
                this.ConnectChanged.trigger(ConnectivityStatus.Disconnected);
            }
            else {
                throw new Error(util.format("unknown connection status raised by FIX socket : %o", tsMsg));
            }
        };

        _log : Logger = log("tribeca:gateway:OkCoinOE");
        constructor(private _socket : OkCoinFixBridge) {
            _socket.subscribe("ConnectionStatus", this.onConnectionStatus);
            _socket.subscribe("ExecRpt", this.onMessage);
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
            var fix = new OkCoinFixBridge();
            super(
                new OkCoinMarketDataGateway(socket),
                config.GetString("OkCoinOrderDestination") == "OkCoin" ? <IOrderEntryGateway>new OkCoinOrderEntryGateway(fix) : new NullOrderGateway(),
                new NullPositionGateway(), //new OkCoinPositionGateway(config),
                new OkCoinBaseGateway());
            }
    }
}

