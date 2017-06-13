import Models = require("../share/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import moment = require("moment");
import _ = require('lodash');
import fs = require("fs");
import Persister = require("./persister");
import stream = require("stream");

enum TimedType {
    Interval,
    Timeout
}

class Timed {
    constructor(
        public action : () => void,
        public time : moment.Moment,
        public type : TimedType,
        public interval : moment.Duration) {}
}

export class BacktestTimeProvider implements Utils.IBacktestingTimeProvider {
    constructor(private _internalTime : moment.Moment, private _endTime : moment.Moment) { }

    utcNow = () => this._internalTime.toDate();

    private _immediates = new Array<() => void>();
    setImmediate = (action: () => void) => this._immediates.push(action);

    private _timeouts : Timed[] = [];
    setTimeout = (action: () => void, time: moment.Duration) => {
        this.setAction(action, time, TimedType.Timeout);
    };

    setInterval = (action: () => void, time: moment.Duration) => {
        this.setAction(action, time, TimedType.Interval);
    };

    private setAction  = (action: () => void, time: moment.Duration, type : TimedType) => {
        var dueTime = this._internalTime.clone().add(time);

        if (dueTime.toDate().getTime() - this.utcNow().getTime() < 0) {
            return;
        }

        this._timeouts.push(new Timed(action, dueTime, type, time));
        this._timeouts.sort((a, b) => a.time.toDate().getTime() - b.time.toDate().getTime());
    };

    scrollTimeTo = (time : moment.Moment) => {
        if (time.toDate().getTime() - this.utcNow().getTime() < 0) {
            throw new Error("Cannot reverse time!");
        }

        while (this._immediates.length > 0) {
            this._immediates.pop()();
        }

        while (this._timeouts.length > 0 && _.first(this._timeouts).time.toDate().getTime() - time.toDate().getTime() < 0) {
            var evt : Timed = this._timeouts.shift();
            this._internalTime = evt.time;
            evt.action();
            if (evt.type === TimedType.Interval) {
                this.setAction(evt.action, evt.interval, evt.type);
            }
        }

        this._internalTime = time;
    };
}

export class BacktestGateway implements Interfaces.IPositionGateway, Interfaces.IOrderEntryGateway, Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();

    OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();

    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => { return null/*Promise.resolve(0)*/; };

    generateClientOrderId = () => {
        return "BACKTEST-" + parseInt((Math.random()+'').substr(-8), 10);
    }

    public cancelsByClientOrderId = true;

    private _openBidOrders : {[orderId: string]: Models.OrderStatusReport} = {};
    private _openAskOrders : {[orderId: string]: Models.OrderStatusReport} = {};

    sendOrder = (order : Models.OrderStatusReport) => {
        this.timeProvider.setTimeout(() => {
            if (order.side === Models.Side.Bid) {
                this._openBidOrders[order.orderId] = order;
                this._quoteHeld += order.price*order.quantity;
                this._quoteAmount -= order.price*order.quantity;
            }
            else {
                this._openAskOrders[order.orderId] = order;
                this._baseHeld += order.quantity;
                this._baseAmount -= order.quantity;
            }

            this.OrderUpdate.trigger({ orderId: order.orderId, orderStatus: Models.OrderStatus.Working });
        }, moment.duration(3));
    };

    cancelOrder = (cancel : Models.OrderStatusReport) => {
        this.timeProvider.setTimeout(() => {
            if (cancel.side === Models.Side.Bid) {
                var existing = this._openBidOrders[cancel.orderId];
                if (typeof existing === "undefined") {
                    this.OrderUpdate.trigger({orderId: cancel.orderId, orderStatus: Models.OrderStatus.Rejected});
                    return;
                }
                this._quoteHeld -= existing.price * existing.quantity;
                this._quoteAmount += existing.price * existing.quantity;
                delete this._openBidOrders[cancel.orderId];
            }
            else {
                var existing = this._openAskOrders[cancel.orderId];
                if (typeof existing === "undefined") {
                    this.OrderUpdate.trigger({orderId: cancel.orderId, orderStatus: Models.OrderStatus.Rejected});
                    return;
                }
                this._baseHeld -= existing.quantity;
                this._baseAmount += existing.quantity;
                delete this._openAskOrders[cancel.orderId];
            }

            this.OrderUpdate.trigger({ orderId: cancel.orderId, orderStatus: Models.OrderStatus.Cancelled });
        }, moment.duration(3));
    };

    replaceOrder = (replace : Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private onMarketData = (market : Models.Market) => {
        this._openAskOrders = this.tryToMatch(<any>_.values(this._openAskOrders), market.bids, Models.Side.Ask);
        this._openBidOrders = this.tryToMatch(<any>_.values(this._openBidOrders), market.asks, Models.Side.Bid);

        this.MarketData.trigger(market);
    };


    private tryToMatch = (orders: Models.OrderStatusReport[], marketSides: Models.MarketSide[], side: Models.Side) => {
        if (orders.length === 0 || marketSides.length === 0)
            return _.keyBy(orders, k => k.orderId);

        var cmp = side === Models.Side.Ask ? (m, o) => o < m : (m, o) => o > m;
        _.forEach(orders, order => {
            _.forEach(marketSides, mkt => {
                if ((cmp(mkt.price, order.price) || order.type === Models.OrderType.Market) && order.quantity > 0) {

                    var px = order.price;
                    if (order.type === Models.OrderType.Market) px = mkt.price;

                    var update : Models.OrderStatusUpdate = { orderId: order.orderId, lastPrice: px };

                    if (mkt.size >= order.quantity) {
                        update.orderStatus = Models.OrderStatus.Complete;
                        update.lastQuantity = order.quantity;
                    }
                    else {
                        update.partiallyFilled = true;
                        update.orderStatus = Models.OrderStatus.Working;
                        update.lastQuantity = mkt.size;
                    }
                    this.OrderUpdate.trigger(update);

                    if (side === Models.Side.Bid) {
                        this._baseAmount += update.lastQuantity;
                        this._quoteHeld -= (update.lastQuantity*px);
                    }
                    else {
                        this._baseHeld -= update.lastQuantity;
                        this._quoteAmount += (update.lastQuantity*px);
                    }

                    order.quantity = order.quantity - update.lastQuantity;
                };
            });
        });

        var liveOrders = _.filter(orders, o => o.quantity > 0);

        if (liveOrders.length > 5)
            console.warn("more than 5 outstanding " + Models.Side[side] + " orders open");

        return _.keyBy(liveOrders, k => k.orderId);
    };

    private onMarketTrade = (trade : Models.MarketTrade) => {
        this._openAskOrders = this.tryToMatch(<any>_.values(this._openAskOrders), [trade], Models.Side.Ask);
        this._openBidOrders = this.tryToMatch(<any>_.values(this._openBidOrders), [trade], Models.Side.Bid);

        this.MarketTrade.trigger(new Models.GatewayMarketTrade(trade.price, trade.size, trade.time, false, trade.make_side));
    };

    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    private recomputePosition = () => {
        this.PositionUpdate.trigger(new Models.CurrencyPosition(this._baseAmount, this._baseHeld, Models.Currency.BTC));
        this.PositionUpdate.trigger(new Models.CurrencyPosition(this._quoteAmount, this._quoteHeld, Models.Currency.USD));
    };

    private _baseHeld = 0;
    private _quoteHeld = 0;

    constructor(
            private _inputData: (Models.Market | Models.MarketTrade)[],
            private _baseAmount : number,
            private _quoteAmount : number,
            private timeProvider: Utils.IBacktestingTimeProvider) {}

    public run = () => {
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);

        var hasProcessedMktData = false;

        this.timeProvider.setInterval(() => this.recomputePosition(), moment.duration(15, "seconds"));

        this._inputData.forEach(i => {
            this.timeProvider.scrollTimeTo(moment(i.time));

            if (typeof i["make_side"] !== "undefined") {
                this.onMarketTrade(<Models.MarketTrade>i);
            }
            else if (typeof i["bids"] !== "undefined" || typeof i["asks"] !== "undefined") {
                this.onMarketData(<Models.Market>i);

                if (!hasProcessedMktData) {
                    this.recomputePosition();
                    hasProcessedMktData = true;
                }
            }
        });

        this.recomputePosition();
    };
}

