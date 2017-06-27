import Models = require("../share/models");
import mongodb = require('mongodb');

export class Repository {
  public loadDBSize = (): Promise<number> => {
    return new Promise<number>((resolve, reject) => {
      this._db.then(db => {
        db.stats((err, result) => {
          if (err) reject(err);
          else if (result == null) resolve(0);
          else resolve(result.dataSize || 0);
        });
      });
    });
  };

  public loadLatest = async (dbName: string, defaults: any): Promise<any> => {
    const coll = await this.loadCollection(dbName);
    const rows = await coll.find({ exchange: this._exchange, pair: this._pair }, {_id: 0}).limit(1).toArray();
    return rows.length === 0
      ? defaults
      : this.converter(Object.assign(defaults, rows[0]));
  };

  public loadAll = async (dbName: string): Promise<any[]> => {
    const selector = { exchange: this._exchange, pair: this._pair };
    const coll = await this.loadCollection(dbName);
    let query = coll.find(selector, {_id: 0});
    const count = await coll.count(selector);
    query = query.limit(10000);
    if (count !== 0) query = query.skip(Math.max(count - 10000, 0));
    return (await query.toArray()).map(this.converter);
  };

  private _cleanQueue: any = {};
  public reclean = (dbName: string, time: Date) => {
      this._cleanQueue[dbName] = time;
  };

  private _persistQueue: any = {};
  public persist = (dbName: string, report: any) => {
      if (dbName === 'trades')
        this.collection['trades'].then(coll => {
          coll.insertOne(this.converter(report), err => { if (err) console.error('persister', err, 'Unable to persist', 'trades', report); });
        });
      else {
        if (typeof this._persistQueue[dbName] === 'undefined')
          this._persistQueue[dbName] = [];
        this._persistQueue[dbName].push(report);
      }
  };

  public repersist = (report: Models.Trade) => {
      this.collection['trades'].then(coll => {
        coll.deleteOne({ tradeId: report.tradeId }, err => {
            if (err) console.error('persister', err, 'Unable to deleteOne', 'trades', report);
            else if (report.Kqty !== -1)
              coll.insertOne(this.converter(report), err => { if (err) console.error('persister', err, 'Unable to repersist', 'trades', report); });
        });
      });
  };

  private _db: Promise<mongodb.Db>;

  private loadDb = (url: string) => {
    return new Promise<mongodb.Db>((resolve, reject) => {
      mongodb.MongoClient.connect(url, (err, db) => {
          if (err) reject(err);
          else resolve(db);
      });
    });
  };

  private collection: Promise<mongodb.Collection>[] = [];

  private loadCollection = (dbName: string) => {
    if (typeof this.collection[dbName] === 'undefined')
      this.collection[dbName] = this._db.then(db => db.collection(dbName));
    return this.collection[dbName];
  }

  private converter = (x: any):any => {
      if (typeof x.time === "undefined") x.time = new Date();
      if (typeof x.exchange === "undefined") x.exchange = this._exchange;
      if (typeof x.pair === "undefined") x.pair = this._pair;
      return x;
  };

  constructor(
    url: string,
    private _exchange: Models.Exchange,
    private _pair: Models.CurrencyPair
  ) {
    this._db = this.loadDb(url);

    setInterval(() => {
      for (let dbName in this._persistQueue) {
        if (this._persistQueue[dbName].length) {
          this.collection[dbName].then(coll => {
            if (typeof this._cleanQueue[dbName] !== 'undefined' || (dbName!='rfv' && dbName!='mkt'))
              coll.deleteMany({ time: ((dbName=='rfv' || dbName=='mkt') && typeof this._cleanQueue[dbName] !== 'undefined')?{ $lt: this._cleanQueue[dbName] }:{ $exists:true } }, err => {
                  if (err) console.error('persister', err, 'Unable to clean', dbName);
                  if (typeof this._cleanQueue[dbName] !== 'undefined') delete this._cleanQueue[dbName];
              });
            coll.insertMany(this._persistQueue[dbName].map(this.converter).filter(x => x.exchange === this._exchange), (err, r) => {
                if (err) console.error('persister', err, 'Unable to insert', dbName, this._persistQueue[dbName]);
                this._persistQueue[dbName].length = 0;
            });
          });
        }
      }
    }, 7000);
  }
}
