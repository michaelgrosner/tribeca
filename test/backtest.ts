import assert = require("assert");
import Backtest = require("../src/server/backtest");
import Interfaces = require("../src/server/interfaces");
import Utils = require("../src/server/utils");
import Persister = require("../src/server/persister");
import Models = require("../src/share/models");
import moment = require("moment");

describe("BacktestTests", () => {
    var timeProvider : Backtest.BacktestTimeProvider;

    beforeEach(() => {
        var t0 = moment.unix(1);
        var t1 = moment.unix(10);
        timeProvider = new Backtest.BacktestTimeProvider(t0, t1);
    });

    it("Should increment time", () => {
        timeProvider.scrollTimeTo(moment.unix(2));
        assert.equal(moment(timeProvider.utcNow()).diff(moment.unix(2)), 0);
    });

    it("Should not allow rewinding time", () => {
        timeProvider.scrollTimeTo(moment.unix(6));
        assert.throws(() => timeProvider.scrollTimeTo(moment.unix(2)));
    });

    it("Should handle timeouts", () => {
        var triggered = false;
        timeProvider.setTimeout(() => triggered = true, moment.duration(4, "seconds"));

        timeProvider.scrollTimeTo(moment.unix(2));
        assert.equal(triggered, false, "should not yet be triggered");

        timeProvider.scrollTimeTo(moment.unix(7));
        assert.equal(triggered, true, "should be triggered");
    });

    it("Should handle timeouts in order", () => {
        var triggeredFirst = false;
        timeProvider.setTimeout(() => triggeredFirst = true, moment.duration(4, "seconds"));

        var triggeredSecond = false;
        timeProvider.setTimeout(() => triggeredSecond = true, moment.duration(7, "seconds"));

        timeProvider.scrollTimeTo(moment.unix(2));
        assert.equal(triggeredFirst, false, "1 should not yet be triggered");
        assert.equal(triggeredSecond, false, "2 should not yet be triggered");

        timeProvider.scrollTimeTo(moment.unix(7));
        assert.equal(triggeredFirst, true, "1 should be triggered");
        assert.equal(triggeredSecond, false, "2 should not yet be triggered");

        timeProvider.scrollTimeTo(moment.unix(9));
        assert.equal(triggeredFirst, true, "1 should be triggered");
        assert.equal(triggeredSecond, true, "2 should be triggered");
    });

    it("Should handle intervals", () => {
        var nTimes = 0;
        timeProvider.setInterval(() => nTimes += 1, moment.duration(2, "seconds"));

        timeProvider.scrollTimeTo(moment.unix(9));
        assert.equal(nTimes, 3);
    });

    it("Should handle both intervals and timeouts", () => {
        var nTimes = 0;
        timeProvider.setInterval(() => nTimes += 1, moment.duration(2, "seconds"));

        var triggeredFirst = false;
        timeProvider.setTimeout(() => triggeredFirst = true, moment.duration(4, "seconds"));

        var triggeredSecond = false;
        timeProvider.setTimeout(() => triggeredSecond = true, moment.duration(7, "seconds"));

        timeProvider.scrollTimeTo(moment.unix(2));
        assert.equal(nTimes, 0);
        assert.equal(triggeredFirst, false, "1 should not yet be triggered");
        assert.equal(triggeredSecond, false, "2 should not yet be triggered");

        timeProvider.scrollTimeTo(moment.unix(7));
        assert.equal(nTimes, 2);
        assert.equal(triggeredFirst, true, "1 should be triggered");
        assert.equal(triggeredSecond, false, "2 should not yet be triggered");

        timeProvider.scrollTimeTo(moment.unix(9));
        assert.equal(nTimes, 3);
        assert.equal(triggeredFirst, true, "1 should be triggered");
        assert.equal(triggeredSecond, true, "2 should be triggered");
    });
});

describe("BacktestGatewayTests", () => {
    it("should read market data", () => {
        assert.equal(new Models.MarketSide(1, 2).toString(), 'px=1;size=2');

        var inputData : Array<Models.Market | Models.MarketTrade> = [
            new Models.Market([new Models.MarketSide(10, 5)], [new Models.MarketSide(20, 5)], moment.unix(1).toDate()),
            new Models.Market([new Models.MarketSide(15, 5)], [new Models.MarketSide(20, 5)], moment.unix(10).toDate()),
        ];

        var timeProvider = new Backtest.BacktestTimeProvider(moment.unix(1), moment.unix(40));
        var gateway = new Backtest.BacktestGateway(inputData, 10, 5000, timeProvider);

        gateway.MarketData.on(m => {
            gateway.sendOrder(<Models.OrderStatusReport>{
                orderId: "A",
                side: Models.Side.Ask,
                quantity: 3,
                type: Models.OrderType.Limit,
                price: 12,
                timeInForce: Models.TimeInForce.GTC,
                exchange: Models.Exchange.Null,
                preferPostOnly: false
            });
        });

        var gotTrade = false;
        gateway.OrderUpdate.on(o => {
            if (o.orderStatus === Models.OrderStatus.Complete) {
                gotTrade = true;
                assert.equal(12, o.lastPrice);
                assert.equal(3, o.lastQuantity);
            }
        });

        // gateway.PositionUpdate.on(p => {
            // console.log(Models.Currency[p.currency], p.amount, p.heldAmount);
        // });

        gateway.run();

        assert(gotTrade !== false, "never got trade");
    });
});
