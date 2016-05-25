/// <reference path="../../typings/main.d.ts" />

export interface ITimestamped {
    time : moment.Moment;
}

export class Timestamped<T> implements ITimestamped {
    constructor(public data: T, public time: moment.Moment) {}

    public toString() {
        return "time=" + toUtcFormattedTime(this.time) + ";data=" + this.data;
    }
}

export class MarketSide {
    constructor(public price: number,
                public size: number) { }

    public toString() {
        return "px=" + this.price + ";size=" + this.size;
    }
}

export class GatewayMarketTrade implements ITimestamped {
    constructor(public price: number,
                public size: number,
                public time: moment.Moment,
                public onStartup: boolean,
                public make_side: Side) { }
}

export function marketSideEquals(t: MarketSide, other: MarketSide, tol?: number) {
    tol = tol || 1e-4;
    if (other == null) return false;
    return Math.abs(t.price - other.price) > tol && Math.abs(t.size - other.size) > tol;
}

export class Market implements ITimestamped {
    constructor(public bids: MarketSide[],
                public asks: MarketSide[],
                public time: moment.Moment) { }

    public toString() {
        return "asks: [" + this.asks.join(";") + "] bids: [" + this.bids.join(";") + "]";
    }
}

export class MarketTrade implements ITimestamped {
    constructor(public exchange: Exchange,
                public pair: CurrencyPair,
                public price: number,
                public size: number,
                public time: moment.Moment,
                public quote: TwoSidedQuote,
                public bid: MarketSide,
                public ask: MarketSide,
                public make_side: Side) {}
}

export enum GatewayType { MarketData, OrderEntry, Position }
export enum Currency { USD, BTC, LTC, EUR, GBP, CNY , ETH }
export enum ConnectivityStatus { Connected, Disconnected }
export enum Exchange { Null, HitBtc, OkCoin, AtlasAts, BtcChina, Coinbase, Bitfinex }
export enum Side { Bid, Ask, Unknown }
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
    constructor(public side: Side,
                public quantity: number,
                public type: OrderType,
                public price: number,
                public timeInForce: TimeInForce,
                public exchange: Exchange,
                public generatedTime: moment.Moment,
                public preferPostOnly: boolean,
                public msg?: string) {
                    this.msg = msg || null;
                }
}

export class CancelReplaceOrder {
    constructor(public origOrderId: string,
                public quantity: number,
                public price: number,
                public exchange: Exchange,
                public generatedTime: moment.Moment) {}
}

export class OrderCancel {
    constructor(public origOrderId: string,
                public exchange: Exchange,
                public generatedTime: moment.Moment) {}
}

export class BrokeredOrder implements Order {
    constructor(public orderId: string,
                public side: Side,
                public quantity: number,
                public type: OrderType,
                public price: number,
                public timeInForce: TimeInForce,
                public exchange: Exchange,
                public preferPostOnly: boolean) {}
}

export class BrokeredReplace implements Order {
    constructor(public orderId: string,
                public origOrderId: string,
                public side: Side,
                public quantity: number,
                public type: OrderType,
                public price: number,
                public timeInForce: TimeInForce,
                public exchange: Exchange,
                public exchangeId: string,
                public preferPostOnly: boolean) {}
}

export class BrokeredCancel {
    constructor(public clientOrderId: string,
                public requestId: string,
                public side: Side,
                public exchangeId: string) {}
}

export class SentOrder {
    constructor(public sentOrderClientId: string) {}
}

export class OrderGatewayActionReport {
    constructor(public sentTime: moment.Moment) {}
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
    time? : moment.Moment;
    lastQuantity? : number;
    lastPrice? : number;
    leavesQuantity? : number;
    cumQuantity? : number;
    averagePrice? : number;
    liquidity? : Liquidity;
    exchange? : Exchange;
    computationalLatency? : number;
    version? : number;
    preferPostOnly?: boolean;

    partiallyFilled? : boolean;
    pendingCancel? : boolean;
    pendingReplace? : boolean;
    cancelRejected? : boolean;
}

export class OrderStatusReportImpl implements OrderStatusReport, ITimestamped {
    constructor(public pair: CurrencyPair,
                public side: Side,
                public quantity: number,
                public type: OrderType,
                public price: number,
                public timeInForce: TimeInForce,
                public orderId: string,
                public exchangeId: string,
                public orderStatus: OrderStatus,
                public rejectMessage: string,
                public time: moment.Moment,
                public lastQuantity: number,
                public lastPrice: number,
                public leavesQuantity: number,
                public cumQuantity: number,
                public averagePrice: number,
                public liquidity: Liquidity,
                public exchange: Exchange,
                public computationalLatency: number,
                public version: number,
                public partiallyFilled: boolean,
                public pendingCancel: boolean,
                public pendingReplace: boolean,
                public cancelRejected: boolean,
                public preferPostOnly: boolean) {}

