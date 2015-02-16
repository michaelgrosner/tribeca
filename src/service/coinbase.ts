/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("./config");
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import io = require("socket.io-client");
import moment = require("moment");
import WebSocket = require('ws');
import events = module("events");
var CoinbaseExchange = require("coinbase-exchange");

var key = "8ac44274479da86e3bca6bc5ab841058";
var secret = "5H+Lnu/Op9nTJy1qUkvlpTnQvbA5ChGmyrXTQk/W5luS31eACAXAEEB4wnl1i/QxKTW0IuaJzqPMCXusIm7CMw==";

interface StateChange {
    old : string;
    new : string;
}

// A valid order has been received and is now active. This message is emitted for every single valid order as soon as
// the matching engine receives it whether it fills immediately or not.
// The received message does not indicate a resting order on the order book. It simply indicates a new incoming order
// which as been accepted by the matching engine for processing. Received orders may cause match message to follow if
// they are able to begin being filled (taker behavior). Self-trade prevention may also trigger change messages to
// follow if the order size needs to be adjusted. Orders which are not fully filled or canceled due to self-trade
// prevention result in an open message and become resting orders on the order book.
interface CoinbaseReceived {
    type : string; // received
    sequence : number;
    order_id: string; // guid
    size: string; // "0.00"
    price: string; // "0.00"
    side: string; // "buy"/"sell"
}

// The order is now open on the order book. This message will only be sent for orders which are not fully filled
// immediately. remaining_size will indicate how much of the order is unfilled and going on the book.
interface CoinbaseOpen {
    type: string; // "open",
    sequence: number;
    order_id: string; // guid
    price: number;
    remaining_size: number;
    side: string;
}

// The order is no longer on the order book. Sent for all orders for which there was a received message.
// This message can result from an order being canceled or filled. There will be no more messages for this order_id a
// fter a done message. remaining_size indicates how much of the order went unfilled; this will be 0 for filled orders.
interface CoinbaseDone {
    type: string; // done
    sequence: number;
    price: string;
    order_id: string;
    reason: string; // "filled"??
    side: string;
    remaining_size: string;
}

// A trade occurred between two orders. The aggressor or taker order is the one executing immediately after
// being received and the maker order is a resting order on the book. The side field indicates the maker order side.
// If the side is sell this indicates the maker was a sell order and the match is considered an up-tick. A buy side
// match is a down-tick.
interface CoinbaseMatch {
    type: string; // "match"
    trade_id: number; //": 10,
    sequence: number;
    maker_order_id: string;
    taker_order_id: string;
    time: string; // "2014-11-07T08:19:27.028459Z",
    size: string;
    price: string;
    side: string;
}

// An order has changed in size. This is the result of self-trade prevention adjusting the order size.
// Orders can only decrease in size. change messages are sent anytime an order changes in size;
// this includes resting orders (open) as well as received but not yet open.
interface CoinbaseChange {
    type: string; //"change",
    sequence: number;
    order_id: string;
    time: string;
    new_size: string;
    old_size: string;
    price: string;
    side: string;
}

interface CoinbaseOrderBook extends events.EventEmitter {
    on(event: 'statechange', cb: (data : StateChange) => void) : CoinbaseOrderBook;
    on(event: 'received', cd: (msg : CoinbaseReceived) => void) : CoinbaseOrderBook;
    on(event: 'open', cd: (msg : CoinbaseOpen) => void) : CoinbaseOrderBook;
    on(event: 'done', cd: (msg : CoinbaseDone) => void) : CoinbaseOrderBook;
    on(event: 'match', cd: (msg : CoinbaseMatch) => void) : CoinbaseOrderBook;
    on(event: 'change', cd: (msg : CoinbaseChange) => void) : CoinbaseOrderBook;
    on(event: 'error', cd: (msg : Error) => void) : CoinbaseOrderBook;
}

class CoinbaseSocket {
    private _ws : WebSocket;
    private _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseSocket");

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    private onConnectionStatusChange = () => {
        if (this._ws.readyState === WebSocket.OPEN) {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        }
        else {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        }
    };

