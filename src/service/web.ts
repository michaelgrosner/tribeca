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

export class HttpPublisher<T> implements Messaging.IPublish<T> {
    private _snapshot : () => T[] = null;

    constructor(
            private route : string,
            private _wrapped : Messaging.IPublish<T>,
            private _httpApp : express.Application,
            snapshot : () => T[] = null) {
        this.registerSnapshot(snapshot);

        _httpApp.get("/data/" + route, (req : express.Request, res : express.Response) => {
            var data = this._snapshot();

            var max = req.param("max", null);
            if (max !== null) {
                data = _.last(data, parseInt(max));
            }

            res.json(data);
        });
    }

    public publish = this._wrapped.publish;

    public registerSnapshot = (generator : () => T[]) =>  {
        this._snapshot = generator;
        return this._wrapped.registerSnapshot(generator);
    }
}

export class StandaloneHttpPublisher<T> {
    private _snapshot : () => T[] = null;

    constructor(
            private route : string,
            private _httpApp : express.Application,
            snapshot : () => T[] = null) {
        this.registerSnapshot(snapshot);

        _httpApp.get("/data/" + route, (req : express.Request, res : express.Response) => {
            var data = this._snapshot();

            var max = req.param("max", null);
            if (max !== null) {
                data = _.last(data, parseInt(max));
            }

            res.json(data);
        });
    }

    public registerSnapshot = (generator : () => T[]) =>  {
        this._snapshot = generator;
    }
}