import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class TopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Top;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(market, fv, params, position);
    };
}

export class InverseTopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseTop;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(market, fv, params, position);
    };
}

export class InverseJoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseJoin;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(market, fv, params, position);
    };
}

export class JoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Join;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(market, fv, params, position);
    };
}

function computeTopJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.Top && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.widthPing / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.Top && genQt.askSz > .2) {
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

function computeInverseJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) {
    var genQt = StyleHelpers.getQuoteAtTopOfMarket(filteredMkt, params);

    var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
    if (mktWidth > params.widthPing) {
        genQt.askPx += params.widthPing;
        genQt.bidPx -= params.widthPing;
    }

    if (params.mode === Models.QuotingMode.InverseTop) {
        if (genQt.bidSz > .2) genQt.bidPx += .01;
        if (genQt.askSz > .2) genQt.askPx -= .01;
    }

    if (mktWidth < (2.0 * params.widthPing / 3.0)) {
        genQt.askPx += params.widthPing / 4.0;
        genQt.bidPx -= params.widthPing / 4.0;
    }

    let latestPosition = position.latestReport;
    genQt.bidSz = (params.percentageValues && latestPosition != null)
      ? params.buySizePercentage * latestPosition.value / 100
      : params.buySize;
    genQt.askSz = (params.percentageValues && latestPosition != null)
      ? params.sellSizePercentage * latestPosition.value / 100
      : params.sellSize;

    return genQt;
}