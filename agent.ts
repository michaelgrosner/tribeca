/// <reference path="models.ts" />

class Result {
    constructor(public restSide: Side, public restBroker: IBroker,
                public hideBroker: IBroker, public profit: number,
                public rest: MarketSide, public hide: MarketSide) {}
}

class OrderBrokerAggregator {
    _brokers : Array<IBroker>;
    _log : Logger = log("Hudson:BrokerAggregator");
    _ui : UI;
    _brokersByExch : { [exchange: number]: IBroker} = {};

    OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();

    constructor(brokers : Array<IBroker>, ui : UI) {
        this._brokers = brokers;
        this._ui = ui;

        this._brokers.forEach(b => {
            b.OrderUpdate.on(u => {
                this.OrderUpdate.trigger(u);
                this._ui.sendOrderStatusUpdate(u);
            });
        });

        for (var i = 0; i < brokers.length; i++)
            this._brokersByExch[brokers[i].exchange()] = brokers[i];

        this._ui.NewOrder.on(this.submitOrder);
        this._ui.CancelOrder.on(this.cancelOrder);
        this._ui.ReplaceOrder.on(this.cancelReplaceOrder);
    }

    public brokers = () => {
        return this._brokers;
    };

    public submitOrder = (o : SubmitNewOrder) => {
        try {
            this._brokersByExch[o.exchange].sendOrder(o);
        }
        catch (e) {
            this._log("Exception while sending order", o, e, e.stack);
        }
    };

    public cancelReplaceOrder = (o : CancelReplaceOrder) => {
        try {
            this._brokersByExch[o.exchange].replaceOrder(o);
        }
        catch (e) {
            this._log("Exception while cancel/replacing order", o, e, e.stack);
        }
    };

    public cancelOrder = (o : OrderCancel) => {
        try {
            this._brokersByExch[o.exchange].cancelOrder(o);
        }
        catch (e) {
            this._log("Exception while cancelling order", o, e, e.stack);
        }
    };
}

class Agent {
    _brokers : Array<IBroker>;
    _log : Logger = log("Hudson:Agent");
    _ui : UI;

    constructor(brokers : Array<IBroker>, ui : UI) {
        this._brokers = brokers;
        this._ui = ui;

        this._brokers.forEach(b => {
            b.MarketData.on(this.onNewMarketData);
        });
    }

    private onNewMarketData = (book : MarketBook) => {
        this.recalcMarkets(book);
        this._ui.sendUpdatedMarket(book);
    };

    private recalcMarkets = (book : MarketBook) => {
        var activeBrokers = this._brokers.filter(b => b.currentBook() != null);

        if (activeBrokers.length <= 1)
            return;

        var results : Result[] = [];
        activeBrokers.filter(b => b.makeFee() < 0)
            .forEach(restBroker => {
                activeBrokers.forEach(hideBroker => {
                    if (restBroker.exchange() == hideBroker.exchange()) return;

                    // need to determine whether or not I'm already on the market
                    var restTop = restBroker.currentBook().top;
                    var hideTop = hideBroker.currentBook().top;

                    var pBid = -(1 + restBroker.makeFee()) * restTop.bid.price + (1 + hideBroker.takeFee()) * hideTop.bid.price;
                    var pAsk = +(1 + restBroker.makeFee()) * restTop.ask.price - (1 + hideBroker.takeFee()) * hideTop.ask.price;

                    if (pBid > 0) {
                        var p = Math.min(restTop.bid.size, hideTop.bid.size);
                        results.push(new Result(Side.Bid, restBroker, hideBroker, pBid * p, restTop.bid, hideTop.bid));
                    }

                    if (pAsk > 0) {
                        var p = Math.min(restTop.ask.size, hideTop.ask.size);
                        results.push(new Result(Side.Ask, restBroker, hideBroker, pAsk * p, restTop.ask, hideTop.ask));
                    }
                })
            });

        var bestResult : Result = null;
        var bestProfit: number = Number.MIN_VALUE;
        for (var i = 0; i < results.length; i++) {
            var r = results[i];
            if (bestProfit < r.profit) {
                bestProfit = r.profit;
                bestResult = r;
            }
        }

        // TODO: think about sizing, currently doing 0.025 BTC - risk mitigation
        // TODO: some sort of account limits interface
        var action : string;
        var restExch = bestResult.restBroker.exchange();
        if (bestResult == null && this._lastBestResult !== null) {
            action = "STOP";
            // remove fill notification
            bestResult.restBroker.OrderUpdate.off(o => Agent.arbFire(o, bestResult.hideBroker));

            // cancel open order
            bestResult.restBroker.cancelOrder(new OrderCancel(this._activeOrderIds[restExch], restExch));
            delete this._activeOrderIds[restExch];
        }
        // new
        else if (bestResult !== null && this._lastBestResult == null) {
            action = "START";
            // set up fill notification
            bestResult.restBroker.OrderUpdate.on(o => Agent.arbFire(o, bestResult.hideBroker));

            // send an order
            var sent = bestResult.restBroker.sendOrder(new OrderImpl(bestResult.restSide, 0.025, OrderType.Limit, bestResult.rest.price, TimeInForce.GTC));
            this._activeOrderIds[restExch] = sent.sentOrderClientId;
        }
        // cxl-rpl
        else if (bestResult !== null && this._lastBestResult !== null && bestResult.rest.price !== this._lastBestResult.rest.price) {
            action = "MODIFY";
            // cxl-rpl live order
            // TODO: think about sizing
            // TODO: only replace this when this exchange price has ticked
            var sent = bestResult.restBroker.replaceOrder(new CancelReplaceOrder(this._activeOrderIds[restExch], 0.025, bestResult.rest.price, restExch));
            this._activeOrderIds[restExch] = sent.sentOrderClientId;
        }
        // no change, rest broker price has remained the same
        else if (bestResult !== null && this._lastBestResult !== null) {
            action = "NO CHANGE";
        }
        else {
            throw Error("should not have ever gotten here");
        }

        this._log("%s :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", action, bestResult.profit, Side[bestResult.restSide],
                bestResult.restBroker.name(), bestResult.rest.price, bestResult.hideBroker.name(), bestResult.hide.price);

        this._lastBestResult = bestResult;
    };

    private static arbFire(o : OrderStatusReport, hideBroker : IBroker) {
        if (o.orderStatus !== OrderStatus.Filled || o.orderStatus !== OrderStatus.PartialFill) return;
        // TODO: think about sizing
        var px = o.side == Side.Ask ? hideBroker.currentBook().top.ask.price : hideBroker.currentBook().top.bid.price;
        hideBroker.sendOrder(new OrderImpl(o.side, o.quantity, o.type, px, TimeInForce.IOC));
    }

    private _lastBestResult : Result = null;
    private _activeOrderIds : { [ exch : number] : string} = {};
}