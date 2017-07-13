/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="utils.ts"/>

import Utils = require("./utils");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import q = require("q");

export interface IExchangeDetailsGateway {
    name(): string;
    makeFee(): number;
    takeFee(): number;
    exchange(): Models.Exchange;
    minTickIncrement: number;
    hasSelfTradePrevention: boolean;
}

export interface IGateway {
    ConnectChanged: Utils.Evt<Models.ConnectivityStatus>;
}

export interface IBrokerConnectivity {
    connectStatus: Models.ConnectivityStatus;
    ConnectChanged: Utils.Evt<Models.ConnectivityStatus>;
}

export interface IMarketDataGateway extends IGateway {
    MarketData: Utils.Evt<Models.Market>;
    MarketTrade: Utils.Evt<Models.GatewayMarketTrade>;
}

export interface IOrderEntryGateway extends IGateway {
    sendOrder(order: Models.OrderStatusReport): void;
    cancelOrder(cancel: Models.OrderStatusReport): void;
    replaceOrder(replace: Models.OrderStatusReport): void;
    
    OrderUpdate: Utils.Evt<Models.OrderStatusUpdate>;
    
    cancelsByClientOrderId: boolean;
    generateClientOrderId(): string;
    
    supportsCancelAllOpenOrders() : boolean;
    cancelAllOpenOrders() : q.Promise<number>;
}

export interface IPositionGateway {
    PositionUpdate: Utils.Evt<Models.CurrencyPosition>;
}

export class CombinedGateway {
    constructor(
        public md: IMarketDataGateway,
        public oe: IOrderEntryGateway,
        public pg: IPositionGateway,
        public base: IExchangeDetailsGateway) { }
}

export interface IMarketTradeBroker {
    MarketTrade: Utils.Evt<Models.MarketSide>;
    marketTrades: Models.MarketSide[];
}

export interface IMarketDataBroker {
    MarketData: Utils.Evt<Models.Market>;
    currentBook: Models.Market;
}

export interface ITradeBroker {
    Trade: Utils.Evt<Models.Trade>;
}

export interface IOrderBroker extends ITradeBroker {
    sendOrder(order: Models.SubmitNewOrder): Models.SentOrder;
    cancelOrder(cancel: Models.OrderCancel);
    replaceOrder(replace: Models.CancelReplaceOrder): Models.SentOrder;
    OrderUpdate: Utils.Evt<Models.OrderStatusReport>;
    cancelOpenOrders(): void;
}

export interface IPositionBroker {
    getPosition(currency: Models.Currency): Models.CurrencyPosition;
    latestReport: Models.PositionReport;
    NewReport: Utils.Evt<Models.PositionReport>;
}

export interface IOrderStateCache {
    allOrders: Map<string, Models.OrderStatusReport>;
    exchIdsToClientIds: Map<string, string>;
}

export interface IBroker extends IBrokerConnectivity {
    makeFee(): number;
    takeFee(): number;
    exchange(): Models.Exchange;

    minTickIncrement: number;
    pair: Models.CurrencyPair;

    hasSelfTradePrevention: boolean;
}

export interface IEwmaCalculator {
    latest: number;
    Updated: Utils.Evt<any>;
}

export interface IRepository<T> {
    NewParameters: Utils.Evt<any>;
    latest: T;
}

export interface IPublishMessages {
    publish(text: string): void;
}