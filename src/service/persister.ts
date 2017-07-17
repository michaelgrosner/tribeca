/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="interfaces.ts"/>
/// <reference path="config.ts"/>

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import mongodb = require('mongodb');
import moment = require('moment');
import Interfaces = require("./interfaces");
import Config = require("./config");
import log from "./logging";

export function loadDb(config: Config.IConfigProvider) {
    return mongodb.MongoClient.connect(config.GetString("MongoDbUrl"));
}

export interface Persistable {
    time?: Date;
    pair?: Models.CurrencyPair;
    exchange?: Models.Exchange;
}

export interface IPersist<T> {
    persist(data: T): void;
}

export interface ILoadLatest<T> extends IPersist<T> {
    loadLatest(): Promise<T>;
}

export interface ILoadAll<T> extends IPersist<T> {
    loadAll(limit?: number, query?: Object): Promise<T[]>;
}

export class RepositoryPersister<T> implements ILoadLatest<T> {
    private _log = log("tribeca:exchangebroker:repopersister");

    public loadLatest = async (): Promise<T> => {
        const selector = { exchange: this._exchange, pair: this._pair };
        const docs = await this.collection.find(selector)
                .limit(1)
                .project({ _id: 0 })
                .sort({ $natural: -1 })
                .toArray();

        if (docs.length === 0) 
            return this._defaultParameter;
        
        var v = <T>_.defaults(docs[0], this._defaultParameter);
        return this.converter(v);
    };

    public persist = async (report: T) => {
        try {
            await this.collection.insertOne(this.converter(report));
            this._log.info("Persisted", report);
        } catch (err) {
            this._log.error(err, "Unable to insert", this._dbName, report);
        }
    };

    private converter = (x: any) : T => {
        if (typeof x.exchange === "undefined")
            x.exchange = this._exchange;
        if (typeof x.pair === "undefined")
            x.pair = this._pair;
        return x;
    };

    constructor(
        private collection: mongodb.Collection,
        private _defaultParameter: T,
        private _dbName: string,
        private _exchange: Models.Exchange,
        private _pair: Models.CurrencyPair) {
    }
}

export class Persister<T extends Persistable> implements ILoadAll<T> {
    private _log = log("persister");

    public loadAll = (limit?: number, query?: any): Promise<T[]> => {
        const selector: Object = { exchange: this._exchange, pair: this._pair };
        _.assign(selector, query);
        return this.loadInternal(selector, limit);
    };

    private loadInternal = async (selector: Object, limit?: number) : Promise<T[]> => {
        let query = this.collection.find(selector, {_id: 0});

        if (limit !== null) {
            const count = await this.collection.count(selector);
            query = query.limit(limit);
            if (count !== 0)
                query = query.skip(Math.max(count - limit, 0));
        }

        const loaded = _.map(await query.toArray(), this.converter);

        this._log.info({
            selector: selector,
            limit: limit,
            nLoaded: loaded.length,
            dbName: this._dbName
        }, "load docs completed");

        return loaded;
    };

    private _persistQueue : T[] = [];
    public persist = (report: T) => {
        this._persistQueue.push(report);
    };

    private converter = (x: T) : T => {
        if (typeof x.time === "undefined")
            x.time = new Date();
        if (typeof x.exchange === "undefined")
            x.exchange = this._exchange;
        if (typeof x.pair === "undefined")
            x.pair = this._pair;
        return x;
    };

    constructor(
        time: Utils.ITimeProvider,
        private collection: mongodb.Collection,
        private _dbName: string,
        private _exchange: Models.Exchange,
        private _pair: Models.CurrencyPair) {
            this._log = log("persister:"+_dbName);

            time.setInterval(async () => {
                if (this._persistQueue.length === 0) 
                    return;

                const docs = _.map(this._persistQueue, this.converter);

                try {
                    const result = await collection.insertMany(docs);
                    if (result.result && result.result.ok) {
                        this._persistQueue.length = 0;
                    }
                    else {
                        this._log.warn("Unable to insert, retrying soon", this._dbName, this._persistQueue);
                    }
                } catch (err) {
                    this._log.error(err, "Unable to insert, retrying soon", this._dbName, this._persistQueue);
                }
            }, moment.duration(10, "seconds"));
    }
}