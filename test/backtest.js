"use strict";
exports.__esModule = true;
var assert = require("assert");
var Backtest = require("../src/server/backtest");
var Models = require("../src/share/models");
var moment = require("moment");
describe("BacktestTests", function () {
    var timeProvider;
    beforeEach(function () {
        var t0 = moment.unix(1);
        var t1 = moment.unix(10);
        timeProvider = new Backtest.BacktestTimeProvider(t0, t1);
    });
    it("Should increment time", function () {
        timeProvider.scrollTimeTo(moment.unix(2));
        assert.equal(timeProvider.utcNow().diff(moment.unix(2)), 0);
    });
    it("Should not allow rewinding time", function () {
        timeProvider.scrollTimeTo(moment.unix(6));
        assert.throws(function () { return timeProvider.scrollTimeTo(moment.unix(2)); });
    });
    it("Should handle timeouts", function () {
        var triggered = false;
        timeProvider.setTimeout(function () { return triggered = true; }, moment.duration(4, "seconds"));
        timeProvider.scrollTimeTo(moment.unix(2));
        assert.equal(triggered, false, "should not yet be triggered");
        timeProvider.scrollTimeTo(moment.unix(7));
        assert.equal(triggered, true, "should be triggered");
    });
    it("Should handle timeouts in order", function () {
        var triggeredFirst = false;
        timeProvider.setTimeout(function () { return triggeredFirst = true; }, moment.duration(4, "seconds"));
        var triggeredSecond = false;
        timeProvider.setTimeout(function () { return triggeredSecond = true; }, moment.duration(7, "seconds"));
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
    it("Should handle intervals", function () {
        var nTimes = 0;
        timeProvider.setInterval(function () { return nTimes += 1; }, moment.duration(2, "seconds"));
        timeProvider.scrollTimeTo(moment.unix(9));
        assert.equal(nTimes, 3);
    });
    it("Should handle both intervals and timeouts", function () {
        var nTimes = 0;
        timeProvider.setInterval(function () { return nTimes += 1; }, moment.duration(2, "seconds"));
        var triggeredFirst = false;
        timeProvider.setTimeout(function () { return triggeredFirst = true; }, moment.duration(4, "seconds"));
        var triggeredSecond = false;
        timeProvider.setTimeout(function () { return triggeredSecond = true; }, moment.duration(7, "seconds"));
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
describe("BacktestGatewayTests", function () {
    it("should read market data", function () {
        assert.equal(new Models.MarketSide(1, 2).toString(), 'px=1;size=2');
        var inputData = [
            new Models.Market([new Models.MarketSide(10, 5)], [new Models.MarketSide(20, 5)], moment.unix(1)),
            new Models.Market([new Models.MarketSide(15, 5)], [new Models.MarketSide(20, 5)], moment.unix(10)),
        ];
        var timeProvider = new Backtest.BacktestTimeProvider(moment.unix(1), moment.unix(40));
        var gateway = new Backtest.BacktestGateway(inputData, 10, 5000, timeProvider);
        gateway.MarketData.once(function (m) {
            gateway.sendOrder({
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
        gateway.OrderUpdate.on(function (o) {
            if (o.orderStatus === Models.OrderStatus.Complete) {
                gotTrade = true;
                assert.equal(12, o.lastPrice);
                assert.equal(3, o.lastQuantity);
            }
        });
        /*gateway.PositionUpdate.on(p => {
            console.log(Models.Currency[p.currency], p.amount, p.heldAmount);
        });*/
        gateway.run();
        assert(gotTrade !== false, "never got trade");
    });
});
