/// <reference path="../typings/tsd.d.ts" />
/// <reference path="../src/service/safety.ts" />

import assert = require("assert");
import Safety = require("../src/service/safety");
import Interfaces = require("../src/service/interfaces");
import Utils = require("../src/service/utils");
import Models = require("../src/common/models");

class StubSafetyRepo implements Interfaces.IRepository<Models.SafetySettings> {
    NewParameters = new Utils.Evt();
    latest = Models.SafetySettings;
}

class StubQLRepo implements Interfaces.IRepository<Models.QuotingParameters> {
    NewParameters = new Utils.Evt();
    latest = Models.QuotingParameters;
}

class StubTradeBroker implements Interfaces.ITradeBroker {
    Trade = new Utils.Evt<Models.Trade>();
}

describe("SafetySettings", () => {
    it("Should trigger safety settings", () => {
        var mockSafetyParameters = new StubSafetyRepo();
        mockSafetyParameters.latest = new Models.SafetySettings(2, 500, 1);
        var mockTradeBroker = new StubTradeBroker();
        var mockQlRepo = new StubQLRepo();
        mockQlRepo.latest = new Models.QuotingParameters(null, 1, null, null, null, null, null);
        var mockMessages = {publish: m => {}};
        var safety = new Safety.SafetySettingsManager(mockSafetyParameters, mockTradeBroker, mockQlRepo, mockMessages);

        var safetyViolated = false;
        safety.SafetySettingsViolated.on(() => safetyViolated = true);

        mockTradeBroker.Trade.trigger(new Models.Trade(null, Utils.date(), null, null, 1, 1, Models.Side.Bid, null));
        mockTradeBroker.Trade.trigger(new Models.Trade(null, Utils.date(), null, null, 1, 1, Models.Side.Bid, null));

        assert(safety.canEnable === false);
        assert(safetyViolated === true);
    });
});