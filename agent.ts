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
    }

    public brokers = () => {
        return this._brokers;
    };

    public submitOrder = (o : SubmitNewOrder) => {
        try {
            this._brokersByExch[o.exchange].sendOrder(o);
        }
        catch (e) {
            this._log("Exception while sending order", o, e);
        }
    };

    public cancelReplaceOrder = (o : CancelReplaceOrder) => {
        try {
            this._brokersByExch[o.exchange].replaceOrder(o);
        }
        catch (e) {
            this._log("Exception while cancel/replacing order", o, e);
        }
    };

    public cancelOrder = (o : OrderCancel) => {
        try {
            this._brokersByExch[o.exchange].cancelOrder(o);
        }
        catch (e) {
            this._log("Exception while cancelling order", o, e);
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

        this._handleResult(bestResult);

        this._lastBestResult = bestResult;
    };

    private _lastBestResult : Result = null;
    private _handleResult = (result : Result) =>  {
        // cancel
        var action : string;
        if (result == null && this._lastBestResult !== null) {
            action = "STOP";
        }
        // new
        else if (result !== null && this._lastBestResult == null) {
            action = "START";
            // need to set up fill notification & triggering
        }
        // cxl-rpl
        else if (result !== null && this._lastBestResult !== null) {
            action = "MODIFY";
        }
        else {
            throw Error("should not have ever gotten here");
        }

        this._log("%s :: p=%d > %s Rest (%s) %d :: Hide (%s) %d", action, result.profit, Side[result.restSide],
                result.restBroker.name(), result.rest.price, result.hideBroker.name(), result.hide.price);
    };
}