class BacktestGatewayDetails implements Interfaces.IExchangeDetailsGateway {
    minTickIncrement: number = 0.01;
    minSize: number = 0.01;

    public get hasSelfTradePrevention() {
        return false;
    }

    name(): string {
        return "Null";
    }

    makeFee(): number {
        return 0;
    }

    takeFee(): number {
        return 0;
    }

    exchange(): Models.Exchange {
        return Models.Exchange.Null;
    }
}

export class BacktestParameters {
    startingBasePosition: number;
    startingQuotePosition: number;
    quotingParameters: Models.QuotingParameters;
    id: string;
}

export class BacktestPersister<T> implements Persister.ILoadAll<T>, Persister.ILoadLatest<T> {
    public load = (exchange: Models.Exchange, pair: Models.CurrencyPair, limit?: number): Promise<T[]> => {
        return this.loadAll(limit);
    };

    public loadAll = (limit?: number): Promise<T[]> => {
      return null;
      // if (this.initialData) {
        // if (limit) {
          // return new Promise((resolve, reject) => { resolve(this.initialData.slice(limit * -1)); });
        // }
        // else {
          // return new Promise((resolve, reject) => { resolve(this.initialData); });
        // }
      // }
      // return new Promise((resolve, reject) => { resolve([]); });
    };

