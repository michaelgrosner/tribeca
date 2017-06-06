import Models = require("../share/models");
import Interfaces = require("./interfaces");
import Persister = require("./persister");
import Publish = require("./publish");
import _ = require("lodash");
import Utils = require("./utils");

export class QuotingParametersRepository implements Interfaces.IRepository<Models.QuotingParameters> {
  NewParameters = new Utils.Evt();

  constructor(
    private _paramsPersister: Persister.IPersist<Models.QuotingParameters>,
    private _pub: Publish.IPublish<Models.QuotingParameters>,
    rec: Publish.IReceive<Models.QuotingParameters>,
    initParams: Models.QuotingParameters,
  ) {
    if (_pub) _pub.registerSnapshot(() => [this.latest]);
    if (rec) rec.registerReceiver(this.updateParameters);
    this._latest = initParams;
  }

  private _validator: (a: Models.QuotingParameters) => boolean = (p: Models.QuotingParameters) => p.buySize > 0 || p.sellSize > 0 || p.buySizePercentage > 0 || p.sellSizePercentage > 0 || p.widthPing > 0 || p.widthPong > 0 || p.widthPingPercentage > 0 || p.widthPongPercentage > 0;

  private _paramsEqual: (a: Models.QuotingParameters, b: Models.QuotingParameters) => boolean = (a: Models.QuotingParameters, b: Models.QuotingParameters) => !_.isEqual(a, b);

  private _latest: Models.QuotingParameters;
  public get latest(): Models.QuotingParameters {
    return this._latest;
  }

  public updateParameters = (newParams: Models.QuotingParameters) => {
    if (this._validator(newParams) && this._paramsEqual(newParams, this._latest)) {

      if (newParams.mode===Models.QuotingMode.Depth)
        newParams.widthPercentage = false;

      this._latest = newParams;
      this._paramsPersister.persist(newParams)
      this.NewParameters.trigger();
    }

    this._pub.publish(this.latest);
  };
}
