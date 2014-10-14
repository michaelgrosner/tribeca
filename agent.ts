/// <reference path="models.ts" />

class Result {
    constructor(public restSide: Side, public restBroker: IBroker,
                public hideBroker: IBroker, public profit: number) {}
}

class Agent {
    _brokers : Array<IBroker>;
    _log : Logger = log("Hudson:Agent");
    _ui : UI;
    _brokersByExch : { [exchange: number]: IBroker} = {};

    constructor(brokers : Array<IBroker>, ui : UI) {
        this._brokers = brokers;
        this._ui = ui;

        this._brokers.forEach(b => {
            b.MarketData.on(this.onNewMarketData);
            b.OrderUpdate.on(this._ui.sendOrderStatusUpdate);
        });

        for (var i = 0; i < brokers.length; i++)
            this._brokersByExch[brokers[i].exchange()] = brokers[i];

        this._ui.NewOrder.on(this.submitOrder);
    }

    private submitOrder = (o : SubmitNewOrder) => {
        try {
            this._brokersByExch[o.exchange].sendOrder(o);
        }
        catch (e) {
            this._log("Exception while sending order", o, e);
        }
    };

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
                        results.push(new Result(Side.Bid, restBroker, hideBroker, pBid * p));
                    }

                    if (pAsk > 0) {
                        var p = Math.min(restTop.ask.size, hideTop.ask.size);
                        results.push(new Result(Side.Ask, restBroker, hideBroker, pAsk * p));
                    }
                })
            });

        if (results.length == 0) return;

        var bestResult : Result;
        var bestProfit: number = Number.MIN_VALUE;
        for (var i = 0; i < results.length; i++) {
            var r = results[i];
            if (bestProfit < r.profit) {
                bestProfit = r.profit;
                bestResult = r;
            }
        }

        //bestResult.restBroker.ensureOrderAt(bestResult.rest);
        //bestResult.hideBroker.waitForPriceAt(bestResult.hide);

        this._log("Trigger p=%d > %s Rest (%s) %j :: Hide (%s) %j", bestResult.profit, Side[bestResult.restSide],
            bestResult.restBroker.name(), bestResult.hideBroker.name());


    };
}