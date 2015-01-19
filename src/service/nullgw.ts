/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />

import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class NullOrderGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    sendOrder(order : Models.BrokeredOrder) : Models.OrderGatewayActionReport {
        setTimeout(() => this.trigger(order.orderId, Models.OrderStatus.Working), 10);
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    cancelOrder(cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport {
        setTimeout(() => this.trigger(cancel.clientOrderId, Models.OrderStatus.Complete), 10);
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    replaceOrder(replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    }

    private trigger(orderId : string, status : Models.OrderStatus) {
        var rpt : Models.OrderStatusReport = {
            orderId: orderId,
            orderStatus: status,
            time: Utils.date()
        };
        this.OrderUpdate.trigger(rpt);
    }

    constructor() {
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 500);
    }
}

export class NullPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    constructor() {
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(500, Models.Currency.USD)), 2500);
        setInterval(() => this.PositionUpdate.trigger(new Models.CurrencyPosition(2, Models.Currency.BTC)), 5000);
    }
}

class NullMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    MarketTrade = new Utils.Evt<Models.MarketSide>();

    constructor() {
        setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), 500);
        setInterval(() => this.MarketData.trigger(this.generateMarketData()), 500);
        setInterval(() => this.MarketTrade.trigger(this.genSingleLevel()), 500);
    }

    private genSingleLevel = () => new Models.MarketSide(Math.random(), Math.random(), Utils.date());

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
    name() : string {
        return "Null";
    }

    makeFee() : number {
        return 0;
    }

    takeFee() : number {
        return 0;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.Null;
    }
}

export class NullGateway extends Interfaces.CombinedGateway {
    constructor() {
        super(new NullMarketDataGateway(), new NullOrderGateway(), new NullPositionGateway(), new NullGatewayDetails());
    }
}