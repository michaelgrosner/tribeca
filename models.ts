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
}

interface IGateway {
    MarketData : Evt<MarketBook>;
    ConnectChanged : Evt<ConnectivityStatus>;
    name() : string;
    makeFee() : number;
    takeFee() : number;
    sendOrder(order : BrokeredOrder);
}

interface IBroker {
    MarketData : Evt<MarketBook>;
    name() : string;
    currentBook() : MarketBook;
    makeFee() : number;
    takeFee() : number;
    sendOrder(order : Order);
}

class ExchangeBroker implements IBroker {
    _activeOrders : { [orderId: string]: BrokeredOrder } = {};
    sendOrder(order : Order) {
        var brokeredOrder : BrokeredOrder = {
            orderId: new Date().getTime().toString(32),
            side: order.side,
            quantity: order.quantity,
            type: order.type,
            price: order.price,
            timeInForce: order.timeInForce};
        this._activeOrders[brokeredOrder.orderId] = brokeredOrder;
        this._gateway.sendOrder(brokeredOrder);
    }

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
    }
}

class Agent {
    _brokers : Array<IBroker>;

    constructor(brokers : Array<IBroker>) {
        this._brokers = brokers;
        this._brokers.forEach(b => {
            b.MarketData.on(this.onNewMarketData)
        });
    }

    private onNewMarketData = () => {
        var activeBrokers = this._brokers.filter(b => b.currentBook() != null);

        if (activeBrokers.length <= 1)
            return;

        var results = [];
        activeBrokers.forEach(b1 => {
            activeBrokers.forEach(b2 => {
                // dont even bother if makeFee > 0
                if (b1.makeFee() > 0) return;

                var b1Top = b1.currentBook().top;
                var b2Top = b2.currentBook().top;

                // TODO: verify formulae
                var pBid = - (1 + b1.makeFee()) * b1Top.bidPrice + (1 - b2.takeFee()) * b2Top.bidPrice;
                var pAsk = (1 + b1.makeFee()) * b1Top.askPrice + (1 - b2.takeFee()) * b2Top.askPrice;

                if (pBid > 0)
                    results.push({restSide: Side.Bid, restBroker: b1, hideBroker: b2});

                if (pAsk > 0)
                    results.push({restSide: Side.Ask, restBroker: b1, hideBroker: b2});
            })
        });
    };
}