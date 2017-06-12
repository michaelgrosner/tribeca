import StyleHelpers = require("./helpers");
import Models = require("../../share/models");
import MidMarket = require("./mid-market");
import TopJoin = require("./top-join");
import PingPong = require("./ping-pong");
import Depth = require("./depth");

class NullQuoteGenerator implements StyleHelpers.QuoteStyle {
  Mode = null;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote|null => {
    return null;
  };
}

export class QuotingStyleRegistry {
  private _mapping : StyleHelpers.QuoteStyle[];

  constructor() {
    this._mapping = [
      new MidMarket.MidMarketQuoteStyle(),
      new TopJoin.InverseJoinQuoteStyle(),
      new TopJoin.InverseTopOfTheMarketQuoteStyle(),
      new TopJoin.JoinQuoteStyle(),
      new TopJoin.TopOfTheMarketQuoteStyle(),
      new PingPong.PingPongQuoteStyle(),
      new PingPong.BoomerangQuoteStyle(),
      new PingPong.AK47QuoteStyle(),
      new PingPong.HamelinRatQuoteStyle(),
      new Depth.DepthQuoteStyle()
    ].sort((a,b) => a.Mode > b.Mode ? 1 : (a.Mode < b.Mode ? -1 : 0));
  }

  private static NullQuoteGenerator : StyleHelpers.QuoteStyle = new NullQuoteGenerator();

  public Get = (mode: Models.QuotingMode): StyleHelpers.QuoteStyle => {
    return this._mapping[mode] || QuotingStyleRegistry.NullQuoteGenerator;
  };
}
