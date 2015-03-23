/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />

import Utils = require("./utils");
import Models = require("../common/models");
import Messaging = require("../common/messaging");

export interface IExchangeDetailsGateway {
    name() : string;
    makeFee() : number;
    takeFee() : number;
    exchange() : Models.Exchange;
}

export interface IGateway {
    ConnectChanged : Utils.Evt<Models.ConnectivityStatus>;
}

export interface IMarketDataGateway extends IGateway {
    MarketData : Utils.Evt<Models.Market>;
    MarketTrade : Utils.Evt<Models.GatewayMarketTrade>;
}

export interface IOrderEntryGateway extends IGateway {
    sendOrder(order : Models.BrokeredOrder) : Models.OrderGatewayActionReport;
    cancelOrder(cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport;
    replaceOrder(replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport;
    OrderUpdate : Utils.Evt<Models.OrderStatusReport>;
    cancelsByClientOrderId : boolean;
    generateClientOrderId() : string;
}

export interface IPositionGateway {
    PositionUpdate : Utils.Evt<Models.CurrencyPosition>;
}

export class CombinedGateway {
    constructor(
        public md : IMarketDataGateway,
        public oe : IOrderEntryGateway,
        public pg : IPositionGateway,
        public base : IExchangeDetailsGateway) { }
}

export interface IMarketTradeBroker {
    MarketTrade : Utils.Evt<Models.MarketSide>;
    marketTrades : Models.MarketSide[];
}

export interface IMarketDataBroker {
    MarketData : Utils.Evt<Models.Market>;
    currentBook : Models.Market;
}

export interface IOrderBroker {
    sendOrder(order : Models.SubmitNewOrder) : Models.SentOrder;
    cancelOrder(cancel : Models.OrderCancel);
    replaceOrder(replace : Models.CancelReplaceOrder) : Models.SentOrder;
    OrderUpdate : Utils.Evt<Models.OrderStatusReport>;
    cancelOpenOrders() : void;

    Trade : Utils.Evt<Models.Trade>;
}

export interface IPositionBroker {
    getPosition(currency : Models.Currency) : Models.CurrencyPosition;
}

export interface IOrderStateCache {
    allOrders : { [orderId: string]: Models.OrderStatusReport[] };
    allOrdersFlat : Models.OrderStatusReport[];
    exchIdsToClientIds : { [exchId: string] : string};
}

export interface IBroker {
    makeFee() : number;
    takeFee() : number;
    exchange() : Models.Exchange;
    pair : Models.CurrencyPair;

    connectStatus : Models.ConnectivityStatus;
    ConnectChanged : Utils.Evt<Models.ConnectivityStatus>;
}

export interface IEwmaCalculator {
    latest : number;
    Updated : Utils.Evt<any>;
}

export class Repository<T> {
    constructor(private _name : string,
                private _validator : (a : T) => boolean,
                private _paramsEqual : (a : T, b : T) => boolean,
                defaultParameter : T,
                private _rec : Messaging.IReceive<T>,
                private _pub : Messaging.IPublish<T>) {
        _pub.registerSnapshot(() => [this.latest]);
        _rec.registerReceiver(this.updateParameters);
        this._latest = defaultParameter;
    }

    private _log : Utils.Logger = Utils.log("tribeca:"+this._name);
    NewParameters = new Utils.Evt();

    private _latest : T;
    public get latest() : T {
        return this._latest;
    }

    public updateParameters = (newParams : T) => {
        if (this._validator(newParams) && this._paramsEqual(newParams, this._latest)) {
            this._latest = newParams;
            this._log("Changed parameters %j", this.latest);
            this.NewParameters.trigger();
        }

        this._pub.publish(this.latest);
    };
}