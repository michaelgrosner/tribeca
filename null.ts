/// <reference path="utils.ts" />

class NullOrderGateway implements IOrderEntryGateway {
    OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();
    ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();

    sendOrder(order : BrokeredOrder) : OrderGatewayActionReport {
        setTimeout(() => this.trigger(order.orderId, OrderStatus.Working), 10);
        return new OrderGatewayActionReport(date());
    }

    cancelOrder(cancel : BrokeredCancel) : OrderGatewayActionReport {
        setTimeout(() => this.trigger(cancel.clientOrderId, OrderStatus.Complete), 10);
        return new OrderGatewayActionReport(date());
    }

    replaceOrder(replace : BrokeredReplace) : OrderGatewayActionReport {
        this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    }

    private trigger(orderId : string, status : OrderStatus) {
        var rpt : OrderStatusReport = {
            orderId: orderId,
            orderStatus: status,
            time: date()
        };
        this.OrderUpdate.trigger(rpt);
    }

    constructor() {
        setTimeout(() => this.ConnectChanged.trigger(ConnectivityStatus.Connected), 500);
    }
}

class NullPositionGateway implements IPositionGateway {
    PositionUpdate : Evt<CurrencyPosition> = new Evt<CurrencyPosition>();

    constructor() {
        setInterval(() => this.PositionUpdate.trigger(new CurrencyPosition(500, Currency.USD)), 2500);
        setInterval(() => this.PositionUpdate.trigger(new CurrencyPosition(2, Currency.BTC)), 5000);
    }
}