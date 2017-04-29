import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class PingPongQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.PingPong;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computePingPongQuote(input);
    };
}

export class BoomerangQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Boomerang;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computeBoomerangQuote(input);
    };
}

export class AK47QuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.AK47;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        return computeAK47Quote(input);
    };
}

function computePingPongQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    if (input.params.mode === Models.QuotingMode.PingPong && genQt.bidSz > .2) {
        genQt.bidPx += input.minTickIncrement;
    }

    var minBid = input.fv.price - input.params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.params.mode === Models.QuotingMode.PingPong && genQt.askSz > .2) {
        genQt.askPx -= input.minTickIncrement;
    }

    var minAsk = input.fv.price + input.params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = input.params.buySize;
    genQt.askSz = input.params.sellSize;
    const latestPosition = input.position.latestReport;
    genQt.bidSz = (input.params.percentageValues && latestPosition != null)
        ? input.params.buySizePercentage * latestPosition.value / 100
        : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && latestPosition != null)
        ? input.params.sellSizePercentage * latestPosition.value / 100
        : input.params.sellSize;
    const tbp = input.latestTargetPosition;
    if (tbp !== null) {
      const targetBasePosition = tbp.data;
      const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
        genQt.bidSz = Math.max(genQt.bidSz, targetBasePosition - totalBasePosition);
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
        genQt.askSz = Math.max(genQt.askSz, totalBasePosition - targetBasePosition);
    }

    return genQt;
}

function computeBoomerangQuote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    if (input.params.mode === Models.QuotingMode.Boomerang && genQt.bidSz > .2) {
        genQt.bidPx += input.minTickIncrement;
    }

    var minBid = input.fv.price - input.params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.params.mode === Models.QuotingMode.Boomerang && genQt.askSz > .2) {
        genQt.askPx -= input.minTickIncrement;
    }

    var minAsk = input.fv.price + input.params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = input.params.buySize;
    genQt.askSz = input.params.sellSize;
    const latestPosition = input.position.latestReport;
    genQt.bidSz = (input.params.percentageValues && latestPosition != null)
        ? input.params.buySizePercentage * latestPosition.value / 100
        : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && latestPosition != null)
        ? input.params.sellSizePercentage * latestPosition.value / 100
        : input.params.sellSize;
    const tbp = input.latestTargetPosition;
    if (tbp !== null) {
      const targetBasePosition = tbp.data;
      const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
        genQt.bidSz = Math.max(genQt.bidSz, targetBasePosition - totalBasePosition);
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
        genQt.askSz = Math.max(genQt.askSz, totalBasePosition - targetBasePosition);
    }

    return genQt;
}

function computeAK47Quote(input: StyleHelpers.QuoteInput) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(input);

    if (input.params.mode === Models.QuotingMode.AK47 && genQt.bidSz > .2) {
        genQt.bidPx += input.minTickIncrement;
    }

    var minBid = input.fv.price - input.params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (input.params.mode === Models.QuotingMode.AK47 && genQt.askSz > .2) {
        genQt.askPx -= input.minTickIncrement;
    }

    var minAsk = input.fv.price + input.params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = input.params.buySize;
    genQt.askSz = input.params.sellSize;
    const latestPosition = input.position.latestReport;
    genQt.bidSz = (input.params.percentageValues && latestPosition != null)
        ? input.params.buySizePercentage * latestPosition.value / 100
        : input.params.buySize;
    genQt.askSz = (input.params.percentageValues && latestPosition != null)
        ? input.params.sellSizePercentage * latestPosition.value / 100
        : input.params.sellSize;
    const tbp = input.latestTargetPosition;
    if (tbp !== null) {
      const targetBasePosition = tbp.data;
      const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
        genQt.bidSz = Math.max(genQt.bidSz, targetBasePosition - totalBasePosition);
      if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
        genQt.askSz = Math.max(genQt.askSz, totalBasePosition - targetBasePosition);
    }

    return genQt;
}
