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

var uuid = require('node-uuid');
import CoinbaseExchange = require("./coinbase-api");
var SortedArrayMap = require("collections/sorted-array-map");

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
    client_oid? : string;
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
    on(event: 'received', cd: (msg : Models.Timestamped<CoinbaseReceived>) => void) : CoinbaseOrderBook;
    on(event: 'open', cd: (msg : Models.Timestamped<CoinbaseOpen>) => void) : CoinbaseOrderBook;
    on(event: 'done', cd: (msg : Models.Timestamped<CoinbaseDone>) => void) : CoinbaseOrderBook;
    on(event: 'match', cd: (msg : Models.Timestamped<CoinbaseMatch>) => void) : CoinbaseOrderBook;
    on(event: 'change', cd: (msg : Models.Timestamped<CoinbaseChange>) => void) : CoinbaseOrderBook;
    on(event: 'error', cd: (msg : Models.Timestamped<Error>) => void) : CoinbaseOrderBook;

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
    error? : string;
    message? : string;
}

interface CoinbaseAccountInformation {
    id : string;
    balance : string;
    hold : string;
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

    private static Eq = (a, b) => Math.abs(a - b) < 1e-4;

    private static BidCmp = (a, b) => {
        if (CoinbaseMarketDataGateway.Eq(a, b)) return 0;
        return a > b ? -1 : 1
    };

    private static AskCmp = (a, b) => {
        if (CoinbaseMarketDataGateway.Eq(a, b)) return 0;
        return a > b ? 1 : -1
    };

    private _bids : any = new SortedArrayMap([], CoinbaseMarketDataGateway.Eq, CoinbaseMarketDataGateway.BidCmp);
    private _asks : any = new SortedArrayMap([], CoinbaseMarketDataGateway.Eq, CoinbaseMarketDataGateway.AskCmp);

    private getStorage = (side : Models.Side) : any => {
        if (side === Models.Side.Bid) return this._bids;
        if (side === Models.Side.Ask) return this._asks;
    };

    private addToOrderBook = (storage : any, price : number, size : number, order_id : string) => {
        var priceLevelStorage : PriceLevel = storage.get(price);

        if (typeof priceLevelStorage === "undefined") {
            var pl = new PriceLevel();
            pl.marketUpdate.price = price;
            priceLevelStorage = pl;
            storage.set(price, pl);
        }

        priceLevelStorage.marketUpdate.size += size;
        priceLevelStorage.orders[order_id] = size;
    };

    private onReceived = (msg : CoinbaseReceived, t : Moment) => {
        var price = convertPrice(msg.price);
        var size = convertSize(msg.size);
        var side = convertSide(msg);

        var otherSide = side === Models.Side.Bid ? Models.Side.Ask : Models.Side.Bid;
        var storage = this.getStorage(otherSide);

        var changed = false;
        var remaining_size = size;
        for (var i = 0; i < storage.store.length; i++) {
            if (remaining_size <= 0) break;

            var kvp = storage.store.array[i];
            var price_level = kvp.key;

            if (side === Models.Side.Bid && price < price_level) break;
            if (side === Models.Side.Ask && price > price_level) break;

            var level_size = kvp.value.marketUpdate.size;

            if (level_size <= remaining_size) {
                storage.delete(price_level);
                remaining_size -= level_size;
                changed = true;
            }
        }

        if (changed) {
            if (otherSide === Models.Side.Bid) this.reevalBids();
            if (otherSide === Models.Side.Ask) this.reevalAsks();
            this.raiseMarketData(t);
        }
    }

    private onOpen = (msg : CoinbaseOpen, t : Moment) => {
        var price = convertPrice(msg.price);
        var side = convertSide(msg);
        var storage = this.getStorage(side);
        this.addToOrderBook(storage, price, convertSize(msg.remaining_size), msg.order_id);
        this.onOrderBookChanged(t, side, price);
    };

    private onDone = (msg : CoinbaseDone, t : Moment) => {
        var price = convertPrice(msg.price);
        var side = convertSide(msg);
        var storage = this.getStorage(side);

        var priceLevelStorage = storage.get(price);

        if (typeof priceLevelStorage === "undefined")
            return;

        var orderSize = priceLevelStorage.orders[msg.order_id];
        if (typeof orderSize === "undefined")
            return;

        priceLevelStorage.marketUpdate.size -= orderSize;
        delete priceLevelStorage.orders[msg.order_id];

        if (_.isEmpty(priceLevelStorage.orders)) {
            storage.delete(price);
        }

        this.onOrderBookChanged(t, side, price);
    };

