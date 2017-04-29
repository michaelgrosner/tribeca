"use strict";
exports.__esModule = true;
var Timestamped = (function () {
    function Timestamped(data, time) {
        this.data = data;
        this.time = time;
    }
    Timestamped.prototype.toString = function () {
        return "time=" + (this.time === null ? null : this.time.format('D/M HH:mm:ss,SSS')) + ";data=" + this.data;
    };
    return Timestamped;
}());
exports.Timestamped = Timestamped;
exports.Prefixes = {
    SUBSCRIBE: '_',
    SNAPSHOT: '=',
    MESSAGE: '-',
    DELAYED: '.'
};
exports.Topics = {
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
    TradesChart: 'A',
    WalletChart: 'B',
    EWMAChart: 'C'
};
var MarketSide = (function () {
    function MarketSide(price, size) {
        this.price = price;
        this.size = size;
    }
    MarketSide.prototype.toString = function () {
        return "px=" + this.price + ";size=" + this.size;
    };
    return MarketSide;
}());
exports.MarketSide = MarketSide;
var GatewayMarketTrade = (function () {
    function GatewayMarketTrade(price, size, time, onStartup, make_side) {
        this.price = price;
        this.size = size;
        this.time = time;
        this.onStartup = onStartup;
        this.make_side = make_side;
    }
    return GatewayMarketTrade;
}());
exports.GatewayMarketTrade = GatewayMarketTrade;
function marketSideEquals(t, other, tol) {
    tol = tol || 1e-4;
    if (other == null)
        return false;
    return Math.abs(t.price - other.price) > tol && Math.abs(t.size - other.size) > tol;
}
exports.marketSideEquals = marketSideEquals;
var Market = (function () {
    function Market(bids, asks, time) {
        this.bids = bids;
        this.asks = asks;
        this.time = time;
    }
    Market.prototype.toString = function () {
        return "asks: [" + this.asks.join(";") + "] bids: [" + this.bids.join(";") + "]";
    };
    return Market;
}());
exports.Market = Market;
var MarketTrade = (function () {
    function MarketTrade(exchange, pair, price, size, time, quote, bid, ask, make_side) {
        this.exchange = exchange;
        this.pair = pair;
        this.price = price;
        this.size = size;
        this.time = time;
        this.quote = quote;
        this.bid = bid;
        this.ask = ask;
        this.make_side = make_side;
    }
    return MarketTrade;
}());
exports.MarketTrade = MarketTrade;
var Currency;
(function (Currency) {
    Currency[Currency["USD"] = 0] = "USD";
    Currency[Currency["BTC"] = 1] = "BTC";
    Currency[Currency["LTC"] = 2] = "LTC";
    Currency[Currency["EUR"] = 3] = "EUR";
    Currency[Currency["GBP"] = 4] = "GBP";
    Currency[Currency["CNY"] = 5] = "CNY";
    Currency[Currency["ETH"] = 6] = "ETH";
    Currency[Currency["BFX"] = 7] = "BFX";
    Currency[Currency["RRT"] = 8] = "RRT";
    Currency[Currency["ZEC"] = 9] = "ZEC";
    Currency[Currency["BCN"] = 10] = "BCN";
    Currency[Currency["DASH"] = 11] = "DASH";
    Currency[Currency["DOGE"] = 12] = "DOGE";
    Currency[Currency["DSH"] = 13] = "DSH";
    Currency[Currency["EMC"] = 14] = "EMC";
    Currency[Currency["FCN"] = 15] = "FCN";
    Currency[Currency["LSK"] = 16] = "LSK";
    Currency[Currency["NXT"] = 17] = "NXT";
    Currency[Currency["QCN"] = 18] = "QCN";
    Currency[Currency["SDB"] = 19] = "SDB";
    Currency[Currency["SCB"] = 20] = "SCB";
    Currency[Currency["STEEM"] = 21] = "STEEM";
    Currency[Currency["XDN"] = 22] = "XDN";
    Currency[Currency["XEM"] = 23] = "XEM";
    Currency[Currency["XMR"] = 24] = "XMR";
    Currency[Currency["ARDR"] = 25] = "ARDR";
    Currency[Currency["WAVES"] = 26] = "WAVES";
    Currency[Currency["BTU"] = 27] = "BTU";
    Currency[Currency["MAID"] = 28] = "MAID";
    Currency[Currency["AMP"] = 29] = "AMP";
})(Currency = exports.Currency || (exports.Currency = {}));
function toCurrency(c) {
    return Currency[c.toUpperCase()];
}
exports.toCurrency = toCurrency;
function fromCurrency(c) {
    var t = Currency[c];
    if (t)
        return t.toUpperCase();
    return undefined;
}
exports.fromCurrency = fromCurrency;
var GatewayType;
(function (GatewayType) {
    GatewayType[GatewayType["MarketData"] = 0] = "MarketData";
    GatewayType[GatewayType["OrderEntry"] = 1] = "OrderEntry";
    GatewayType[GatewayType["Position"] = 2] = "Position";
})(GatewayType = exports.GatewayType || (exports.GatewayType = {}));
var ConnectivityStatus;
(function (ConnectivityStatus) {
    ConnectivityStatus[ConnectivityStatus["Connected"] = 0] = "Connected";
    ConnectivityStatus[ConnectivityStatus["Disconnected"] = 1] = "Disconnected";
})(ConnectivityStatus = exports.ConnectivityStatus || (exports.ConnectivityStatus = {}));
var Exchange;
(function (Exchange) {
    Exchange[Exchange["Null"] = 0] = "Null";
    Exchange[Exchange["HitBtc"] = 1] = "HitBtc";
    Exchange[Exchange["OkCoin"] = 2] = "OkCoin";
    Exchange[Exchange["AtlasAts"] = 3] = "AtlasAts";
    Exchange[Exchange["BtcChina"] = 4] = "BtcChina";
    Exchange[Exchange["Coinbase"] = 5] = "Coinbase";
    Exchange[Exchange["Bitfinex"] = 6] = "Bitfinex";
})(Exchange = exports.Exchange || (exports.Exchange = {}));
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
    TimeInForce[TimeInForce["IOC"] = 0] = "IOC";
    TimeInForce[TimeInForce["FOK"] = 1] = "FOK";
    TimeInForce[TimeInForce["GTC"] = 2] = "GTC";
})(TimeInForce = exports.TimeInForce || (exports.TimeInForce = {}));
var OrderStatus;
(function (OrderStatus) {
    OrderStatus[OrderStatus["New"] = 0] = "New";
    OrderStatus[OrderStatus["Working"] = 1] = "Working";
    OrderStatus[OrderStatus["Complete"] = 2] = "Complete";
    OrderStatus[OrderStatus["Cancelled"] = 3] = "Cancelled";
    OrderStatus[OrderStatus["Rejected"] = 4] = "Rejected";
    OrderStatus[OrderStatus["Other"] = 5] = "Other";
})(OrderStatus = exports.OrderStatus || (exports.OrderStatus = {}));
var Liquidity;
(function (Liquidity) {
    Liquidity[Liquidity["Make"] = 0] = "Make";
    Liquidity[Liquidity["Take"] = 1] = "Take";
})(Liquidity = exports.Liquidity || (exports.Liquidity = {}));
exports.orderIsDone = function (status) {
    switch (status) {
        case OrderStatus.Complete:
        case OrderStatus.Cancelled:
        case OrderStatus.Rejected:
            return true;
        default:
            return false;
    }
};
var SubmitNewOrder = (function () {
    function SubmitNewOrder(side, quantity, type, price, timeInForce, exchange, generatedTime, preferPostOnly, msg) {
        this.side = side;
        this.quantity = quantity;
        this.type = type;
        this.price = price;
        this.timeInForce = timeInForce;
        this.exchange = exchange;
        this.generatedTime = generatedTime;
        this.preferPostOnly = preferPostOnly;
        this.msg = msg;
        this.msg = msg || null;
    }
    return SubmitNewOrder;
}());
exports.SubmitNewOrder = SubmitNewOrder;
var CancelReplaceOrder = (function () {
    function CancelReplaceOrder(origOrderId, quantity, price, exchange, generatedTime) {
        this.origOrderId = origOrderId;
        this.quantity = quantity;
        this.price = price;
        this.exchange = exchange;
        this.generatedTime = generatedTime;
    }
    return CancelReplaceOrder;
}());
exports.CancelReplaceOrder = CancelReplaceOrder;
var OrderCancel = (function () {
    function OrderCancel(origOrderId, exchange, generatedTime) {
        this.origOrderId = origOrderId;
        this.exchange = exchange;
        this.generatedTime = generatedTime;
    }
    return OrderCancel;
}());
exports.OrderCancel = OrderCancel;
var SentOrder = (function () {
    function SentOrder(sentOrderClientId) {
        this.sentOrderClientId = sentOrderClientId;
    }
    return SentOrder;
}());
exports.SentOrder = SentOrder;
var EWMAChart = (function () {
    function EWMAChart(ewmaQuote, ewmaShort, ewmaLong, fairValue, time) {
        this.ewmaQuote = ewmaQuote;
        this.ewmaShort = ewmaShort;
        this.ewmaLong = ewmaLong;
        this.fairValue = fairValue;
        this.time = time;
    }
    return EWMAChart;
}());
exports.EWMAChart = EWMAChart;
var TradeChart = (function () {
    function TradeChart(price, side, quantity, value, type, time) {
        this.price = price;
        this.side = side;
        this.quantity = quantity;
        this.value = value;
        this.type = type;
        this.time = time;
    }
    return TradeChart;
}());
exports.TradeChart = TradeChart;
var Trade = (function () {
    function Trade(tradeId, time, exchange, pair, price, quantity, side, value, liquidity, Ktime, Kqty, Kprice, Kvalue, Kdiff, feeCharged, loadedFromDB) {
        this.tradeId = tradeId;
        this.time = time;
        this.exchange = exchange;
        this.pair = pair;
        this.price = price;
        this.quantity = quantity;
        this.side = side;
        this.value = value;
        this.liquidity = liquidity;
        this.Ktime = Ktime;
        this.Kqty = Kqty;
        this.Kprice = Kprice;
        this.Kvalue = Kvalue;
        this.Kdiff = Kdiff;
        this.feeCharged = feeCharged;
        this.loadedFromDB = loadedFromDB;
    }
    return Trade;
}());
exports.Trade = Trade;
var CurrencyPosition = (function () {
    function CurrencyPosition(amount, heldAmount, currency) {
        this.amount = amount;
        this.heldAmount = heldAmount;
        this.currency = currency;
    }
    CurrencyPosition.prototype.toString = function () {
        return "currency=" + Currency[this.currency] + ";amount=" + this.amount;
    };
    return CurrencyPosition;
}());
exports.CurrencyPosition = CurrencyPosition;
var PositionReport = (function () {
    function PositionReport(baseAmount, quoteAmount, baseHeldAmount, quoteHeldAmount, value, quoteValue, pair, exchange, time) {
        this.baseAmount = baseAmount;
        this.quoteAmount = quoteAmount;
        this.baseHeldAmount = baseHeldAmount;
        this.quoteHeldAmount = quoteHeldAmount;
        this.value = value;
        this.quoteValue = quoteValue;
        this.pair = pair;
        this.exchange = exchange;
        this.time = time;
    }
    return PositionReport;
}());
exports.PositionReport = PositionReport;
var OrderRequestFromUI = (function () {
    function OrderRequestFromUI(side, price, quantity, timeInForce, orderType) {
        this.side = side;
        this.price = price;
        this.quantity = quantity;
        this.timeInForce = timeInForce;
        this.orderType = orderType;
    }
    return OrderRequestFromUI;
}());
exports.OrderRequestFromUI = OrderRequestFromUI;
var FairValue = (function () {
    function FairValue(price, time) {
        this.price = price;
        this.time = time;
    }
    return FairValue;
}());
exports.FairValue = FairValue;
var QuoteAction;
(function (QuoteAction) {
    QuoteAction[QuoteAction["New"] = 0] = "New";
    QuoteAction[QuoteAction["Cancel"] = 1] = "Cancel";
})(QuoteAction = exports.QuoteAction || (exports.QuoteAction = {}));
var QuoteSent;
(function (QuoteSent) {
    QuoteSent[QuoteSent["First"] = 0] = "First";
    QuoteSent[QuoteSent["Modify"] = 1] = "Modify";
    QuoteSent[QuoteSent["UnsentDuplicate"] = 2] = "UnsentDuplicate";
    QuoteSent[QuoteSent["Delete"] = 3] = "Delete";
    QuoteSent[QuoteSent["UnsentDelete"] = 4] = "UnsentDelete";
    QuoteSent[QuoteSent["UnableToSend"] = 5] = "UnableToSend";
})(QuoteSent = exports.QuoteSent || (exports.QuoteSent = {}));
var Quote = (function () {
    function Quote(price, size) {
        this.price = price;
        this.size = size;
        this.Tol = 1e-3;
    }
    Quote.prototype.equals = function (other) {
        return Math.abs(this.price - other.price) < this.Tol && Math.abs(this.size - other.size) < this.Tol;
    };
    return Quote;
}());
exports.Quote = Quote;
var TwoSidedQuote = (function () {
    function TwoSidedQuote(bid, ask, time) {
        this.bid = bid;
        this.ask = ask;
        this.time = time;
    }
    return TwoSidedQuote;
}());
exports.TwoSidedQuote = TwoSidedQuote;
var QuoteStatus;
(function (QuoteStatus) {
    QuoteStatus[QuoteStatus["Live"] = 0] = "Live";
    QuoteStatus[QuoteStatus["Held"] = 1] = "Held";
})(QuoteStatus = exports.QuoteStatus || (exports.QuoteStatus = {}));
var TwoSidedQuoteStatus = (function () {
    function TwoSidedQuoteStatus(bidStatus, askStatus) {
        this.bidStatus = bidStatus;
        this.askStatus = askStatus;
    }
    return TwoSidedQuoteStatus;
}());
exports.TwoSidedQuoteStatus = TwoSidedQuoteStatus;
var CurrencyPair = (function () {
    function CurrencyPair(base, quote) {
        this.base = base;
        this.quote = quote;
    }
    CurrencyPair.prototype.toString = function () {
        return Currency[this.base] + "/" + Currency[this.quote];
    };
    return CurrencyPair;
}());
exports.CurrencyPair = CurrencyPair;
function currencyPairEqual(a, b) {
    return a.base === b.base && a.quote === b.quote;
}
exports.currencyPairEqual = currencyPairEqual;
var QuotingMode;
(function (QuotingMode) {
    QuotingMode[QuotingMode["Top"] = 0] = "Top";
    QuotingMode[QuotingMode["Mid"] = 1] = "Mid";
    QuotingMode[QuotingMode["Join"] = 2] = "Join";
    QuotingMode[QuotingMode["InverseJoin"] = 3] = "InverseJoin";
    QuotingMode[QuotingMode["InverseTop"] = 4] = "InverseTop";
    QuotingMode[QuotingMode["PingPong"] = 5] = "PingPong";
    QuotingMode[QuotingMode["Boomerang"] = 6] = "Boomerang";
    QuotingMode[QuotingMode["AK47"] = 7] = "AK47";
})(QuotingMode = exports.QuotingMode || (exports.QuotingMode = {}));
var FairValueModel;
(function (FairValueModel) {
    FairValueModel[FairValueModel["BBO"] = 0] = "BBO";
    FairValueModel[FairValueModel["wBBO"] = 1] = "wBBO";
})(FairValueModel = exports.FairValueModel || (exports.FairValueModel = {}));
var AutoPositionMode;
(function (AutoPositionMode) {
    AutoPositionMode[AutoPositionMode["Off"] = 0] = "Off";
    AutoPositionMode[AutoPositionMode["EwmaBasic"] = 1] = "EwmaBasic";
})(AutoPositionMode = exports.AutoPositionMode || (exports.AutoPositionMode = {}));
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
    PongAt[PongAt["LongPingFair"] = 1] = "LongPingFair";
    PongAt[PongAt["ShortPingAggressive"] = 2] = "ShortPingAggressive";
    PongAt[PongAt["LongPingAggressive"] = 3] = "LongPingAggressive";
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
    SOP[SOP["x2trds"] = 1] = "x2trds";
    SOP[SOP["x3trds"] = 2] = "x3trds";
    SOP[SOP["x2Sz"] = 3] = "x2Sz";
    SOP[SOP["x3Sz"] = 4] = "x3Sz";
    SOP[SOP["x2trdsSz"] = 5] = "x2trdsSz";
    SOP[SOP["x3trdsSz"] = 6] = "x3trdsSz";
})(SOP = exports.SOP || (exports.SOP = {}));
var ExchangePairMessage = (function () {
    function ExchangePairMessage(exchange, pair, data) {
        this.exchange = exchange;
        this.pair = pair;
        this.data = data;
    }
    return ExchangePairMessage;
}());
exports.ExchangePairMessage = ExchangePairMessage;
var ProductAdvertisement = (function () {
    function ProductAdvertisement(exchange, pair, environment, minTick) {
        this.exchange = exchange;
        this.pair = pair;
        this.environment = environment;
        this.minTick = minTick;
    }
    return ProductAdvertisement;
}());
exports.ProductAdvertisement = ProductAdvertisement;
var ApplicationState = (function () {
    function ApplicationState(memory, hour, freq, dbsize) {
        this.memory = memory;
        this.hour = hour;
        this.freq = freq;
        this.dbsize = dbsize;
    }
    return ApplicationState;
}());
exports.ApplicationState = ApplicationState;
var RegularFairValue = (function () {
    function RegularFairValue(time, value) {
        this.time = time;
        this.value = value;
    }
    return RegularFairValue;
}());
exports.RegularFairValue = RegularFairValue;
var TradeSafety = (function () {
    function TradeSafety(buy, sell, combined, buyPing, sellPong, time) {
        this.buy = buy;
        this.sell = sell;
        this.combined = combined;
        this.buyPing = buyPing;
        this.sellPong = sellPong;
        this.time = time;
    }
    return TradeSafety;
}());
exports.TradeSafety = TradeSafety;
var TargetBasePositionValue = (function () {
    function TargetBasePositionValue(data, sideAPR, time) {
        this.data = data;
        this.sideAPR = sideAPR;
        this.time = time;
    }
    return TargetBasePositionValue;
}());
exports.TargetBasePositionValue = TargetBasePositionValue;
