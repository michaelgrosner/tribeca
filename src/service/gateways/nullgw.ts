/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="../interfaces.ts"/>

import * as _ from "lodash";
import * as Q from "q";
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import Config = require("../config");
import Promises = require("../promises");

var uuid = require('node-uuid');

type MatchingEngineCommand = Order | Cancel | Replace | BatchOrder;

interface Order {
    kind: "order"
    id: string
    price: number
    cumQty: number
    leavesQty: number
    priortity?: number
    isPlacedOrder: boolean
    side: Models.Side
    type: Models.OrderType
    tif: Models.TimeInForce
}

interface BatchOrder {
    kind: "batch_order"
    orders: Order[]
}

interface Cancel {
    kind: "cancel"
    id: string
    isPlacedOrder: boolean
}

interface Replace {
    kind: "replace"
    id: string
    isPlacedOrder: boolean
    price: number
    newQty: number
}

interface OrderUpdate {
    kind: "order"
    payload: Models.OrderStatusUpdate,
    isPlacedOrder: boolean
}

interface Market {
    kind: "order_book"
    payload: Models.Market
}

interface Trade {
    kind: "market_trade"
    payload: Models.GatewayMarketTrade
}

type MatchingEngineUpdate = OrderUpdate | Market | Trade;

class MatchingEngine {
    private _priority = 0;
    private readonly _buys = new Map<string, Order>();
    private readonly _sells = new Map<string, Order>();

    private _availableBase = 10;
    private _availableQuote = 5000;
    private _heldBase = 0;
    private _heldQuote = 0;

    public readonly Update = new Utils.Evt<MatchingEngineUpdate>();

    constructor(private readonly _pair: Models.CurrencyPair) {}

    public post = async (command: MatchingEngineCommand) => {
        await Promises.delay(5);
        switch (command.kind) {
            case "order": return this.accept([command]);                
            case "cancel": return this.cancel([command]);
            case "batch_order": return this.accept(command.orders);                
            case "replace": return this.replace(command);
        }
    };

    private hasEnoughFunds = (order: Order) : boolean => {
        if (!order.isPlacedOrder) return true;

        let notEnoughFunds = 
            (order.side === Models.Side.Bid && order.leavesQty*order.price < this._availableQuote) ||
            (order.side === Models.Side.Ask && order.leavesQty < this._availableBase);

        if (!notEnoughFunds) {
            if (order.side === Models.Side.Bid) {
                this._availableQuote -= order.leavesQty*order.price;
                this._heldQuote += order.leavesQty*order.price;
            }
            else {
                this._availableBase -= order.leavesQty;
                this._heldBase += order.leavesQty;
            }
            return true;
        }
            
        this.Update.trigger({
            kind: "order",
            isPlacedOrder: true,
            payload: {
                exchangeId: order.priortity.toString(),
                pendingCancel: false,
                orderStatus: Models.OrderStatus.Rejected,
                orderId: order.id,
                leavesQuantity: 0,
                rejectMessage: "not enough funds"
            }
        });

        return false;
    }

    private accept = (orders: Order[]) => {
        let anyWereAccepted = false;
        for (let order of orders) {
            if (!this.hasEnoughFunds(order)) continue;

            order.priortity = this._priority++;

            let sideMap = order.side === Models.Side.Bid ? this._buys : this._sells;
            sideMap.set(order.id, order);

            this.Update.trigger({
                kind: "order",
                isPlacedOrder: order.isPlacedOrder,
                payload: {
                    exchangeId: order.priortity.toString(),
                    orderStatus: Models.OrderStatus.Working,
                    orderId: order.id,
                    leavesQuantity: order.leavesQty
                }
            });

            anyWereAccepted = true;
        }

        if (anyWereAccepted) this.raiseMarket();
    };

    private cancel = (cancels: Cancel[]) => {
        let anyWereAccepted = false;
        for (let cancel of cancels) {
            const orderId = cancel.id;

            let existing : Order;
            if (this._buys.has(orderId)) {
                existing = this._buys.get(orderId);
                this._buys.delete(orderId);
            }
            else if (this._sells.has(orderId)) {
                existing = this._sells.get(orderId);
                this._sells.delete(orderId);
            }

            this.Update.trigger({
                kind: "order",
                isPlacedOrder: cancel.isPlacedOrder,
                payload: {
                    orderId: orderId,
                    orderStatus: existing ? Models.OrderStatus.Cancelled : Models.OrderStatus.Rejected,
                    pendingCancel: false
                }
            });

            if (existing && cancel.isPlacedOrder) {
                if (existing.side === Models.Side.Bid) {
                    this._availableQuote += existing.leavesQty*existing.price;
                    this._heldQuote -= existing.leavesQty*existing.price;
                }
                else {
                    this._availableBase += existing.leavesQty;
                    this._heldBase -= existing.leavesQty;
                }
            }

            if (existing) anyWereAccepted = true;
        }

        if (anyWereAccepted) this.raiseMarket();
    }

