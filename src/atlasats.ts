/// <reference path="../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />
/// <reference path="null.ts" />
/// <reference path="config.ts" />

module AtlasAts {

    var Faye = require('faye');
    var request = require("request");
    var crypto = require('crypto');
    var url = require("url");

    // example order reject:
    //      {"limit":0.01,"reject":{"reason":"risk_buying_power"},"tif":"GTC","status":"REJECTED","type":"LIMIT","currency":"USD","executed":0,"clid":"WEB","side":"BUY","oid":"1352-101614-000056-004","item":"BTC","account":1352,"quantity":0.01,"left":0,"average":0}

    // example order fill:
    //      {"limit":390,"tif":"GTC","status":"OPEN","ack":{"oref":"4696TJEGPJYZA0","time":"2014-10-16 00:12:35"},"type":"LIMIT","currency":"USD","executed":0,"clid":"WEB","side":"SELL","oid":"1352-101614-001825-009","item":"BTC","account":1352,"quantity":0.0001,"left":0.0001,"average":0}
    //      {"executions":[{"liquidity":"R","time":"2014-10-16 00:12:35","price":392.47,"quantity":0.0001,"venue":"CROX","commission":-0.00007849400000000001,"eid":"4696TJEGPJYZA02_4696TJEGPJZ0"}],"limit":390,"tif":"GTC","status":"DONE","ack":{"oref":"4696TJEGPJYZA0","time":"2014-10-16 00:12:35"},"type":"LIMIT","currency":"USD","executed":0.0001,"clid":"WEB","side":"SELL","oid":"1352-101614-001825-009","item":"BTC","account":1352,"quantity":0.0001,"left":0,"average":392.47}

    // example cancel ack:
    //      {"limit":350,"tif":"GTC","status":"DONE","ack":{"oref":"4696TJEGPKHMA0","time":"2014-10-16 01:08:30"},"urout":{"time":"2014-10-16 01:08:38"},"type":"LIMIT","currency":"USD","executed":0,"clid":"WEB","side":"BUY","oid":"1352-101614-011421-012","item":"BTC","account":1352,"quantity":0.0001,"left":0,"average":0}

    interface AtlasAtsExecutionReportReject {
        reason : string;
    }
    interface AtlasAtsAck {
        oref : string;
        time : string;
    }
    interface AtlasAtsExecutions {
        liquidity : string;
        time : string;
        price : number;
        quantity : number;
        venue : string;
        commission : string;
        eid : string;
    }
    interface AtlasAtsExecutionReport {
        limit : string;
        reject? : AtlasAtsExecutionReportReject;
        ack? : AtlasAtsAck;
        executions? : AtlasAtsExecutions[];
        tif : string;
        status : string;
        type : string;
        currency : string;
        executed : number;
        clid: string;
        side : string;
        oid : string;
        item : string;
        account : string;
        quantity : number;
        left : number;
        average : number;
    }

    interface AtlasAtsQuote {
        id : string;
        mm : string;
        price : number;
        symbol : string;
        side : string;
        size : number;
        currency : string
    }

    interface AtlasAtsMarketUpdate {
        symbol : string;
        currency : string;
        bidsize : number;
        bid : number;
        asksize : number;
        ask : number;
    }

    interface AtlasAtsOrder {
        action: string;
        item: string;
        currency: string;
        side: string;
        quantity: number;
        type: string;
        price: number;
        clid: string;
    }

    interface AtlasAtsCancelOrder {
        action: string;
        oid: string;
    }

    class AtlasAtsSocket {
        _client : any;
        _secret : string;
        _token : string;
        _nounce : number = 1;
        _log : Logger = log("tribeca:gateway:AtlasAtsSocket");

        constructor(config : IConfigProvider) {
            this._secret = config.GetString("AtlasAtsSecret");
            this._token = config.GetString("AtlasAtsMultiToken");
            this._client = new Faye.Client(url.resolve(config.GetString("AtlasAtsHttpUrl"), '/api/v1/streaming'), {
                endpoints: {
                    websocket: config.GetString("AtlasAtsWsUrl")
                }
            });

            this._client.addExtension({
                outgoing: (msg, cb) => {
                    if (msg.channel != '/meta/handshake') {
                        msg.ext = this.signMessage(msg.channel, msg);
                    }
                    cb(msg);
                },
                incoming: (msg, cb) => {
                    if (msg.hasOwnProperty('successful') && !msg.successful) {
                        this._log("UNSUCCESSFUL %o", msg);
                    }
                    cb(msg);
                }
            });
        }

        private signMessage(channel : string, msg : any) {
            var inp : string = [this._token, this._nounce, channel, 'data' in msg ? JSON.stringify(msg['data']) : ''].join(":");
            var signature : string = crypto.createHmac('sha256', this._secret).update(inp).digest('hex').toString().toUpperCase();
            var sign = {ident: {key: this._token, signature: signature, nounce: this._nounce}};
            this._nounce += 1;
            return sign;
        }

        send = (msg : string) : void => {
            this._client.send(msg);
        };

