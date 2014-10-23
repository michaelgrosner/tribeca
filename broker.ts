/// <reference path="utils.ts" />

class ExchangeBroker implements IBroker {
    cancelOpenOrders() : void {
        for (var k in this._allOrders) {
            if (!this._allOrders.hasOwnProperty(k)) continue;
            var e : OrderStatusReport = this._allOrders[k].last();

            switch (e.orderStatus) {
                case OrderStatus.New:
                case OrderStatus.Working:
                    this.cancelOrder(new OrderCancel(e.orderId, e.exchange));
                    break;
            }
        }
    }

    allOrderStates() : Array<OrderStatusReport> {
        var os : Array<OrderStatusReport> = [];
        for (var k in this._allOrders) {
            var e = this._allOrders[k];
            for (var i = 0; i < e.length; i++) {
                os.push(e[i]);
            }
        }
        return os;
    }

    OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();
    _allOrders : { [orderId: string]: OrderStatusReport[] } = {};

    private static generateOrderId = () => {
        return new Date().getTime().toString(32)
    };

    sendOrder = (order : Order) : SentOrder => {
        var rpt : OrderStatusReport = {
            orderId: ExchangeBroker.generateOrderId(),
            side: order.side,
            quantity: order.quantity,
            type: order.type,
            time: new Date(),
            price: order.price,
            timeInForce: order.timeInForce,
            orderStatus: OrderStatus.New,
            exchange: this.exchange()};
        this._log("sending order %o", rpt);
        this._allOrders[rpt.orderId] = [rpt];
        var brokeredOrder = new BrokeredOrder(rpt.orderId, rpt.side, rpt.quantity, rpt.type,
            rpt.price, rpt.timeInForce, rpt.exchange);
        this._oeGateway.sendOrder(brokeredOrder);
        this.onOrderUpdate(rpt);
        return new SentOrder(rpt.orderId);
    };

    replaceOrder = (replace : CancelReplaceOrder) : SentOrder => {
        var rpt = this._allOrders[replace.origOrderId].last();
        var br = new BrokeredReplace(replace.origOrderId, replace.origOrderId, rpt.side,
            replace.quantity, rpt.type, replace.price, rpt.timeInForce, rpt.exchange, rpt.exchangeId);
        this._log("cancel-replacing order %o %o", rpt, br);
        this._oeGateway.replaceOrder(br);

        var rpt : OrderStatusReport = {
            orderId: replace.origOrderId,
            orderStatus: OrderStatus.Working,
            pendingReplace: true,
            price: replace.price,
            quantity: replace.quantity,
            time: new Date()};
        this.onOrderUpdate(rpt);

        return new SentOrder(rpt.orderId);
    };

    cancelOrder = (cancel : OrderCancel) => {
        var rpt = this._allOrders[cancel.origOrderId].last();
        var cxl = new BrokeredCancel(cancel.origOrderId, ExchangeBroker.generateOrderId(), rpt.side, rpt.exchangeId);
        this._log("cancelling order %o with %o", rpt, cxl);

        var rpt : OrderStatusReport = {
            orderId: cancel.origOrderId,
            orderStatus: OrderStatus.Working,
            pendingCancel: true,
            time: new Date()};
        this.onOrderUpdate(rpt);

        this._oeGateway.cancelOrder(cxl);
    };

