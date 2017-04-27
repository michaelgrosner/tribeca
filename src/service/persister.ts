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
import Q = require("q");
import moment = require('moment');
import Interfaces = require("./interfaces");
import Config = require("./config");

export function loadDb(config: Config.IConfigProvider) {
    var deferred = Q.defer<mongodb.Db>();
    mongodb.MongoClient.connect(config.GetString("MongoDbUrl"), (err, db) => {
        if (err) deferred.reject(err);
        else deferred.resolve(db);
    });
    return deferred.promise;
}

export interface Persistable {
    time?: moment.Moment|Date;
    pair?: Models.CurrencyPair;
    exchange?: Models.Exchange;
}

export class LoaderSaver {
    public loader = (x: Persistable) => {
        if (typeof x.time !== "undefined")
            x.time = moment.isMoment(x.time) ? x.time : moment(x.time);
        if (typeof x.exchange === "undefined")
            x.exchange = this._exchange;
        if (typeof x.pair === "undefined")
            x.pair = this._pair;
    };

    public saver = (x: Persistable) => {
        if (typeof x.time !== "undefined")
            x.time = (moment.isMoment(x.time) ? <moment.Moment>x.time : moment(x.time)).toDate();
        if (typeof x.exchange === "undefined")
            x.exchange = this._exchange;
        if (typeof x.pair === "undefined")
            x.pair = this._pair;
    };

    constructor(private _exchange: Models.Exchange, private _pair: Models.CurrencyPair) { }
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

export class RepositoryPersister<T extends Persistable> implements ILoadLatest<T> {
    private _log = Utils.log("tribeca:exchangebroker:repopersister");

    public loadLatest = async (): Promise<T> => {
        const coll = await this.collection;

        const selector = { exchange: this._exchange, pair: this._pair };
        const docs = await coll.find(selector)
                .limit(1)
                .project({ _id: 0 })
                .sort({ $natural: -1 })
                .toArray();

        if (docs.length === 0) 
            return this._defaultParameter;
        
        var v = <T>_.defaults(docs[0], this._defaultParameter);
        this._loader(v); 
        return v;
    };

    public persist = (report: T) => {
        this._saver(report);
        this.collection.then(coll => {
            coll.insertOne(report, err => {
                if (err)
                    this._log.error(err, "Unable to insert", this._dbName, report);
                else
                    this._log.info("Persisted", report);
            });
        }).done();
    };

    collection: Q.Promise<mongodb.Collection>;
    constructor(
        db: Q.Promise<mongodb.Db>,
        private _defaultParameter: T,
        private _dbName: string,
        private _exchange: Models.Exchange,
        private _pair: Models.CurrencyPair,
        private _loader: (p: Persistable) => void,
        private _saver: (p: Persistable) => void) {
        this.collection = db.then(db => db.collection(this._dbName));
    }
}

export class Persister<T extends Persistable> implements ILoadAll<T> {
    private _log = Utils.log("persister");

    public loadAll = (limit?: number, query?: any): Promise<T[]> => {
        const selector: Object = { exchange: this._exchange, pair: this._pair };
        _.assign(selector, query);
        return this.loadInternal(selector, limit);
    };

    private loadInternal = async (selector: Object, limit?: number) : Promise<T[]> => {
        let query = this.collection.find(selector).project({ _id: 0 });

        if (limit !== null) {
            const count = await this.collection.count(selector);
            query = query.limit(limit);
            if (count !== 0)
                query = query.skip(Math.max(count - limit, 0));
        }

        const loaded = await query.toArray();
        _.forEach(loaded, this._loader);

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

    constructor(
        time: Utils.ITimeProvider,
        private collection: mongodb.Collection,
        private _dbName: string,
        private _exchange: Models.Exchange,
        private _pair: Models.CurrencyPair,
        private _loader: (p: Persistable) => void,
        private _saver: (p: Persistable) => void) {
            this._log = Utils.log("persister:"+_dbName);

            time.setInterval(() => {
                if (this._persistQueue.length === 0) return;
                
                this._persistQueue.forEach(this._saver);
                collection.insertMany(this._persistQueue, (err, r) => {
                    if (r.result.ok) {
                        this._persistQueue.length = 0;
                    }
                    else if (err)
                        this._log.error(err, "Unable to insert", this._dbName, this._persistQueue);
                }, );
            }, moment.duration(10, "seconds"));
    }
}