    constructor() {
        this._ws = new WebSocket("wss://ws-feed.exchange.coinbase.com")
        this._ws.on("open", this.onConnectionStatusChange);
        this._ws.on("close", this.onConnectionStatusChange);
        this._ws.on("error", err => {
            this.onConnectionStatusChange();
            this._log("err: %s", err);
        });
        this._ws.on("message", (msg) => this.NewEvent.trigger(JSON.parse(msg)));
    }

    NewEvent = new Utils.Evt<CoinbaseMarketEvent>();
    subscribe = (product : string, handler : (msg : CoinbaseMarketEvent) => void) => {
        this._ws.send(JSON.stringify({
            "type": "subscribe",
            "product_id": product
        }));
    };
}

class CoinbaseMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.MarketSide>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseMD");
    _client : any;
    constructor(config : Config.IConfigProvider) {
        this._client = new CoinbaseExchange.OrderBook();
    }
}

class CoinbaseOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this.sendAuth("OrderCancel", {clientOrderId: cancel.clientOrderId,
            cancelRequestClientOrderId: cancel.requestId,
            symbol: "BTCUSD",
            side: HitBtcOrderEntryGateway.getSide(cancel.side)});
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var hitBtcOrder : NewOrder = {
            clientOrderId: order.orderId,
            symbol: "BTCUSD",
            side: HitBtcOrderEntryGateway.getSide(order.side),
            quantity: order.quantity * _lotMultiplier,
            type: HitBtcOrderEntryGateway.getType(order.type),
            price: Utils.roundFloat(order.price),
            timeInForce: HitBtcOrderEntryGateway.getTif(order.timeInForce)
        };

        this.sendAuth("NewOrder", hitBtcOrder);
        return new Models.OrderGatewayActionReport(Utils.date());
    };



    private onExecutionReport = (tsMsg : Models.Timestamped<ExecutionReport>) => {
        var t = tsMsg.time;
        var msg = tsMsg.data;

        var ordStatus = HitBtcOrderEntryGateway.getStatus(msg);
        var status : Models.OrderStatusReport = {
            exchangeId: msg.orderId,
            orderId: msg.clientOrderId,
            orderStatus: ordStatus,
            time: t,
            rejectMessage: msg.orderRejectReason,
            lastQuantity: msg.lastQuantity > 0 ? msg.lastQuantity / _lotMultiplier : undefined,
            lastPrice: msg.lastQuantity > 0 ? msg.lastPrice : undefined,
            leavesQuantity: ordStatus == Models.OrderStatus.Working ? msg.leavesQuantity / _lotMultiplier : undefined,
            cumQuantity: msg.cumQuantity / _lotMultiplier,
            averagePrice: msg.averagePrice
        };

        this.OrderUpdate.trigger(status);
    };

    private onCancelReject = (tsMsg : Models.Timestamped<CancelReject>) => {
        var msg = tsMsg.data;
        var status : Models.OrderStatusReport = {
            orderId: msg.clientOrderId,
            rejectMessage: msg.rejectReasonText,
            orderStatus: Models.OrderStatus.Rejected,
            cancelRejected: true,
            time: tsMsg.time
        };
        this.OrderUpdate.trigger(status);
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

     _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseOE");
    constructor(config : Config.IConfigProvider) {
    }
}


class CoinbasePositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbasePG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    constructor(config : Config.IConfigProvider) {
        setInterval(this.onTick, 15000);
    }
}

class CoinbaseBaseGateway implements Interfaces.IExchangeDetailsGateway {
    exchange() : Models.Exchange {
        return Models.Exchange.Coinbase;
    }

    makeFee() : number {
        return 0;
    }

    takeFee() : number {
        return 0;
    }

    name() : string {
        return "Coinbase";
    }
}

export class Coinbase extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var orderGateway = config.GetString("CoinbaseOrderDestination") == "Coinbase" ?
            <Interfaces.IOrderEntryGateway>new CoinbaseOrderEntryGateway(config)
            : new NullGateway.NullOrderGateway();

        var positionGateway = config.environment() == Config.Environment.Dev ?
            new NullGateway.NullPositionGateway() :
            new CoinbasePositionGateway(config);

        super(
            new CoinbaseMarketDataGateway(config),
            orderGateway,
            positionGateway,
            new CoinbaseBaseGateway());
    }
}