    public persist = (report: T) => { };

    public repersist = (report: T) => { };

    public clean = (time: Date) => { };

    public loadDBSize = (): Promise<T> => {
      return null;
      // return new Promise((resolve, reject) => { resolve(null); });
    };

    public loadLatest = (): Promise<T> => {
      return null;
      // if (this.initialData)
          // return new Promise((resolve, reject) => { resolve(_.last(this.initialData)); });
    };

    constructor(private initialData?: T[]) {
        this.initialData = initialData || null;
    }
}

export class BacktestExchange extends Interfaces.CombinedGateway {
    constructor(private gw: BacktestGateway) {
        super(gw, gw, gw, new BacktestGatewayDetails());
    }

    public run = () => this.gw.run();
};

// backtest server

import express = require('express');
import util = require("util");

var backtestServer = () => {
    process.on("uncaughtException", err => {
      console.error(new Date().toISOString().slice(11, -1), 'backtest', 'Unhandled exception!', err);
      process.exit(1);
    });

    process.on("unhandledRejection", (reason, p) => {
      console.error(new Date().toISOString().slice(11, -1), 'backtest', 'Unhandled promise rejection!', reason, p);
      process.exit(1);
    });

    process.on("exit", () => {
      console.info(new Date().toISOString().slice(11, -1), 'backtest', 'Exiting with code', 1);
      process.exit(1);
    });

    process.on("SIGINT", () => {
      console.info(new Date().toISOString().slice(11, -1), 'backtest', 'Handling SIGINT');
      process.exit(1);
    });

    var mdFile = process.env['MD_FILE'];
    var paramFile = process.env['PARAM_FILE'];
    var savedProgressFile = process.env["PROGRESS_FILE"] || "nextParameters_saved.txt";
    var backtestResultFile = process.env["RESULT_FILE"] || 'backtestResults.txt';

    var rawParams = fs.readFileSync(paramFile, 'utf8');
    var parameters: BacktestParameters[] = JSON.parse(rawParams);
    if (fs.existsSync(savedProgressFile)) {
        var l = parseInt(fs.readFileSync(savedProgressFile, 'utf8'));
        parameters = parameters.slice(l * -1);
    }
    else if (fs.existsSync(backtestResultFile)) {
        fs.unlinkSync(backtestResultFile);
    }

    console.log("loaded input data...");

    var app = express();
    app.use(require('body-parser').json({limit: '200mb'}));
    app.use(require("compression")());

    var server = app.listen(5001, () => {
        var host = server.address().address;
        var port = server.address().port;

        console.log('Backtest server listening at http://%s:%s', host, port);
    });

    app.get("/inputData", (req, res) => {
        console.log("Starting inputData download for", req.ip);
        res.sendFile(mdFile, (err) => {
            if (err) console.error("Error while transmitting input data to", req.ip);
            else console.log("Ending inputData download for", req.ip);
        });
    });

    app.get("/nextParameters", (req, res) => {
        if (_.some(parameters)) {
            var id = parameters.length;
            var served = parameters.shift();
            if (typeof served["id"] === "undefined")
                served.id = id.toString();

            console.log("Serving parameters id =", served.id, " to", req.ip);
            res.json(served);
            fs.writeFileSync(savedProgressFile, parameters.length, {encoding: 'utf8'});

            if (!_.some(parameters)) {
                console.log("Done serving parameters");
            }
        }
        else {
            res.json("done");
            if (fs.existsSync(savedProgressFile))
                fs.unlinkSync(savedProgressFile);
        }
    });

    app.post("/result", (req, res) => {
        var params = req.body;
        console.log("Accept backtest results, volume =", params[2].volume.toFixed(2), "val =",
            params[1].value.toFixed(2), "qVal =", params[1].quoteValue.toFixed(2));
        fs.appendFileSync(backtestResultFile, JSON.stringify(params)+"\n");
    });
}

if (process.argv[1].indexOf("backtest.js") > 1) {
    backtestServer();
}
