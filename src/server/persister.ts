import Models = require("../share/models");
import mongodb = require('mongodb');
import * as Promises from './promises';

export class Repository {
  public loadDBSize = async (): Promise<number> => {
      var deferred = Promises.defer<number>();
      this.db.then(db => {
        db.stats((err, arr) => {
          if (err) deferred.reject(err);
          else if (arr == null) deferred.resolve(0);
          else deferred.resolve(arr.dataSize || 0);
        });
      });
      return deferred.promise;
  };


  public loadLatest = async (dbName: string, defaults: any): Promise<any> => {
    const coll = await this.loadCollection(dbName);

    const selector = { exchange: this._exchange, pair: this._pair };
    const docs = await coll.find(selector)
            .limit(1)
            .project({ _id: 0 })
            .sort({ $natural: -1 })
            .toArray();

    if (docs.length === 0) return defaults;

    var v = Object.assign(defaults, docs[0]);
    return this.converter(v);
  };

  public loadAll = async (dbName: string, limit?: number): Promise<any[]> => {
    const selector: Object = { exchange: this._exchange, pair: this._pair };
    const coll = await this.loadCollection(dbName);
    let query = coll.find(selector, {_id: 0});

    if (limit !== null) {
      const count = await coll.count(selector);
      query = query.limit(limit);
      if (count !== 0)
        query = query.skip(Math.max(count - limit, 0));
    }

    const loaded = (await query.toArray()).map(this.converter);

    return loaded;
  };

  private converter = (x: any):any => {
      if (typeof x.time === "undefined")
          x.time = new Date();
      if (typeof x.exchange === "undefined")
          x.exchange = this._exchange;
      if (typeof x.pair === "undefined")
          x.pair = this._pair;
      return x;
  };

  private _persistQueue: any = {};
  public persist = (dbName: string, report: any) => {
      if (typeof this._persistQueue[dbName] === 'undefined')
        this._persistQueue[dbName] = [];
      this._persistQueue[dbName].push(report);
  };

  public reclean = (dbName: string, time: Date) => {
      this.loadCollection(dbName).then(coll => {
        coll.deleteMany({ time: (dbName=='rfv'||dbName=='mkt')?{ $lt: time }:{ $exists:true } }, err => {
            if (err) console.error('persister', err, 'Unable to clean', dbName);
        });
      });
  };

  public repersist = (report: any) => {
      this.loadCollection('trades').then(coll => {
        if ((<any>report).Kqty<0)
          coll.deleteOne({ tradeId: (<any>report).tradeId }, err => {
              if (err) console.error('persister', err, 'Unable to deleteOne', 'trades', report);
          });
        else
          coll.updateOne({ tradeId: (<any>report).tradeId }, { $set: { time: (<any>report).time, quantity : (<any>report).quantity, value : (<any>report).value, Ktime: (<any>report).Ktime, Kqty : (<any>report).Kqty, Kprice : (<any>report).Kprice, Kvalue : (<any>report).Kvalue, Kdiff : (<any>report).Kdiff } }, err => {
              if (err) console.error('persister', err, 'Unable to repersist', 'trades', report);
          });
      });
  };

  private db: Promise<mongodb.Db>;

  private loadDb = (url: string) => {
      var deferred = Promises.defer<mongodb.Db>();
      mongodb.MongoClient.connect(url, (err, db) => {
          if (err) deferred.reject(err);
          else deferred.resolve(db);
      });
      return deferred.promise;
  };

  private collection: Promise<mongodb.Collection>[] = [];

  private loadCollection = (dbName: string) => {
    if (typeof this.collection[dbName] === 'undefined')
      this.collection[dbName] = this.db.then(db => db.collection(dbName));
    return this.collection[dbName];
  }

  constructor(
    url: string,
    private _exchange: Models.Exchange,
    private _pair: Models.CurrencyPair
  ) {
    this.db = this.loadDb(url);

    setInterval(() => {
      for (let dbName in this._persistQueue) {
        if (this._persistQueue[dbName].length) {
          if (typeof this.collection[dbName] !== 'undefined') {
            this.collection[dbName].then(coll => {
              if (dbName != 'trades' && dbName!='rfv' && dbName!='mkt')
                coll.deleteMany({ time: { $exists:true } }, err => {
                    if (err) console.error('persister', err, 'Unable to deleteMany', dbName);
                });
              coll.insertMany(this._persistQueue[dbName].map(this.converter), (err, r) => {
                  if (r.result && r.result.ok) this._persistQueue[dbName].length = 0;
                  if (err) console.error('persister', err, 'Unable to insert', dbName, this._persistQueue[dbName]);
              }, );
            });
          }
        }
      }
    }, 13000);
  }
}
