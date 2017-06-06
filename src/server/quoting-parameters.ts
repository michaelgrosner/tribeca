import Models = require("../share/models");
import Persister = require("./persister");
import Publish = require("./publish");
import _ = require("lodash");
import Utils = require("./utils");

export class QuotingParametersRepository {
  NewParameters = new Utils.Evt();

  private _latest: Models.QuotingParameters;
  public get latest(): Models.QuotingParameters {
    return this._latest;
  }

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

  public updateParameters = (p: Models.QuotingParameters) => {
    if (p.buySize > 0 || p.sellSize > 0 || p.buySizePercentage > 0 || p.sellSizePercentage > 0 || p.widthPing > 0 || p.widthPong > 0 || p.widthPingPercentage > 0 || p.widthPongPercentage > 0) {

      if (p.mode===Models.QuotingMode.Depth)
        p.widthPercentage = false;

      this._latest = p;
      this._paramsPersister.persist(p)
      this.NewParameters.trigger();
    }

    this._pub.publish(this._latest);
  };
}