    private onMatch = (msg : CoinbaseMatch, t : Moment) => {
        var price = convertPrice(msg.price);
        var size = convertSize(msg.size);
        var side = convertSide(msg);

        var makerStorage = this.getStorage(side);
        var priceLevelStorage = makerStorage.get(price);

        if (typeof priceLevelStorage !== "undefined") {
            priceLevelStorage.marketUpdate.size -= size;
            priceLevelStorage.orders[msg.maker_order_id] -= size;

            if (priceLevelStorage.orders[msg.maker_order_id] < 1e-4)
                delete priceLevelStorage.orders[msg.maker_order_id];

            if (_.isEmpty(priceLevelStorage.orders)) {
                makerStorage.delete(price);
            }

            this.onOrderBookChanged(t, side, price);
        }

        this.MarketTrade.trigger(new Models.GatewayMarketTrade(price, size, convertTime(msg.time), false, side));
    };

    private onChange = (msg : CoinbaseChange, t : Moment) => {
        this._log("onChange", msg);

        var price = convertPrice(msg.price);
        var side = convertSide(msg);
        var storage = this.getStorage(side);

        var priceLevelStorage : PriceLevel = storage.get(convertPrice(msg.price));

        if (typeof priceLevelStorage === "undefined")
            return;

        var oldSize = priceLevelStorage.orders[msg.order_id];
        if (typeof oldSize === "undefined")
            return;

        var newSize = convertSize(msg.new_size);

        priceLevelStorage.orders[msg.order_id] = newSize;
        priceLevelStorage.marketUpdate.size -= (oldSize - newSize);

        this.onOrderBookChanged(t, side, price);
    };

    private _cachedBids : Models.MarketSide[] = null;
    private _cachedAsks : Models.MarketSide[] = null;

    private reevalBids = () => {
        this._cachedBids = _.map(this._bids.store.slice(0, 5), s => (<any>s).value.marketUpdate);
    };

    private reevalAsks = () => {
        this._cachedAsks = _.map(this._asks.store.slice(0, 5), s => (<any>s).value.marketUpdate);
    };

    private onOrderBookChanged = (t : Moment, side : Models.Side, price : number) => {
        if (side === Models.Side.Bid) {
            if (price < _.last(this._cachedBids).price) return;
            else this.reevalBids();
        }

        if (side === Models.Side.Ask) {
            if (price > _.last(this._cachedAsks).price) return;
            else this.reevalAsks();
        }

        this.raiseMarketData(t);
    };

    private onStateChange = (s : StateChange) => {
        var t = Utils.date();
        var status = convertConnectivityStatus(s);

        if (status === Models.ConnectivityStatus.Connected) {
            var add = (st, u) =>
                this.addToOrderBook(st, convertPrice(u.price), convertSize(u.size), u.id);

            _.forEach(this._client.book.asks, a => add(this._asks, a));
            _.forEach(this._client.book.bids, b => add(this._bids, b));
        }
        else {
            this._asks.clear();
            this._bids.clear();
        }

        this.ConnectChanged.trigger(status);

        this.reevalBids();
        this.reevalAsks();
        this.raiseMarketData(t);
    };

    private raiseMarketData = (t : Moment) => {
        if (typeof this._cachedBids[0] !== "undefined" && typeof this._cachedAsks[0] !== "undefined") {
            if (this._cachedBids[0].price > this._cachedAsks[0].price) {
                this._log("WARNING: crossed Coinbase market detected! bid:", this._cachedBids[0].price, "ask:", this._cachedAsks[0].price);
                (<any>this._client).changeState('error');
                return;
            }

            this.MarketData.trigger(new Models.Market(this._cachedBids, this._cachedAsks, t));
        }
    };

