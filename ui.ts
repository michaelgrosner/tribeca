/// <reference path="models.ts" />

var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var path = require("path");

interface OrderRequestFromUI {
    exchange : string;
    side : string;
    price : number;
    quantity : number;
    timeInForce : string;
    orderType : string
}

interface ReplaceRequestFromUI {
    price : number;
    quantity : number;
}

class UI {
    _log : Logger = log("tribeca:ui");

    constructor(private _env : string,
                private _brokers : Array<IBroker>,
                private _agent : Agent,
                private _orderAgg : OrderBrokerAggregator,
                private _mdAgg : MarketDataAggregator,
                private _posAgg : PositionAggregator) {

        app.get('/', (req, res) => {
            res.sendFile(path.join(__dirname, "index.html"));
        });

        this._mdAgg.MarketData.on(this.sendUpdatedMarket);
        this._orderAgg.OrderUpdate.on(this.sendOrderStatusUpdate);
        this._posAgg.PositionUpdate.on(this.sendPositionUpdate);
        this._agent.ActiveChanged.on(s => io.emit("active-changed", s));
        this._agent.BestResultChanged.on(s => io.emit("result-change", s));
        this._brokers.forEach(b => b.ConnectChanged.on(cs => this.sendUpdatedConnectionStatus(b.exchange(), cs)));

        http.listen(3000, () => this._log('listening on *:3000'));

        io.on('connection', sock => {
            sock.emit("hello", this._env);
            sock.emit("enums", {
                availableConnectivityStatuses: ConnectivityStatus,
                availableTifs: TimeInForce,
                availableExchanges: Exchange,
                availableSides: Side,
                availableOrderTypes: OrderType,
                availableOrderStatuses: OrderStatus,
                availableLiquidityTypes: Liquidity,
                availableCurrencies: Currency
            });

            this._brokers.forEach(b => {
                this.sendUpdatedConnectionStatus(b.exchange(), b.connectStatus);
                this.sendUpdatedMarket(b.currentBook);
                sock.emit("order-status-report-snapshot", b.allOrderStates());

                // UI only knows about BTC and USD - for now
                [Currency.BTC, Currency.USD].forEach(c => {
                    this.sendPositionUpdate(b.getPosition(c));
                });
            });

            sock.emit("active-changed", this._agent.Active);

            if (this._agent.LastBestResult != null)
                sock.emit("result-change", this._agent.LastBestResult);

            sock.on("submit-order", (o : OrderRequestFromUI) => {
                this._log("got new order %o", o);
                var order = new SubmitNewOrder(Side[o.side], o.quantity, OrderType[o.orderType],
                    o.price, TimeInForce[o.timeInForce], Exchange[o.exchange], date());
                _orderAgg.submitOrder(order);
            });

            sock.on("cancel-order", (o : OrderStatusReport) => {
                this._log("got new cancel req %o", o);
                _orderAgg.cancelOrder(new OrderCancel(o.orderId, o.exchange, date()));
            });

            sock.on("cancel-replace", (o : OrderStatusReport, replace : ReplaceRequestFromUI) => {
                this._log("got new cxl-rpl req %o with %o", o, replace);
                _orderAgg.cancelReplaceOrder(new CancelReplaceOrder(o.orderId, replace.quantity, replace.price, o.exchange, date()));
            });

            sock.on("active-change-request", (to : boolean) => {
                _agent.changeActiveStatus(to);
            });
        });
    }

    sendUpdatedConnectionStatus = (exch : Exchange, cs : ConnectivityStatus) => {
        io.emit("connection-status", exch, cs);
    };

    sendPositionUpdate = (msg : ExchangeCurrencyPosition) => {
        io.emit("position-report", msg);
    };

    sendOrderStatusUpdate = (msg : OrderStatusReport) => {
        io.emit("order-status-report", msg);
    };

    sendUpdatedMarket = (book : MarketBook) => {
        if (book == null) return;
        var b = {bidPrice: book.top.bid.price,
            bidSize: book.top.bid.size,
            askPrice: book.top.ask.price,
            askSize: book.top.ask.size,
            exchangeName: book.exchangeName};
        io.emit("market-book", b);
    };
}