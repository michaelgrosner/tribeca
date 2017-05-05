/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="../interfaces.ts"/>

import * as _ from "lodash";
import * as Q from "q";
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import Config = require("../config");
var uuid = require('node-uuid');

interface Order {
    id: string,
    price: number,
    cumQty: number,
    leavesQty: number,
    priortity: number,
    isPlacedOrder: boolean,
    side: Models.Side
}

export class TestingGateway implements Interfaces.IPositionGateway, Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    private _priority = 0;
    private readonly _buys = new Map<string, Order>();
    private readonly _sells = new Map<string, Order>();
    
    supportsCancelAllOpenOrders = () : boolean => { return true; };

    cancelAllOpenOrders = () : Q.Promise<number> => { 
        const d = Q.defer<number>();
        setImmediate(() => {
            const placedOrders = [...this._buys.values()]
                .concat([...this._sells.values()])
                .filter(o => o.isPlacedOrder);
            
            for (let p of placedOrders)
                this.cancel(p.side, p.id);
            
            d.resolve(placedOrders.length);
        });
        return d.promise;
     };

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => uuid.v1();

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

                if (o.isPlacedOrder) {
                    this.OrderUpdate.trigger({
                        orderId: o.id,
                        lastPrice: fillPx,
                        lastQuantity: fillQty,
                        liquidity: liq,
                        orderStatus: o.leavesQty <= 0.001 ? Models.OrderStatus.Complete : Models.OrderStatus.Working
                    });
                }

                if (liq === Models.Liquidity.Make)
                    this.MarketTrade.trigger(new Models.GatewayMarketTrade(fillPx, fillQty, 
                        new Date(), false, o.side));
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
    };

    sendOrder(order: Models.OrderStatusReport) {
        setImmediate(() => {
            let sideMap = order.side === Models.Side.Bid ? this._buys : this._sells;

            sideMap.set(order.orderId, {
                id: order.orderId,
                price: order.price,
                cumQty: 0,
                leavesQty: order.quantity,
                priortity: this._priority++,
                isPlacedOrder: true,
                side: order.side
            });

            this.OrderUpdate.trigger({
                pendingCancel: false,
                orderStatus: Models.OrderStatus.Working,
                orderId: order.orderId,
                leavesQuantity: order.quantity
            })

            setImmediate(this.match);
        });
    }

    cancelOrder(cancel: Models.OrderStatusReport) {
        setImmediate(() => {
            this.cancel(cancel.side, cancel.orderId);
        });
    }

    private cancel = (side: Models.Side, orderId: string) => {
        let existing : Order;
        if (side === Models.Side.Bid) {
            existing = this._buys.get(orderId);
            this._buys.delete(orderId);
        }
        else {
            existing = this._sells.get(orderId);
            this._sells.delete(orderId);
        }

        this.OrderUpdate.trigger({
            orderId: orderId,
            orderStatus: existing ? Models.OrderStatus.Cancelled : Models.OrderStatus.Rejected,
            pendingCancel: false
        });
    }

    replaceOrder(replace: Models.OrderStatusReport) {
        let existing : Order;
        if (replace.side === Models.Side.Bid) {
            existing = this._buys.get(replace.orderId);
        }
        else {
            existing = this._sells.get(replace.orderId);
        }

        if (existing) {
            existing.price = replace.price;
            existing.leavesQty = replace.quantity - existing.cumQty;
        }
        
        this.OrderUpdate.trigger({
            orderId: replace.orderId,
            orderStatus: existing ? Models.OrderStatus.Cancelled : Models.OrderStatus.Rejected,
            pendingReplace: false
        });
    }

    private _basePosition = 10;
    private _quotePosition = 5000;
    private recomputePosition = () => {
        const heldBase = _([...this._sells.values()]).filter(s => s.isPlacedOrder).sumBy(s => s.leavesQty);
        const heldQuote = _([...this._buys.values()]).filter(s => s.isPlacedOrder).sumBy(s => s.price*s.leavesQty);

        const b = new Models.CurrencyPosition(this._basePosition, heldBase, this._pair.base);
        this.PositionUpdate.trigger(b);
        const q = new Models.CurrencyPosition(this._quotePosition, heldQuote, this._pair.quote);
        this.PositionUpdate.trigger(q);
    }

    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    private getPrice = (sign: number) => 
        Utils.roundNearest(1000 + sign * 100 * Math.random(), this.minTickIncrement);

    private generateOrder = (crossing: boolean = false) => {
        const side = Math.random() > .5 ? Models.Side.Bid : Models.Side.Ask;
        const sign = Models.Side.Ask === side ? 1 : -1;
        const price = this.getPrice(crossing ? -sign : sign);
        const qty = Math.random();
        const sideOrders = side === Models.Side.Bid ? this._buys : this._sells;
        const o : Order = {
            id: this.generateClientOrderId(),
            isPlacedOrder: false,
            leavesQty: qty,
            price: price,
            priortity: this._priority++,
            cumQty: 0,
            side: side
        }

        sideOrders.set(o.id, o);
    };

    private createMarketActivity = () => {
        if (Math.random() > .55) {
            this.generateOrder(Math.random() > .05);
        }
        else {
            const side = (Math.random() > .5) ? this._buys : this._sells;
            const o = [...side.values()].filter(o => !o.isPlacedOrder);
            if (o.length > 0) {
                this.cancel(o[0].side, o[0].id);
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

        this.MarketData.trigger(new Models.Market(
            getMarketSide(this._buys, -1), 
            getMarketSide(this._sells, 1), 
            new Date()));
    };

    public get hasSelfTradePrevention() { return false; }
    name = () => "null";
    makeFee = () => 0;
    takeFee = () => 0;
    exchange = () => Models.Exchange.Null;

    constructor(public minTickIncrement: number, private readonly _pair: Models.CurrencyPair) {
        setTimeout(() => {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
            _.times(500, () => this.generateOrder());
            this.raiseMarket();
            this.recomputePosition();      
        }, 500);

        setInterval(this.recomputePosition, 15000);
        setInterval(this.createMarketActivity, 1000);
    }
}

class NullGateway extends Interfaces.CombinedGateway {
    constructor(config: Config.IConfigProvider, pair: Models.CurrencyPair) {
        const minTick = config.GetNumber("NullGatewayTick");
        const all = new TestingGateway(minTick, pair);
        super(all, all, all, all);
    }
}

export async function createNullGateway(config: Config.IConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    return new NullGateway(config, pair);
}