    public onOrderUpdate = (osr : OrderStatusReport) => {

        if (!this._allOrders.hasOwnProperty(osr.orderId)) {
            var keys = [];
            for (var k in this._allOrders)
                if (this._allOrders.hasOwnProperty(k))
                    keys.push(k);
            this._log("ERROR: cannot find orderId from %o, existing: %o", osr, keys);
        }

        var orig : OrderStatusReport = this._allOrders[osr.orderId].last();
        this._log("got gw update %o, applying to %o", osr, orig);

        var cumQuantity = osr.cumQuantity || orig.cumQuantity;
        var quantity = osr.quantity || orig.quantity;

        var o : OrderStatusReport = {
            orderId: osr.orderId || orig.orderId,
            orderStatus: osr.orderStatus || orig.orderStatus,
            rejectMessage: osr.rejectMessage || orig.rejectMessage,
            time: osr.time || orig.time,
            lastQuantity: osr.lastQuantity,
            lastPrice: osr.lastPrice,
            leavesQuantity: osr.leavesQuantity || orig.leavesQuantity,
            cumQuantity: cumQuantity,
            averagePrice: osr.averagePrice || orig.averagePrice,
            side: osr.side || orig.side,
            quantity: quantity,
            type: osr.type || orig.type,
            price: osr.price || orig.price,
            timeInForce: osr.timeInForce || orig.timeInForce,
            exchange: osr.exchange || orig.exchange,
            exchangeId: osr.exchangeId || orig.exchangeId,
            partiallyFilled: cumQuantity > 0 && cumQuantity !== quantity,
            pendingCancel: osr.pendingCancel,
            pendingReplace: osr.pendingReplace,
            cancelRejected: osr.cancelRejected
        };

        this._allOrders[osr.orderId].push(o);
        this._log("applied gw update -> %o", o);

        this.OrderUpdate.trigger(o);
    };

    makeFee() : number {
        return this._baseGateway.makeFee();
    }

    takeFee() : number {
        return this._baseGateway.takeFee();
    }

    MarketData : Evt<MarketBook> = new Evt<MarketBook>();

    name() : string {
        return this._baseGateway.name();
    }

    exchange() : Exchange {
        return this._baseGateway.exchange();
    }

    _currentBook : MarketBook = null;
    _mdGateway : IMarketDataGateway;
    _baseGateway : IExchangeDetailsGateway;
    _oeGateway : IOrderEntryGateway;
    _log : Logger;

    public currentBook = () : MarketBook => {
        return this._currentBook;
    };

    private handleMarketData = (book : MarketBook) => {
        if (this._currentBook == null || (!book.top.equals(this._currentBook.top) || !book.second.equals(this._currentBook.second))) {
            this._currentBook = book;
            this.MarketData.trigger(book);
            this._log(book);
        }
    };

    public onConnect = (gwName : string, cs : ConnectivityStatus) => {
        this._log(gwName, "Connection status changed ", ConnectivityStatus[cs]);
    };

    constructor(mdGateway : IMarketDataGateway, baseGateway : IExchangeDetailsGateway, oeGateway : IOrderEntryGateway) {
        this._log = log("tribeca:exchangebroker:" + baseGateway.name());

        this._mdGateway = mdGateway;
        this._oeGateway = oeGateway;
        this._baseGateway = baseGateway;

        this._mdGateway.MarketData.on(this.handleMarketData);
        this._mdGateway.ConnectChanged.on(s => this.onConnect("MD", s));

        this._oeGateway.OrderUpdate.on(this.onOrderUpdate);
        this._oeGateway.ConnectChanged.on(s => this.onConnect("OE", s));
    }
}

class NullOrderGateway implements IOrderEntryGateway {
    OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();
    ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();

    sendOrder(order : BrokeredOrder) {
        setTimeout(() => this.trigger(order.orderId, OrderStatus.Working), 10);
    }

    cancelOrder(cancel : BrokeredCancel) {
        setTimeout(() => this.trigger(cancel.clientOrderId, OrderStatus.Complete), 10);
    }

    replaceOrder(replace : BrokeredReplace) {
        this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        this.sendOrder(replace);
    }

    private trigger(orderId : string, status : OrderStatus) {
        var rpt : OrderStatusReport = {
            orderId: orderId,
            orderStatus: status,
            time: new Date()
        };
        this.OrderUpdate.trigger(rpt);
    }

    constructor() {
        this.ConnectChanged.trigger(ConnectivityStatus.Connected);
    }
}