        on = (channel : string, handler: () => void) => {
            this._client.on(channel, raw => handler());
        };

        subscribe<T>(channel : string, handler: (newMsg : Timestamped<T>) => void) {
            this._client.subscribe(channel, raw => {
                try {
                    var t = date();
                    handler(new Timestamped(JSON.parse(raw), t));
                }
                catch (e) {
                    this._log("Error parsing msg %o", raw);
                    throw e;
                }
            });
        }
    }

    class AtlasAtsBaseGateway implements IExchangeDetailsGateway {
        name() : string {
            return "AtlasAts";
        }

        makeFee() : number {
            return -0.001;
        }

        takeFee() : number {
            return 0.002;
        }

        exchange() : Exchange {
            return Exchange.AtlasAts;
        }
    }

    interface AtlasAtsPosition {
        size : number;
        item : string;
    }

    interface AtlasAtsPositionReport {
        buyingpower : number;
        positions : Array<AtlasAtsPosition>;
    }

    class AtlasAtsPositionGateway implements IPositionGateway {
        PositionUpdate : Evt<CurrencyPosition> = new Evt<CurrencyPosition>();

        private static _convertItem(item : string) : Currency {
            switch (item) {
                case "BTC":
                    return Currency.BTC;
                case "USD":
                    return Currency.USD;
                case "LTC":
                    return Currency.LTC;
                default:
                    return null;
            }
        }

        _log : Logger = log("tribeca:gateway:AtlasAtsPG");
        _sendUrl : string;
        _simpleToken : string;

        private onTimerTick = () => {
            request({
                url: url.resolve(this._sendUrl, "/api/v1/account"),
                headers: {"Authorization": "Token token=\""+this._simpleToken+"\""},
                method: "GET"
            }, (err, resp, body) => {
                try {
                    var rpt : AtlasAtsPositionReport = JSON.parse(body);

                    this.PositionUpdate.trigger(new CurrencyPosition(rpt.buyingpower, Currency.USD));
                    rpt.positions.forEach(p => {
                        var currency = AtlasAtsPositionGateway._convertItem(p.item);
                        if (currency == null) return;
                        var position = new CurrencyPosition(p.size, currency);
                        this.PositionUpdate.trigger(position);
                    });
                }
                catch (e) {
                    this._log("Exception when downloading positions: %o, body: %s", e, body);
                }
            });
        };

        constructor(config : IConfigProvider) {
            setInterval(this.onTimerTick, 15000);
            this._sendUrl = url.resolve(config.GetString("AtlasAtsHttpUrl"), "/api/v1/orders");
            this._simpleToken = config.GetString("AtlasAtsSimpleToken");
        }
    }

    class AtlasAtsOrderEntryGateway implements IOrderEntryGateway {
        ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();
        _log : Logger = log("tribeca:gateway:AtlasAtsOE");
        OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();
        _simpleToken : string;
        _account : string;

        private static _convertTif(tif : TimeInForce) {
            switch (tif) {
                case TimeInForce.FOK:
                    return "FOK";
                case TimeInForce.GTC:
                    return "GTC";
                case TimeInForce.IOC:
                    return "IOC";
            }
        }

        sendOrder = (order : BrokeredOrder) : OrderGatewayActionReport => {
            var o : AtlasAtsOrder = {
                action: "order:create",
                item: "BTC",
                currency: "USD",
                side: order.side == Side.Bid ? "BUY" : "SELL",
                quantity: order.quantity,
                type: order.type == OrderType.Limit ? "limit" : "market",
                price: order.price,
                clid: order.orderId,
                tif: AtlasAtsOrderEntryGateway._convertTif(order.timeInForce)
            };

            request({
                url: this._orderUrl,
                body: JSON.stringify(o),
                headers: {"Authorization": "Token token=\""+this._simpleToken+"\"", "Content-Type": "application/json"},
                method: "POST"
            }, (err, resp, body) => {
                try {
                    var t = date();
                    this.onExecRpt(new Timestamped(JSON.parse(body), t));
                }
                catch (e) {
                    this._log("url: %s, err: %o, body: %o", this._orderUrl, err, body);
                    throw e;
                }
            });

            return new OrderGatewayActionReport(date());
        };

        replaceOrder = (replace : BrokeredReplace) : OrderGatewayActionReport => {
            this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
            return this.sendOrder(replace);
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
            request({
                url: this._orderUrl + "/" + exchangeId,
                headers: {"Authorization": "Token token=\""+this._simpleToken+"\""},
                method: "DELETE"
            }, (err, resp, body) => {
                this._log("cxl-resp", err, body);
                var msg = JSON.parse(body);

                if (!err && msg.status !== "error") {
                    var rpt : OrderStatusReport = {
                        orderId: cancel.clientOrderId,
                        orderStatus: OrderStatus.Cancelled,
                        time: date()
                    };
                    this.OrderUpdate.trigger(rpt);
                } else {
                    var rpt : OrderStatusReport = {
                        orderId: cancel.clientOrderId,
                        orderStatus: OrderStatus.Rejected,
                        rejectMessage: msg.message,
                        cancelRejected: true,
                        time: date()
                    };
                    this.OrderUpdate.trigger(rpt);
                }
            });
        };