    _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseMD");
    constructor(private _client : CoinbaseOrderBook) {
        this._client.on("statechange", m => this.onStateChange(m));
        this._client.on("received", m => this.onReceived(m.data, m.time));
        this._client.on("open", m => this.onOpen(m.data, m.time));
        this._client.on("done", m => this.onDone(m.data, m.time));
        this._client.on("match", m => this.onMatch(m.data, m.time));
        this._client.on("change", m => this.onChange(m.data, m.time));
    }
}

class CoinbaseOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    generateClientOrderId = () : string => {
        return uuid.v1();
    }

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this._authClient.cancelOrder(cancel.exchangeId, (err? : Error, resp? : any, ack? : CoinbaseOrderAck) => {
            var status : Models.OrderStatusReport
            var t = Utils.date();

            var msg = null;
            if (err) {
                if (err.message) msg = err.message;
            }
            else if (ack != null) {
                if (ack.message) msg = ack.message;
                if (ack.error) msg = ack.error;
            }

            if (msg !== null) {
                status = {
                    orderId: cancel.clientOrderId,
                    rejectMessage: msg,
                    orderStatus: Models.OrderStatus.Rejected,
                    cancelRejected: true,
                    time: t,
                    leavesQuantity: 0
                };

                if (msg === "You have exceeded your request rate of 5 r/s." || msg === "BadRequest") {
                    setTimeout(() => this.cancelOrder(cancel), 500);
                }
                
            }
            else {
                status = {
                    orderId: cancel.clientOrderId,
                    orderStatus: Models.OrderStatus.Cancelled,
                    time: t,
                    leavesQuantity: 0
                };
            }

            this.OrderUpdate.trigger(status);
        });
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var cb = (err? : Error, resp? : any, ack? : CoinbaseOrderAck) => {
            var status : Models.OrderStatusReport
            var t = Utils.date();

            if (ack == null || typeof ack.id === "undefined") {
                this._log("NO EXCHANGE ID PROVIDED FOR ORDER ID:", order.orderId, err, ack);
            }

            var msg = null;
            if (err) {
                if (err.message) msg = err.message;
            }
            else if (ack != null) {
                if (ack.message) msg = ack.message;
                if (ack.error) msg = ack.error;
            }
            else if (ack == null) {
                msg = "No ack provided!!"
            }

            if (msg !== null) {
                status = {
                    orderId: order.orderId,
                    rejectMessage: msg,
                    orderStatus: Models.OrderStatus.Rejected,
                    time: t
                };
            }
            else {
                status = {
                    exchangeId: ack.id,
                    orderId: order.orderId,
                    orderStatus: Models.OrderStatus.Working,
                    time: t
                };
            }

            this.OrderUpdate.trigger(status);
        };

        var o : CoinbaseOrder = {
            client_oid : order.orderId,
            size : order.quantity.toString(),
            price : order.price.toString(),
            product_id : "BTC-USD"
        };

        if (order.side === Models.Side.Bid) 
            this._authClient.buy(o, cb);
        else if (order.side === Models.Side.Ask) 
            this._authClient.sell(o, cb);

        return new Models.OrderGatewayActionReport(Utils.date());
    };

    public cancelsByClientOrderId = false;

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private onStateChange = (s : StateChange) => {
        var status = convertConnectivityStatus(s);
        this.ConnectChanged.trigger(status);
    };

    private onReceived = (tsMsg : Models.Timestamped<CoinbaseReceived>) => { 
        var msg = tsMsg.data;
        if (typeof msg.client_oid === "undefined" || !this._orderData.allOrders.hasOwnProperty(msg.client_oid))
            return;

        var status : Models.OrderStatusReport = {
            exchangeId: msg.order_id,
            orderId: msg.client_oid,
            orderStatus: Models.OrderStatus.Working,
            time: tsMsg.time,
            leavesQuantity: convertSize(msg.size)
        };

        this.OrderUpdate.trigger(status);
    };

    private onOpen = (tsMsg : Models.Timestamped<CoinbaseOpen>) => { 
        var msg = tsMsg.data;
        var orderId = this._orderData.exchIdsToClientIds[msg.order_id];
        if (typeof orderId === "undefined")
            return;

        var t = Utils.date();
        var status : Models.OrderStatusReport = {
            orderId: orderId,
            orderStatus: Models.OrderStatus.Working,
            time: tsMsg.time,
            leavesQuantity: convertSize(msg.remaining_size)
        };

        this.OrderUpdate.trigger(status);
    };

    private onDone = (tsMsg : Models.Timestamped<CoinbaseDone>) => { 
        var msg = tsMsg.data;
        var orderId = this._orderData.exchIdsToClientIds[msg.order_id];
        if (typeof orderId === "undefined")
            return;

        var ordStatus = msg.reason === "filled" 
            ? Models.OrderStatus.Complete 
            : Models.OrderStatus.Cancelled;

        var status : Models.OrderStatusReport = {
            orderId: orderId,
            orderStatus: ordStatus,
            time: tsMsg.time,
            leavesQuantity: 0
        };

        this.OrderUpdate.trigger(status);
    };

    private onMatch = (tsMsg : Models.Timestamped<CoinbaseMatch>) => { 
        var msg = tsMsg.data;
        var liq : Models.Liquidity = Models.Liquidity.Make;
        var client_oid = this._orderData.exchIdsToClientIds[msg.maker_order_id];

        if (typeof client_oid === "undefined") {
            liq = Models.Liquidity.Take;
            client_oid = this._orderData.exchIdsToClientIds[msg.taker_order_id];
        }

        if (typeof client_oid === "undefined") 
            return;

        var status : Models.OrderStatusReport = {
            orderId: client_oid,
            orderStatus: Models.OrderStatus.Working,
            time: tsMsg.time,
            lastQuantity: convertSize(msg.size),
            lastPrice: convertPrice(msg.price),
            liquidity: liq
        };

        this.OrderUpdate.trigger(status);
    };

    private onChange = (tsMsg : Models.Timestamped<CoinbaseChange>) => { 
        var msg = tsMsg.data;
        var orderId = this._orderData.exchIdsToClientIds[msg.order_id];
        if (typeof orderId === "undefined")
            return;

        var status : Models.OrderStatusReport = {
            orderId: orderId,
            orderStatus: Models.OrderStatus.Working,
            time: tsMsg.time,
            quantity: convertSize(msg.new_size)
        };

        this.OrderUpdate.trigger(status);
    };

     _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbaseOE");
    constructor(
        private _orderData : Interfaces.IOrderStateCache,
        private _orderBook : CoinbaseOrderBook,
        private _authClient : CoinbaseAuthenticatedClient) {

        this._orderBook.on("statechange", this.onStateChange);

        this._orderBook.on("received", this.onReceived)
        this._orderBook.on("open", this.onOpen);
        this._orderBook.on("done", this.onDone);
        this._orderBook.on("match", this.onMatch);
        this._orderBook.on("change", this.onChange);
    }
}


class CoinbasePositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:CoinbasePG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private onTick = () => {
        this._authClient.getAccounts((err? : Error, resp? : any, data? : CoinbaseAccountInformation[]) => {
            _.forEach(data, d => {
                var c = d.currency === "BTC" ? Models.Currency.BTC : Models.Currency.USD;
                var rpt = new Models.CurrencyPosition(convertPrice(d.available), convertPrice(d.hold), c);
                this.PositionUpdate.trigger(rpt);
            });
        });
    };

    constructor(private _authClient : CoinbaseAuthenticatedClient) {
        setInterval(this.onTick, 7500);
        this.onTick();
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
    constructor(config : Config.IConfigProvider, orders : Interfaces.IOrderStateCache) {
        var orderbook = new CoinbaseExchange.OrderBook("BTC-USD", config.GetString("CoinbaseWebsocketUrl"), config.GetString("CoinbaseRestUrl"));
        var authClient = new CoinbaseExchange.AuthenticatedClient(config.GetString("CoinbaseApiKey"), 
            config.GetString("CoinbaseSecret"), config.GetString("CoinbasePassphrase"), config.GetString("CoinbaseRestUrl"));

        var orderGateway = config.GetString("CoinbaseOrderDestination") == "Coinbase" ?
            <Interfaces.IOrderEntryGateway>new CoinbaseOrderEntryGateway(orders, orderbook, authClient)
            : new NullGateway.NullOrderGateway();

        var positionGateway = new CoinbasePositionGateway(authClient);
        var mdGateway = new CoinbaseMarketDataGateway(orderbook);

        super(
            mdGateway,
            orderGateway,
            positionGateway,
            new CoinbaseBaseGateway());
    }
}