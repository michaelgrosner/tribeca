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
import _ = require('lodash');
var CoinbaseExchange = require("coinbase-exchange");
var SortedArrayMap = require("collections/sorted-array-map");

var passphrase = "h9g47GcHDC8GHx48Hner";
var key = "6c945837d033823150e9e543642b2aeb";
var secret = "H/6BPPfJtkZRjiIDSEaRpluFqkN0B+o1FThT5iufWEtSr+xahT2NN4V+m1GYvKCn/DYCYzt8fbcrf4E5DQ1JNw==";

interface StateChange {
    old : string;
    new : string;
}

interface CoinbaseBase {
    type: string;
    side: string; // "buy"/"sell"
    sequence : number;
}

// A valid order has been received and is now active. This message is emitted for every single valid order as soon as
// the matching engine receives it whether it fills immediately or not.
// The received message does not indicate a resting order on the order book. It simply indicates a new incoming order
// which as been accepted by the matching engine for processing. Received orders may cause match message to follow if
// they are able to begin being filled (taker behavior). Self-trade prevention may also trigger change messages to
// follow if the order size needs to be adjusted. Orders which are not fully filled or canceled due to self-trade
// prevention result in an open message and become resting orders on the order book.
interface CoinbaseReceived extends CoinbaseBase {
    order_id: string; // guid
    size: string; // "0.00"
    price: string; // "0.00"
}

// The order is now open on the order book. This message will only be sent for orders which are not fully filled
// immediately. remaining_size will indicate how much of the order is unfilled and going on the book.
interface CoinbaseOpen extends CoinbaseBase {
    order_id: string; // guid
    price: string;
    remaining_size: string;
}

// The order is no longer on the order book. Sent for all orders for which there was a received message.
// This message can result from an order being canceled or filled. There will be no more messages for this order_id
// after a done message. remaining_size indicates how much of the order went unfilled; this will be 0 for filled orders.
interface CoinbaseDone extends CoinbaseBase {
    price: string;
    order_id: string;
    reason: string; // "filled"??
    remaining_size: string;
}

// A trade occurred between two orders. The aggressor or taker order is the one executing immediately after
// being received and the maker order is a resting order on the book. The side field indicates the maker order side.
// If the side is sell this indicates the maker was a sell order and the match is considered an up-tick. A buy side
// match is a down-tick.
interface CoinbaseMatch extends CoinbaseBase {
    trade_id: number; //": 10,
    maker_order_id: string;
    taker_order_id: string;
    time: string; // "2014-11-07T08:19:27.028459Z",
    size: string;
    price: string;
}

// An order has changed in size. This is the result of self-trade prevention adjusting the order size.
// Orders can only decrease in size. change messages are sent anytime an order changes in size;
// this includes resting orders (open) as well as received but not yet open.
interface CoinbaseChange extends CoinbaseBase {
    order_id: string;
    time: string;
    new_size: string;
    old_size: string;
    price: string;
}

interface CoinbaseEntry {
    size : string;
    price : string;
    id : string;
}

interface CoinbaseBookStorage {
    sequence : string;
    bids : { [id : string] : CoinbaseEntry };
    asks : { [id : string] : CoinbaseEntry };
}

interface CoinbaseOrderBook {
    on(event: string, cb: Function) : CoinbaseOrderBook;
    on(event: 'statechange', cb: (data : StateChange) => void) : CoinbaseOrderBook;
    on(event: 'received', cd: (msg : CoinbaseReceived) => void) : CoinbaseOrderBook;
    on(event: 'open', cd: (msg : CoinbaseOpen) => void) : CoinbaseOrderBook;
    on(event: 'done', cd: (msg : CoinbaseDone) => void) : CoinbaseOrderBook;
    on(event: 'match', cd: (msg : CoinbaseMatch) => void) : CoinbaseOrderBook;
    on(event: 'change', cd: (msg : CoinbaseChange) => void) : CoinbaseOrderBook;
    on(event: 'error', cd: (msg : Error) => void) : CoinbaseOrderBook;

