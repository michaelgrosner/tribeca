export var Topics = {
  FairValue: 'a',
  Connectivity: 'd',
  MarketData: 'e',
  QuotingParametersChange: 'f',
  OrderStatusReports: 'i',
  ProductAdvertisement: 'j',
  ApplicationState: 'k',
  EWMAStats: 'l',
  STDEVStats: 'm',
  Position: 'n',
  Profit: 'o',
  SubmitNewOrder: 'p',
  CancelOrder: 'q',
  MarketTrade: 'r',
  Trades: 's',
  QuoteStatus: 'u',
  TargetBasePosition: 'v',
  TradeSafetyValue: 'w',
  CancelAllOrders: 'x',
  CleanAllClosedTrades: 'y',
  CleanAllTrades: 'z',
  CleanTrade: 'A',
  MarketChart: 'D',
  Notepad: 'E',
  MarketDataLongTerm: 'H'
};

export class MarketSide {
    constructor(public price: number,
                public size: number,
                public cssMod: number) { }
}

export class Market {
    constructor(public bids: MarketSide[],
                public asks: MarketSide[],
                public diff?: boolean) { }
}

export class MarketStats {
    constructor(public fv: number,
                public bid: number,
                public ask: number,
                public time: number) { }
}

export class MarketTrade {
    constructor(public price: number,
                public quantity: number,
                public time: number,
                public side: Side) {}
}

export enum Connectivity { Disconnected, Connected }
export enum Side { Bid, Ask }
export enum OrderType { Limit, Market }
export enum TimeInForce { GTC, IOC, FOK }
export enum OrderStatus { Waiting, Working, Terminated }

export interface IStdev {
    fv: number;
    fvMean: number;
    tops: number;
    topsMean: number;
    bid: number;
    bidMean: number;
    ask: number;
    askMean: number;
}

export interface IEwma {
  ewmaVeryLong: number;
  ewmaLong: number;
  ewmaMedium: number;
  ewmaShort: number;
  ewmaQuote: number;
  ewmaWidth: number;
  ewmaTrendDiff: number;
}

export class MarketChart {
    constructor(public stdevWidth:     IStdev = null,
                public ewma:            IEwma = null,
                public tradesBuySize:  number = 0,
                public tradesSellSize: number = 0) {}
}

export class TradeChart {
    constructor(public price:     number = 0,
                public side:        Side = null,
                public quantity: number  = 0,
                public value:    number  = 0,
                public pong:     boolean = false) {}
}

export class Trade {
    constructor(public tradeId: string,
                public time: number,
                public price: number,
                public quantity: number,
                public side: Side,
                public value: number,
                public Ktime: number,
                public Kqty: number,
                public Kprice: number,
                public Kvalue: number,
                public delta: number,
                public feeCharged: number,
                public isPong: boolean,
                public loadedFromDB: boolean) {}
}

export class Order {
    constructor(public orderId: string,
                public exchangeId: string,
                public side: Side,
                public quantity: number,
                public type: OrderType,
                public isPong: boolean,
                public price: number,
                public timeInForce: TimeInForce,
                public status: OrderStatus,
                public time: number,
                public latency: number) {}
}

export class OrderSide {
    constructor(public orderId: string,
                public side: Side,
                public price: number,
                public quantity: number) {}
}

export class Wallet {
    constructor(public amount: number = 0,
                public held:   number = 0,
                public value:  number = 0,
                public profit: number = 0) {}
}

export class PositionReport {
    constructor(public base:  Wallet = new Wallet(),
                public quote: Wallet = new Wallet()) {}
}

export class OrderRequestFromUI {
    constructor(public side: Side,
                public price: number,
                public quantity: number,
                public timeInForce: TimeInForce,
                public type: OrderType) {}
}

export class CleanTradeRequestFromUI {
    constructor(public tradeId: string) {}
}

export class ConnectionStatus {
    constructor(public online: Connectivity = null) {}
}

export class ExchangeState {
    constructor(public agree:  Connectivity = null,
                public online: Connectivity = null) {}
}

export class AgreeRequestFromUI {
    constructor(public agree: Connectivity) {}
}

export class OrderCancelRequestFromUI {
    constructor(public orderId: string,
                public exchange: string) {}
}

export class FairValue {
    constructor(public price: number = 0) {}
}

export class Quote {
    constructor(public price: number,
                public size: number,
                public isPong: boolean) {}
}

export class TwoSidedQuote {
    constructor(public bid: Quote,
                public ask: Quote) {}
}

export enum QuoteStatus { Disconnected, Live, DisabledQuotes, MissingData, UnknownHeld, WidthTooHigh, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed, UpTrendHeld, DownTrendHeld }

export enum SideAPR { Off, Buy, Sell }

export class TwoSidedQuoteStatus {
    constructor(public bidStatus:        QuoteStatus = QuoteStatus.Disconnected,
                public askStatus:        QuoteStatus = QuoteStatus.Disconnected,
                public sideAPR:              SideAPR = SideAPR.Off,
                public quotesInMemoryWaiting: number = 0,
                public quotesInMemoryWorking: number = 0,
                public quotesInMemoryZombies: number = 0) {}
}

