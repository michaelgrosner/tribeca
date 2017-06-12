import StyleHelpers = require("./helpers");
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

export function computeTopJoinQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    var widthPing = (input.params.widthPercentage)
        ? input.params.widthPingPercentage * input.fv.price / 100
        : input.params.widthPing;

    if (input.params.mode === Models.QuotingMode.Top && genQt.bidSz > .2) {
        genQt.bidPx += input.minTickIncrement;
    }

    var minBid = input.fv.price - widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.params.mode === Models.QuotingMode.Top && genQt.askSz > .2) {
        genQt.askPx -= input.minTickIncrement;
    }

    var minAsk = input.fv.price + widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = input.params.buySize;
    genQt.askSz = input.params.sellSize;
    genQt.bidSz = (input.params.percentageValues && input.latestPosition != null)
        ? input.params.buySizePercentage * input.latestPosition.value / 100
        : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && input.latestPosition != null)
        ? input.params.sellSizePercentage * input.latestPosition.value / 100
        : input.params.sellSize;
    const tbp = input.latestTargetPosition;
    if (tbp !== null) {
      const targetBasePosition = tbp.data;
      const totalBasePosition = input.latestPosition.baseAmount + input.latestPosition.baseHeldAmount;
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
        genQt.bidSz = Math.max(genQt.bidSz, targetBasePosition - totalBasePosition);
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
        genQt.askSz = Math.max(genQt.askSz, totalBasePosition - targetBasePosition);
    }

    return genQt;
}

function computeInverseJoinQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    var widthPing = (input.params.widthPercentage)
        ? input.params.widthPingPercentage * input.fv.price / 100
        : input.params.widthPing;

    var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
    if (mktWidth > widthPing) {
        genQt.askPx += widthPing;
        genQt.bidPx -= widthPing;
    }

    if (input.params.mode === Models.QuotingMode.InverseTop) {
        if (genQt.bidSz > .2) genQt.bidPx += input.minTickIncrement;
        if (genQt.askSz > .2) genQt.askPx -= input.minTickIncrement;
    }

    if (mktWidth < (2.0 * widthPing / 3.0)) {
        genQt.askPx += widthPing / 4.0;
        genQt.bidPx -= widthPing / 4.0;
    }

    genQt.bidSz = input.params.buySize;
    genQt.askSz = input.params.sellSize;
    genQt.bidSz = (input.params.percentageValues && input.latestPosition != null)
        ? input.params.buySizePercentage * input.latestPosition.value / 100
        : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && input.latestPosition != null)
        ? input.params.sellSizePercentage * input.latestPosition.value / 100
        : input.params.sellSize;
    const tbp = input.latestTargetPosition;
    if (tbp !== null) {
      const targetBasePosition = tbp.data;
      const totalBasePosition = input.latestPosition.baseAmount + input.latestPosition.baseHeldAmount;
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
        genQt.bidSz = Math.max(genQt.bidSz, targetBasePosition - totalBasePosition);
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
        genQt.askSz = Math.max(genQt.askSz, totalBasePosition - targetBasePosition);
    }

    return genQt;
}