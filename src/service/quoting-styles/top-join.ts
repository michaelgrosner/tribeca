/// <reference path="../../common/models.ts" />

import StyleHelpers = require("./helpers");
import Models = require("../../common/models");

export class TopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Top;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(market, fv, params);
    };
}

export class InverseTopOfTheMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseTop;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(market, fv, params);
    };
}

export class InverseJoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.InverseJoin;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computeInverseJoinQuote(market, fv, params);
    };
}

export class PingPongQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.PingPong;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computePingPongQuote(market, fv, params);
    };
}

export class BoomerangQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Boomerang;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computeBoomerangQuote(market, fv, params);
    };
}

export class AK47QuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.AK47;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computeAK47Quote(market, fv, params);
    };
}

export class JoinQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Join;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        return computeTopJoinQuote(market, fv, params);
    };
}

function getQuoteAtTopOfMarket(filteredMkt: Models.Market, params: Models.QuotingParameters): StyleHelpers.GeneratedQuote {
    var topBid = (filteredMkt.bids[0].size > params.stepOverSize ? filteredMkt.bids[0] : filteredMkt.bids[1]);
    if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
    var bidPx = topBid.price;

    var topAsk = (filteredMkt.asks[0].size > params.stepOverSize ? filteredMkt.asks[0] : filteredMkt.asks[1]);
    if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
    var askPx = topAsk.price;

    return new StyleHelpers.GeneratedQuote(bidPx, topBid.size, askPx, topAsk.size);
}

function computeTopJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
    var genQt = getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.Top && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.width / 2.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.Top && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.width / 2.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = params.buySize;
    genQt.askSz = params.sellSize;

    return genQt;
}

function computeInverseJoinQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
    var genQt = getQuoteAtTopOfMarket(filteredMkt, params);

    var mktWidth = Math.abs(genQt.askPx - genQt.bidPx);
    if (mktWidth > params.width) {
        genQt.askPx += params.width;
        genQt.bidPx -= params.width;
    }

    if (params.mode === Models.QuotingMode.InverseTop) {
        if (genQt.bidSz > .2) genQt.bidPx += .01;
        if (genQt.askSz > .2) genQt.askPx -= .01;
    }

    if (mktWidth < (2.0 * params.width / 3.0)) {
        genQt.askPx += params.width / 4.0;
        genQt.bidPx -= params.width / 4.0;
    }

    genQt.bidSz = params.buySize;
    genQt.askSz = params.sellSize;

    return genQt;
}

function computePingPongQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
    var genQt = getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.PingPong && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.width / 8.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.PingPong && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.width / 8.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = params.buySize;
    genQt.askSz = params.sellSize;

    return genQt;
}

function computeBoomerangQuote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
    var genQt = getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.Boomerang && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.width / 8.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.Boomerang && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.width / 8.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = params.buySize;
    genQt.askSz = params.sellSize;

    return genQt;
}

function computeAK47Quote(filteredMkt: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) {
    var genQt = getQuoteAtTopOfMarket(filteredMkt, params);

    if (params.mode === Models.QuotingMode.AK47 && genQt.bidSz > .2) {
        genQt.bidPx += .01;
    }

    var minBid = fv.price - params.width / 8.0;
    genQt.bidPx = Math.min(minBid, genQt.bidPx);

    if (params.mode === Models.QuotingMode.AK47 && genQt.askSz > .2) {
        genQt.askPx -= .01;
    }

    var minAsk = fv.price + params.width / 8.0;
    genQt.askPx = Math.max(minAsk, genQt.askPx);

    genQt.bidSz = params.buySize;
    genQt.askSz = params.sellSize;

    return genQt;
}
