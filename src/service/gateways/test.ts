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
import * as OrderState from "../order-state";

const make_fee = -.0025;
const take_fee = .005;
const min_size = 0.0001;

var uuid = require('node-uuid');

type ExchangeClientCommand = OrderRequest | Cancel | Replace;

interface MatchingEngineOrderState {
    clientId: string
    priortity: number
    state: Models.OrderStatusReport
}

interface MatchingEngineRequest {
    clientId: string
    request: OrderRequest | Cancel | Replace
}

interface OrderRequest {
    kind: "order",
    order: Models.OrderStatusUpdate;
}

interface Cancel {
    kind: "cancel"
    id: string
}

interface Replace {
    kind: "replace"
    id: string
    price: number
    newQty: number
}

interface OrderUpdate {
    kind: "order"
    payload: Models.OrderStatusUpdate,
}

interface Market {
    kind: "order_book"
    payload: Models.Market
}

interface Trade {
    kind: "market_trade"
    payload: Models.GatewayMarketTrade
}

type MatchingEngineUpdate = Market | Trade;

class ExchangeClient {
    private readonly _orders = new  Map<string, Models.OrderStatusReport>();

    private _availableBase = 100;
    private _availableQuote = 50000;
    private _heldBase = 0;
    private _heldQuote = 0;

    public readonly Update = new Utils.Evt<MatchingEngineUpdate | OrderUpdate>();

    constructor(
        private readonly _clientId: string,
        private readonly _engine: MatchingEngine,
        private readonly _pair: Models.CurrencyPair) {
            _engine.subscribe(_clientId, this.interceptStatus);
            _engine.Update.on(d => this.Update.trigger(d));
        }

    public post = async (command: ExchangeClientCommand) => {
        await Promises.delay(5);

        if (this.intercept(command)) {
            this._engine.post({clientId: this._clientId, request: command});
        }
    };

    private intercept = (command: ExchangeClientCommand) : boolean => {
        switch (command.kind) {
            case "order":
                return this.interceptOrder(command);
            case "cancel": 
                return true;
        }
        return false;
    }

    private interceptOrder = (command : OrderRequest) : boolean => {
        const order = command.order;
        const hasEnoughFunds = 
            (order.side === Models.Side.Bid && order.quantity*order.price < this._availableQuote) ||
            (order.side === Models.Side.Ask && order.quantity < this._availableBase);

        if (hasEnoughFunds) {
            if (order.side === Models.Side.Bid) {
                this._availableQuote -= order.quantity*order.price;
                this._heldQuote += order.quantity*order.price;
            }
            else {
                this._availableBase -= order.quantity;
                this._heldBase += order.quantity;
            }
            return true;
        }
        else {
            const p : Models.OrderStatusReport = <Models.OrderStatusReport>{
                orderStatus: Models.OrderStatus.Rejected, 
                orderId: command.order.orderId
            };
            this.Update.trigger({kind: "order", payload: p});
            return false;
        }
    }

    private interceptStatus = (report: Models.OrderStatusReport) => {
        if (report.openQtyChange > 0) {
            this.recalcHeldPositions();
            
            if (report.side === Models.Side.Bid) {
                this._availableQuote += report.openQtyChange*report.price;
            }
            else {
                this._availableBase += report.openQtyChange;
            }
        }

        this.Update.trigger({kind: "order", payload: report})
    }

    private recalcHeldPositions = () => {
        this._heldBase = _([...this._orders.values()]).filter(s => s.side === Models.Side.Ask).sumBy(i => i.leavesQuantity);
        this._heldQuote = _([...this._orders.values()]).filter(s => s.side === Models.Side.Bid).sumBy(i => i.leavesQuantity*i.price);
    };

    public cancelAll = () => {
        return Q.delay(5).then(() => {
            for (let o of this._orders.values())
                this._engine.post({
                    clientId: this._clientId, 
                    request: {kind: "cancel", id: o.orderId}
                });

            return this._orders.size;
        });
    };

    public getPositions = async () => {
        await Promises.delay(5);

        return [
            new Models.CurrencyPosition(this._availableBase + this._heldBase, this._heldBase, this._pair.base),
            new Models.CurrencyPosition(this._availableQuote + this._heldQuote, this._heldQuote, this._pair.quote)]
    };
}

class MatchingEngine {
    private _priority = 0;
    private readonly _buys = new Map<string, MatchingEngineOrderState>();
    private readonly _sells = new Map<string, MatchingEngineOrderState>();
    private readonly _processor : OrderState.OrderStateProcessor;

    public readonly Update = new Utils.Evt<MatchingEngineUpdate>();

    constructor(private readonly _pair: Models.CurrencyPair) {
        this._processor = new OrderState.OrderStateProcessor(new Utils.RealTimeProvider(), make_fee, take_fee);
    }

