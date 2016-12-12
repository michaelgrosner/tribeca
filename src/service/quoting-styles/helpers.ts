/// <reference path="../../common/models.ts" />

import Models = require("../../common/models");

export class GeneratedQuote {
    constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
}

export interface QuoteStyle {
    Mode : Models.QuotingMode;
    GenerateQuote(market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : GeneratedQuote;
}

export function getQuoteAtTopOfMarket(filteredMkt: Models.Market, params: Models.QuotingParameters): GeneratedQuote {
    var topBid = (filteredMkt.bids[0].size > params.stepOverSize ? filteredMkt.bids[0] : filteredMkt.bids[1]);
    if (typeof topBid === "undefined") topBid = filteredMkt.bids[0]; // only guaranteed top level exists
    var bidPx = topBid.price;

    var topAsk = (filteredMkt.asks[0].size > params.stepOverSize ? filteredMkt.asks[0] : filteredMkt.asks[1]);
    if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
    var askPx = topAsk.price;

    return new GeneratedQuote(bidPx, topBid.size, askPx, topAsk.size);
}
