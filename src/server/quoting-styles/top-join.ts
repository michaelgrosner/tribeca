import StyleHelpers = require("./helpers");
import Models = require("../../share/models");

export class TopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Top;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(input);
    };
}

export class InverseTopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseTop;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(input);
    };
}

export class InverseJoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseJoin;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(input);
    };
}

export class JoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Join;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(input);
    };
}

export function computeTopJoinQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    if (input.mode !== Models.QuotingMode.Join && genQt.bidSz > .2) {
        genQt.bidPx += input.minTickIncrement;
    }

    var minBid = input.fvPrice - input.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.mode !== Models.QuotingMode.Join && genQt.askSz > .2) {
        genQt.askPx -= input.minTickIncrement;
    }

    var minAsk = input.fvPrice + input.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = input.buySize;
    genQt.askSz = input.sellSize;

    return genQt;
}

function computeInverseJoinQuote(input: StyleHelpers.QuoteInput) {
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
}