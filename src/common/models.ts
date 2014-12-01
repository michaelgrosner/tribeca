/// <reference path="../../typings/tsd.d.ts" />

export class Timestamped<T> {
    constructor(public data : T, public time : Moment) {}
}

export class MarketSide {
    constructor(public price: number, public size: number) { }

    public equals(other : MarketSide) {
        return this.price == other.price && this.size == other.size;
    }
}

export class MarketUpdate {
    constructor(
        public bid : MarketSide,
        public ask : MarketSide,
        public time : Moment) { }

    public equals(other : MarketUpdate) {
        if (other == null) return false;
        return this.ask.equals(other.ask) && this.bid.equals(other.bid);
    }
}

export class Market {
    constructor(
        public update : MarketUpdate,
        public exchange : Exchange,
        public flag : MarketDataFlag) { }
}

export enum GatewayType { MarketData, OrderEntry, Position }
export enum Currency { USD, BTC, LTC }
export enum ConnectivityStatus { Connected, Disconnected }
export enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts }
export enum Side { Bid, Ask }
export enum OrderType { Limit, Market }
export enum TimeInForce { IOC, FOK, GTC }
export enum OrderStatus { New, Working, Complete, Cancelled, Rejected, Other }
export enum Liquidity { Make, Take }

export enum MarketDataFlag {
    NoChange = 0,
    First = 1,
    PriceChanged = 1 << 1,
    SizeChanged = 1 << 2,
    PriceAndSizeChanged = 1 << 3
}

export interface Order {
    side : Side;
    quantity : number;
    type : OrderType;
    price : number;
    timeInForce : TimeInForce;
    exchange : Exchange;
}

export class SubmitNewOrder implements Order {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange,
        public generatedTime: Moment) {}
}

export class CancelReplaceOrder {
    constructor(
        public origOrderId : string,
        public quantity : number,
        public price : number,
        public exchange : Exchange,
        public generatedTime : Moment) {}
}

export class OrderCancel {
    constructor(
        public origOrderId : string,
        public exchange : Exchange,
        public generatedTime : Moment) {}
}

export class BrokeredOrder implements Order {
    constructor(
        public orderId : string,
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange) {}
}

export class BrokeredReplace implements Order {
    constructor(
        public orderId : string,
        public origOrderId : string,
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange,
        public exchangeId : string) {}
}

export class BrokeredCancel {
    constructor(
        public clientOrderId : string,
        public requestId : string,
        public side : Side,
        public exchangeId : string) {}
}

export class SentOrder {
    constructor(public sentOrderClientId : string) {}
}

export class OrderGatewayActionReport {
    constructor(public sentTime : Moment) {}
}

export interface OrderStatusReport {
    side? : Side;
    quantity? : number;
    type? : OrderType;
    price? : number;
    timeInForce? : TimeInForce;
    orderId? : string;
    exchangeId? : string;
    orderStatus? : OrderStatus;
    rejectMessage? : string;
    time? : Moment;
    lastQuantity? : number;
    lastPrice? : number;
    leavesQuantity? : number;
    cumQuantity? : number;
    averagePrice? : number;
    liquidity? : Liquidity;
    exchange? : Exchange;
    computationalLatency? : number;
    version? : number;

    partiallyFilled? : boolean;
    pendingCancel? : boolean;
    pendingReplace? : boolean;
    cancelRejected? : boolean;
}

export class OrderStatusReportImpl implements OrderStatusReport {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public orderId : string,
        public exchangeId : string,
        public orderStatus : OrderStatus,
        public rejectMessage : string,
        public time : Moment,
        public lastQuantity : number,
        public lastPrice : number,
        public leavesQuantity : number,
        public cumQuantity : number,
        public averagePrice : number,
        public liquidity : Liquidity,
        public exchange : Exchange,
        public computationalLatency : number,
        public version : number,
        public partiallyFilled : boolean,
        public pendingCancel : boolean,
        public pendingReplace : boolean,
        public cancelRejected : boolean) {}
}

export class ExchangeCurrencyPosition {
    constructor(public amount : number,
                public currency : Currency,
                public exchange : Exchange) { }
}

export class CurrencyPosition {
    constructor(public amount : number,
                public currency : Currency) {}

    public toExchangeReport(exch : Exchange) {
        return new ExchangeCurrencyPosition(this.amount, this.currency, exch);
    }
}

export interface OrderRequestFromUI {
    exchange : string;
    side : string;
    price : number;
    quantity : number;
    timeInForce : string;
    orderType : string
}

export interface ReplaceRequestFromUI {
    price : number;
    quantity : number;
}