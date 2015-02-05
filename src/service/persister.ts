/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import mongodb = require('mongodb');
import Q = require("q");
import moment = require('moment');
import Interfaces = require("./interfaces");

export function loadDb() {
    var deferred = Q.defer<mongodb.Db>();
    mongodb.MongoClient.connect('mongodb://localhost:27017/tribeca', (err, db) => {
        if (err) deferred.reject(err);
        else deferred.resolve(db);
    });
    return deferred.promise;
}

export class Persister<T> {
     _log : Utils.Logger = Utils.log("tribeca:exchangebroker:persister");

    public load = (exchange : Models.Exchange, pair : Models.CurrencyPair, limit : number = null) : Q.Promise<T[]> => {
        var deferred = Q.defer<T[]>();
        this.collection.then(coll => {
            var selector = {exchange: exchange, pair: pair};
            var options = (limit !== null ? {limit: limit}: {});
            coll.find(selector, options, (err, docs) => {
                if (err) deferred.reject(err);
                else {
                    docs.toArray((err, arr) => {
                        if (err) {
                            deferred.reject(err);
                        }
                        else {
                            _.forEach(arr, this._loader);
                            deferred.resolve(arr);
                        }
                    });
                }
            });
        }).done();

        return deferred.promise;
    };

    public persist = (report : T) => {
        this._saver(report);
        this.collection.then(coll => {
            coll.insert(report, err => {
                if (err)
                    this._log("Unable to insert into %s %s; %o", this._dbName, report, err);
            });
        }).done();
    };

    collection : Q.Promise<mongodb.Collection>;
    constructor(
            db : Q.Promise<mongodb.Db>,
            private _dbName : string,
            private _loader : (any) => void,
            private _saver : (T) => void) {
        this.collection = db.then(db => db.collection(this._dbName));
    }
}

function timeLoader(x) {
    x.time = moment.isMoment(x.time) ? x.time : moment(x.time);
}

function timeSaver(rpt) {
    rpt.time = (moment.isMoment(rpt.time) ? rpt.time : moment(rpt.time)).toISOString();
}

export class OrderStatusPersister extends Persister<Models.OrderStatusReport> {
    constructor(db : Q.Promise<mongodb.Db>) {
        super(db, "osr", timeLoader, timeSaver);
    }
}

export class TradePersister extends Persister<Models.Trade> {
    constructor(db : Q.Promise<mongodb.Db>) {
        super(db, "trades", timeLoader, timeSaver);
    }
}