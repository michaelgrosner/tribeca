import * as moment from "moment";

export interface ITimestamped {
    time : Date;
}

export class Timestamped<T> implements ITimestamped {
    constructor(public data: T, public time: Date) {}

    public toString() {
        return "time=" + (this.time === null ? null : moment(this.time).format('D/M HH:mm:ss,SSS')) + ";data=" + this.data;
    }
}

export var Prefixes = {
  SUBSCRIBE: '_',
  SNAPSHOT: '=',
  MESSAGE: '-',
  DELAYED: '.'
}

export var Topics = {
  FairValue: 'a',
  Quote: 'b',
  ActiveSubscription: 'c',
  ActiveChange: 'd',
  MarketData: 'e',
  QuotingParametersChange: 'f',
  SafetySettings: 'g',
  Product: 'h',
  OrderStatusReports: 'i',
  ProductAdvertisement: 'j',
  ApplicationState: 'k',
  Notepad: 'l',
  ToggleConfigs: 'm',
  Position: 'n',
  ExchangeConnectivity: 'o',
  SubmitNewOrder: 'p',
  CancelOrder: 'q',
  MarketTrade: 'r',
  Trades: 's',
  ExternalValuation: 't',
  QuoteStatus: 'u',
  TargetBasePosition: 'v',
  TradeSafetyValue: 'w',
  CancelAllOrders: 'x',
  CleanAllClosedOrders: 'y',
  CleanAllOrders: 'z',
  CleanTrade: 'A',
  TradesChart: 'B',
  WalletChart: 'C',
  EWMAChart: 'D'
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

export class MarketStats implements ITimestamped {
    constructor(public fv: number,
                public bid: number,
                public ask: number,
                public time: Date) { }
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
    CAD,
    ETH,
    ETC,
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
    AMP,
    XRP,
    KRW,
    IOT
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
export enum Exchange { Null, HitBtc, OkCoin, AtlasAts, BtcChina, Coinbase, Bitfinex, Korbit }
export enum Side { Bid, Ask, Unknown }
export enum OrderType { Limit, Market }
export enum TimeInForce { IOC, FOK, GTC }
export enum OrderStatus { New, Working, Complete, Cancelled, Rejected, Other }
export enum Liquidity { Make, Take }

export interface ProductState {
    advert: ProductAdvertisement;
    fixed: number
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
    orderId : any;
    exchangeId : any;
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
    source: OrderSource;
    partiallyFilled : boolean;
    pendingCancel : boolean;
    pendingReplace : boolean;
    cancelRejected : boolean;
}

export interface OrderStatusUpdate extends Partial<OrderStatusReport> { }

export interface IStdev {
    fv: number;
    tops: number;
    bid: number;
    ask: number;
}

export class EWMAChart implements ITimestamped {
    constructor(public stdevWidth: IStdev,
                public ewmaQuote: number,
                public ewmaShort: number,
                public ewmaMedium: number,
                public ewmaLong: number,
                public fairValue: number,
                public time: Date) {}
}

export class TradeChart implements ITimestamped {
    constructor(public price: number,
                public side: Side,
                public quantity: number,
                public value: number,
                public type: string,
                public time: Date) {}
}

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
                public Ktime: Date,
                public Kqty: number,
                public Kprice: number,
                public Kvalue: number,
                public Kdiff: number,
                public feeCharged: number,
                public loadedFromDB: boolean) {}
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
                public profitBase: number,
                public profitQuote: number,
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

export enum QuotingMode { Top, Mid, Join, InverseJoin, InverseTop, PingPong, Boomerang, AK47, HamelinRat, Depth }
export enum FairValueModel { BBO, wBBO }
export enum AutoPositionMode { Manual, EWMA_LS, EWMA_LMS }
export enum PingAt { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings  }
export enum PongAt { ShortPingFair, LongPingFair, ShortPingAggressive, LongPingAggressive }
export enum APR { Off, Size, SizeWidth }
export enum SOP { Off, x2trades, x3trades, x2Size, x3Size, x2tradesSize, x3tradesSize }
export enum STDEV { Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff }

export interface QuotingParameters {
    widthPing?: number;
    widthPingPercentage?: number;
    widthPong?: number;
    widthPongPercentage?: number;
    widthPercentage?: boolean;
    bestWidth?: boolean;
    buySize?: number;
    buySizePercentage?: number;
    buySizeMax?: boolean;
    sellSize?: number;
    sellSizePercentage?: number;
    sellSizeMax?: boolean;
    pingAt?: PingAt;
    pongAt?: PongAt;
    mode?: QuotingMode;
    fvModel?: FairValueModel;
    targetBasePosition?: number;
    targetBasePositionPercentage?: number;
    positionDivergence?: number;
    positionDivergencePercentage?: number;
    percentageValues?: boolean;
    autoPositionMode?: AutoPositionMode;
    aggressivePositionRebalancing?: APR;
    superTrades?: SOP;
    tradesPerMinute?: number;
    tradeRateSeconds?: number;
    quotingEwmaProtection?: boolean;
    quotingStdevProtection?: STDEV;
    audio?: boolean;
    bullets?: number;
    range?: number;
    ewmaSensiblityPercentage?: number;
    longEwmaPeridos?: number;
    mediumEwmaPeridos?: number;
    shortEwmaPeridos?: number;
    quotingEwmaProtectionPeridos?: number;
    quotingStdevProtectionFactor?: number;
    quotingStdevProtectionPeriods?: number;
    aprMultiplier?: number;
    sopWidthMultiplier?: number;
    cancelOrdersAuto?: boolean;
    cleanPongsAuto?: number;
    stepOverSize?: number;
    profitHourInterval?: number;
    delayUI?: number;
}

export class ExchangePairMessage<T> {
    constructor(public exchange: Exchange, public pair: CurrencyPair, public data: T) { }
}

export class ProductAdvertisement {
    constructor(public exchange: Exchange, public pair: CurrencyPair, public environment: string, public matryoshka: string, public homepage: string, public minTick: number) { }
}

export class ApplicationState {
    constructor(public memory: number, public hour: number, public freq: number, public dbsize: number) { }
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
    constructor(
      public data: number,
      public sideAPR: string[],
      public time: Date
    ) {}
}
