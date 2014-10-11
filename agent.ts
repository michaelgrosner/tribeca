/// <reference path="models.ts" />

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
        var activeBrokers = this._brokers.filter(b => b.currentBook() != null);

        if (activeBrokers.length <= 1)
            return;

        var results = [];
        activeBrokers.filter(b => b.makeFee() < 0)
            .forEach(restBroker => {
                activeBrokers.forEach(hideBroker => {
                    if (restBroker.exchange() == hideBroker.exchange()) return;

                    // need to determine whether or not I'm already on the market
                    var restTop = restBroker.currentBook().top;
                    var hideTop = hideBroker.currentBook().top;

                    // TODO: verify formulae
                    var pBid = -(1 + restBroker.makeFee()) * restTop.bidPrice + (1 + hideBroker.takeFee()) * hideTop.bidPrice;
                    var pAsk = +(1 + restBroker.makeFee()) * restTop.askPrice - (1 + hideBroker.takeFee()) * hideTop.askPrice;

                    if (pBid > 0) {
                        var p = Math.min(restTop.bidSize, hideTop.bidSize);
                        results.push({restSide: Side.Bid, restBroker: restBroker, hideBroker: hideBroker, profit: pBid * p});
                    }

                    if (pAsk > 0) {
                        var p = Math.min(restTop.askSize, hideTop.askSize);
                        results.push({restSide: Side.Ask, restBroker: restBroker, hideBroker: hideBroker, profit: pAsk * p});
                    }
                })
            });

        results.forEach(r => {
            var top2 = r.restBroker.currentBook().top[r.restSide == Side.Bid ? "bidPrice" : "askPrice"];
            var top3 = r.hideBroker.currentBook().top[r.restSide == Side.Bid ? "bidPrice" : "askPrice"];
            this._log("Trigger p=%d > %s Rest (%s) %j :: Hide (%s) %j", r.profit,
                Side[r.restSide], r.restBroker.name(), top2,
                r.hideBroker.name(), top3);
        });

        this._ui.sendUpdatedMarket(book);
    };
}