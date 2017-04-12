import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class PingPongQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.PingPong;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computePingPongQuote(market, fv, params, position);
    };
}

export class BoomerangQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Boomerang;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computeBoomerangQuote(market, fv, params, position);
    };
}

export class AK47QuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.AK47;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computeAK47Quote(market, fv, params, position);
    };
}

function computePingPongQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.PingPong && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.PingPong && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    let latestPosition = position.latestReport;
    genQt.bidSz = (params.percentageValues && latestPosition != null)
      ? params.buySizePercentage * latestPosition.value / 100
      : params.buySize;
    genQt.askSz = (params.percentageValues && latestPosition != null)
      ? params.sellSizePercentage * latestPosition.value / 100
      : params.sellSize;

    return genQt;
}

function computeBoomerangQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.Boomerang && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.Boomerang && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    let latestPosition = position.latestReport;
    genQt.bidSz = (params.percentageValues && latestPosition != null)
      ? params.buySizePercentage * latestPosition.value / 100
      : params.buySize;
    genQt.askSz = (params.percentageValues && latestPosition != null)
      ? params.sellSizePercentage * latestPosition.value / 100
      : params.sellSize;

    return genQt;
}

function computeAK47Quote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.AK47 && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.AK47 && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.widthPing / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    let latestPosition = position.latestReport;
    genQt.bidSz = (params.percentageValues && latestPosition != null)
      ? params.buySizePercentage * latestPosition.value / 100
      : params.buySize;
    genQt.askSz = (params.percentageValues && latestPosition != null)
      ? params.sellSizePercentage * latestPosition.value / 100
      : params.sellSize;

    return genQt;
}
