import StyleHelpers = require("./helpers");
import Models = require("../../share/models");

export class MidMarketQuoteStyle {
  Mode = Models.QuotingMode.Mid;

  GenerateQuote = (input): StyleHelpers.GeneratedQuote => {
    return new StyleHelpers.GeneratedQuote(
      Math.max(input.fvPrice - input.widthPing, 0),
      input.buySize,
      input.fvPrice + input.widthPing,
      input.sellSize
    );
  };
}
