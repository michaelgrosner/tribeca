import * as _ from "lodash";
import * as moment from "moment"

export interface ITimestamped {
    time : Date;
}

export class Timestamped<T> implements ITimestamped {
    constructor(public data: T, public time: Date) {}

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
                public time: Date,
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
                public time: Date) { }

    public toString() {
        return "asks: [" + this.asks.join(";") + "] bids: [" + this.bids.join(";") + "]";
    }
}

export class MarketTrade implements ITimestamped {
    constructor(public exchange: Exchange,
                public pair: CurrencyPair,
                public price: number,
                public size: number,
                public time: Date,
                public quote: TwoSidedQuote,
                public bid: MarketSide,
                public ask: MarketSide,
                public make_side: Side) {}
}

export enum Currency { 
    USD, 
    BTC, 
    LTC, 
    EUR, 
    GBP, 
    CNY, 
    ETH, 
    BFX, 
    RRT, 
    ZEC, 
    BCN, 
    DASH, 
    DOGE, 
    DSH, 
    EMC, 
    FCN, 
    LSK, 
    NXT, 
    QCN, 
    SDB, 
    SCB, 
    STEEM, 
    XDN, 
    XEM, 
    XMR, 
    ARDR, 
    WAVES, 
    BTU, 
    MAID, 
    AMP 
}

export function toCurrency(c: string) : Currency|undefined {
    return Currency[c.toUpperCase()];
}

export function fromCurrency(c: Currency) : string|undefined {
    const t = Currency[c];
    if (t) return t.toUpperCase();
    return undefined;
}

export enum GatewayType { MarketData, OrderEntry, Position }
export enum ConnectivityStatus { Connected, Disconnected }
export enum Exchange { Null, HitBtc, OkCoin, AtlasAts, BtcChina, Coinbase, Bitfinex }
export enum Side { Bid, Ask, Unknown }
export enum OrderType { Limit, Market }
export enum TimeInForce { IOC, FOK, GTC }
export enum OrderStatus { New, Working, Complete, Cancelled, Rejected, Other }
export enum Liquidity { Make, Take }

export const orderIsDone = (status: OrderStatus) => {
    switch (status) {
        case OrderStatus.Complete:
        case OrderStatus.Cancelled:
        case OrderStatus.Rejected:
            return true;
        default:
            return false;
    }
}

export enum MarketDataFlag {
    Unknown = 0,
    NoChange = 1,
    First = 1 << 1,
    PriceChanged = 1 << 2,
    SizeChanged = 1 << 3,
    PriceAndSizeChanged = 1 << 4
}

export enum OrderSource {
    Unknown = 0,
    Quote = 1,
    OrderTicket = 2
}

export class SubmitNewOrder {
    constructor(public side: Side,
                public quantity: number,
                public type: OrderType,
                public price: number,
                public timeInForce: TimeInForce,
                public exchange: Exchange,
                public generatedTime: Date,
                public preferPostOnly: boolean,
                public source: OrderSource,
                public msg?: string) {
                    this.msg = msg || null;
                }
}

export class CancelReplaceOrder {
    constructor(public origOrderId: string,
                public quantity: number,
                public price: number,
                public exchange: Exchange,
                public generatedTime: Date) {}
}

export class OrderCancel {
    constructor(public origOrderId: string,
                public exchange: Exchange,
                public generatedTime: Date) {}
}

export class SentOrder {
    constructor(public sentOrderClientId: string) {}
}

export interface OrderStatusReport {
    pair : CurrencyPair;
    side : Side;
    quantity : number;
    type : OrderType;
    price : number;
    timeInForce : TimeInForce;
    orderId : string;
    exchangeId : string;
    orderStatus : OrderStatus;
    rejectMessage : string;
    time : Date;
    lastQuantity : number;
    lastPrice : number;
    leavesQuantity : number;
    cumQuantity : number;
    averagePrice : number;
    liquidity : Liquidity;
    exchange : Exchange;
    computationalLatency : number;
    version : number;
    preferPostOnly: boolean;
    source: OrderSource,
    partiallyFilled : boolean;
    pendingCancel : boolean;
    pendingReplace : boolean;
    cancelRejected : boolean;
}

export interface OrderStatusUpdate extends Partial<OrderStatusReport> { }

export class Trade implements ITimestamped {
    constructor(public tradeId: string,
                public time: Date,
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
                public time: Date) {}
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
    constructor(public price: number, public time: Date) {}
}

export enum QuoteAction { New, Cancel }
export enum QuoteSent { First, Modify, UnsentDuplicate, Delete, UnsentDelete, UnableToSend }

export class Quote {
    constructor(public price: number,
                public size: number) {}
}

export class TwoSidedQuote implements ITimestamped {
    constructor(public bid: Quote, public ask: Quote, public time: Date) {}
}

export enum QuoteStatus { Live, Held }

export class SerializedQuotesActive {
    constructor(public active: boolean, public time: Date) {}
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

export enum QuotingMode { Top, Mid, Join, InverseJoin, InverseTop, PingPong, Depth }
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

export function toUtcFormattedTime(t: moment.Moment | Date) {
    return (moment.isMoment(t) ? <moment.Moment>t : moment(t)).format('M/D/YY HH:mm:ss,SSS');
}

export function veryShortDate(t: moment.Moment | Date) {
    return (moment.isMoment(t) ? <moment.Moment>t : moment(t)).format('M/D');
}

export function toShortTimeString(t: moment.Moment | Date) {
    return (moment.isMoment(t) ? <moment.Moment>t : moment(t)).format('HH:mm:ss,SSS');
}

export class ExchangePairMessage<T> {
    constructor(public exchange: Exchange, public pair: CurrencyPair, public data: T) { }
}

export class ProductAdvertisement {
    constructor(public exchange: Exchange, public pair: CurrencyPair, public environment: string, public minTick: number) { }
}

export class Message implements ITimestamped {
    constructor(public text: string, public time: Date) {}
}

export class RegularFairValue {
    constructor(public time: Date, public value: number) {}
}

export class TradeSafety {
    constructor(public buy: number,
                public sell: number,
                public combined: number,
                public buyPing: number,
                public sellPong: number,
                public time: Date) {}
}

export class TargetBasePositionValue {
    constructor(public data: number, public time: Date) {}
}

export class CancelAllOrdersRequest {
    constructor() {}
}