        private static getStatus = (raw : string) : OrderStatus => {
            switch (raw) {
                case "DONE":
                    return OrderStatus.Complete;
                case "REJECTED":
                    return OrderStatus.Rejected;
                case "PENDING":
                case "OPEN":
                    return OrderStatus.Working;
                default:
                    return OrderStatus.Other;
            }
        };

        private static getLiquidity = (raw : string) : Liquidity => {
            switch (raw) {
                case "A":
                    return Liquidity.Make;
                case "R":
                    return Liquidity.Take;
                default:
                    throw new Error("unknown liquidity " + raw);
            }
        };

        private onExecRpt = (tsMsg : Timestamped<AtlasAtsExecutionReport>) => {
            var t = tsMsg.time;
            var msg = tsMsg.data;

            var orderStatus = AtlasAtsOrderEntryGateway.getStatus(msg.status);

            // cancel any open orders waiting for oid
            if (this._cancelsWaitingForExchangeOrderId.hasOwnProperty(msg.clid)) {
                var cancel = this._cancelsWaitingForExchangeOrderId[msg.clid];
                this.sendCancel(msg.oid, cancel);
                this._log("Deleting %s late, oid: %s", cancel.clientOrderId, msg.clid);
                delete this._cancelsWaitingForExchangeOrderId[msg.clid];
            }

            var status : OrderStatusReport = {
                exchangeId: msg.oid,
                orderId: msg.clid,
                orderStatus: orderStatus,
                time: t, // doesnt give milliseconds??
                rejectMessage: msg.hasOwnProperty("reject") ? msg.reject.reason : null,
                leavesQuantity: msg.left,
                cumQuantity: msg.executed,
                averagePrice: msg.average,
                partiallyFilled: msg.left != 0
            };
            this.OrderUpdate.trigger(status);

            if (typeof msg.executions !== 'undefined') {
                msg.executions.forEach(exec => {
                    var status : OrderStatusReport = {
                        exchangeId: msg.oid,
                        orderId: msg.clid,
                        lastQuantity: exec.quantity,
                        liquidity: AtlasAtsOrderEntryGateway.getLiquidity(exec.liquidity)
                    };
                    this.OrderUpdate.trigger(status);
                });
            }
        };

        private _orderUrl : string;
        constructor(socket : AtlasAtsSocket, config : IConfigProvider) {
            this._orderUrl = url.resolve(config.GetString("AtlasAtsHttpUrl"), "/api/v1/orders");
            this._simpleToken = config.GetString("AtlasAtsSimpleToken");
            this._account = config.GetString("AtlasAtsAccount");

            socket.subscribe("/account/"+this._account+"/orders", this.onExecRpt);
            socket.on('transport:up', () => this.ConnectChanged.trigger(ConnectivityStatus.Connected));
            socket.on('transport:down', () => this.ConnectChanged.trigger(ConnectivityStatus.Disconnected));
        }
    }

    class AtlasAtsMarketDataGateway implements IMarketDataGateway {
        ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();
        MarketData = new Evt<MarketUpdate>();
        _log : Logger = log("tribeca:gateway:AtlasAtsMD");

        private onMarketData = (tsMsg : Timestamped<AtlasAtsMarketUpdate>) => {
            var t = tsMsg.time;
            var msg : AtlasAtsMarketUpdate = tsMsg.data;
            if (msg.symbol != "BTC" || msg.currency != "USD") return;

            var b = new MarketUpdate(
                new MarketSide(msg.bid, msg.bidsize),
                new MarketSide(msg.ask, msg.asksize),
                t
            );

            this.MarketData.trigger(b);
        };

        private _atlasAtsHttpUrl : string;
        constructor(socket : AtlasAtsSocket, config : IConfigProvider) {
            this._atlasAtsHttpUrl = url.resolve(config.GetString("AtlasAtsHttpUrl"), "/api/v1/market/book");
            socket.subscribe("/market", this.onMarketData);

            socket.on('transport:up', () => this.ConnectChanged.trigger(ConnectivityStatus.Connected));
            socket.on('transport:down', () => this.ConnectChanged.trigger(ConnectivityStatus.Disconnected));

            request.get({
                url: this._atlasAtsHttpUrl,
                qs: {item: "BTC", currency: "USD"}
            }, (er, resp, body) => {
                var t = date();
                this.onMarketData(new Timestamped(JSON.parse(body), t))
            });
        }
    }

    export class AtlasAts extends CombinedGateway {
        constructor(config : IConfigProvider) {
            var socket = new AtlasAtsSocket(config);
            super(
                new AtlasAtsMarketDataGateway(socket, config),
                config.GetString("AtlasAtsOrderDestination") == "AtlasAts" ? <IOrderEntryGateway>new AtlasAtsOrderEntryGateway(socket, config) : new NullOrderGateway(),
                new AtlasAtsPositionGateway(config),
                new AtlasAtsBaseGateway());
        }
    }
}