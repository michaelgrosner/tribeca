/// <reference path="../common/models.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />

import Config = require("./config");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

export class SafetySettingsRepository extends Interfaces.Repository<Models.SafetySettings> {
    constructor() {
        super("ssr",
            (s : Models.SafetySettings) => s.tradesPerMinute > 0,
            (a : Models.SafetySettings, b : Models.SafetySettings) => Math.abs(a.tradesPerMinute - b.tradesPerMinute) >= 0,
            new Models.SafetySettings(2)
        );
    }
}

export class SafetySettingsManager {
    constructor(private _repo : SafetySettingsRepository) {

    }
}