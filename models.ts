/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />

var util = require("util");

class Timestamped<T> {
    constructor(public data : T, public time = date()) {}

    public inspect() {
        return util.inspect({time: this.time.toISOString(), data: this.data});
    }
}

class MarketSide {
    constructor(public price: number, public size: number) { }

    public equals(other : MarketSide) {
        return this.price == other.price && this.size == other.size;
    }
}

class MarketUpdate {
    constructor(
        public bid : MarketSide,
        public ask : MarketSide,
        public time : Moment,
        public exchange : Exchange) { }

    public equals(other : MarketUpdate) {
        if (other == null) return false;
        return this.ask.equals(other.ask) && this.bid.equals(other.bid) && this.exchange == other.exchange;
    }

    public inspect() {
        return util.inspect({bid: this.bid, ask: this.ask,
            time: this.time.toISOString(), exch: Exchange[this.exchange]}, {colors: true});
    }
}

enum GatewayType { MarketData, OrderEntry, Position }
enum Currency { USD, BTC, LTC }
enum ConnectivityStatus { Connected, Disconnected }
enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts }
enum Side { Bid, Ask }
enum OrderType { Limit, Market }
enum TimeInForce { IOC, FOK, GTC }
enum OrderStatus { New, Working, Complete, Cancelled, Rejected, Other }
enum Liquidity { Make, Take }

interface Order {
    side : Side;
    quantity : number;
    type : OrderType;
    price : number;
    timeInForce : TimeInForce;
    exchange : Exchange;
}

class SubmitNewOrder implements Order {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange,
        public generatedTime: Moment) {}

    public inspect()  {
        return util.format("side=%s; quantity=%d; type=%d; price=%d, tif=%s; exch=%s", Side[this.side], this.quantity,
            OrderType[this.type], this.price, TimeInForce[this.timeInForce], Exchange[this.exchange]);
    }
}

class CancelReplaceOrder {
    constructor(
        public origOrderId : string,
        public quantity : number,
        public price : number,
        public exchange : Exchange,
        public generatedTime : Moment) {}

    public inspect()  {
        return util.format("orig=%s; quantity=%d; price=%d, exch=%s",
            this.origOrderId, this.quantity, this.price, Exchange[this.exchange]);
    }
}

class OrderCancel {
    constructor(
        public origOrderId : string,
        public exchange : Exchange,
        public generatedTime : Moment) {}

    public inspect()  {
        return util.format("orig=%s; exch=%s", this.origOrderId, Exchange[this.exchange]);
    }
}

class BrokeredOrder implements Order {
    constructor(
        public orderId : string,
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange) {}

    public inspect()  {
        return util.format("orderId=%s; side=%s; quantity=%d; type=%d; price=%d, tif=%s; exch=%s", this.orderId,
            Side[this.side], this.quantity, OrderType[this.type], this.price,
            TimeInForce[this.timeInForce], Exchange[this.exchange]);
    }
}

class BrokeredReplace implements Order {
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

    public inspect()  {
        return util.format("origCliId=%s; origExchId=%s; orderId=%s; side=%s; quantity=%d; type=%d; price=%d, tif=%s; exch=%s",
            this.origOrderId, this.exchangeId, this.orderId,  Side[this.side], this.quantity,
            OrderType[this.type], this.price, TimeInForce[this.timeInForce], Exchange[this.exchange]);
    }
}

class BrokeredCancel {
    constructor(
        public clientOrderId : string,
        public requestId : string,
        public side : Side,
        public exchangeId : string) {}

    public inspect()  {
        return util.format("reqId=%s; origCliId=%s; origExchId=%s, side=%s",
            this.requestId, this.clientOrderId, this.exchangeId, Side[this.side]);
    }
}

class SentOrder {
    constructor(public sentOrderClientId : string) {}
}

class OrderGatewayActionReport {
    constructor(public sentTime : Moment) {}
}

interface OrderStatusReport {
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
    message? : string;
    computationalLatency? : number;
    version? : number;

    partiallyFilled? : boolean;
    pendingCancel? : boolean;
    pendingReplace? : boolean;
    cancelRejected? : boolean;
}

