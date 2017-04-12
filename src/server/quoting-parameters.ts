import Models = require("../share/models");
import Interfaces = require("./interfaces");
import Persister = require("./persister");
import Publish = require("./publish");
import _ = require("lodash");
import Utils = require("./utils");

class Repository<T> implements Interfaces.IRepository<T> {

  NewParameters = new Utils.Evt();

  constructor(
    private _name: string,
    private _validator: (a: T) => boolean,
    private _paramsEqual: (a: T, b: T) => boolean,
    defaultParameter: T,
    private _rec: Publish.IReceive<T>,
    private _pub: Publish.IPublish<T>
  ) {
    _pub.registerSnapshot(() => [this.latest]);
    _rec.registerReceiver(this.updateParameters);
    this._latest = defaultParameter;
  }

  private _latest: T;
  public get latest(): T {
    return this._latest;
  }

  public updateParameters = (newParams: T) => {
    if (this._validator(newParams) && this._paramsEqual(newParams, this._latest)) {
      this._latest = newParams;
      this.NewParameters.trigger();
    }

    this._pub.publish(this.latest);
  };
}

export class QuotingParametersRepository extends Repository<Models.QuotingParameters> {
  constructor(
    pub: Publish.IPublish<Models.QuotingParameters>,
    rec: Publish.IReceive<Models.QuotingParameters>,
    initParam: Models.QuotingParameters,
    paramsPersister: Persister.IPersist<Models.QuotingParameters>
  ) {
    super("qpr",
      (p: Models.QuotingParameters) => p.buySize > 0 || p.sellSize > 0 || p.buySizePercentage > 0 || p.sellSizePercentage > 0 || p.widthPing > 0 || p.widthPong > 0,
      (a: Models.QuotingParameters, b: Models.QuotingParameters) => !_.isEqual(a, b),
      initParam, rec, pub);
    this.NewParameters.on(() => paramsPersister.persist(this.latest));
  }
}
