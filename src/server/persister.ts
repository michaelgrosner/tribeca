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
    const rows = await coll.find(selector).limit(1).project({ _id: 0 }).sort({ $natural: -1 }).toArray();
    return rows.length === 0
      ? defaults
      : this.converter(Object.assign(defaults, rows[0]));
  };

  public loadAll = async (dbName: string): Promise<any[]> => {
    const selector: Object = { exchange: this._exchange, pair: this._pair };
    const coll = await this.loadCollection(dbName);
    let query = coll.find(selector, {_id: 0});
    const count = await coll.count(selector);
    query = query.limit(10000);
    if (count !== 0) query = query.skip(Math.max(count - 10000, 0));
    return (await query.toArray()).map(this.converter);
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

  private _cleanQueue: any = {};
  public reclean = (dbName: string, time: Date) => {
      this._cleanQueue[dbName] = time;
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
              if (typeof this._cleanQueue[dbName] !== 'undefined')
                coll.deleteMany({ time: (dbName=='rfv'||dbName=='mkt')?{ $lt: this._cleanQueue[dbName] }:{ $exists:true } }, err => {
                    if (err) console.error('persister', err, 'Unable to clean', dbName);
                    else delete this._cleanQueue[dbName];
                });
              if (dbName != 'trades' && dbName!='rfv' && dbName!='mkt')
                coll.deleteMany({ time: { $exists:true } }, err => {
                    if (err) console.error('persister', err, 'Unable to deleteMany', dbName);
                });
              coll.insertMany(this._persistQueue[dbName].map(this.converter).filter(x => x.exchange === this._exchange), (err, r) => {
                  if (err) console.error('persister', err, 'Unable to insert', dbName, this._persistQueue[dbName]);
                  this._persistQueue[dbName].length = 0;
              }, );
            });
          }
        }
      }
    }, 13000);
  }
}
