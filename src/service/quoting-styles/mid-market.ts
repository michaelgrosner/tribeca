/// <reference path="../../common/models.ts" />

import StyleHelpers = require("./helpers");
import Models = require("../../common/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;
    
    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        var width = params.width;
        var size = params.size;
    
        var bidPx = Math.max(fv.price - width, 0);
        var askPx = fv.price + width;
    
        return new StyleHelpers.GeneratedQuote(bidPx, size, askPx, size);
    };
}