/// <reference path="../../typings/tsd.d.ts" />

export class Timestamped<T> {
    constructor(public data : T, public time : Moment) {}

    public toString() {
        return "time=" + toUtcFormattedTime(this.time) + ";data=" + this.data;
    }
}

export class MarketSide {
    constructor(public price : number,
                public size : number) { }

    public toString() {
        return "px="+this.price+";size="+this.size;
    }
}

export class GatewayMarketTrade {
    constructor(public price : number,
                public size : number,
                public time : Moment,
                public onStartup : boolean) { }
}

export function marketSideEquals(t : MarketSide, other : MarketSide, tol : number = 1e-4) {
    if (other == null) return false;
    return Math.abs(t.price - other.price) > tol && Math.abs(t.size - other.size) > tol;
}

export class Market {
    constructor(
        public bids : MarketSide[],
        public asks : MarketSide[],
        public time : Moment) { }

    public toString() {
        return "asks: [" + this.asks.join(";") + "] bids: [" + this.bids.join(";") + "]";
    }
}

export class MarketTrade {
    constructor(public price : number,
                public size : number,
                public time : Moment,
                public quote : TwoSidedQuote,
                public market : Market) {}
}

export enum GatewayType { MarketData, OrderEntry, Position }
export enum Currency { USD, BTC, LTC }
export enum ConnectivityStatus { Connected, Disconnected }
export enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts, BtcChina, Null }
export enum Side { Bid, Ask }
export enum OrderType { Limit, Market }
export enum TimeInForce { IOC, FOK, GTC }
export enum OrderStatus { New, Working, Complete, Cancelled, Rejected, Other }
export enum Liquidity { Make, Take }

export enum MarketDataFlag {
    Unknown = 0,
    NoChange = 1,
    First = 1 << 1,
    PriceChanged = 1 << 2,
    SizeChanged = 1 << 3,
    PriceAndSizeChanged = 1 << 4
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
        public generatedTime: Moment,
        public msg: string = null) {}
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
    pair? : CurrencyPair;
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
        public pair : CurrencyPair,
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

    public toString() {
        var components : string[] = [];

        components.push("orderId="+this.orderId);
        components.push("time="+this.time.format('M/d/YY h:mm:ss,SSS'));
        if (this.exchangeId != null) components.push("exchangeId="+this.exchangeId);
        components.push("pair="+Currency[this.pair.base] + "/" + Currency[this.pair.quote]);
        if (this.exchange != null) components.push("exchange="+Exchange[this.exchange]);
        components.push("orderStatus="+OrderStatus[this.orderStatus]);
        if (this.partiallyFilled) components.push("partiallyFilled");
        if (this.pendingCancel) components.push("pendingCancel");
        if (this.pendingReplace) components.push("pendingReplace");
        if (this.cancelRejected) components.push("cancelRejected");
        components.push("side="+Side[this.side]);
        components.push("quantity="+this.quantity);
        components.push("price="+this.price);
        components.push("tif="+TimeInForce[this.timeInForce]);
        components.push("type="+OrderType[this.type]);
        components.push("version="+this.version);
        if (this.rejectMessage) components.push(this.rejectMessage);
        if (this.computationalLatency != null) components.push("computationalLatency="+this.computationalLatency);
        if (this.lastQuantity != null) components.push("lastQuantity="+this.lastQuantity);
        if (this.lastPrice != null) components.push("lastPrice="+this.lastPrice);
        if (this.leavesQuantity != null) components.push("leavesQuantity="+this.leavesQuantity);
        if (this.cumQuantity != null) components.push("cumQuantity="+this.cumQuantity);
        if (this.averagePrice != null) components.push("averagePrice="+this.averagePrice);
        if (this.liquidity != null) components.push("liquidity="+Liquidity[this.liquidity]);

        return components.join(";");
    }
}

export class Trade {
    constructor(
        public tradeId : string,
        public time : Moment,
        public exchange : Exchange,
        public pair : CurrencyPair,
        public price : number,
        public quantity : number,
        public side : Side) {}
}

export class CurrencyPosition {
    constructor(public amount : number,
                public currency : Currency) {}

    public toString() {
        return "currency=" + Currency[this.currency] + ";amount=" + this.amount;
    }
}

export class OrderRequestFromUI {
    constructor(
        public exchange : string,
        public side : string,
        public price : number,
        public quantity : number,
        public timeInForce : string,
        public orderType : string,
        public pair : CurrencyPair) {}
}

export interface ReplaceRequestFromUI {
    price : number;
    quantity : number;
}

export class FairValue {
    constructor(public price : number,
                public mkt : Market) {}
}

export class TradingDecision {
    constructor(public bidAction : QuoteSent, public askAction : QuoteSent) {}

    public toString() {
        return "bidAction=" + QuoteSent[this.bidAction] + ",askAction=" + QuoteSent[this.askAction];
    }
}

export enum QuoteAction { New, Cancel }
export enum QuoteSent { First, Modify, UnsentDuplicate, Delete, UnsentDelete, UnableToSend }

export class Quote {
    constructor(public type : QuoteAction,
                public side : Side,
                public price : number = null,
                public size : number = null) {}

    public equals(other : Quote, tol : number = 1e-3) {
        return this.type == other.type
            && this.side == other.side
            && Math.abs(this.price - other.price) < tol
            && Math.abs(this.size - other.size) < tol;
    }

    public toString() {
        if (this.type == QuoteAction.New) {
            return "px="+this.price+";sz="+this.size;
        }
        else {
            return "del";
        }
    }
}

export class TwoSidedQuote {
    constructor(public bid : Quote, public ask : Quote) {}

    public toString() {
        return "bid=[" + this.bid + "]; ask=[" + this.ask + "]";
    }
}

export class CurrencyPair {
    constructor(public base : Currency, public quote : Currency) {}

    public toString() {
        return Currency[this.base] + "/" + Currency[this.quote];
    }
}

export function currencyPairEqual(a : CurrencyPair, b : CurrencyPair) : boolean {
    return a.base === b.base && a.quote === b.quote;
}

export enum QuotingMode { Top, Mid }
export enum FairValueModel { BBO, wBBO }

export class QuotingParameters {
    constructor(public width : number, public size : number,
                public mode : QuotingMode, public fvModel : FairValueModel) {}
}

export class SafetySettings {
    constructor(public tradesPerMinute : number) {}
}

export function toUtcFormattedTime(t : Moment) {
    return t.format('M/D/YY HH:mm:ss,SSS');
}

export class ExchangePairMessage<T> {
    constructor(public exchange : Exchange, public pair : CurrencyPair, public data : T) { }
}

export class ProductAdvertisement {
    constructor(public exchange : Exchange, public pair : CurrencyPair) { }
}

export function productAdvertisementsEqual(a : ProductAdvertisement, b : ProductAdvertisement) {
    return a.exchange === b.exchange && currencyPairEqual(a.pair, b.pair);
}