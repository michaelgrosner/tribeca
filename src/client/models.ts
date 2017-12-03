export var Prefixes = {
  SNAPSHOT: '=',
  MESSAGE: '-'
}

export var Topics = {
  FairValue: 'a',
  Quote: 'b',
  ActiveSubscription: 'c',
  ActiveState: 'd',
  MarketData: 'e',
  QuotingParametersChange: 'f',
  SafetySettings: 'g',
  Product: 'h',
  OrderStatusReports: 'i',
  ProductAdvertisement: 'j',
  ApplicationState: 'k',
  Notepad: 'l',
  ToggleSettings: 'm',
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
  CleanAllClosedTrades: 'y',
  CleanAllTrades: 'z',
  CleanTrade: 'A',
  TradesChart: 'B',
  WalletChart: 'C',
  EWMAChart: 'D'
}

export class MarketSide {
    constructor(public price: number,
                public size: number) { }
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
    constructor(public exchange: Exchange,
                public pair: CurrencyPair,
                public price: number,
                public quantity: number,
                public time: number,
                public side: Side) {}
}

export enum Connectivity { Disconnected, Connected }
export enum Exchange { Null, HitBtc, OkCoin, Coinbase, Bitfinex, Korbit, Poloniex }
export enum Side { Bid, Ask, Unknown }
export enum OrderType { Limit, Market }
export enum TimeInForce { IOC, FOK, GTC }
export enum OrderStatus { New, Working, Complete, Cancelled }
export enum Liquidity { Make, Take }

export interface ProductState {
    advert: ProductAdvertisement;
    fixed: number
}

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

export class EWMAChart {
    constructor(public stdevWidth: IStdev,
                public ewmaQuote: number,
                public ewmaSMUDiff: number,
                public ewmaShort: number,
                public ewmaMedium: number,
                public ewmaLong: number,
                public ewmaVeryLong: number,
                public fairValue: number,
                public pingAt: string) {}
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
                public exchange: Exchange,
                public pair: CurrencyPair,
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
                public loadedFromDB: boolean) {}
}

export class CurrencyPosition {
    constructor(public amount: number,
                public heldAmount: number,
                public currency: string) {}
}

export class PositionReport {
    constructor(public baseAmount: number,
                public quoteAmount: number,
                public baseHeldAmount: number,
                public quoteHeldAmount: number,
                public baseValue: number,
                public quoteValue: number,
                public profitBase: number,
                public profitQuote: number,
                public pair: CurrencyPair,
                public exchange: Exchange) {}
}

export class OrderRequestFromUI {
    constructor(public side: string,
                public price: number,
                public quantity: number,
                public timeInForce: string,
                public orderType: string) {}
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

export enum QuoteStatus { Live, Disconnected, DisabledQuotes, MissingData, UnknownHeld, TBPHeld, MaxTradesSeconds, WaitingPing, DepletedFunds, Crossed, UpTrendHeld, DownTrendHeld }

export class TwoSidedQuoteStatus {
    constructor(public bidStatus: QuoteStatus, public askStatus: QuoteStatus, public quotesInMemoryNew: number, public quotesInMemoryWorking: number, public quotesInMemoryDone: number) {}
}

export class CurrencyPair {
    constructor(public base: string, public quote: string) {}
}

export enum QuotingMode { Top, Mid, Join, InverseJoin, InverseTop, HamelinRat, Depth }
export enum QuotingSafety { Off, PingPong, Boomerang, AK47 }
export enum FairValueModel { BBO, wBBO }
export enum AutoPositionMode { Manual, EWMA_LS, EWMA_LMS, EWMA_4 }
export enum DynamicPDivMode { Manual, Linear, Sine, SQRT, Switch }
export enum PingAt { BothSides, BidSide, AskSide, DepletedSide, DepletedBidSide, DepletedAskSide, StopPings }
export enum PongAt { ShortPingFair, LongPingFair, ShortPingAggressive, LongPingAggressive }
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
    quotingEwmaProtection?: boolean;
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
    quotingEwmaProtectionPeriods?: number;
    /* **************************** */
    quotingEwmaSMUProtection?: boolean;
    quotingEwmaSMUThreshold?: number;
    quotingEwmaSMPeriods?: number;
    quotingEwmaSUPeriods?: number;

    flipBidSizesOnDowntrend?: boolean;
    blockBidsOnUptrend?: boolean;
    blockAsksOnDowntrend?: boolean;
    blockDowntrend?: boolean;
    blockUptrend?: boolean;
    reducePDiv?: boolean;
    reducePDivFactor?: number;
    increaseBidSzOnUptrend?: boolean;
    increaseBidSzOnUptrendFactor?: number;

    keepHighs?: boolean;
    highsFactor?: number;
    glueToSMU?: boolean;
    glueToSMUFactor?: number;
    endOfBlockDowntrend?: boolean;
    endOfBlockDowntrendThreshold?: number;
    /* **************************** */
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
    constructor(public exchange: Exchange, public pair: CurrencyPair, public environment: string, public matryoshka: string, public homepage: string, public minTick: number) { }
}

export class ApplicationState {
    constructor(public memory: number, public hour: number, public freq: number, public dbsize: number) { }
}

export class TradeSafety {
    constructor(
      public buy: number,
      public sell: number,
      public combined: number,
      public buyPing: number,
      public sellPong: number
    ) {}
}

export class TargetBasePositionValue {
    constructor(
      public tbp: number,
      public sideAPR: string,
      public pDiv: number
    ) {}
}