class OrderStatusReportImpl implements OrderStatusReport {
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
        public message : string,
        public computationalLatency : number,
        public version : number,
        public partiallyFilled : boolean,
        public pendingCancel : boolean,
        public pendingReplace : boolean,
        public cancelRejected : boolean) {}

    public inspect() {
        return util.inspect({side: Side[this.side], quantity: this.quantity, type: OrderType[this.type], price: this.price,
            timeInForce: TimeInForce[this.timeInForce], orderId: this.orderId, exchangeId: this.exchangeId,
            orderStatus: OrderStatus[this.orderStatus], rejectMessage: this.rejectMessage, time: this.time.toISOString(),
            lastQuantity: this.lastQuantity, lastPrice: this.lastPrice, leavesQuantity: this.leavesQuantity,
            cumQuantity: this.cumQuantity, averagePrice: this.averagePrice, liquidity: Liquidity[this.liquidity],
            exchange: Exchange[this.exchange], message: this.message, computationalLatency: this.computationalLatency,
            version: this.version, partiallyFilled: this.partiallyFilled, pendingCancel: this.pendingCancel,
            pendingReplace: this.pendingReplace, cancelRejected: this.cancelRejected}, {colors: true});
    }
}

interface IExchangeDetailsGateway {
    name() : string;
    makeFee() : number;
    takeFee() : number;
    exchange() : Exchange;
}

interface IGateway {
    ConnectChanged : Evt<ConnectivityStatus>;
}

interface IMarketDataGateway extends IGateway {
    MarketData : Evt<MarketUpdate>;
}

interface IOrderEntryGateway extends IGateway {
    sendOrder(order : BrokeredOrder) : OrderGatewayActionReport;
    cancelOrder(cancel : BrokeredCancel) : OrderGatewayActionReport;
    replaceOrder(replace : BrokeredReplace) : OrderGatewayActionReport;
    OrderUpdate : Evt<OrderStatusReport>;
}

class ExchangeCurrencyPosition {
    constructor(public amount : number,
                public currency : Currency,
                public exchange : Exchange) { }

    public inspect() {
        return util.inspect({amount: this.amount, currency: Currency[this.currency], exchange: Exchange[this.exchange]}, {colors: true});
    }
}

class CurrencyPosition {
    constructor(public amount : number,
                public currency : Currency) {}

    public inspect() {
        return util.inspect({amount: this.amount, currency: Currency[this.currency]}, {colors: true});
    }

    public toExchangeReport(exch : Exchange) {
        return new ExchangeCurrencyPosition(this.amount, this.currency, exch);
    }
}

interface IPositionGateway {
    PositionUpdate : Evt<CurrencyPosition>;
}

class CombinedGateway {
    constructor(
        public md : IMarketDataGateway,
        public oe : IOrderEntryGateway,
        public pg : IPositionGateway,
        public base : IExchangeDetailsGateway) { }
}

interface IBroker {
    MarketData : Evt<MarketUpdate>;

    name() : string;
    currentBook : MarketUpdate;
    makeFee() : number;
    takeFee() : number;
    exchange() : Exchange;

    sendOrder(order : SubmitNewOrder) : SentOrder;
    cancelOrder(cancel : OrderCancel);
    replaceOrder(replace : CancelReplaceOrder) : SentOrder;
    OrderUpdate : Evt<OrderStatusReport>;
    allOrderStates() : Array<OrderStatusReport>;
    cancelOpenOrders() : void;

    // todo: think about it, should fill reports inc/decrement positions? does it matter?
    getPosition(currency : Currency) : ExchangeCurrencyPosition;
    PositionUpdate : Evt<ExchangeCurrencyPosition>;

    connectStatus : ConnectivityStatus;
    ConnectChanged : Evt<ConnectivityStatus>;
}

class Result {
    constructor(public restSide: Side, public restBroker: IBroker,
                public hideBroker: IBroker, public profit: number,
                public rest: MarketSide, public hide: MarketSide,
                public size: number, public generatedTime: Moment) {}

    public toJSON() {
        return {side: this.restSide, size: this.size, profit: this.profit,
            restExch: this.restBroker.exchange(), hideExch: this.hideBroker.exchange(),
            restMkt: this.rest, hide: this.hide};
    }
}