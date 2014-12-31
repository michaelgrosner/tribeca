/// <reference path="../common/models.ts" />

import Utils = require("./utils");
import Models = require("../common/models");

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
    MarketData : Utils.Evt<Models.MarketUpdate>;
}

export interface IOrderEntryGateway extends IGateway {
    sendOrder(order : Models.BrokeredOrder) : Models.OrderGatewayActionReport;
    cancelOrder(cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport;
    replaceOrder(replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport;
    OrderUpdate : Utils.Evt<Models.OrderStatusReport>;
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

export interface IBroker {
    MarketData : Utils.Evt<Models.Market>;
    currentBook : Models.Market;

    name() : string;
    makeFee() : number;
    takeFee() : number;
    exchange() : Models.Exchange;
    pair : Models.CurrencyPair;

    sendOrder(order : Models.SubmitNewOrder) : Models.SentOrder;
    cancelOrder(cancel : Models.OrderCancel);
    replaceOrder(replace : Models.CancelReplaceOrder) : Models.SentOrder;
    OrderUpdate : Utils.Evt<Models.OrderStatusReport>;
    allOrderStates() : Array<Models.OrderStatusReport>;
    cancelOpenOrders() : void;

    // todo: think about it, should fill reports inc/decrement positions? does it matter?
    getPosition(currency : Models.Currency) : Models.ExchangeCurrencyPosition;
    PositionUpdate : Utils.Evt<Models.ExchangeCurrencyPosition>;

    connectStatus : Models.ConnectivityStatus;
    ConnectChanged : Utils.Evt<Models.ConnectivityStatus>;
}