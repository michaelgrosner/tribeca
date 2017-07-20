import Models = require("../share/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import moment = require("moment");
import fs = require("fs");

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

        while (this._timeouts.length > 0 && this._timeouts[0].time.toDate().getTime() - time.toDate().getTime() < 0) {
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

            this._evUp('OrderUpdateGateway', { orderId: order.orderId, orderStatus: Models.OrderStatus.Working });
        }, moment.duration(3));
    };

    cancelOrder = (cancel : Models.OrderStatusReport) => {
        this.timeProvider.setTimeout(() => {
            if (cancel.side === Models.Side.Bid) {
                var existing = this._openBidOrders[cancel.orderId];
                if (typeof existing === "undefined") {
                    this._evUp('OrderUpdateGateway', {orderId: cancel.orderId, orderStatus: Models.OrderStatus.Cancelled});
                    return;
                }
                this._quoteHeld -= existing.price * existing.quantity;
                this._quoteAmount += existing.price * existing.quantity;
                delete this._openBidOrders[cancel.orderId];
            }
            else {
                var existing = this._openAskOrders[cancel.orderId];
                if (typeof existing === "undefined") {
                    this._evUp('OrderUpdateGateway', {orderId: cancel.orderId, orderStatus: Models.OrderStatus.Cancelled});
                    return;
                }
                this._baseHeld -= existing.quantity;
                this._baseAmount += existing.quantity;
                delete this._openAskOrders[cancel.orderId];
            }

            this._evUp('OrderUpdateGateway', { orderId: cancel.orderId, orderStatus: Models.OrderStatus.Cancelled });
        }, moment.duration(3));
    };

    replaceOrder = (replace : Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };

    private onMarketData = (market : Models.Market) => {
        this._openAskOrders = this.tryToMatch(Object.keys(this._openAskOrders).map(k => this._openAskOrders[k]), market.bids, Models.Side.Ask);
        this._openBidOrders = this.tryToMatch(Object.keys(this._openBidOrders).map(k => this._openBidOrders[k]), market.asks, Models.Side.Bid);

        this._evUp('MarketDataGateway', market);
    };


    private tryToMatch = (orders: Models.OrderStatusReport[], marketSides: Models.MarketSide[], side: Models.Side) => {
        if (orders.length === 0 || marketSides.length === 0) {
            var O = {};
            for(var i = Object.keys(orders).length;i--;)
              O[Object.keys(orders).map(k => orders[k])[i].orderId] = Object.keys(orders).map(k => orders[k])[i];
            return O;
        }

        var cmp = side === Models.Side.Ask ? (m, o) => o < m : (m, o) => o > m;
        orders.forEach(order => {
            marketSides.forEach(mkt => {
                if ((cmp(mkt.price, order.price) || order.type === Models.OrderType.Market) && order.quantity > 0) {

                    var px = order.price;
                    if (order.type === Models.OrderType.Market) px = mkt.price;

                    var update : Models.OrderStatusUpdate = { orderId: order.orderId, lastPrice: px };

                    if (mkt.size >= order.quantity) {
                        update.orderStatus = Models.OrderStatus.Complete;
                        update.lastQuantity = order.quantity;
                    }
                    else {
                        update.orderStatus = Models.OrderStatus.Working;
                        update.lastQuantity = mkt.size;
                    }
                    this._evUp('OrderUpdateGateway', update);

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

        var liveOrders = orders.filter(o => o.quantity > 0);

        if (liveOrders.length > 5)
            console.warn("more than 5 outstanding " + Models.Side[side] + " orders open");

        var O = {};
        for(var i = Object.keys(liveOrders).length;i--;)
          O[Object.keys(liveOrders).map(k => liveOrders[k])[i].orderId] = Object.keys(liveOrders).map(k => liveOrders[k])[i];
        return O;
    };

    private onMarketTrade = (trade : Models.MarketTrade) => {
        this._openAskOrders = this.tryToMatch(Object.keys(this._openAskOrders).map(k => this._openAskOrders[k]), [trade], Models.Side.Ask);
        this._openBidOrders = this.tryToMatch(Object.keys(this._openBidOrders).map(k => this._openBidOrders[k]), [trade], Models.Side.Bid);

        this._evUp('MarketTradeGateway', new Models.GatewayMarketTrade(trade.price, trade.size, trade.time, false, trade.make_side));
    };

    private recomputePosition = () => {
        this._evUp('PositionGateway', new Models.CurrencyPosition(this._baseAmount, this._baseHeld, Models.Currency.BTC));
        this._evUp('PositionGateway', new Models.CurrencyPosition(this._quoteAmount, this._quoteHeld, Models.Currency.USD));
    };

    private _baseHeld = 0;
    private _quoteHeld = 0;

    constructor(
      private _inputData: (Models.Market | Models.MarketTrade)[],
      private _baseAmount : number,
      private _quoteAmount : number,
      private timeProvider: Utils.IBacktestingTimeProvider,
      private _evOn,
      private _evUp
    ) {}

    public run = () => {
        this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Connected);
        this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected);

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

export class BacktestExchange extends Interfaces.CombinedGateway {
    constructor(private gw: BacktestGateway) {
        super(gw, new BacktestGatewayDetails());
    }

    public run = () => this.gw.run();
};