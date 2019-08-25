export var Prefixes = {
  SNAPSHOT: '=',
  MESSAGE: '-'
};

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
                public asks: MarketSide[]) { }
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
export enum Side { Bid, Ask, Unknown }
export enum OrderType { Limit, Market }
export enum TimeInForce { GTC, IOC, FOK }
export enum OrderStatus { Waiting, Working, Terminated }
export enum Liquidity { Make, Take }

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
    constructor(public stdevWidth: IStdev,
                public ewma: IEwma,
                public fairValue: number,
                public tradesBuySize: number,
                public tradesSellSize: number) {}
}

export class TradeChart {
    constructor(public price: number,
                public side: Side,
                public quantity: number,
                public value: number,
                public pong: boolean) {}
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
                public Kdiff: number,
                public feeCharged: number,
                public isPong: boolean,
                public loadedFromDB: boolean) {}
}

export class Wallet {
    constructor(public amount: number,
                public held: number,
                public value: number,
                public profit: number) {}
}

export class PositionReport {
    constructor(public base: Wallet,
                public quote: Wallet) {}
}

export class OrderRequestFromUI {
    constructor(public side: string,
                public price: number,
                public quantity: number,
                public timeInForce: string,
                public type: string) {}
}

export class FairValue {
    constructor(public price: number) {}
}

export class Quote {
    constructor(public price: number,
                public size: number,
                public isPong: boolean) {}
}

export class TwoSidedQuote {
    constructor(public bid: Quote, public ask: Quote) {}
}

export enum QuoteStatus { Disconnected, Live, DisabledQuotes, MissingData, UnknownHeld, WidthMustBeSmaller, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed, UpTrendHeld, DownTrendHeld }

export enum SideAPR { Off, Buy, Sell }

export class TwoSidedQuoteStatus {
    constructor(public bidStatus: QuoteStatus,
                public askStatus: QuoteStatus,
                public sideAPR: SideAPR,
                public quotesInMemoryWaiting: number,
                public quotesInMemoryWorking: number,
                public quotesInMemoryZombies: number) {}
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

export interface QuotingParameters {
    widthPing?: any;
    widthPingPercentage?: number;
    widthPong?: any;
    widthPongPercentage?: number;
    widthPercentage?: boolean;
    bestWidth?: boolean;
    orderPctTotal?: OrderPctTotal;
    buySize?: any;
    buySizePercentage?: number;
    buySizeMax?: boolean;
    sellSize?: any;
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
    cleanPongsAuto?: number;
    stepOverSize?: number;
    profitHourInterval?: number;
    delayUI?: number;
}

export class ProductAdvertisement {
    constructor(public exchange: string, public inet: string, public base: string, public quote: string, public symbol: string, public margin: number, public webMarket: string, public webOrders: string, public environment: string, public matryoshka: string, public tickPrice: number, public tickSize: number, public stepPrice: number, public stepSize: number, public minSize: number) { }
}

export class ApplicationState {
    constructor(public addr: string, public freq: number, public theme: number, public memory: number, public dbsize: number) { }
}

export class TradeSafety {
    constructor(
      public buy: number,
      public sell: number,
      public combined: number,
      public buyPing: number,
      public sellPing: number
    ) {}
}

export class TargetBasePositionValue {
    constructor(
      public tbp: number,
      public pDiv: number
    ) {}
}