    public post = async (command: MatchingEngineRequest) => {
        await Promises.delay(5);
        switch (command.request.kind) {
            case "order": return this.accept(command.clientId, command.request);                
            case "cancel": return this.cancel(command.clientId, command.request);
            case "replace": throw new Error("replace not implemented");
        }
    };

    private accept = (clientId: string, order: OrderRequest) => {
        const orderId = order.order.orderId;

        let existing = this._buys.get(orderId) || this._sells.get(orderId);
        if (existing) 
            return this.sendUpdate(clientId, <Models.OrderStatusReport>{
                orderId: orderId,
                orderStatus: Models.OrderStatus.Rejected,
                rejectMessage: "Duplicate ID"
            });

        const state : MatchingEngineOrderState = {
            clientId: clientId,
            priortity: this._priority++,
            state: this._processor.applyOrderUpdate({
                exchangeId: orderId,
                orderStatus: Models.OrderStatus.Working,
                orderId: orderId,
                leavesQuantity: order.order.quantity
            }, <Models.OrderStatusReport>order.order)[0]
        };

        const sideMap = order.order.side === Models.Side.Bid ? this._buys : this._sells;
        sideMap.set(orderId, state);

        let report = state.state;
        this.sendUpdate(clientId, report);

        this.match();

        if (report.timeInForce === Models.TimeInForce.IOC && report.leavesQuantity > 0) {
            report = this._processor.applyOrderUpdate({
                orderStatus: Models.OrderStatus.Cancelled,
                leavesQuantity: 0,
            }, report)[0]
            this.sendUpdate(clientId, report);
            
            sideMap.delete(report.orderId);
        }
    };

    private cancel = (clientId: string, cancel: Cancel) => {
        const orderId = cancel.id;

        let existing : MatchingEngineOrderState;
        if (this._buys.has(orderId)) {
            existing = this._buys.get(orderId);
            this._buys.delete(orderId);
        }
        else if (this._sells.has(orderId)) {
            existing = this._sells.get(orderId);
            this._sells.delete(orderId);
        }

        const [report, _] = this._processor.applyOrderUpdate({
            orderId: orderId,
            orderStatus: existing ? Models.OrderStatus.Cancelled : Models.OrderStatus.Rejected,
            pendingCancel: false
        }, existing.state);

        if (existing)
            existing.state = report;

        this.sendUpdate(existing.clientId, report);

        (report.side === Models.Side.Bid ? this._buys : this._sells).delete(report.orderId);

        if (existing)
            this.raiseMarket();
    }

    private match = () => {
        const match = (buy: MatchingEngineOrderState, sell: MatchingEngineOrderState) => {
            let buyLeaves = buy.state.leavesQuantity;
            let sellLeaves = sell.state.leavesQuantity;

            // already trapped order below in delete
            if ((buyLeaves <= min_size) || (sellLeaves <= min_size)) return;
            let fillQty = Math.min(buyLeaves, sellLeaves);
            buyLeaves -= fillQty;
            sellLeaves -= fillQty;

            if (buyLeaves <= min_size) this._buys.delete(buy.state.orderId);
            if (sellLeaves <= min_size) this._sells.delete(sell.state.orderId);

            const fillPx = buy.priortity < sell.priortity ? buy.state.price : sell.state.price;

            const trigger = (o: MatchingEngineOrderState, newLeaves: number) => {
                let liq = Models.Liquidity.Make;
                if ((o.state.side === Models.Side.Bid && sell.priortity < buy.priortity) || 
                    (o.state.side === Models.Side.Ask && sell.priortity > buy.priortity))
                    liq = Models.Liquidity.Take;

                const [report, trade] = this._processor.applyOrderUpdate({
                    orderId: o.state.orderId,
                    lastPrice: fillPx,
                    lastQuantity: fillQty,
                    liquidity: liq,
                    orderStatus: newLeaves <= min_size ? Models.OrderStatus.Complete : Models.OrderStatus.Working
                }, o.state);

                o.state = report;
                this.sendUpdate(o.clientId, report);

                if (liq === Models.Liquidity.Make)
                    this.Update.trigger({kind: "market_trade", payload: new Models.GatewayMarketTrade(
                        fillPx, fillQty, new Date(), false, o.state.side)});
            }

            trigger(buy, buyLeaves);
            trigger(sell, sellLeaves);
        };

        if (this._buys.size <= 0 || this._sells.size <= 0)
            return;

        const orderedBuys = _.sortBy([...this._buys.values()], o => -1*o.state.price);
        const orderedSells = _.sortBy([...this._sells.values()], o => o.state.price);

        if (orderedBuys[0].state.price < orderedSells[0].state.price)
            return;

        for (let b of orderedBuys) {
            for (let s of orderedSells) {
                if (b.state.price < s.state.price) break;
                match(b, s);
            }
        }

        this.raiseMarket();
    };

