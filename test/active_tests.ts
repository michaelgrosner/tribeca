/// <reference path="../typings/tsd.d.ts" />
/// <reference path="../src/service/safety.ts" />

import assert = require("assert");
import Agent = require("../src/service/arbagent");
import Interfaces = require("../src/service/interfaces");
import Utils = require("../src/service/utils");
import NullGw = require("../src/service/nullgw");
import Models = require("../src/common/models");
import Moment = require("moment");

class MockSafetyNotifier implements Safety.ISafetyManager {
    SafetySettingsViolated = new Utils.Evt<any>();
    SafetyViolationCleared = new Utils.Evt<any>();
    canEnable = false;
}

class MockBrokerConnectivity implements Interfaces.IBrokerConnectivity {
    connectStatus = Models.ConnectivityStatus;
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>;
}

describe("ActiveRepository", () => {
    it("Should handle disconnect/reconnects", () => {
        var activeRepo = new Agent.ActiveRepository(false, new MockSafetyNotifier(), new MockBrokerConnectivity(), null, null);
    });
});