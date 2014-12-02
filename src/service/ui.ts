/// <reference path="agent.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="utils.ts" />

import express = require('express');
var app = express();
var http = require('http').Server(app);
var io = require('socket.io')(http);
import path = require("path");
import Agent = require("./agent");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class UI {
    _log : Utils.Logger = Utils.log("tribeca:ui");

    constructor(private _env : string,
                private _brokers : Array<Interfaces.IBroker>,
                private _agent : Agent.Agent,
                private _orderAgg : Agent.OrderBrokerAggregator,
                private _mdAgg : Agent.MarketDataAggregator,
                private _posAgg : Agent.PositionAggregator) {

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
        this._agent.BestResultChanged.on(x => this.sendResultChange(x));
        this._brokers.forEach(b => b.ConnectChanged.on(cs => this.sendUpdatedConnectionStatus(b.exchange(), cs)));

        http.listen(3000, () => this._log('Listening to admins on *:3000...'));

        io.on('connection', sock => {
            sock.emit("hello", this._env);

            this._brokers.forEach(b => {
                this.sendUpdatedConnectionStatus(b.exchange(), b.connectStatus);
                this.sendUpdatedMarket(b.currentBook);
                sock.emit("order-status-report-snapshot", b.allOrderStates());

                // UI only knows about BTC and USD - for now
                [Models.Currency.BTC, Models.Currency.USD].forEach(c => {
                    this.sendPositionUpdate(b.getPosition(c));
                });
            });

            sock.emit("active-changed", this._agent.Active);

            this.sendResultChange(this._agent.LastBestResult);

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
        io.emit("position-report", msg);
    };

    sendOrderStatusUpdate = (msg : Models.OrderStatusReport) => {
        io.emit("order-status-report", msg);
    };

    sendUpdatedMarket = (book : Models.Market) => {
        if (book == null) return;
        io.emit("market-book", book);
    };

    sendResultChange = (res : Interfaces.Result) => {
        var serialize = (r : Interfaces.Result) => {
            if (r != null) {
                return new Models.ResultMessage(r.restSide, r.restBroker.exchange(), r.hideBroker.exchange(),
                    r.rest, r.hide, r.profit, r.size, r.generatedTime);
            }
            return null;
        };
        io.emit("result-change", new Models.InactableResultMessage(res != null, serialize(res)));
    };
}