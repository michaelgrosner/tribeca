/// <reference path="arbagent.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="utils.ts" />

import Agent = require("./arbagent");
import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class UI {
    private _log : Utils.Logger = Utils.log("tribeca:ui");
    private _exchange : Models.Exchange;

    constructor(private _env : string,
                private _broker : Interfaces.IBroker,
                private _io : any) {
        this._exchange = this._broker.exchange();

        this._io.on('connection', sock => {
            sock.emit("hello", this._env);

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
        });
    }
}