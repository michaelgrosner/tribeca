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

class UI {
    NewOrder : Evt<SubmitNewOrder> = new Evt<SubmitNewOrder>();
    CancelOrder : Evt<OrderCancel> = new Evt<OrderCancel>();
    _log : Logger = log("Hudson:UI");
    _brokers : Array<IBroker>;

    constructor(public brokers : Array<IBroker>) {
        this._brokers = brokers;

        app.get('/', (req, res) => {
            res.sendFile(path.join(__dirname, "index.html"));
        });

        http.listen(3000, () => this._log('listening on *:3000'));

        io.on('connection', sock => {
            sock.emit("hello");
            sock.emit("order-options", {
                availableTifs: TimeInForce,
                availableExchanges: Exchange,
                availableSides: Side,
                availableOrderTypes: OrderType,
                availableOrderStatuses: OrderStatus,
                availableLiquidityTypes: Liquidity
            });

            this._brokers.forEach(b => this.sendUpdatedMarket(b.currentBook()));
            this._brokers.forEach(b => b.allOrderStates().forEach(s => this.sendOrderStatusUpdate(s)));

            sock.on("submit-order", (o : OrderRequestFromUI) => {
                this._log("got new order", o);
                this.NewOrder.trigger({
                    exchange: Exchange[o.exchange],
                    side: Side[o.side],
                    price: o.price,
                    quantity: o.quantity,
                    timeInForce: TimeInForce[o.timeInForce],
                    type: OrderType[o.orderType]
                });
            });

            sock.on("cancel-order", (o : OrderStatusReport) => {
                this._log("got new cancel req", o);
                this.CancelOrder.trigger(new OrderCancel(o.orderId, o.exchange));
            });
        });
    }

    sendOrderStatusUpdate = (msg : OrderStatusReport) => {
        io.emit("order-status-report", msg);
    };

    sendUpdatedMarket = (book : MarketBook) => {
        if (book == null) return;
        var b = {bidPrice: book.top.bid.price,
            bidSize: book.top.bid.size,
            askPrice: book.top.ask.price,
            askSize: book.top.ask.size,
            exchangeName: Exchange[book.exchangeName]};
        io.emit("market-book", b);
    };
}