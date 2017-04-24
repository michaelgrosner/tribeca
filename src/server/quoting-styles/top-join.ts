import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class TopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Top;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(input);
    };
}

export class InverseTopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseTop;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(input);
    };
}

export class InverseJoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseJoin;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(input);
    };
}

export class JoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Join;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(input);
    };
}

function computeTopJoinQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    if (input.params.mode === Models.QuotingMode.Top && genQt.bidSz > .2) {
        genQt.bidPx += input.minTickIncrement;
    }

    var minBid = input.fv.price - input.params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.params.mode === Models.QuotingMode.Top && genQt.askSz > .2) {
        genQt.askPx -= input.minTickIncrement;
    }

    var minAsk = input.fv.price + input.params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    let latestPosition = input.position.latestReport;
    genQt.bidSz = (input.params.percentageValues && latestPosition != null)
      ? input.params.buySizePercentage * latestPosition.value / 100
      : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && latestPosition != null)
      ? input.params.sellSizePercentage * latestPosition.value / 100
      : input.params.sellSize;

    return genQt;
}

function computeInverseJoinQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
    if (mktWidth > input.params.widthPing) {
        genQt.askPx += input.params.widthPing;
        genQt.bidPx -= input.params.widthPing;
    }

    if (input.params.mode === Models.QuotingMode.InverseTop) {
        if (genQt.bidSz > .2) genQt.bidPx += input.minTickIncrement;
        if (genQt.askSz > .2) genQt.askPx -= input.minTickIncrement;
    }

    if (mktWidth < (2.0 * input.params.widthPing / 3.0)) {
        genQt.askPx += input.params.widthPing / 4.0;
        genQt.bidPx -= input.params.widthPing / 4.0;
    }

    let latestPosition = input.position.latestReport;
    genQt.bidSz = (input.params.percentageValues && latestPosition != null)
      ? input.params.buySizePercentage * latestPosition.value / 100
      : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && latestPosition != null)
      ? input.params.sellSizePercentage * latestPosition.value / 100
      : input.params.sellSize;

    return genQt;
}