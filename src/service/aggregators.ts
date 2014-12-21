import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class PositionAggregator {
    PositionUpdate = new Utils.Evt<Models.ExchangeCurrencyPosition>();

    constructor(private _brokers : Array<Interfaces.IBroker>) {
        this._brokers.forEach(b => {
            b.PositionUpdate.on(m => this.PositionUpdate.trigger(m));
        });
    }
}

export class MarketDataAggregator {
    MarketData = new Utils.Evt<Models.Market>();

    constructor(private _brokers : Array<Interfaces.IBroker>) {
        this._brokers.forEach(b => {
            b.MarketData.on(m => this.MarketData.trigger(m));
        });
    }
}

export class OrderBrokerAggregator {
    _log : Utils.Logger = Utils.log("tribeca:brokeraggregator");
    _brokersByExch : { [exchange: number]: Interfaces.IBroker} = {};

    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();

    constructor(private _brokers : Array<Interfaces.IBroker>) {

        this._brokers.forEach(b => {
            b.OrderUpdate.on(o => this.OrderUpdate.trigger(o));
        });

        for (var i = 0; i < this._brokers.length; i++)
            this._brokersByExch[this._brokers[i].exchange()] = this._brokers[i];
    }

    public submitOrder = (o : Models.SubmitNewOrder) => {
        try {
            this._brokersByExch[o.exchange].sendOrder(o);
        }
        catch (e) {
            this._log("Exception while sending order", o, e, e.stack);
        }
    };

    public cancelReplaceOrder = (o : Models.CancelReplaceOrder) => {
        try {
            this._brokersByExch[o.exchange].replaceOrder(o);
        }
        catch (e) {
            this._log("Exception while cancel/replacing order", o, e, e.stack);
        }
    };

    public cancelOrder = (o : Models.OrderCancel) => {
        try {
            this._brokersByExch[o.exchange].cancelOrder(o);
        }
        catch (e) {
            this._log("Exception while cancelling order", o, e, e.stack);
        }
    };
}