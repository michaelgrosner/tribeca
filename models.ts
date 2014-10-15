/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />

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
}

enum ConnectivityStatus { Connected, Disconnected }
enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts }
enum Side { Bid, Ask }
enum OrderType { Limit, Market }
enum TimeInForce { IOC, FOK, GTC }
enum OrderStatus { New, PendingCancel, Working, PartialFill, Filled, Cancelled, Rejected, Other }

class MarketBook {
    constructor(public top: MarketUpdate, public second: MarketUpdate, public exchangeName: Exchange) { }
}

class Order {
    side : Side;
    quantity : number;
    type : OrderType;
    price : number;
    timeInForce : TimeInForce;
}

interface SubmitNewOrder extends Order {
    exchange : Exchange;
}

interface BrokeredOrder extends Order {
    orderId : string;
}

interface GatewayOrderStatusReport {
    orderId : string;
    orderStatus : OrderStatus;
    rejectMessage? : string;
    time : Date;
    lastQuantity? : number;
    lastPrice? : number;
    leavesQuantity? : number;
    cumQuantity? : number;
    averagePrice? : number;
}

interface OrderStatusReport extends Order, GatewayOrderStatusReport {
    exchange : Exchange;
    message? : string;
}

interface OrderCancel {
    side : Side;
}

interface BrokeredCancel extends OrderCancel {
    clientOrderId : string;
    requestId : string;
}

interface IGateway {
    MarketData : Evt<MarketBook>;
    ConnectChanged : Evt<ConnectivityStatus>;
    name() : string;
    makeFee() : number;
    takeFee() : number;
    exchange() : Exchange;
    sendOrder(order : BrokeredOrder);
    cancelOrder(cancel : BrokeredCancel);
    OrderUpdate : Evt<GatewayOrderStatusReport>;
}

interface IBroker {
    MarketData : Evt<MarketBook>;
    name() : string;
    currentBook() : MarketBook;
    makeFee() : number;
    takeFee() : number;
    exchange() : Exchange;
    sendOrder(order : Order);
    OrderUpdate : Evt<OrderStatusReport>;
    allOrders() : Array<OrderStatusReport>;
}