    private replace = (replace: Replace) => {
        throw new Error("replace not implemented")
    }

    private match = () => {
        const match = (buy: Order, sell: Order) => {
            let fillQty = Math.min(buy.leavesQty, sell.leavesQty);
            buy.leavesQty -= fillQty;
            sell.leavesQty -= fillQty;

            if (buy.leavesQty <= 0.001) this._buys.delete(buy.id);
            if (sell.leavesQty <= 0.001) this._sells.delete(sell.id);

            const fillPx = buy.priortity < sell.priortity ? buy.price : sell.price;

            const trigger = (o: Order) => {
                let liq = Models.Liquidity.Make;
                if ((o.side === Models.Side.Bid && sell.priortity < buy.priortity) || 
                    (o.side === Models.Side.Ask && sell.priortity > buy.priortity))
                    liq = Models.Liquidity.Take;

                this.Update.trigger({
                    kind: "order",
                    isPlacedOrder: o.isPlacedOrder,
                    payload: {
                        orderId: o.id,
                        lastPrice: fillPx,
                        lastQuantity: fillQty,
                        liquidity: liq,
                        orderStatus: o.leavesQty <= 0.001 ? Models.OrderStatus.Complete : Models.OrderStatus.Working
                    }
                });

                if (o.isPlacedOrder) {
                    if (o.side === Models.Side.Bid) {
                        this._availableQuote += fillQty*fillPx;
                        this._heldQuote -= fillQty*fillPx;
                    }
                    else {
                        this._availableBase += fillQty;
                        this._heldBase -= fillQty;
                    }
                }

                if (liq === Models.Liquidity.Make)
                    this.Update.trigger({kind: "market_trade", payload: new Models.GatewayMarketTrade(
                        fillPx, fillQty, new Date(), false, o.side)});
            }

            trigger(buy);
            trigger(sell);
        };

        if (this._buys.size <= 0 || this._sells.size <= 0)
            return;

        const orderedBuys = _.sortBy(Array.from(this._buys.values()), o => -1*o.price);
        const orderedSells = _.sortBy(Array.from(this._sells.values()), o => o.price);

        if (orderedBuys[0].price < orderedSells[0].price)
            return;

        for (let buy of orderedBuys) {
            for (let sell of orderedSells) {
                if (buy.price < sell.price) break;
                match(buy, sell);
            }
        }

        this.raiseMarket();
    };

    private raiseMarket = () => {
        const getMarketSide = (side: Map<string, Order>, i: number) : Models.MarketSide[] => {
            const byPrice = _.values(_.groupBy([...side.values()], g => g.price));
            return _(byPrice)
                .sortBy(s => i * s[0].price)
                .take(25)
                .map(b => new Models.MarketSide(b[0].price, _.sumBy(b, x => x.leavesQty)))
                .value();
        }

        this.Update.trigger({
            kind: "order_book", 
            payload: new Models.Market(
                getMarketSide(this._buys, -1), 
                getMarketSide(this._sells, 1), 
                new Date())
        });
    };

    public cancelAll = () => {
        return Q.delay(5).then(() => {
            const placedOrders = [...this._buys.values()]
                    .concat([...this._sells.values()])
                    .filter(o => o.isPlacedOrder)
                    .map<Cancel>(o => { return {kind: "cancel", id: o.id, isPlacedOrder: true}; });

            this.cancel(placedOrders);

            return placedOrders.length;
        });
    };

    public getPositions = async () => {
        await Promises.delay(5);

        return [
            new Models.CurrencyPosition(this._availableBase + this._heldBase, this._heldBase, this._pair.base),
            new Models.CurrencyPosition(this._availableQuote + this._heldQuote, this._heldQuote, this._pair.quote)]
    };
}

class TestOrderGenerator {
    private readonly _orders = new Set<string>();

    private getPrice = (sign: number) => 
        Utils.roundNearest(1000 + sign * 100 * Math.random(), this.minTickIncrement);

