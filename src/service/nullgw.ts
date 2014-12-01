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