    state : string;
    book : CoinbaseBookStorage;
}

interface CoinbaseOrder {
    client_oid? : string;
    price : string;
    size : string;
    product_id : string;
    stp? : string;
}

interface CoinbaseOrderAck {
    id : string;
}

interface CoinbaseAccountInformation {
    id : string;
    balance : string;
    holds : string;
    available : string;
    currency : string;
}

interface CoinbaseRESTTrade {
    time: string;
    trade_id: number;
    price: string;
    size: string;
    side: string;
}

interface CoinbaseAuthenticatedClient {
    buy(order : CoinbaseOrder, cb: (err? : Error, response? : any, ack? : CoinbaseOrderAck) => void);
    sell(order : CoinbaseOrder, cb: (err? : Error, response? : any, ack? : CoinbaseOrderAck) => void);
    cancelOrder(id : string, cb: (err? : Error, response? : any) => void);
    getOrders(cb: (err? : Error, response? : any, ack? : CoinbaseOpen[]) => void);
    getProductTrades(product : string, cb: (err? : Error, response? : any, ack? : CoinbaseRESTTrade[]) => void);
    getAccounts(cb: (err? : Error, response? : any, info? : CoinbaseAccountInformation[]) => void);
    getAccount(accountID : string, cb: (err? : Error, response? : any, info? : CoinbaseAccountInformation) => void);
}

function convertConnectivityStatus(s : StateChange) {
    return s.new === "processing" ? Models.ConnectivityStatus.Connected : Models.ConnectivityStatus.Disconnected;
}

function convertSide(msg : CoinbaseBase) : Models.Side {
    return msg.side === "buy" ? Models.Side.Bid : Models.Side.Ask;
}

function convertPrice(pxStr : string) {
    return parseFloat(pxStr);
}

function convertSize(szStr : string) {
    return parseFloat(szStr);
}

function convertTime(time : string) {
    return moment(time);
}

class PriceLevel {
    orders : { [id: string]: number} = {};
    marketUpdate = new Models.MarketSide(0, 0);
}

class CoinbaseMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.MarketSide>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private static Eq = (a, b) => a.price === b.price;

    private static BidCmp = (a, b) => {
        if (CoinbaseMarketDataGateway.Eq(a, b)) return 0;
        return a.price > b.price ? 1 : -1
    };

    private static AskCmp = (a, b) => {
        if (CoinbaseMarketDataGateway.Eq(a, b)) return 0;
        return a.price > b.price ? -1 : 1
    };

    private _bids = new SortedArrayMap([], CoinbaseMarketDataGateway.Eq, CoinbaseMarketDataGateway.BidCmp);
    private _asks = new SortedArrayMap([], CoinbaseMarketDataGateway.Eq, CoinbaseMarketDataGateway.AskCmp);

    private getStorage = (msg : CoinbaseBase) : any => {
        if (msg.side === "buy") return this._bids;
        if (msg.side === "sell") return this._asks;
    };

    private onOpen = (msg : CoinbaseOpen, t : Moment) => {
        var storage = this.getStorage(msg);

        var priceLevelStorage = storage.get(msg.price);
        if (typeof priceLevelStorage === "undefined") {
            var pl = new PriceLevel();
            pl.marketUpdate.price = convertPrice(msg.price);
            priceLevelStorage = pl;
            storage.set(msg.price, pl);
        }

        var size = convertSize(msg.remaining_size);
        priceLevelStorage.marketUpdate.size += size;
        priceLevelStorage.orders[msg.order_id] = size;

        this.onOrderBookChanged(t);
    };

    private onDone = (msg : CoinbaseDone, t : Moment) => {
        var storage = this.getStorage(msg);

        if (msg.reason === "canceled") {
            var priceLevelStorage = storage.get(msg.price);

            var orderSize = priceLevelStorage.orders[msg.order_id];
            if (typeof priceLevelStorage === "undefined" || typeof orderSize === "undefined") {
                this._log("cannot find order book order %j to cancel!", msg);
                return;
            }

            priceLevelStorage.marketUpdate.size -= orderSize;
            delete priceLevelStorage.orders[msg.order_id];

            if (_.isEmpty(priceLevelStorage.orders)) {
                storage.delete(msg.price);
            }

            this.onOrderBookChanged(t);
        }
    };

    private onMatch = (msg : CoinbaseMatch, t : Moment) => {
        var storage = this.getStorage(msg);

        this.MarketTrade.trigger(new Models.GatewayMarketTrade(convertPrice(msg.price), convertSize(msg.size),
            convertTime(msg.time), false));
    };

    private onChange = (msg : CoinbaseChange, t : Moment) => {
        var storage = this.getStorage(msg);
        this.onOrderBookChanged(t);
    };

    private onOrderBookChanged = (t : Moment) => {
        this.MarketData.trigger(new Models.Market(null, null, t));
    };

    private onStateChange = (s : StateChange) => {
        this.ConnectChanged.trigger(convertConnectivityStatus(s));
    };

    _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseMD");
    constructor(private _client : CoinbaseOrderBook) {
        this._client.on("statechange", this.onStateChange);
        this._client.on("open", m => this.onOpen(m, Utils.date()));
        this._client.on("done", m => this.onDone(m, Utils.date()));
        this._client.on("match", m => this.onMatch(m, Utils.date()));
        this._client.on("change", m => this.onChange(m, Utils.date()));
    }
}

class CoinbaseOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this._authClient.cancelOrder(cancel.exchangeId, (err? : Error) => {
            if (err) {
                var status : Models.OrderStatusReport = {
                    orderId: cancel.clientOrderId,
                    rejectMessage: err.message,
                    orderStatus: Models.OrderStatus.Rejected,
                    cancelRejected: true,
                    time: Utils.date()
                };
                this.OrderUpdate.trigger(status);
            }
        });
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var func;
        if (order.side === Models.Side.Bid) func = this._authClient.buy;
        else if (order.side === Models.Side.Ask) func = this._authClient.sell;

        func(order, (err? : Error, resp? : any, ack? : CoinbaseOrderAck) => {
            if (err) {
                var status : Models.OrderStatusReport = {
                    orderId: order.orderId,
                    rejectMessage: err.message,
                    orderStatus: Models.OrderStatus.Rejected,
                    time: Utils.date()
                };
                this.OrderUpdate.trigger(status);
            }
            else if (ack) {
                // pick this up off real-time feed
            }
            else {
                this._log("something went wrong with sending order %j", order);
            }
        });

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    /*private onExecutionReport = (tsMsg : Models.Timestamped<ExecutionReport>) => {
        this.OrderUpdate.trigger(status);
    };*/

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

     _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseOE");
    constructor(
        private _orderBook : CoinbaseOrderBook,
        private _authClient : CoinbaseAuthenticatedClient) {}
}


class CoinbasePositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbasePG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private onTick = () => {
        this._authClient.getAccounts((err? : Error, resp? : any, data? : CoinbaseAccountInformation[]) => {
            this._log(data);
        });
    };

    constructor(private _authClient : CoinbaseAuthenticatedClient) {
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
        var orderbook = new CoinbaseExchange.OrderBook();
        var authClient = new CoinbaseExchange.AuthenticatedClient(key, secret, passphrase);

        var orderGateway = config.GetString("CoinbaseOrderDestination") == "Coinbase" ?
            <Interfaces.IOrderEntryGateway>new CoinbaseOrderEntryGateway(orderbook, authClient)
            : new NullGateway.NullOrderGateway();

        var positionGateway = config.environment() == Config.Environment.Dev ?
            new NullGateway.NullPositionGateway() :
            new CoinbasePositionGateway(authClient);

        super(
            new CoinbaseMarketDataGateway(orderbook),
            orderGateway,
            positionGateway,
            new CoinbaseBaseGateway());
    }
}