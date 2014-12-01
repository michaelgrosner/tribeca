/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="nullgw.ts" />

var Faye = require('faye');
import request = require("request");
import crypto = require('crypto');
import url = require("url");
import Config = require("./config");
import Models = require("../common/models");
import NullGateway = require("./nullgw");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

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
    _log : Utils.Logger = Utils.log("tribeca:gateway:AtlasAtsSocket");

    constructor(config : Config.IConfigProvider) {
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

    subscribe<T>(channel : string, handler: (newMsg : Models.Timestamped<T>) => void) {
        this._client.subscribe(channel, raw => {
            try {
                var t = Utils.date();
                handler(new Models.Timestamped(JSON.parse(raw), t));
            }
            catch (e) {
                this._log("Error parsing msg %o", raw);
                throw e;
            }
        });
    }
}

class AtlasAtsBaseGateway implements Interfaces.IExchangeDetailsGateway {
    name() : string {
        return "AtlasAts";
    }

    makeFee() : number {
        return -0.001;
    }

    takeFee() : number {
        return 0.002;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.AtlasAts;
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

class AtlasAtsPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private static _convertItem(item : string) : Models.Currency {
        switch (item) {
            case "BTC":
                return Models.Currency.BTC;
            case "USD":
                return Models.Currency.USD;
            case "LTC":
                return Models.Currency.LTC;
            default:
                return null;
        }
    }

    _log : Utils.Logger = Utils.log("tribeca:gateway:AtlasAtsPG");
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

                this.PositionUpdate.trigger(new Models.CurrencyPosition(rpt.buyingpower, Models.Currency.USD));
                rpt.positions.forEach(p => {
                    var currency = AtlasAtsPositionGateway._convertItem(p.item);
                    if (currency == null) return;
                    var position = new Models.CurrencyPosition(p.size, currency);
                    this.PositionUpdate.trigger(position);
                });
            }
            catch (e) {
                this._log("Exception when downloading positions: %o, body: %s", e, body);
            }
        });
    };

    constructor(config : Config.IConfigProvider) {
        setInterval(this.onTimerTick, 15000);
        this._sendUrl = url.resolve(config.GetString("AtlasAtsHttpUrl"), "/api/v1/orders");
        this._simpleToken = config.GetString("AtlasAtsSimpleToken");
    }
}

class AtlasAtsOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    _log : Utils.Logger = Utils.log("tribeca:gateway:AtlasAtsOE");
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    _simpleToken : string;
    _account : string;

    private static _convertTif(tif : Models.TimeInForce) {
        switch (tif) {
            case Models.TimeInForce.FOK:
                return "FOK";
            case Models.TimeInForce.GTC:
                return "GTC";
            case Models.TimeInForce.IOC:
                return "IOC";
        }
    }

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var o : AtlasAtsOrder = {
            action: "order:create",
            item: "BTC",
            currency: "USD",
            side: order.side == Models.Side.Bid ? "BUY" : "SELL",
            quantity: order.quantity,
            type: order.type == Models.OrderType.Limit ? "limit" : "market",
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
                var t = Utils.date();
                this.onExecRpt(new Models.Timestamped(JSON.parse(body), t));
            }
            catch (e) {
                this._log("url: %s, err: %o, body: %o", this._orderUrl, err, body);
                throw e;
            }
        });

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    private _cancelsWaitingForExchangeOrderId : {[clId : string] : Models.BrokeredCancel} = {};
    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        // race condition! i cannot cancel an order before I get the exchangeId (oid); register it for deletion on the ack
        if (typeof cancel.exchangeId !== "undefined") {
            this.sendCancel(cancel.exchangeId, cancel);
        }
        else {
            this._cancelsWaitingForExchangeOrderId[cancel.clientOrderId] = cancel;
            this._log("Registered %s for late deletion", cancel.clientOrderId);
        }

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    private sendCancel = (exchangeId : string, cancel : Models.BrokeredCancel) => {
        request({
            url: this._orderUrl + "/" + exchangeId,
            headers: {"Authorization": "Token token=\""+this._simpleToken+"\""},
            method: "DELETE"
        }, (err, resp, body) => {
            this._log("cxl-resp", err, body);
            var msg = JSON.parse(body);

            if (!err && msg.status !== "error") {
                var rpt : Models.OrderStatusReport = {
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Cancelled,
                    time: Utils.date()
                };
                this.OrderUpdate.trigger(rpt);
            } else {
                var rpt : Models.OrderStatusReport = {
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Rejected,
                    rejectMessage: msg.message,
                    cancelRejected: true,
                    time: Utils.date()
                };
                this.OrderUpdate.trigger(rpt);
            }
        });
    };

    private static getStatus = (raw : string) : Models.OrderStatus => {
        switch (raw) {
            case "DONE":
                return Models.OrderStatus.Complete;
            case "REJECTED":
                return Models.OrderStatus.Rejected;
            case "PENDING":
            case "OPEN":
                return Models.OrderStatus.Working;
            default:
                return Models.OrderStatus.Other;
        }
    };

    private static getLiquidity = (raw : string) : Models.Liquidity => {
        switch (raw) {
            case "A":
                return Models.Liquidity.Make;
            case "R":
                return Models.Liquidity.Take;
            default:
                throw new Error("unknown liquidity " + raw);
        }
    };

    private onExecRpt = (tsMsg : Models.Timestamped<AtlasAtsExecutionReport>) => {
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

        var status : Models.OrderStatusReport = {
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
                var status : Models.OrderStatusReport = {
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
    constructor(socket : AtlasAtsSocket, config : Config.IConfigProvider) {
        this._orderUrl = url.resolve(config.GetString("AtlasAtsHttpUrl"), "/api/v1/orders");
        this._simpleToken = config.GetString("AtlasAtsSimpleToken");
        this._account = config.GetString("AtlasAtsAccount");

        socket.subscribe("/account/"+this._account+"/orders", this.onExecRpt);
        socket.on('transport:up', () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        socket.on('transport:down', () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
    }
}

class AtlasAtsMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    MarketData = new Utils.Evt<Models.MarketUpdate>();
    _log : Utils.Logger = Utils.log("tribeca:gateway:AtlasAtsMD");

    private onMarketData = (tsMsg : Models.Timestamped<AtlasAtsMarketUpdate>) => {
        var t = tsMsg.time;
        var msg : AtlasAtsMarketUpdate = tsMsg.data;
        if (msg.symbol != "BTC" || msg.currency != "USD") return;

        var b = new Models.MarketUpdate(
            new Models.MarketSide(msg.bid, msg.bidsize),
            new Models.MarketSide(msg.ask, msg.asksize),
            t
        );

        this.MarketData.trigger(b);
    };

    private _atlasAtsHttpUrl : string;
    constructor(socket : AtlasAtsSocket, config : Config.IConfigProvider) {
        this._atlasAtsHttpUrl = url.resolve(config.GetString("AtlasAtsHttpUrl"), "/api/v1/market/book");
        socket.subscribe("/market", this.onMarketData);

        socket.on('transport:up', () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        socket.on('transport:down', () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));

        request.get({
            url: this._atlasAtsHttpUrl,
            qs: {item: "BTC", currency: "USD"}
        }, (er, resp, body) => {
            var t = Utils.date();
            this.onMarketData(new Models.Timestamped(JSON.parse(body), t))
        });
    }
}

export class AtlasAts extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var socket = new AtlasAtsSocket(config);

        var orderEntry = config.GetString("AtlasAtsOrderDestination") == "AtlasAts"
            ? <Interfaces.IOrderEntryGateway>new AtlasAtsOrderEntryGateway(socket, config)
            : new NullGateway.NullOrderGateway();

        super(
            new AtlasAtsMarketDataGateway(socket, config),
            orderEntry,
            new AtlasAtsPositionGateway(config),
            new AtlasAtsBaseGateway());
    }
}