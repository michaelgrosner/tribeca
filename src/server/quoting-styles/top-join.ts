import StyleHelpers = require("./helpers");
import Models = require("../../share/models");

export class TopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.Top;

  GenerateQuote = (input): StyleHelpers.GeneratedQuote => {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    if (input.mode !== Models.QuotingMode.Join && genQt.bidSz > .2)
      genQt.bidPx += input.minTickIncrement;

    var minBid = input.fvPrice - input.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.mode !== Models.QuotingMode.Join && genQt.askSz > .2)
      genQt.askPx -= input.minTickIncrement;

    var minAsk = input.fvPrice + input.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = input.buySize;
    genQt.askSz = input.sellSize;

    return genQt;
  }
}

export class InverseTopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.InverseTop;

  GenerateQuote = (input): StyleHelpers.GeneratedQuote => {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
    if (mktWidth > input.widthPing) {
        genQt.askPx += input.widthPing;
        genQt.bidPx -= input.widthPing;
    }

    if (input.mode === Models.QuotingMode.InverseTop) {
        if (genQt.bidSz > .2) genQt.bidPx += input.minTickIncrement;
        if (genQt.askSz > .2) genQt.askPx -= input.minTickIncrement;
    }

    if (mktWidth < (2.0 * input.widthPing / 3.0)) {
        genQt.askPx += input.widthPing / 4.0;
        genQt.bidPx -= input.widthPing / 4.0;
    }

    genQt.bidSz = input.buySize;
    genQt.askSz = input.sellSize;

    return genQt;
  };
}

export class InverseJoinQuoteStyle extends InverseTopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.InverseJoin;
}

export class JoinQuoteStyle extends TopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.Join;
}

export class PingPongQuoteStyle extends TopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.PingPong;
}

export class BoomerangQuoteStyle extends TopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.Boomerang;
}

export class AK47QuoteStyle extends TopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.AK47;
}

export class HamelinRatQuoteStyle extends TopOfTheMarketQuoteStyle {
  Mode = Models.QuotingMode.HamelinRat;
}