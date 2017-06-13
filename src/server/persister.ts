import Models = require("../share/models");
import Utils = require("./utils");
import _ = require("lodash");
import mongodb = require('mongodb');
import moment = require('moment');
import Config = require("./config");
import * as Promises from './promises';

export function loadDb(config: Config.ConfigProvider) {
    var deferred = Promises.defer<mongodb.Db>();
    mongodb.MongoClient.connect(config.GetString("MongoDbUrl"), (err, db) => {
        if (err) deferred.reject(err);
        else deferred.resolve(db);
    });
    return deferred.promise;
}

export interface Persistable {
    time?: Date;
    pair?: Models.CurrencyPair;
    exchange?: Models.Exchange;
}

export interface IPersist<T> {
    persist(data: T): void;
    clean(time: Date): void;
    repersist(report: T): void;
}

export interface ILoadLatest<T> extends IPersist<T> {
    loadLatest(): Promise<T>;
    loadDBSize(): Promise<T>;
}

export interface ILoadAll<T> extends IPersist<T> {
    loadAll(limit?: number, query?: Object): Promise<T[]>;
}

export class RepositoryPersister<T extends Persistable> implements ILoadLatest<T> {
    public loadDBSize = async (): Promise<T> => {

        var deferred = Promises.defer<T>();

        this.db.then(db => {
          db.stats((err, arr) => {
            if (err) {
              deferred.reject(err);
            }
            else if (arr == null) {
              deferred.resolve(this._defaultParameter);
            }
            else {
              var v = <T>_.defaults(arr.dataSize, this._defaultParameter);
              deferred.resolve(v);
            }
          });
        });

        return deferred.promise;
    };


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
        return this.converter(v);
    };

    public repersist = (report: T) => { };

    public clean = (time: Date) => { };

    public persist = (report: T) => {
        this.collection.then(coll => {
            if (this._dbName != 'trades')
              coll.deleteMany({ _id: { $exists:true } }, err => {
                  if (err) console.error('persister', err, 'Unable to deleteMany', this._dbName, report);
              });
            coll.insertOne(this.converter(report), err => {
                if (err) console.error('persister', err, 'Unable to insert', this._dbName, report);
            });
        });
    };

    private converter = (x: T) : T => {
        if (typeof x.exchange === "undefined")
            x.exchange = this._exchange;
        if (typeof x.pair === "undefined")
            x.pair = this._pair;
        return x;
    };

    collection: Promise<mongodb.Collection>;
    constructor(
        private db: Promise<mongodb.Db>,
        private _defaultParameter: T,
        private _dbName: string,
        private _exchange: Models.Exchange,
        private _pair: Models.CurrencyPair
    ) {
        if (this._dbName != 'dataSize')
          this.collection = db.then(db => db.collection(this._dbName));
    }
}

export class Persister<T extends Persistable> implements ILoadAll<T> {
    public loadAll = (limit?: number, query?: any): Promise<T[]> => {
        const selector: Object = { exchange: this._exchange, pair: this._pair };
        if (query && query.time && this._dbName == "trades") delete query.time;
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

        return loaded;
    };

    private _persistQueue : T[] = [];
    public persist = (report: T) => {
        this._persistQueue.push(report);
    };

    public clean = (time: Date) => {
        this.collection.deleteMany({ time: (this._dbName=='rfv'||this._dbName=='mkt')?{ $lt: time }:{ $exists:true } }, err => {
            if (err) console.error('persister', err, 'Unable to clean', this._dbName);
        });
    };

    public repersist = (report: T) => {
        if ((<any>report).Kqty<0)
          this.collection.deleteOne({ tradeId: (<any>report).tradeId }, err => {
              if (err) console.error('persister', err, 'Unable to deleteOne', this._dbName, report);
          });
        else
          this.collection.updateOne({ tradeId: (<any>report).tradeId }, { $set: { time: (<any>report).time, quantity : (<any>report).quantity, value : (<any>report).value, Ktime: (<any>report).Ktime, Kqty : (<any>report).Kqty, Kprice : (<any>report).Kprice, Kvalue : (<any>report).Kvalue, Kdiff : (<any>report).Kdiff } }, err => {
              if (err) console.error('persister', err, 'Unable to repersist', this._dbName, report);
          });
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
        private _pair: Models.CurrencyPair
    ) {
            time.setInterval(() => {
                if (this._persistQueue.length === 0) return;

                if (this._dbName != 'trades'&&this._dbName!='rfv'&&this._dbName!='mkt')
                  collection.deleteMany({ time: { $exists:true } }, err => {
                      if (err) console.error('persister', err, 'Unable to deleteMany', this._dbName);
                  });
                // collection.insertOne(report, err => {
                    // if (err)
                        // console.error('persister', err, 'Unable to insert', this._dbName, report);
                // });
                collection.insertMany(_.map(this._persistQueue, this.converter), (err, r) => {
                    if (r.result && r.result.ok) this._persistQueue.length = 0;
                    if (err) console.error('persister', err, 'Unable to insert', this._dbName, this._persistQueue);
                }, );
            }, moment.duration(10, "seconds"));
    }
}
