/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
///<reference path="../interfaces.ts"/>

import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
var uuid = require('node-uuid');

export class NullOrderGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Q.Promise<number> => { return Q(0); };

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => {
        return uuid.v1();
    }

    sendOrder(order: Models.BrokeredOrder): Models.OrderGatewayActionReport {
        if (order.timeInForce == Models.TimeInForce.IOC)
            throw new Error("Cannot send IOCs");
        setTimeout(() => this.trigger(order.orderId, Models.OrderStatus.Working, order), 10);
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    cancelOrder(cancel: Models.BrokeredCancel): Models.OrderGatewayActionReport {
        setTimeout(() => this.trigger(cancel.clientOrderId, Models.OrderStatus.Complete), 10);
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    replaceOrder(replace: Models.BrokeredReplace): Models.OrderGatewayActionReport {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    }

    private trigger(orderId: string, status: Models.OrderStatus, order?: Models.BrokeredOrder) {
        var rpt: Models.OrderStatusReport = {
            orderId: orderId,
            orderStatus: status,
            time: Utils.date()
        };
        this.OrderUpdate.trigger(rpt);

        if (status === Models.OrderStatus.Working && Math.random() < .1) {
            var rpt: Models.OrderStatusReport = {
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

    constructor() {
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(500, 50, Models.Currency.USD)), 2500);
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(500, 50, Models.Currency.EUR)), 2500);
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(500, 50, Models.Currency.GBP)), 2500);
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(2, .5, Models.Currency.BTC)), 5000);
    }
}

export class NullMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    constructor() {
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 500);
        setInterval(() => this.MarketData.trigger(this.generateMarketData()), 5000);
        setInterval(() => this.MarketTrade.trigger(this.genMarketTrade()), 15000);
    }

    private genMarketTrade = () => new Models.GatewayMarketTrade(Math.random(), Math.random(), Utils.date(), false, Models.Side.Bid);

    private genSingleLevel = () => new Models.MarketSide(200 + 100 * Math.random(), Math.random());

    private generateMarketData = () => {
        var genSide = () => {
            var s = [];
            for (var x = 0; x < 5; x++) {
                s.push(this.genSingleLevel());
            }
            return s;
        };
        return new Models.Market(genSide(), genSide(), Utils.date());
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
    
    private static AllPairs = [
        new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD),
        new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.EUR),
        new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.GBP)
    ];
    public get supportedCurrencyPairs() {
        return NullGatewayDetails.AllPairs;
    }
}

export class NullGateway extends Interfaces.CombinedGateway {
    constructor() {
        super(new NullMarketDataGateway(), new NullOrderGateway(), new NullPositionGateway(), new NullGatewayDetails());
    }
}