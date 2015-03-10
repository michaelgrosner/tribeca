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
    private _snapshot : (limit? : number) => T[]|Q.Promise<T[]> = null;

    constructor(
            private route : string,
            private _httpApp : express.Application,
            snapshot : (limit? : number) => T[]|Q.Promise<T[]> = null) {
        this.registerSnapshot(snapshot);

        _httpApp.get("/data/" + route, (req : express.Request, res : express.Response) => {
            var rawMax = req.param("max", null);
            var max = (rawMax === null ? null : parseInt(rawMax));

            var handler = (d : T[]) => {
                if (max !== null)
                    d = _.last(d, max);
                res.json(d);
            }

            var data : T[]|Q.Promise<T[]> = this._snapshot(max);
            if (!data.then) {
                handler(data);
            }
            else {
                data.then(handler);
            }
        });
    }

    public registerSnapshot = (generator : (limit? : number) => T[]|Q.Promise<T[]>) =>  {
        this._snapshot = generator;
    }
}