import Models = require("../../share/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");

export class NullOrderGateway implements Interfaces.IOrderEntryGateway {
    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => { return Promise.resolve(0); };

    public cancelsByClientOrderId = true;

    generateClientOrderId = (): string => {
        return new Date().valueOf().toString().substr(-9);
    }

    private raiseTimeEvent = (o: Models.OrderStatusReport) => {
        this._evUp('OrderUpdateGateway', {
            orderId: o.orderId,
            computationalLatency: new Date().valueOf() - o.time
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
            time: new Date().getTime()
        };
        this._evUp('OrderUpdateGateway', rpt);

        if (status === Models.OrderStatus.Working && Math.random() < .1) {
            var rpt: Models.OrderStatusUpdate = {
                orderId: orderId,
                orderStatus: status,
                time: new Date().getTime(),
                lastQuantity: order.quantity,
                lastPrice: order.price,
                liquidity: Math.random() < .5 ? Models.Liquidity.Make : Models.Liquidity.Take
            };
            setTimeout(() => this._evUp('OrderUpdateGateway', rpt), 1000);
        }
    }

    constructor(private _evUp) {
        setTimeout(() => this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected), 500);
    }
}

export class NullGateway extends Interfaces.CombinedGateway {
    constructor(
      gwSymbol,
      cfString,
      _evUp
    ) {
        super(
          new NullOrderGateway(_evUp)
        );
    }
}
