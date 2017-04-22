/// <reference path="../../common/models.ts" />

import StyleHelpers = require("./helpers");
import Models = require("../../common/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;
    
    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        var width = input.params.width;
        var size = input.params.size;
    
        var bidPx = Math.max(input.fv.price - width, 0);
        var askPx = input.fv.price + width;
    
        return new StyleHelpers.GeneratedQuote(bidPx, size, askPx, size);
    };
}