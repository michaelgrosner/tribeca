/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import util = require("util");
import express = require("express");
import Q = require("q");
import Persister = require("./persister");

export class StandaloneHttpPublisher<T> {
    constructor(
        private _wrapped: Messaging.IPublish<T>,
        private route: string,
        private _httpApp: express.Application,
        private _persister: Persister.ILoadAll<T>,
        snapshot: () => T[] = null) {
        this.registerSnapshot(snapshot);

        _httpApp.get("/data/" + route, (req: express.Request, res: express.Response) => {
            var rawMax = req.param("max", null);
            var max = (rawMax === null ? null : parseInt(rawMax));

            var handler = (d: T[]) => {
                if (max !== null)
                    d = _.last(d, max);
                res.json(d);
            };

            _persister.loadAll(max).then(handler);
        });
    }

    public publish = this._wrapped.publish;

    public registerSnapshot = (generator: () => T[]) => {
        return this._wrapped.registerSnapshot(generator);
    }
}