    private generateOrder = (tif: Models.TimeInForce, crossing: boolean = false) => {
        const side = Math.random() > .5 ? Models.Side.Bid : Models.Side.Ask;
        const sign = Models.Side.Ask === side ? 1 : -1;
        const price = this.getPrice(crossing ? -sign : sign);
        const qty = Math.random();
        const o : Order = {
            kind: "order",
            id: uuid.v1(),
            isPlacedOrder: false,
            leavesQty: qty,
            price: price,
            cumQty: 0,
            side: side,
            type: Models.OrderType.Limit,
            tif: tif
        }

        this._orders.add(o.id);

        return o;
    };

    private createMarketActivity = () => {
        if (Math.random() > .55) {
            const shouldCross = Math.random() > .05;
            const tif = shouldCross ? Models.TimeInForce.IOC : Models.TimeInForce.GTC;
            const o = this.generateOrder(tif, shouldCross);

            this._engine.post(o);
        }
        else if (this._orders.size > 0) {
            const cxl : Cancel = {
                kind: "cancel",
                id: _.sample([...this._orders.values()]),
                isPlacedOrder: false,
            }

            this._engine.post(cxl);
        }
    };

    public start = () => {
        setInterval(this.createMarketActivity, 1000);
        const bulkOrder = _.times(500, () => this.generateOrder(Models.TimeInForce.GTC));
        this._engine.post({kind: "batch_order", orders: bulkOrder});
    };

    private onUpdate = (u: MatchingEngineUpdate) => {
        if (u.kind === "order" && !u.isPlacedOrder && Models.orderIsDone(u.payload.orderStatus)) {
            this._orders.delete(u.payload.orderId);
        }
    };

    constructor(
            public minTickIncrement: number, 
            private readonly _pair: Models.CurrencyPair,
            private readonly _engine: MatchingEngine) {
        _engine.Update.on(this.onUpdate);
    }
}

export class TestingGateway implements Interfaces.IPositionGateway, Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    
    supportsCancelAllOpenOrders = () : boolean => { return true; };

    cancelAllOpenOrders = () => this._engine.cancelAll();

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => uuid.v1();

    sendOrder(order: Models.OrderStatusReport) {
        const o : Order = {
            kind: "order",
            id: order.orderId,
            price: order.price,
            cumQty: 0,
            leavesQty: order.quantity,
            isPlacedOrder: true,
            side: order.side,
            tif: order.timeInForce,
            type: order.type
        };

        this._engine.post(o);
    }

    cancelOrder(cancel: Models.OrderStatusReport) {
        const c : Cancel = {
            kind: "cancel", 
            id: cancel.orderId, 
            isPlacedOrder: true, 
        };

        this._engine.post(c);
    }

    replaceOrder(replace: Models.OrderStatusReport) {
        const r : Replace = {
            kind: "replace",
            id: replace.orderId,
            isPlacedOrder: true,
            newQty: replace.quantity,
            price: replace.price,
        };

        this._engine.post(r);
    }

    private downloadPosition = async () => {
        for (let p of await this._engine.getPositions()) {
            this.PositionUpdate.trigger(p);
        }
    }

    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    public get hasSelfTradePrevention() { return false; }
    name = () => "null";
    makeFee = () => 0;
    takeFee = () => 0;
    exchange = () => Models.Exchange.Null;

    constructor(
            public minTickIncrement: number, 
            private readonly _pair: Models.CurrencyPair,
            private readonly _engine: MatchingEngine) {
        setTimeout(() => {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
            this.downloadPosition();      
        }, 500);

        setInterval(this.downloadPosition, 15000);

        _engine.Update.on(this.onEngineUpdate);
    }

    private onEngineUpdate = (u: MatchingEngineUpdate) => {
        switch (u.kind) {
            case "market_trade": 
                return this.MarketTrade.trigger(u.payload);
            case "order": 
                if (u.isPlacedOrder)
                    return this.OrderUpdate.trigger(u.payload);
            case "order_book": 
                return this.OrderUpdate.trigger(u.payload);
        }
    };
}

class NullGateway extends Interfaces.CombinedGateway {
    constructor(config: Config.IConfigProvider, pair: Models.CurrencyPair) {
        const minTick = config.GetNumber("NullGatewayTick");
        const matchingEngine = new MatchingEngine(pair);
        const generator = new TestOrderGenerator(minTick, pair, matchingEngine);
        const all = new TestingGateway(minTick, pair, matchingEngine);

        generator.start();

        super(all, all, all, all);
    }
}

export async function createNullGateway(config: Config.IConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    return new NullGateway(config, pair);
}