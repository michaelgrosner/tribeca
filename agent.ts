/// <reference path="models.ts" />

class Result {
    constructor(public restSide: Side, public restBroker: IBroker,
                public hideBroker: IBroker, public profit: number,
                public rest: MarketSide, public hide: MarketSide,
                public size: number) {}
}

class MarketDataAggregator {
    MarketData : Evt<MarketBook> = new Evt<MarketBook>();

    constructor(private _brokers : Array<IBroker>) {
        this._brokers.forEach(b => {
            b.MarketData.on(m => this.MarketData.trigger(m));
        });
    }
}

class OrderBrokerAggregator {
    _log : Logger = log("tribeca:brokeraggregator");
    _brokersByExch : { [exchange: number]: IBroker} = {};

    OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();

    constructor(private _brokers : Array<IBroker>) {

        this._brokers.forEach(b => {
            b.OrderUpdate.on(o => this.OrderUpdate.trigger(o));
        });

        for (var i = 0; i < brokers.length; i++)
            this._brokersByExch[brokers[i].exchange()] = brokers[i];
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
    _log : Logger = log("tribeca:agent");

    constructor(private _brokers : Array<IBroker>,
                private _mdAgg : MarketDataAggregator,
                private _orderAgg : OrderBrokerAggregator) {
        _mdAgg.MarketData.on(this.recalcMarkets);
    }

    Active : boolean = false;
    ActiveChanged = new Evt<boolean>();

    changeActiveStatus = (to : boolean) => {
        if (this.Active != to) {
            this.Active = to;

            if (this.Active) {
                this.recalcMarkets();
            }
            else if (!this.Active && this._lastBestResult != null) {
                this.stop(this._lastBestResult, true);
            }

            this._log("changing active status to %o", to);
            this.ActiveChanged.trigger(to);
        }
    };

    private recalcMarkets = () => {
        if (!this.Active) return;

        var activeBrokers = this._brokers.filter(b => b.currentBook() != null);

        if (activeBrokers.length <= 1)
            return;

        var bestResult : Result = null;
        var bestProfit: number = Number.MIN_VALUE;
        activeBrokers.forEach(restBroker => {
            activeBrokers.forEach(hideBroker => {
                if (restBroker.exchange() == hideBroker.exchange()) return;

                // need to determine whether or not I'm already on the market
                var restTop = restBroker.currentBook().top;
                var hideTop = hideBroker.currentBook().top;

                var bidSize = Math.min(.025, restTop.bid.size, hideTop.bid.size);
                var pBid = bidSize * (-(1 + restBroker.makeFee()) * restTop.bid.price + (1 + hideBroker.takeFee()) * hideTop.bid.price);

                var askSize = Math.min(.025, restTop.ask.size, hideTop.ask.size);
                var pAsk = askSize * (+(1 + restBroker.makeFee()) * restTop.ask.price - (1 + hideBroker.takeFee()) * hideTop.ask.price);

                if (pBid > bestProfit && pBid > 0) {
                    bestProfit = pBid;
                    bestResult = new Result(Side.Bid, restBroker, hideBroker, pBid, restTop.bid, hideTop.bid, bidSize);
                }

                if (pAsk > bestProfit && pAsk > 0) {
                    bestProfit = pAsk;
                    bestResult = new Result(Side.Ask, restBroker, hideBroker, pAsk, restTop.ask, hideTop.ask, askSize);
                }
            })
        });

        // TODO: think about sizing, currently doing 0.025 BTC - risk mitigation
        // TODO: some sort of account limits interface
        if (bestResult == null && this._lastBestResult !== null) {
            this.stop(this._lastBestResult, true);
        }
        else if (bestResult !== null && this._lastBestResult == null) {
            this.start(bestResult);
        }
        else if (bestResult !== null && this._lastBestResult !== null) {
            if (bestResult.restBroker.exchange() != this._lastBestResult.restBroker.exchange()
                    || bestResult.restSide != this._lastBestResult.restSide) {
                this.stop(this._lastBestResult, true);
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
            this._log("NOTHING");
        }
    };

    private noChange = (r : Result) => {
        this._log("NO CHANGE :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Side[r.restSide],
                r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);
    };

    private modify = (r : Result) => {
        var restExch = r.restBroker.exchange();
        // cxl-rpl live order
        var sent = r.restBroker.replaceOrder(new CancelReplaceOrder(this._activeOrderIds[restExch], r.size, r.rest.price, restExch));
        this._activeOrderIds[restExch] = sent.sentOrderClientId;

        this._log("MODIFY :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Side[r.restSide],
            r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);

        this._lastBestResult = r;
    };

    private start = (r : Result) => {
        var restExch = r.restBroker.exchange();
        // set up fill notification
        r.restBroker.OrderUpdate.on(this.arbFire);

        // send an order
        var sent = r.restBroker.sendOrder(new OrderImpl(r.restSide, r.size, OrderType.Limit, r.rest.price, TimeInForce.GTC));
        this._activeOrderIds[restExch] = sent.sentOrderClientId;

        this._log("START :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", r.profit, Side[r.restSide],
            r.restBroker.name(), r.rest.price, r.hideBroker.name(), r.hide.price);

        this._lastBestResult = r;
    };

    private stop = (lr : Result, sendCancel : boolean) => {
        // remove fill notification
        lr.restBroker.OrderUpdate.off(this.arbFire);

        // cancel open order
        var restExch = lr.restBroker.exchange();
        if (sendCancel) lr.restBroker.cancelOrder(new OrderCancel(this._activeOrderIds[restExch], restExch));
        delete this._activeOrderIds[restExch];

        this._log("STOP :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", lr.profit,
            Side[lr.restSide], lr.restBroker.name(),
            lr.rest.price, lr.hideBroker.name(), lr.hide.price);

        this._lastBestResult = null;
    };

    private arbFire = (o : OrderStatusReport) => {
        if (o.lastQuantity == null || o.lastQuantity == undefined)
            return;

        var hideBroker = this._lastBestResult.hideBroker;
        var px = o.side == Side.Ask
            ? hideBroker.currentBook().top.ask.price
            : hideBroker.currentBook().top.bid.price;
        hideBroker.sendOrder(new OrderImpl(o.side, o.lastQuantity, o.type, px, TimeInForce.IOC));

        this._log("ARBFIRE :: %s for %d at %d on %s", Side[o.side], o.lastQuantity, px, Exchange[hideBroker.exchange()]);

        this.stop(this._lastBestResult, o.orderStatus == OrderStatus.Complete);
    };

    private _lastBestResult : Result = null;
    private _activeOrderIds : { [ exch : number] : string} = {};
}