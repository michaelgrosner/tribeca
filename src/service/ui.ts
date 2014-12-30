/// <reference path="arbagent.ts" />
/// <reference path="aggregators.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="utils.ts" />

import express = require('express');
var app = express();
var http = (<any>require('http')).Server(app);
var io = require('socket.io')(http);
import path = require("path");
import Agent = require("./arbagent");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import Aggregators = require("./aggregators");

export class UI {
    _log : Utils.Logger = Utils.log("tribeca:ui");

    constructor(private _env : string,
                private _brokers : Array<Interfaces.IBroker>,
                private _agent : Agent.Trader,
                private _orderAgg : Aggregators.OrderBrokerAggregator,
                private _mdAgg : Aggregators.MarketDataAggregator,
                private _posAgg : Aggregators.PositionAggregator) {

        var adminPath = path.join(__dirname, "..", "admin", "admin");
        app.get('/', (req, res) => {
            res.sendFile(path.join(adminPath, "index.html"));
        });
        app.use(express.static(adminPath));
        app.use(express.static(path.join(__dirname, "..", "admin", "common")));
        app.use(express.static(path.join(__dirname, "..", "admin")));

        this._mdAgg.MarketData.on(x => this.sendUpdatedMarket(x));
        this._orderAgg.OrderUpdate.on(x => this.sendOrderStatusUpdate(x));
        this._posAgg.PositionUpdate.on(x => this.sendPositionUpdate(x));
        this._agent.ActiveChanged.on(s => io.emit("active-changed", s));
        this._agent.NewTradingDecision.on(x => this.sendResultChange(x));
        this._brokers.forEach(b => b.ConnectChanged.on(cs => this.sendUpdatedConnectionStatus(b.exchange(), cs)));

        http.listen(3000, () => this._log('Listening to admins on *:3000...'));

        io.on('connection', sock => {
            sock.emit("hello", this._env);

            this._brokers.forEach(b => {
                this.sendResultChange(this._agent.getTradingDecision(b.exchange()));
            });

            sock.emit("active-changed", this._agent.Active);

            sock.on("subscribe-order-status-report", () => {
                this._brokers.forEach(b => {
                    var states = b.allOrderStates();
                    sock.emit("order-status-report-snapshot", states.slice(Math.max(states.length - 100, 1)));
                });
            });

            sock.on("subscribe-position-report", () => {
                this._brokers.forEach(b => {
                    [Models.Currency.BTC, Models.Currency.USD, Models.Currency.LTC].forEach(c => {
                        this.sendPositionUpdate(b.getPosition(c));
                    });
                });
            });

            sock.on("subscribe-market-book", () => {
                this._brokers.forEach(b => {
                    this.sendUpdatedMarket(this._mdAgg.getCurrentBook(b.exchange()));
                });
            });

            sock.on("subscribe-connection-status", () => {
                this._brokers.forEach(b => {
                    this.sendUpdatedConnectionStatus(b.exchange(), b.connectStatus);
                });
            });

            sock.on("submit-order", (o : Models.OrderRequestFromUI) => {
                this._log("got new order %o", o);
                var order = new Models.SubmitNewOrder(Models.Side[o.side], o.quantity, Models.OrderType[o.orderType],
                    o.price, Models.TimeInForce[o.timeInForce], Models.Exchange[o.exchange], Utils.date());
                _orderAgg.submitOrder(order);
            });

            sock.on("cancel-order", (o : Models.OrderStatusReport) => {
                this._log("got new cancel req %o", o);
                _orderAgg.cancelOrder(new Models.OrderCancel(o.orderId, o.exchange, Utils.date()));
            });

            sock.on("cancel-replace", (o : Models.OrderStatusReport, replace : Models.ReplaceRequestFromUI) => {
                this._log("got new cxl-rpl req %o with %o", o, replace);
                _orderAgg.cancelReplaceOrder(new Models.CancelReplaceOrder(o.orderId, replace.quantity, replace.price, o.exchange, Utils.date()));
            });

            sock.on("active-change-request", (to : boolean) => {
                _agent.changeActiveStatus(to);
            });
        });
    }

    sendUpdatedConnectionStatus = (exch : Models.Exchange, cs : Models.ConnectivityStatus) => {
        io.emit("connection-status", exch, cs);
    };

    sendPositionUpdate = (msg : Models.ExchangeCurrencyPosition) => {
        if (msg == null) return;
        io.emit("position-report", msg);
    };

    sendOrderStatusUpdate = (msg : Models.OrderStatusReport) => {
        if (msg == null) return;
        io.emit("order-status-report", msg);
    };

    sendUpdatedMarket = (book : Models.Market) => {
        if (book == null) return;
        io.emit("market-book", book);
    };

    sendResultChange = (res : Models.TradingDecision) => {
        if (res == null) return;
        io.emit(Messaging.Topics.NewTradingDecision, res);
    };
}