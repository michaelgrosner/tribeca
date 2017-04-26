/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
///<reference path="../interfaces.ts"/>

import * as _ from "lodash";
import * as Q from "q";
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import Config = require("../config");
var uuid = require('node-uuid');

export class NullOrderGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Q.Promise<number> => { return Q(0); };

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => {
        return uuid.v1();
    }

    private raiseTimeEvent = (o: Models.OrderStatusReport) => {
        this.OrderUpdate.trigger({
            orderId: o.orderId,
            computationalLatency: Utils.fastDiff(Utils.date(), o.time)
        })
    };

    sendOrder(order: Models.OrderStatusReport) {
        if (order.timeInForce == Models.TimeInForce.IOC)
            throw new Error("Cannot send IOCs");
        setTimeout(() => this.trigger(order.orderId, Models.OrderStatus.Working, order), 10);
        this.raiseTimeEvent(order);
    }

    cancelOrder(cancel: Models.OrderStatusReport) {
        setTimeout(() => this.trigger(cancel.orderId, Models.OrderStatus.Complete), 10);
        this.raiseTimeEvent(cancel);
    }

    replaceOrder(replace: Models.OrderStatusReport) {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    }

    private trigger(orderId: string, status: Models.OrderStatus, order?: Models.OrderStatusReport) {
        var rpt: Models.OrderStatusUpdate = {
            orderId: orderId,
            orderStatus: status,
            time: Utils.date()
        };
        this.OrderUpdate.trigger(rpt);

        if (status === Models.OrderStatus.Working && Math.random() < .1) {
            var rpt: Models.OrderStatusUpdate = {
                orderId: orderId,
                orderStatus: status,
                time: Utils.date(),
                lastQuantity: order.quantity,
                lastPrice: order.price,
                liquidity: Math.random() < .5 ? Models.Liquidity.Make : Models.Liquidity.Take
            };
            setTimeout(() => this.OrderUpdate.trigger(rpt), 1000);
        }
    }

    constructor() {
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 500);
    }
}

export class NullPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    constructor(pair: Models.CurrencyPair) {
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(500, 50, pair.base)), 2500);
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(500, 50, pair.quote)), 2500);
    }
}

export class NullMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    constructor(private _minTick: number) {
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 500);
        setInterval(() => this.MarketData.trigger(this.generateMarketData()), 5000*Math.random());
        setInterval(() => this.MarketTrade.trigger(this.genMarketTrade()), 15000);
    }

    private getPrice = (sign: number) => Utils.roundNearest(1000 + sign * 100 * Math.random(), this._minTick);

    private genMarketTrade = () => {
        const side = (Math.random() > .5 ? Models.Side.Bid : Models.Side.Ask);
        const sign = Models.Side.Ask === side ? 1 : -1;
        return new Models.GatewayMarketTrade(this.getPrice(sign), Math.random(), Utils.date(), false, side);
    }

    private genSingleLevel = (sign: number) => 
        new Models.MarketSide(this.getPrice(sign), Math.random());

    private readonly Depth: number = 25;
    private generateMarketData = () => {
        const genSide = (sign: number) => {
            const s = _.times(this.Depth, _ => this.genSingleLevel(sign));
            return _.sortBy(s, i => sign*i.price);
        };
        return new Models.Market(genSide(-1), genSide(1), Utils.date());
    };
}

class NullGatewayDetails implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    name(): string {
        return "Null";
    }

    makeFee(): number {
        return 0;
    }

    takeFee(): number {
        return 0;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.Null;
    }

    constructor(public minTickIncrement: number) {}
}

class NullGateway extends Interfaces.CombinedGateway {
    constructor(config: Config.IConfigProvider, pair: Models.CurrencyPair) {
        const minTick = config.GetNumber("NullGatewayTick");
        super(
            new NullMarketDataGateway(minTick), 
            new NullOrderGateway(), 
            new NullPositionGateway(pair), 
            new NullGatewayDetails(minTick));
    }
}

export async function createNullGateway(config: Config.IConfigProvider, pair: Models.CurrencyPair) : Promise<Interfaces.CombinedGateway> {
    return new NullGateway(config, pair);
}