    public toString() {
        var components: string[] = [];

        components.push("orderId=" + this.orderId);
        components.push("time=" + this.time.format('M/d/YY h:mm:ss,SSS'));
        if (typeof this.exchangeId !== "undefined") components.push("exchangeId=" + this.exchangeId);
        components.push("pair=" + Currency[this.pair.base] + "/" + Currency[this.pair.quote]);
        if (typeof this.exchange !== "undefined") components.push("exchange=" + Exchange[this.exchange]);
        components.push("orderStatus=" + OrderStatus[this.orderStatus]);
        if (this.partiallyFilled) components.push("partiallyFilled");
        if (this.pendingCancel) components.push("pendingCancel");
        if (this.pendingReplace) components.push("pendingReplace");
        if (this.cancelRejected) components.push("cancelRejected");
        components.push("side=" + Side[this.side]);
        components.push("quantity=" + this.quantity);
        components.push("price=" + this.price);
        components.push("tif=" + TimeInForce[this.timeInForce]);
        components.push("type=" + OrderType[this.type]);
        components.push("version=" + this.version);
        if (typeof this.rejectMessage !== "undefined") components.push(this.rejectMessage);
        if (typeof this.computationalLatency !== "undefined") components.push("computationalLatency=" + this.computationalLatency);
        if (typeof this.lastQuantity !== "undefined") components.push("lastQuantity=" + this.lastQuantity);
        if (typeof this.lastPrice !== "undefined") components.push("lastPrice=" + this.lastPrice);
        if (typeof this.leavesQuantity !== "undefined") components.push("leavesQuantity=" + this.leavesQuantity);
        if (typeof this.cumQuantity !== "undefined") components.push("cumQuantity=" + this.cumQuantity);
        if (typeof this.averagePrice !== "undefined") components.push("averagePrice=" + this.averagePrice);
        if (typeof this.liquidity !== "undefined") components.push("liquidity=" + Liquidity[this.liquidity]);

        return components.join(";");
    }
}

export class Trade implements ITimestamped {
    constructor(public tradeId: string,
                public time: moment.Moment,
                public exchange: Exchange,
                public pair: CurrencyPair,
                public price: number,
                public quantity: number,
                public side: Side,
                public value: number,
                public liquidity: Liquidity,
                public feeCharged: number) {}
}

export class CurrencyPosition {
    constructor(public amount: number,
                public heldAmount: number,
                public currency: Currency) {}

    public toString() {
        return "currency=" + Currency[this.currency] + ";amount=" + this.amount;
    }
}

export class PositionReport {
    constructor(public baseAmount: number,
                public quoteAmount: number,
                public baseHeldAmount: number,
                public quoteHeldAmount: number,
                public value: number,
                public quoteValue: number,
                public pair: CurrencyPair,
                public exchange: Exchange,
                public time: moment.Moment) {}
}

export class OrderRequestFromUI {
    constructor(public side: string,
                public price: number,
                public quantity: number,
                public timeInForce: string,
                public orderType: string) {}
}

export interface ReplaceRequestFromUI {
    price : number;
    quantity : number;
}

export class FairValue implements ITimestamped {
    constructor(public price: number, public time: moment.Moment) {}
}

export enum QuoteAction { New, Cancel }
export enum QuoteSent { First, Modify, UnsentDuplicate, Delete, UnsentDelete, UnableToSend }

export class Quote {
    constructor(public price: number,
                public size: number) {}

    private static Tol = 1e-3;
    public equals(other: Quote) {
        return Math.abs(this.price - other.price) < Quote.Tol && Math.abs(this.size - other.size) < Quote.Tol;
    }
}

export class TwoSidedQuote implements ITimestamped {
    constructor(public bid: Quote, public ask: Quote, public time: moment.Moment) {}
}

export enum QuoteStatus { Live, Held }

export class SerializedQuotesActive {
    constructor(public active: boolean, public time: moment.Moment) {}
}

export class TwoSidedQuoteStatus {
    constructor(public bidStatus: QuoteStatus, public askStatus: QuoteStatus) {}
}

export class CurrencyPair {
    constructor(public base: Currency, public quote: Currency) {}

    public toString() {
        return Currency[this.base] + "/" + Currency[this.quote];
    }
}

export function currencyPairEqual(a: CurrencyPair, b: CurrencyPair): boolean {
    return a.base === b.base && a.quote === b.quote;
}

export enum QuotingMode { Top, Mid, Join, InverseJoin, InverseTop, PingPong }
export enum FairValueModel { BBO, wBBO }
export enum AutoPositionMode { Off, EwmaBasic }

export class QuotingParameters {
    constructor(public width: number,
                public size: number,
                public mode: QuotingMode,
                public fvModel: FairValueModel,
                public targetBasePosition: number,
                public positionDivergence: number,
                public ewmaProtection: boolean,
                public autoPositionMode: AutoPositionMode,
                public aggressivePositionRebalancing: boolean,
                public tradesPerMinute: number,
                public tradeRateSeconds: number,
                public longEwma: number,
                public shortEwma: number,
                public quotingEwma: number,
                public aprMultiplier: number,
                public stepOverSize: number) {}
}

export function toUtcFormattedTime(t: moment.Moment) {
    return t.format('M/D/YY HH:mm:ss,SSS');
}

export function toShortTimeString(t: moment.Moment) {
    return t.format('HH:mm:ss,SSS');
}

export class ExchangePairMessage<T> {
    constructor(public exchange: Exchange, public pair: CurrencyPair, public data: T) { }
}

export class ProductAdvertisement {
    constructor(public exchange: Exchange, public pair: CurrencyPair, public environment: string) { }
}

export class Message implements ITimestamped {
    constructor(public text: string, public time: moment.Moment) {}
}

export class RegularFairValue {
    constructor(public time: moment.Moment, public value: number) {}
}

export class TradeSafety {
    constructor(public buy: number,
                public sell: number,
                public combined: number,
                public buyPing: number,
                public sellPong: number,
                public time: moment.Moment) {}
}

export class TargetBasePositionValue {
    constructor(public data: number, public time: moment.Moment) {}
}

export class CancelAllOrdersRequest {
    constructor() {}
}
