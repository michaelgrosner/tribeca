/// <reference path="arbagent.ts" />
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

export class UI {
    private _log : Utils.Logger = Utils.log("tribeca:ui");
    private _exchange : Models.Exchange;

    constructor(private _env : string,
                private _pair : Models.CurrencyPair,
                private _broker : Interfaces.IBroker,
                private _quoteGenerator : Agent.QuoteGenerator,
                private _paramRepo : Agent.QuotingParametersRepository) {
        this._exchange = this._broker.exchange();

        var adminPath = path.join(__dirname, "..", "admin", "admin");
        app.get('/', (req, res) => {
            res.sendFile(path.join(adminPath, "index.html"));
        });
        app.use(express.static(adminPath));
        app.use(express.static(path.join(__dirname, "..", "admin", "common")));
        app.use(express.static(path.join(__dirname, "..", "admin")));
        app.use(express.static(path.join(__dirname, "..", "admin")));

        this._broker.MarketData.on(this.sendUpdatedMarket);
        this._broker.OrderUpdate.on(x => this.sendOrderStatusUpdate(x));
        this._broker.PositionUpdate.on(x => this.sendPositionUpdate(x));
        this._broker.ConnectChanged.on(x => this.sendUpdatedConnectionStatus(this._exchange, x));
        this._quoteGenerator.ActiveChanged.on(this.sendActiveChange);
        this._quoteGenerator.NewValue.on(this.sendFairValue);
        this._quoteGenerator.NewQuote.on(this.sendQuote);
        this._quoteGenerator.NewTradingDecision.on(this.sendResultChange);
        this._paramRepo.NewParameters.on(this.sendQuotingParameters);

        http.listen(3000, () => this._log('Listening to admins on *:3000...'));

        io.on('connection', sock => {
            sock.emit("hello", this._env);
            sock.emit("active-changed", this.sendActiveChange());

            sock.on("subscribe-new-trading-decision", () => {
                this.sendResultChange();
            });

            sock.on("subscribe-order-status-report", () => {
                var states = this._broker.allOrderStates();
                sock.emit("order-status-report-snapshot", this._wrapOutgoingMessage(states.slice(Math.max(states.length - 100, 1))));
            });

            sock.on("subscribe-position-report", () => {
                this.sendPositionUpdate(this._broker.getPosition(this._pair.base));
                this.sendPositionUpdate(this._broker.getPosition(this._pair.quote));
            });

            sock.on("subscribe-market-book", () => {
                this.sendUpdatedMarket();
            });

            sock.on("subscribe-connection-status", () => {
                this.sendUpdatedConnectionStatus(this._exchange, this._broker.connectStatus);
            });

            sock.on("subscribe-fair-value", () => {
                this.sendFairValue();
            });

            sock.on("subscribe-quote", () => {
                this.sendQuote();
            });

            sock.on("subscribe-parameter-updates", () => {
                this.sendQuotingParameters();
            });

            // TODO: if i ever truly go to multi-server, these would need to be filtered. ideally client-side to the right server
            sock.on("submit-order", (o : Models.OrderRequestFromUI) => {
                this._log("got new order", o);
                var order = new Models.SubmitNewOrder(Models.Side[o.side], o.quantity, Models.OrderType[o.orderType],
                    o.price, Models.TimeInForce[o.timeInForce], Models.Exchange[o.exchange], Utils.date());
                this._broker.sendOrder(order);
            });

            sock.on("cancel-order", (o : Models.OrderStatusReport) => {
                this._log("got new cancel req %o", o);
                this._broker.cancelOrder(new Models.OrderCancel(o.orderId, o.exchange, Utils.date()));
            });

            sock.on("cancel-replace", (o : Models.OrderStatusReport, replace : Models.ReplaceRequestFromUI) => {
                this._log("got new cxl-rpl req %o with %o", o, replace);
                this._broker.replaceOrder(new Models.CancelReplaceOrder(o.orderId, replace.quantity, replace.price, o.exchange, Utils.date()));
            });

            sock.on("active-change-request", (to : Models.ExchangePairMessage<boolean>) => {
                this._log("got active-change-request %j", to);
                if (to.exchange === this._exchange && to.pair.base === this._pair.base && to.pair.quote === this._pair.quote)
                    _quoteGenerator.changeActiveStatus(to.data);
            });

            sock.on("parameters-update-request", (p : Models.ExchangePairMessage<Models.QuotingParameters>) => {
                this._paramRepo.updateParameters(p.data);
            });
        });
    }

    sendActiveChange = () => {
        setTimeout(() => io.emit("active-changed", this._wrapOutgoingMessage(this._quoteGenerator.Active)), 0);
    };

    sendUpdatedConnectionStatus = (exch : Models.Exchange, cs : Models.ConnectivityStatus) => {
        io.emit("connection-status", exch, cs);
    };

    sendPositionUpdate = (msg : Models.ExchangeCurrencyPosition) => {
        if (msg == null) return;
        io.emit("position-report", msg);
    };

    sendOrderStatusUpdate = (msg : Models.OrderStatusReport) => {
        if (msg == null) return;
        setTimeout(() => io.emit("order-status-report", this._wrapOutgoingMessage(msg)), 0);
    };

    sendUpdatedMarket = () => {
        var book = this._broker.currentBook;
        if (book == null) return;
        setTimeout(() => io.emit("market-book", this._wrapOutgoingMessage(book)), 0);
    };

    sendResultChange = () => {
        var res : Models.TradingDecision = this._quoteGenerator.latestDecision;
        if (res == null) return;
        setTimeout(() => io.emit("new-trading-decision", this._wrapOutgoingMessage(res)), 0);
    };

    sendFairValue = () => {
        var fv = this._quoteGenerator.latestFairValue;
        if (fv == null) return;
        setTimeout(() => io.emit("fair-value", this._wrapOutgoingMessage(fv)), 0);
    };

    sendQuote = () => {
        var quote = this._quoteGenerator.latestQuote;
        if (quote == null) return;
        setTimeout(() => io.emit("quote", this._wrapOutgoingMessage(quote)), 0);
    };

    sendQuotingParameters = () => {
        var p = this._paramRepo.latest;
        if (p == null) return;
        setTimeout(() => io.emit("parameter-updates", this._wrapOutgoingMessage(p)), 0);
    };

    private _wrapOutgoingMessage = <T>(msg : T) : Models.ExchangePairMessage<T> => {
        return new Models.ExchangePairMessage<T>(this._exchange, this._pair, msg);
    }
}