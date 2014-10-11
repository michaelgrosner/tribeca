/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />

interface MarketUpdate {
    bidPrice : number;
    bidSize : number;
    askPrice : number;
    askSize : number;
    time : Date;
}

enum ConnectivityStatus { Connected, Disconnected }
enum Exchange { Coinsetter, HitBtc, OkCoin, AtlasAts }
enum Side { Bid, Ask }
enum OrderType { Limit, Market }
enum TimeInForce { IOC, FOK, GTC }
enum OrderStatus { New, PendingCancel, Working, PartialFill, Filled, Cancelled, Rejected, Other }

interface MarketBook {
    top : MarketUpdate;
    second : MarketUpdate;
    exchangeName : Exchange;
}

interface Order {
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
    status: OrderStatus
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
}

class ExchangeBroker implements IBroker {
    OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();
    _allOrders : { [orderId: string]: BrokeredOrder } = {};
    _activeOrder : BrokeredOrder;

    sendOrder = (order : Order) => {
        var brokeredOrder : BrokeredOrder = {
            orderId: new Date().getTime().toString(32),
            side: order.side,
            quantity: order.quantity,
            type: order.type,
            price: order.price,
            timeInForce: order.timeInForce,
            status: OrderStatus.New};
        this._allOrders[brokeredOrder.orderId] = brokeredOrder;
        this._gateway.sendOrder(brokeredOrder);
    };

    public onOrderUpdate = (osr : GatewayOrderStatusReport) => {
        var orig : Order = this._allOrders[osr.orderId];
        var o : OrderStatusReport = {
            orderId: osr.orderId,
            orderStatus: osr.orderStatus,
            rejectMessage: osr.rejectMessage,
            time: osr.time,
            lastQuantity: osr.lastQuantity,
            lastPrice: osr.lastPrice,
            leavesQuantity: osr.leavesQuantity,
            cumQuantity: osr.cumQuantity,
            averagePrice: osr.averagePrice,
            side: orig.side,
            quantity: orig.quantity,
            type: orig.type,
            price: orig.price,
            timeInForce: orig.timeInForce,
            exchange: this.exchange()
        };
        this.OrderUpdate.trigger(o);
    };

    makeFee() : number {
        return this._gateway.makeFee();
    }

    takeFee() : number {
        return this._gateway.takeFee();
    }

    MarketData : Evt<MarketBook> = new Evt();

    name() : string {
        return this._gateway.name();
    }

    exchange() : Exchange {
        return this._gateway.exchange();
    }

    _currentBook : MarketBook = null;
    _gateway : IGateway;
    _log : Logger;

    public currentBook = () : MarketBook => {
        return this._currentBook;
    };

    private marketUpdatesEqual = (update1 : MarketUpdate, update2 : MarketUpdate) : boolean => {
        return update1.askPrice == update2.askPrice &&
            update1.bidPrice == update2.bidPrice &&
            update1.askSize == update2.askSize &&
            update1.askPrice == update2.askPrice;
    };

    private handleMarketData = (book : MarketBook) => {
        if (this._currentBook == null || (!this.marketUpdatesEqual(book.top, this._currentBook.top) || !this.marketUpdatesEqual(book.second, this._currentBook.second))) {
            this._currentBook = book;
            this.MarketData.trigger(book);
            this._log(book);
        }
    };

    public onConnect = (cs : ConnectivityStatus) => {
        this._log("Connection status changed ", ConnectivityStatus[cs]);
    };

    constructor(gateway : IGateway) {
        this._log = log("Hudson:ExchangeBroker:" + gateway.name());
        this._gateway = gateway;
        this._gateway.MarketData.on(this.handleMarketData);
        this._gateway.ConnectChanged.on(this.onConnect);
        this._gateway.OrderUpdate.on(this.onOrderUpdate);
    }
}