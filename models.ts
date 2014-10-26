/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />

var util = require("util");

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
        public time : Date) { }

    public equals(other : MarketUpdate) {
        return this.ask.equals(other.ask) && this.bid.equals(other.bid);
    }

    public inspect() {
        return util.inspect({bid: this.bid, ask: this.ask, time: this.time.toISOString()});
    }
}

enum Currency { USD, BTC }
enum ConnectivityStatus { Connected, Disconnected }
enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts }
enum Side { Bid, Ask }
enum OrderType { Limit, Market }
enum TimeInForce { IOC, FOK, GTC }
enum OrderStatus { New, Working, Complete, Rejected, Other }
enum Liquidity { Make, Take }

class MarketBook {
    constructor(public top: MarketUpdate, public second: MarketUpdate, public exchangeName: Exchange) { }

    public inspect() {
        return util.inspect({top: this.top, second: this.second, exchangeName: Exchange[this.exchangeName]});
    }
}

interface Order {
    side : Side;
    quantity : number;
    type : OrderType;
    price : number;
    timeInForce : TimeInForce;
}

class OrderImpl implements Order {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce) {}

    public inspect()  {
        return util.format("side=%s; quantity=%d; type=%d; price=%d, tif=%s", Side[this.side], this.quantity,
            OrderType[this.type], this.price, TimeInForce[this.timeInForce]);
    }
}

class SubmitNewOrder implements Order {
    constructor(
        public side : Side,
        public quantity : number,
        public type : OrderType,
        public price : number,
        public timeInForce : TimeInForce,
        public exchange : Exchange) {}

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
        public exchange : Exchange) {}

    public inspect()  {
        return util.format("orig=%s; quantity=%d; price=%d, exch=%s",
            this.origOrderId, this.quantity, this.price, Exchange[this.exchange]);
    }
}

class OrderCancel {
    constructor(
        public origOrderId : string,
        public exchange : Exchange) {}

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

    partiallyFilled? : boolean;
    pendingCancel? : boolean;
    pendingReplace? : boolean;
    cancelRejected? : boolean;
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
    MarketData : Evt<MarketBook>;
}

interface IOrderEntryGateway extends IGateway {
    sendOrder(order : BrokeredOrder);
    cancelOrder(cancel : BrokeredCancel);
    replaceOrder(replace : BrokeredReplace);
    OrderUpdate : Evt<OrderStatusReport>;
}

class CurrencyPosition {
    constructor(public amount : number, public currency : Currency) { }
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
    MarketData : Evt<MarketBook>;
    name() : string;
    currentBook() : MarketBook;
    makeFee() : number;
    takeFee() : number;
    exchange() : Exchange;
    sendOrder(order : Order) : SentOrder;
    cancelOrder(cancel : OrderCancel);
    replaceOrder(replace : CancelReplaceOrder) : SentOrder;
    OrderUpdate : Evt<OrderStatusReport>;
    allOrderStates() : Array<OrderStatusReport>;
    cancelOpenOrders() : void;
    getPosition(currency : Currency) : CurrencyPosition;
}

class Result {
    constructor(public restSide: Side, public restBroker: IBroker,
                public hideBroker: IBroker, public profit: number,
                public rest: MarketSide, public hide: MarketSide,
                public size: number) {}

    public toJSON() {
        return {side: this.restSide, size: this.size, profit: this.profit,
            restExch: this.restBroker.exchange(), hideExch: this.hideBroker.exchange(),
            restMkt: this.rest, hide: this.hide};
    }
}