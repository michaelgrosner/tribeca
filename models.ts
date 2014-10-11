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
enum Exchange { Coinsetter, HitBtc, OkCoin }
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
            timeInForce: orig.timeInForce
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
        if (!this._currentBook !== null || !this.marketUpdatesEqual(book.top, this._currentBook.top) || !this.marketUpdatesEqual(book.second, this._currentBook.second)) {
            this._currentBook = book;
            this.MarketData.trigger(book);
            this._log(book);
        }
    };

    public onConnect = (cs : ConnectivityStatus) => {
        this._log("Connection status changed ", ConnectivityStatus[cs]);
    };

    constructor(gateway : IGateway) {
        this._log = log("ExchangeBroker:" + gateway.name());
        this._gateway = gateway;
        this._gateway.MarketData.on(this.handleMarketData);
        this._gateway.ConnectChanged.on(this.onConnect);
        this._gateway.OrderUpdate.on(this.onOrderUpdate);
    }
}

class Agent {
    _brokers : Array<IBroker>;
    _log : Logger = log("Agent");
    _ui : UI;

    constructor(brokers : Array<IBroker>, ui : UI) {
        this._brokers = brokers;
        this._brokers.forEach(b => {
            b.MarketData.on(this.onNewMarketData)
        });
        this._ui = ui;
    }

    private onNewMarketData = (book : MarketBook) => {
        var activeBrokers = this._brokers.filter(b => b.currentBook() != null);

        if (activeBrokers.length <= 1)
            return;

        var results = [];
        activeBrokers.filter(b => b.makeFee() < 0)
            .forEach(restBroker => {
                activeBrokers.forEach(hideBroker => {
                    // optimize?
                    if (restBroker.name() == hideBroker.name()) return;

                    // need to determine whether or not I'm already on the market
                    var restTop = restBroker.currentBook().top;
                    var hideTop = hideBroker.currentBook().top;

                    // TODO: verify formulae
                    var pBid = -(1 + restBroker.makeFee()) * restTop.bidPrice + (1 + hideBroker.takeFee()) * hideTop.bidPrice;
                    var pAsk = +(1 + restBroker.makeFee()) * restTop.askPrice - (1 + hideBroker.takeFee()) * hideTop.askPrice;

                    if (pBid > 0) {
                        var p = Math.min(restTop.bidSize, hideTop.bidSize);
                        results.push({restSide: Side.Bid, restBroker: restBroker, hideBroker: hideBroker, profit: pBid * p});
                    }

                    if (pAsk > 0) {
                        var p = Math.min(restTop.askSize, hideTop.askSize);
                        results.push({restSide: Side.Ask, restBroker: restBroker, hideBroker: hideBroker, profit: pAsk * p});
                    }
                })
            });

        results.forEach(r => {
            var top2 = r.restBroker.currentBook().top[r.restSide == Side.Bid ? "bidPrice" : "askPrice"];
            var top3 = r.hideBroker.currentBook().top[r.restSide == Side.Bid ? "bidPrice" : "askPrice"];
            this._log("Trigger p=%d > %s Rest (%s) %j :: Hide (%s) %j", r.profit,
                Side[r.restSide], r.restBroker.name(), top2,
                r.hideBroker.name(), top3);
        });

        this._ui.sendUpdatedMarket(book);
    };
}