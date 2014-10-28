/// <reference path="utils.ts" />

class ExchangeBroker implements IBroker {
    PositionUpdate = new Evt<ExchangeCurrencyPosition>();
    private _currencies : { [currency : number] : ExchangeCurrencyPosition } = {};
    public getPosition(currency : Currency) : ExchangeCurrencyPosition {
        return this._currencies[currency];
    }

    private onPositionUpdate = (rpt : CurrencyPosition) => {
        if (typeof this._currencies[rpt.currency] === "undefined" || this._currencies[rpt.currency].amount != rpt.amount) {
            var newRpt = rpt.toExchangeReport(this.exchange());
            this._currencies[rpt.currency] = newRpt;
            this.PositionUpdate.trigger(newRpt);
            this._log("New currency report: %o", newRpt);
        }
    };

    cancelOpenOrders() : void {
        for (var k in this._allOrders) {
            if (!this._allOrders.hasOwnProperty(k)) continue;
            var e : OrderStatusReport = this._allOrders[k].last();

            switch (e.orderStatus) {
                case OrderStatus.New:
                case OrderStatus.Working:
                    this.cancelOrder(new OrderCancel(e.orderId, e.exchange, date()));
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
        // use moment.js?
        return new Date().getTime().toString(32)
    };

    sendOrder = (order : SubmitNewOrder) : SentOrder => {
        var orderId = ExchangeBroker.generateOrderId();
        var exch = this.exchange();
        var brokeredOrder = new BrokeredOrder(orderId, order.side, order.quantity, order.type, order.price, order.timeInForce, exch);

        var sent = this._oeGateway.sendOrder(brokeredOrder);

        var rpt : OrderStatusReport = {
            orderId: orderId,
            side: order.side,
            quantity: order.quantity,
            type: order.type,
            time: sent.sentTime,
            price: order.price,
            timeInForce: order.timeInForce,
            orderStatus: OrderStatus.New,
            exchange: exch,
            computationalLatency: sent.sentTime.diff(order.generatedTime)};
        this._allOrders[rpt.orderId] = [rpt];
        this._log("sent order %o", rpt);
        this.onOrderUpdate(rpt);

        return new SentOrder(rpt.orderId);
    };

    replaceOrder = (replace : CancelReplaceOrder) : SentOrder => {
        var rpt = this._allOrders[replace.origOrderId].last();
        var br = new BrokeredReplace(replace.origOrderId, replace.origOrderId, rpt.side,
            replace.quantity, rpt.type, replace.price, rpt.timeInForce, rpt.exchange, rpt.exchangeId);

        var sent = this._oeGateway.replaceOrder(br);

        var rpt : OrderStatusReport = {
            orderId: replace.origOrderId,
            orderStatus: OrderStatus.Working,
            pendingReplace: true,
            price: replace.price,
            quantity: replace.quantity,
            time: sent.sentTime,
            computationalLatency: sent.sentTime.diff(replace.generatedTime)};
        this._log("cancel-replaced order %o %o", rpt, br);
        this.onOrderUpdate(rpt);

        return new SentOrder(rpt.orderId);
    };

    cancelOrder = (cancel : OrderCancel) => {
        var rpt = this._allOrders[cancel.origOrderId].last();
        var cxl = new BrokeredCancel(cancel.origOrderId, ExchangeBroker.generateOrderId(), rpt.side, rpt.exchangeId);
        var sent = this._oeGateway.cancelOrder(cxl);
        this._log("cancelled order %o with %o", rpt, cxl);

        var rpt : OrderStatusReport = {
            orderId: cancel.origOrderId,
            orderStatus: OrderStatus.Working,
            pendingCancel: true,
            time: sent.sentTime,
            computationalLatency: sent.sentTime.diff(cancel.generatedTime)};
        this.onOrderUpdate(rpt);
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
            computationalLatency: osr.computationalLatency,
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
    _log : Logger;

    public get currentBook() : MarketBook {
        return this._currentBook;
    }

    private handleMarketData = (book : MarketBook) => {
        if (!book.equals(this.currentBook)) {
            this._currentBook = book;
            this.MarketData.trigger(book);
            this._log(book);
        }
    };

    ConnectChanged = new Evt<ConnectivityStatus>();
    private mdConnected = ConnectivityStatus.Disconnected;
    private oeConnected = ConnectivityStatus.Disconnected;
    private _connectStatus = ConnectivityStatus.Disconnected;
    public onConnect = (gwType : GatewayType, cs : ConnectivityStatus) => {
        if (gwType == GatewayType.MarketData) this.mdConnected = cs;
        if (gwType == GatewayType.OrderEntry) this.oeConnected = cs;

        var newStatus = this.mdConnected == ConnectivityStatus.Connected && this.oeConnected == ConnectivityStatus.Connected
            ? ConnectivityStatus.Connected
            : ConnectivityStatus.Disconnected;

        if (newStatus != this._connectStatus)
            this.ConnectChanged.trigger(newStatus);

        this._connectStatus = newStatus;
        this._log(GatewayType[gwType], "Connection status changed ", ConnectivityStatus[cs]);
    };

    public get connectStatus() : ConnectivityStatus {
        return this._connectStatus;
    }

    constructor(private _mdGateway : IMarketDataGateway,
                private _baseGateway : IExchangeDetailsGateway,
                private _oeGateway : IOrderEntryGateway,
                private _posGateway : IPositionGateway) {
        this._log = log("tribeca:exchangebroker:" + this._baseGateway.name());

        this._mdGateway.MarketData.on(this.handleMarketData);
        this._mdGateway.ConnectChanged.on(s => {
            if (s == ConnectivityStatus.Disconnected) this._currentBook = null;
            this.onConnect(GatewayType.MarketData, s);
        });

        this._oeGateway.OrderUpdate.on(this.onOrderUpdate);
        this._oeGateway.ConnectChanged.on(s => {
            this.onConnect(GatewayType.OrderEntry, s)
        });

        this._posGateway.PositionUpdate.on(this.onPositionUpdate);
    }
}