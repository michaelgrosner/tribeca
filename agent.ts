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

    constructor(brokers : Array<IBroker>) {
        this._brokers = brokers;

        this._brokers.forEach(b => {
            b.MarketData.on(this.recalcMarkets);
        });
    }

    Active : boolean = false;
    ActiveChanged = new Evt<boolean>();

    changeActiveStatus = (to : boolean) => {
        if (this.Active != to) {
            this.Active = to;
            this.recalcMarkets();
            this._log("changing active status to %o", to);
            this.ActiveChanged.trigger(to);
        }
    };

    private recalcMarkets = () => {
        if (!this.Active) return;

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
            if (bestProfit < r.profit && r.restBroker.exchange() == Exchange.HitBtc) {
                bestProfit = r.profit;
                bestResult = r;
            }
        }

        // TODO: think about sizing, currently doing 0.025 BTC - risk mitigation
        // TODO: some sort of account limits interface
        if (bestResult == null && this._lastBestResult !== null) {
            this.stop(this._lastBestResult);
        }
        else if (bestResult !== null && this._lastBestResult == null) {
            this.start(bestResult);
        }
        else if (bestResult !== null && this._lastBestResult !== null) {
            if (bestResult.restBroker.exchange() != this._lastBestResult.restBroker.exchange()
                    || bestResult.restSide != this._lastBestResult.restSide) {
                this.stop(this._lastBestResult);
                this.start(bestResult);
            }
            else if (bestResult.rest.price !== this._lastBestResult.rest.price) {
                this.modify(bestResult);
            }
            else {
                this.noChange(bestResult);
            }
        }
        else {
            throw Error("should not have ever gotten here");
        }

        this._lastBestResult = bestResult;
    };

    private noChange = (bestResult : Result) => {
        this._log("NO CHANGE :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", bestResult.profit, Side[bestResult.restSide],
                bestResult.restBroker.name(), bestResult.rest.price, bestResult.hideBroker.name(), bestResult.hide.price);
    };

    private modify = (bestResult : Result) => {
        var restExch = bestResult.restBroker.exchange();
        // cxl-rpl live order
        // TODO: think about sizing
        // TODO: only replace this when this exchange price has ticked
        var sent = bestResult.restBroker.replaceOrder(new CancelReplaceOrder(this._activeOrderIds[restExch], 0.025, bestResult.rest.price, restExch));
        this._activeOrderIds[restExch] = sent.sentOrderClientId;

        this._log("MODIFY :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", bestResult.profit, Side[bestResult.restSide],
            bestResult.restBroker.name(), bestResult.rest.price, bestResult.hideBroker.name(), bestResult.hide.price);
    };

    private start = (bestResult : Result) => {
        var restExch = bestResult.restBroker.exchange();
        // set up fill notification
        bestResult.restBroker.OrderUpdate.on(o => Agent.arbFire(o, bestResult.hideBroker));

        // send an order
        var sent = bestResult.restBroker.sendOrder(new OrderImpl(bestResult.restSide, 0.025, OrderType.Limit, bestResult.rest.price, TimeInForce.GTC));
        this._activeOrderIds[restExch] = sent.sentOrderClientId;

        this._log("START :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", bestResult.profit, Side[bestResult.restSide],
            bestResult.restBroker.name(), bestResult.rest.price, bestResult.hideBroker.name(), bestResult.hide.price);
    };

    private stop = (lastBestResult : Result) => {
        // remove fill notification
        lastBestResult.restBroker.OrderUpdate.off(o => Agent.arbFire(o, lastBestResult.hideBroker));

        // cancel open order
        var restExch = lastBestResult.restBroker.exchange();
        lastBestResult.restBroker.cancelOrder(new OrderCancel(this._activeOrderIds[restExch], restExch));
        delete this._activeOrderIds[restExch];

        this._log("STOP :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", lastBestResult.profit,
            Side[lastBestResult.restSide], lastBestResult.restBroker.name(),
            lastBestResult.rest.price, lastBestResult.hideBroker.name(), lastBestResult.hide.price);
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