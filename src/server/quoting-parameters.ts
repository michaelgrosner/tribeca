import Models = require("../share/models");
import Publish = require("./publish");

export class QuotingParametersRepository {
  private _latest: Models.QuotingParameters;
  public get latest(): Models.QuotingParameters {
    return this._latest;
  }

  constructor(
    private _sqlite,
    private _publisher: Publish.Publisher,
    private _evUp,
    initParams: Models.QuotingParameters
  ) {
    if (_publisher) {
      _publisher.registerSnapshot(Models.Topics.QuotingParametersChange, () => [this.latest]);
      _publisher.registerReceiver(Models.Topics.QuotingParametersChange, this.updateParameters);
    }
    this._latest = initParams;
  }

  public updateParameters = (p: Models.QuotingParameters) => {
    if (p.buySize > 0 || p.sellSize > 0 || p.buySizePercentage > 0 || p.sellSizePercentage > 0 || p.widthPing > 0 || p.widthPong > 0 || p.widthPingPercentage > 0 || p.widthPongPercentage > 0) {

      if (p.mode===Models.QuotingMode.Depth)
        p.widthPercentage = false;

      this._latest = p;
      this._sqlite.insert(Models.Topics.QuotingParametersChange, p);
      this._evUp('QuotingParameters', p);
    }

    this._publisher.publish(Models.Topics.QuotingParametersChange, this._latest);
  };
}
