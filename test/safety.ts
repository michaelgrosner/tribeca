/// <reference path="../typings/tsd.d.ts" />
/// <reference path="../src/service/safety.ts" />

import assert = require("assert");
import Safety = require("../src/service/safety");
import Interfaces = require("../src/service/interfaces");
import Utils = require("../src/service/utils");
import Persister = require("../src/service/persister");
import Models = require("../src/common/models");
import Messaging = require("../src/common/messaging");
import Moment = require("moment");

class StubRepo<T> implements Interfaces.IRepository<T> {
    updateParameters(newParams: T): void {}
    NewParameters = new Utils.Evt();
    latest : T;
}

class StubTradeBroker implements Interfaces.ITradeBroker {
    Trade = new Utils.Evt<Models.Trade>();
}

class StubPublisher<T> implements Messaging.IPublish<T> {
    publish = (p1: T) => {};
    registerSnapshot = (generator : () => T[]) => this;
}

class StubPersister<T> implements Persister.IPersist<T> {
    persist(data: T): void { }
}

describe("SafetySettings", () => {
    it("Should trigger safety settings", () => {
        var mockSafetyParameters = new StubRepo<Models.SafetySettings>();
        mockSafetyParameters.latest = new Models.SafetySettings(2, 500, 1);
        var mockTradeBroker = new StubTradeBroker();
        var mockQlRepo = new StubRepo<Models.QuotingParameters>();
        mockQlRepo.latest = new Models.QuotingParameters(null, 1, null, null, null, null, null, null);
        var safety = new Safety.SafetyCalculator(mockSafetyParameters, mockTradeBroker, mockQlRepo, new StubPublisher(), new StubPersister());

        var tradesInput = [
            ["1", Moment.duration(0, "seconds"), 257.01, .0577, Models.Side.Bid, 14.829],
            ["2", Moment.duration(15, "seconds"), 257.01, .0211, Models.Side.Bid, 5.423],
            ["3", Moment.duration(3, "minutes"), 257.20, .5, Models.Side.Ask, 128.60],
            ["4", Moment.duration(3, "minutes"), 257.01, .5, Models.Side.Bid, 128.505],
            ["5", Moment.duration(4, "minutes"), 257.20, .5, Models.Side.Ask, 128.60],
        ];

        var pair = new Models.CurrencyPair(Models.Currency.BTC, Models.Currency.USD);
        var convertToTrade = t => new Models.Trade(t[0], Moment(Utils.date().diff(t[1])), Models.Exchange.Coinbase, pair, t[2], t[3], t[4], t[5]);

        tradesInput.forEach(ti => {
            var t = convertToTrade(ti);
            mockTradeBroker.Trade.trigger(t);
        });

        assert(Math.abs(safety.latest.combined - 0.4212) < 1e-4);
        assert(Math.abs(safety.latest.buy) < 1e-4);
        assert(Math.abs(safety.latest.combined - 0.4212) < 1e-4);
    });
});