    private raiseMarket = () => {
        const getMarketSide = (side: Map<string, MatchingEngineOrderState>, i: number) : Models.MarketSide[] => {
            const byPrice = _.values(_.groupBy([...side.values()], g => g.state.price));
            return _(byPrice)
                .sortBy(s => i * s[0].state.price)
                .take(25)
                .map(b => new Models.MarketSide(b[0].state.price, _.sumBy(b, x => x.state.leavesQuantity)))
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

    private _orderCallbacks = new Map<string, (o: Models.OrderStatusReport) => void>();
    public subscribe = (clientId: string, handler: (o: Models.OrderStatusReport) => void) => {
        this._orderCallbacks.set(clientId, handler);
    };

    private sendUpdate = (clientId: string, o: Models.OrderStatusReport) => {
        const handler = this._orderCallbacks.get(clientId);
        if (handler) handler(o);
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
        const o : Models.OrderStatusUpdate = {
            orderId: uuid.v4(),
            quantity: qty,
            price: price,
            side: side,
            type: Models.OrderType.Limit,
            timeInForce: tif
        }

        this._orders.add(o.orderId);

        return o;
    };

    private createMarketActivity = () => {
        if (Math.random() > .55) {
            const shouldCross = Math.random() > .05;
            const tif = shouldCross ? Models.TimeInForce.IOC : Models.TimeInForce.GTC;

            this._client.post({
                kind: "order", 
                order: this.generateOrder(tif, shouldCross)
            });
        }
        else if (this._orders.size > 0) {
            this._client.post({
                kind: "cancel",
                id: _.sample([...this._orders.values()]),
            });
        }
    };

    public start = () => {
        setInterval(this.createMarketActivity, 1000);

        for (let o of _.times(500, () => this.generateOrder(Models.TimeInForce.GTC))) {
            this._client.post({kind: "order", order: o});
        }
    };

    private onUpdate = (u: OrderUpdate) => {
        if (Models.orderIsDone(u.payload.orderStatus)) {
            this._orders.delete(u.payload.orderId);
        }
    };

    constructor(
            public minTickIncrement: number, 
            private readonly _pair: Models.CurrencyPair,
            private readonly _client: ExchangeClient) {
        _client.Update.on(this.onUpdate);
    }
}

export class TestingGateway implements Interfaces.IPositionGateway, Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    
    supportsCancelAllOpenOrders = () : boolean => { return true; };

    cancelAllOpenOrders = () => this._client.cancelAll();

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => uuid.v1();

    sendOrder(order: Models.OrderStatusReport) {
        this._client.post({kind: "order", order: order});
    }

    cancelOrder(cancel: Models.OrderStatusReport) {
        const c : Cancel = {
            kind: "cancel", 
            id: cancel.orderId, 
        };

        this._client.post(c);
    }

    replaceOrder(replace: Models.OrderStatusReport) {
        const r : Replace = {
            kind: "replace",
            id: replace.orderId,
            newQty: replace.quantity,
            price: replace.price,
        };

        this._client.post(r);
    }

    private downloadPosition = async () => {
        for (let p of await this._client.getPositions()) {
            this.PositionUpdate.trigger(p);
        }
    }

    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    public get hasSelfTradePrevention() { return false; }
    name = () => "test";
    makeFee = () => make_fee;
    takeFee = () => take_fee;
    exchange = () => Models.Exchange.Test;

    constructor(
            public minTickIncrement: number, 
            private readonly _pair: Models.CurrencyPair,
            private readonly _client: ExchangeClient) {
        setTimeout(() => {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
            this.downloadPosition();      
        }, 500);

        setInterval(this.downloadPosition, 15000);

        _client.Update.on(this.onEngineUpdate);
    }

    private onEngineUpdate = (u: MatchingEngineUpdate | OrderUpdate) => {
        switch (u.kind) {
            case "market_trade": 
                return this.MarketTrade.trigger(u.payload);
            case "order": 
                return this.OrderUpdate.trigger(u.payload);
            case "order_book": 
                return this.MarketData.trigger(u.payload);
        }
    };
}

class TestGateway extends Interfaces.CombinedGateway {
    constructor(config: Config.IConfigProvider, pair: Models.CurrencyPair) {
        const minTick = config.GetNumber("NullGatewayTick");
        const matchingEngine = new MatchingEngine(pair);
        const generator = new TestOrderGenerator(minTick, pair, new ExchangeClient("gen", matchingEngine, pair));
        const all = new TestingGateway(minTick, pair, new ExchangeClient("me", matchingEngine, pair));

        generator.start();

        super(all, all, all, all);
    }
}

export async function createTestGateway(config: Config.IConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    return new TestGateway(config, pair);
}