export enum QuotingMode { Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth }
export enum OrderPctTotal { Value, Side, TBPValue, TBPSide }
export enum QuotingSafety { Off, PingPong, PingPoing, Boomerang, AK47 }
export enum FairValueModel { BBO, wBBO, rwBBO }
export enum AutoPositionMode { Manual, EWMA_LS, EWMA_LMS, EWMA_4 }
export enum DynamicPDivMode { Manual, Linear, Sine, SQRT, Switch }
export enum PingAt { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings }
export enum PongAt { ShortPingFair, AveragePingFair, LongPingFair, ShortPingAggressive, AveragePingAggressive, LongPingAggressive }
export enum APR { Off, Size, SizeWidth }
export enum SOP { Off, Trades, Size, TradesSize }
export enum STDEV { Off, OnFV, OnFVAPROff, OnTops, OnTopsAPROff, OnTop, OnTopAPROff }

export class PortfolioParameters {
    constructor(public currency: string = "") {}
};

export interface QuotingParameters {
    widthPing?: number;
    widthPingPercentage?: number;
    widthPong?: number;
    widthPongPercentage?: number;
    widthPercentage?: boolean;
    bestWidth?: boolean;
    orderPctTotal?: OrderPctTotal;
    buySize?: number;
    buySizePercentage?: number;
    buySizeMax?: boolean;
    sellSize?: number;
    sellSizePercentage?: number;
    sellSizeMax?: boolean;
    pingAt?: PingAt;
    pongAt?: PongAt;
    mode?: QuotingMode;
    safety?: QuotingSafety
    fvModel?: FairValueModel;
    targetBasePosition?: number;
    targetBasePositionPercentage?: number;
    positionDivergence?: number;
    positionDivergencePercentage?: number;
    positionDivergenceMin?: number;
    positionDivergencePercentageMin?: number;
    positionDivergenceMode?: number;
    percentageValues?: boolean;
    autoPositionMode?: AutoPositionMode;
    aggressivePositionRebalancing?: APR;
    superTrades?: SOP;
    tradesPerMinute?: number;
    tradeRateSeconds?: number;
    protectionEwmaWidthPing?: boolean;
    protectionEwmaQuotePrice?: boolean;
    quotingEwmaTrendProtection?: boolean;
    quotingEwmaTrendThreshold?: number;
    quotingStdevProtection?: STDEV;
    quotingStdevBollingerBands?: boolean;
    audio?: boolean;
    bullets?: number;
    range?: number;
    rangePercentage?: number;
    ewmaSensiblityPercentage?: number;
    veryLongEwmaPeriods?: number;
    longEwmaPeriods?: number;
    mediumEwmaPeriods?: number;
    shortEwmaPeriods?: number;
    extraShortEwmaPeriods?: number;
    ultraShortEwmaPeriods?: number;
    protectionEwmaPeriods?: number;
    quotingStdevProtectionFactor?: number;
    quotingStdevProtectionPeriods?: number;
    aprMultiplier?: number;
    sopWidthMultiplier?: number;
    sopSizeMultiplier?: number;
    sopTradesMultiplier?: number;
    cancelOrdersAuto?: boolean;
    lifetime?: number;
    cleanPongsAuto?: number;
    stepOverSize?: number;
    profitHourInterval?: number;
    delayUI?: number;
}

export class ProductAdvertisement {
    constructor(public exchange:    string = "",
                public inet:        string = "",
                public base:        string = "",
                public quote:       string = "",
                public symbol:      string = "",
                public margin:      number = 0,
                public webMarket:   string = "",
                public webOrders:   string = "",
                public environment: string = "",
                public matryoshka:  string = "",
                public source:      string = "",
                public tickPrice:   number = 8,
                public tickSize:    number = 8,
                public stepPrice:   number = 1e-8,
                public stepSize:    number = 1e-8,
                public minSize:     number = 1e-8) { }
}

export class ApplicationState {
    constructor(public addr: string,
                public freq: number,
                public theme: number,
                public memory: number,
                public dbsize: number) { }
}

export class TradeSafety {
    constructor(
      public buy:      number = 0,
      public sell:     number = 0,
      public combined: number = 0,
      public buyPing:  number = 0,
      public sellPing: number = 0) {}
}

export class TargetBasePositionValue {
    constructor(
      public tbp:  number = 0,
      public pDiv: number = 0) {}
}

export class Map {
    constructor(
      public val: number,
      public str: string) {}
}

export function getMap<T>(enumObject: T) {
    let names: Map[] = [];
    for (let mem in enumObject) {
      if (!enumObject.hasOwnProperty(mem)) continue;
      let val = parseInt(mem, 10);
      if (val >= 0) {
        let str = String(enumObject[mem]);
        if (str == 'AK47') str = 'AK-47';
        names.push({ 'str': str, 'val': val });
      }
    }
    return names;
  }
