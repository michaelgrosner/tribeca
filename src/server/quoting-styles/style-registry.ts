import StyleHelpers = require("./helpers");
import MidMarket = require("./mid-market");
import TopJoin = require("./top-join");
import Depth = require("./depth");

class NullQuoteGenerator {
  Mode = null;

  GenerateQuote = (input): StyleHelpers.GeneratedQuote => {
    return null;
  };
}

export class QuotingStyleRegistry {
  private _mapQuoteGenerators;
  private _nullQuoteGenerator;

  constructor() {
    this._nullQuoteGenerator = new NullQuoteGenerator();
    this._mapQuoteGenerators = [
      new MidMarket.MidMarketQuoteStyle(),
      new TopJoin.InverseJoinQuoteStyle(),
      new TopJoin.InverseTopOfTheMarketQuoteStyle(),
      new TopJoin.JoinQuoteStyle(),
      new TopJoin.TopOfTheMarketQuoteStyle(),
      new TopJoin.PingPongQuoteStyle(),
      new TopJoin.BoomerangQuoteStyle(),
      new TopJoin.AK47QuoteStyle(),
      new TopJoin.HamelinRatQuoteStyle(),
      new Depth.DepthQuoteStyle()
    ].sort((a,b) => a.Mode > b.Mode ? 1 : (a.Mode < b.Mode ? -1 : 0));
  }

  public GenerateQuote = (input): StyleHelpers.GeneratedQuote => {
    return (this._mapQuoteGenerators[input.mode] || this._nullQuoteGenerator).GenerateQuote(input);
  };
}
