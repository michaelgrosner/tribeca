"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.TargetBasePositionValue = exports.TradeSafety = exports.ApplicationState = exports.ProductAdvertisement = exports.STDEV = exports.SOP = exports.APR = exports.PongAt = exports.PingAt = exports.DynamicPDivMode = exports.AutoPositionMode = exports.FairValueModel = exports.QuotingSafety = exports.OrderPctTotal = exports.QuotingMode = exports.TwoSidedQuoteStatus = exports.SideAPR = exports.QuoteStatus = exports.TwoSidedQuote = exports.Quote = exports.FairValue = exports.OrderRequestFromUI = exports.PositionReport = exports.Wallet = exports.Trade = exports.TradeChart = exports.MarketChart = exports.Liquidity = exports.OrderStatus = exports.TimeInForce = exports.OrderType = exports.Side = exports.Connectivity = exports.MarketTrade = exports.MarketStats = exports.Market = exports.MarketSide = exports.Topics = exports.Prefixes = void 0;
exports.Prefixes = {
    SNAPSHOT: '=',
    MESSAGE: '-'
};
exports.Topics = {
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
class MarketSide {
    constructor(price, size, cssMod) {
        this.price = price;
        this.size = size;
        this.cssMod = cssMod;
    }
}
exports.MarketSide = MarketSide;
class Market {
    constructor(bids, asks) {
        this.bids = bids;
        this.asks = asks;
    }
}
exports.Market = Market;
class MarketStats {
    constructor(fv, bid, ask, time) {
        this.fv = fv;
        this.bid = bid;
        this.ask = ask;
        this.time = time;
    }
}
exports.MarketStats = MarketStats;
class MarketTrade {
    constructor(price, quantity, time, side) {
        this.price = price;
        this.quantity = quantity;
        this.time = time;
        this.side = side;
    }
}
exports.MarketTrade = MarketTrade;
var Connectivity;
(function (Connectivity) {
    Connectivity[Connectivity["Disconnected"] = 0] = "Disconnected";
    Connectivity[Connectivity["Connected"] = 1] = "Connected";
})(Connectivity = exports.Connectivity || (exports.Connectivity = {}));
var Side;
(function (Side) {
    Side[Side["Bid"] = 0] = "Bid";
    Side[Side["Ask"] = 1] = "Ask";
    Side[Side["Unknown"] = 2] = "Unknown";
})(Side = exports.Side || (exports.Side = {}));
var OrderType;
(function (OrderType) {
    OrderType[OrderType["Limit"] = 0] = "Limit";
    OrderType[OrderType["Market"] = 1] = "Market";
})(OrderType = exports.OrderType || (exports.OrderType = {}));
var TimeInForce;
(function (TimeInForce) {
    TimeInForce[TimeInForce["GTC"] = 0] = "GTC";
    TimeInForce[TimeInForce["IOC"] = 1] = "IOC";
    TimeInForce[TimeInForce["FOK"] = 2] = "FOK";
})(TimeInForce = exports.TimeInForce || (exports.TimeInForce = {}));
var OrderStatus;
(function (OrderStatus) {
    OrderStatus[OrderStatus["Waiting"] = 0] = "Waiting";
    OrderStatus[OrderStatus["Working"] = 1] = "Working";
    OrderStatus[OrderStatus["Terminated"] = 2] = "Terminated";
})(OrderStatus = exports.OrderStatus || (exports.OrderStatus = {}));
var Liquidity;
(function (Liquidity) {
    Liquidity[Liquidity["Make"] = 0] = "Make";
    Liquidity[Liquidity["Take"] = 1] = "Take";
})(Liquidity = exports.Liquidity || (exports.Liquidity = {}));
class MarketChart {
    constructor(stdevWidth, ewma, fairValue, tradesBuySize, tradesSellSize) {
        this.stdevWidth = stdevWidth;
        this.ewma = ewma;
        this.fairValue = fairValue;
        this.tradesBuySize = tradesBuySize;
        this.tradesSellSize = tradesSellSize;
    }
}
exports.MarketChart = MarketChart;
class TradeChart {
    constructor(price, side, quantity, value, pong) {
        this.price = price;
        this.side = side;
        this.quantity = quantity;
        this.value = value;
        this.pong = pong;
    }
}
exports.TradeChart = TradeChart;
class Trade {
    constructor(tradeId, time, price, quantity, side, value, Ktime, Kqty, Kprice, Kvalue, delta, feeCharged, isPong, loadedFromDB) {
        this.tradeId = tradeId;
        this.time = time;
        this.price = price;
        this.quantity = quantity;
        this.side = side;
        this.value = value;
        this.Ktime = Ktime;
        this.Kqty = Kqty;
        this.Kprice = Kprice;
        this.Kvalue = Kvalue;
        this.delta = delta;
        this.feeCharged = feeCharged;
        this.isPong = isPong;
        this.loadedFromDB = loadedFromDB;
    }
}
exports.Trade = Trade;
class Wallet {
    constructor(amount, held, value, profit) {
        this.amount = amount;
        this.held = held;
        this.value = value;
        this.profit = profit;
    }
}
exports.Wallet = Wallet;
class PositionReport {
    constructor(base, quote) {
        this.base = base;
        this.quote = quote;
    }
}
exports.PositionReport = PositionReport;
class OrderRequestFromUI {
    constructor(side, price, quantity, timeInForce, type) {
        this.side = side;
        this.price = price;
        this.quantity = quantity;
        this.timeInForce = timeInForce;
        this.type = type;
    }
}
exports.OrderRequestFromUI = OrderRequestFromUI;
class FairValue {
    constructor(price) {
        this.price = price;
    }
}
exports.FairValue = FairValue;
class Quote {
    constructor(price, size, isPong) {
        this.price = price;
        this.size = size;
        this.isPong = isPong;
    }
}
exports.Quote = Quote;
class TwoSidedQuote {
    constructor(bid, ask) {
        this.bid = bid;
        this.ask = ask;
    }
}
exports.TwoSidedQuote = TwoSidedQuote;
var QuoteStatus;
(function (QuoteStatus) {
    QuoteStatus[QuoteStatus["Disconnected"] = 0] = "Disconnected";
    QuoteStatus[QuoteStatus["Live"] = 1] = "Live";
    QuoteStatus[QuoteStatus["DisabledQuotes"] = 2] = "DisabledQuotes";
    QuoteStatus[QuoteStatus["MissingData"] = 3] = "MissingData";
    QuoteStatus[QuoteStatus["UnknownHeld"] = 4] = "UnknownHeld";
    QuoteStatus[QuoteStatus["WidthTooHigh"] = 5] = "WidthTooHigh";
    QuoteStatus[QuoteStatus["TBPHeld"] = 6] = "TBPHeld";
    QuoteStatus[QuoteStatus["MaxTradesSeconds"] = 7] = "MaxTradesSeconds";
    QuoteStatus[QuoteStatus["WaitingPing"] = 8] = "WaitingPing";
    QuoteStatus[QuoteStatus["DepletedFunds"] = 9] = "DepletedFunds";
    QuoteStatus[QuoteStatus["Crossed"] = 10] = "Crossed";
    QuoteStatus[QuoteStatus["UpTrendHeld"] = 11] = "UpTrendHeld";
    QuoteStatus[QuoteStatus["DownTrendHeld"] = 12] = "DownTrendHeld";
})(QuoteStatus = exports.QuoteStatus || (exports.QuoteStatus = {}));
var SideAPR;
(function (SideAPR) {
    SideAPR[SideAPR["Off"] = 0] = "Off";
    SideAPR[SideAPR["Buy"] = 1] = "Buy";
    SideAPR[SideAPR["Sell"] = 2] = "Sell";
})(SideAPR = exports.SideAPR || (exports.SideAPR = {}));
class TwoSidedQuoteStatus {
    constructor(bidStatus, askStatus, sideAPR, quotesInMemoryWaiting, quotesInMemoryWorking, quotesInMemoryZombies) {
        this.bidStatus = bidStatus;
        this.askStatus = askStatus;
        this.sideAPR = sideAPR;
        this.quotesInMemoryWaiting = quotesInMemoryWaiting;
        this.quotesInMemoryWorking = quotesInMemoryWorking;
        this.quotesInMemoryZombies = quotesInMemoryZombies;
    }
}
exports.TwoSidedQuoteStatus = TwoSidedQuoteStatus;
var QuotingMode;
(function (QuotingMode) {
    QuotingMode[QuotingMode["Top"] = 0] = "Top";
    QuotingMode[QuotingMode["Mid"] = 1] = "Mid";
    QuotingMode[QuotingMode["Join"] = 2] = "Join";
    QuotingMode[QuotingMode["InverseJoin"] = 3] = "InverseJoin";
    QuotingMode[QuotingMode["InverseTop"] = 4] = "InverseTop";
    QuotingMode[QuotingMode["HamelinRat"] = 5] = "HamelinRat";
    QuotingMode[QuotingMode["Depth"] = 6] = "Depth";
})(QuotingMode = exports.QuotingMode || (exports.QuotingMode = {}));
var OrderPctTotal;
(function (OrderPctTotal) {
    OrderPctTotal[OrderPctTotal["Value"] = 0] = "Value";
    OrderPctTotal[OrderPctTotal["Side"] = 1] = "Side";
    OrderPctTotal[OrderPctTotal["TBPValue"] = 2] = "TBPValue";
    OrderPctTotal[OrderPctTotal["TBPSide"] = 3] = "TBPSide";
})(OrderPctTotal = exports.OrderPctTotal || (exports.OrderPctTotal = {}));
var QuotingSafety;
(function (QuotingSafety) {
    QuotingSafety[QuotingSafety["Off"] = 0] = "Off";
    QuotingSafety[QuotingSafety["PingPong"] = 1] = "PingPong";
    QuotingSafety[QuotingSafety["PingPoing"] = 2] = "PingPoing";
    QuotingSafety[QuotingSafety["Boomerang"] = 3] = "Boomerang";
    QuotingSafety[QuotingSafety["AK47"] = 4] = "AK47";
})(QuotingSafety = exports.QuotingSafety || (exports.QuotingSafety = {}));
var FairValueModel;
(function (FairValueModel) {
    FairValueModel[FairValueModel["BBO"] = 0] = "BBO";
    FairValueModel[FairValueModel["wBBO"] = 1] = "wBBO";
    FairValueModel[FairValueModel["rwBBO"] = 2] = "rwBBO";
})(FairValueModel = exports.FairValueModel || (exports.FairValueModel = {}));
var AutoPositionMode;
(function (AutoPositionMode) {
    AutoPositionMode[AutoPositionMode["Manual"] = 0] = "Manual";
    AutoPositionMode[AutoPositionMode["EWMA_LS"] = 1] = "EWMA_LS";
    AutoPositionMode[AutoPositionMode["EWMA_LMS"] = 2] = "EWMA_LMS";
    AutoPositionMode[AutoPositionMode["EWMA_4"] = 3] = "EWMA_4";
})(AutoPositionMode = exports.AutoPositionMode || (exports.AutoPositionMode = {}));
var DynamicPDivMode;
(function (DynamicPDivMode) {
    DynamicPDivMode[DynamicPDivMode["Manual"] = 0] = "Manual";
    DynamicPDivMode[DynamicPDivMode["Linear"] = 1] = "Linear";
    DynamicPDivMode[DynamicPDivMode["Sine"] = 2] = "Sine";
    DynamicPDivMode[DynamicPDivMode["SQRT"] = 3] = "SQRT";
    DynamicPDivMode[DynamicPDivMode["Switch"] = 4] = "Switch";
})(DynamicPDivMode = exports.DynamicPDivMode || (exports.DynamicPDivMode = {}));
var PingAt;
(function (PingAt) {
    PingAt[PingAt["BothSides"] = 0] = "BothSides";
    PingAt[PingAt["BidSide"] = 1] = "BidSide";
    PingAt[PingAt["AskSide"] = 2] = "AskSide";
    PingAt[PingAt["DepletedSide"] = 3] = "DepletedSide";
    PingAt[PingAt["DepletedBidSide"] = 4] = "DepletedBidSide";
    PingAt[PingAt["DepletedAskSide"] = 5] = "DepletedAskSide";
    PingAt[PingAt["StopPings"] = 6] = "StopPings";
})(PingAt = exports.PingAt || (exports.PingAt = {}));
var PongAt;
(function (PongAt) {
    PongAt[PongAt["ShortPingFair"] = 0] = "ShortPingFair";
    PongAt[PongAt["AveragePingFair"] = 1] = "AveragePingFair";
    PongAt[PongAt["LongPingFair"] = 2] = "LongPingFair";
    PongAt[PongAt["ShortPingAggressive"] = 3] = "ShortPingAggressive";
    PongAt[PongAt["AveragePingAggressive"] = 4] = "AveragePingAggressive";
    PongAt[PongAt["LongPingAggressive"] = 5] = "LongPingAggressive";
})(PongAt = exports.PongAt || (exports.PongAt = {}));
var APR;
(function (APR) {
    APR[APR["Off"] = 0] = "Off";
    APR[APR["Size"] = 1] = "Size";
    APR[APR["SizeWidth"] = 2] = "SizeWidth";
})(APR = exports.APR || (exports.APR = {}));
var SOP;
(function (SOP) {
    SOP[SOP["Off"] = 0] = "Off";
    SOP[SOP["Trades"] = 1] = "Trades";
    SOP[SOP["Size"] = 2] = "Size";
    SOP[SOP["TradesSize"] = 3] = "TradesSize";
})(SOP = exports.SOP || (exports.SOP = {}));
var STDEV;
(function (STDEV) {
    STDEV[STDEV["Off"] = 0] = "Off";
    STDEV[STDEV["OnFV"] = 1] = "OnFV";
    STDEV[STDEV["OnFVAPROff"] = 2] = "OnFVAPROff";
    STDEV[STDEV["OnTops"] = 3] = "OnTops";
    STDEV[STDEV["OnTopsAPROff"] = 4] = "OnTopsAPROff";
    STDEV[STDEV["OnTop"] = 5] = "OnTop";
    STDEV[STDEV["OnTopAPROff"] = 6] = "OnTopAPROff";
})(STDEV = exports.STDEV || (exports.STDEV = {}));
class ProductAdvertisement {
    constructor(exchange, inet, base, quote, symbol, margin, webMarket, webOrders, environment, matryoshka, tickPrice, tickSize, stepPrice, stepSize, minSize) {
        this.exchange = exchange;
        this.inet = inet;
        this.base = base;
        this.quote = quote;
        this.symbol = symbol;
        this.margin = margin;
        this.webMarket = webMarket;
        this.webOrders = webOrders;
        this.environment = environment;
        this.matryoshka = matryoshka;
        this.tickPrice = tickPrice;
        this.tickSize = tickSize;
        this.stepPrice = stepPrice;
        this.stepSize = stepSize;
        this.minSize = minSize;
    }
}
exports.ProductAdvertisement = ProductAdvertisement;
class ApplicationState {
    constructor(addr, freq, theme, memory, dbsize) {
        this.addr = addr;
        this.freq = freq;
        this.theme = theme;
        this.memory = memory;
        this.dbsize = dbsize;
    }
}
exports.ApplicationState = ApplicationState;
class TradeSafety {
    constructor(buy, sell, combined, buyPing, sellPing) {
        this.buy = buy;
        this.sell = sell;
        this.combined = combined;
        this.buyPing = buyPing;
        this.sellPing = sellPing;
    }
}
exports.TradeSafety = TradeSafety;
class TargetBasePositionValue {
    constructor(tbp, pDiv) {
        this.tbp = tbp;
        this.pDiv = pDiv;
    }
}
exports.TargetBasePositionValue = TargetBasePositionValue;
