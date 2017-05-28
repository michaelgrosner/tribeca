/// <reference path="../../common/models.ts" />
 
 import StyleHelpers = require("./helpers");
 import Models = require("../../common/models");

export class DepthQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Depth;
    
    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        const depth = input.params.width;
        const size = input.params.size;
        
        let bidPx = input.market.bids[0].price;
        let bidDepth = 0;
        for (let b of input.market.bids) {
            bidDepth += b.size;
            if (bidDepth >= depth) {
                break
            }
            else {
                bidPx = b.price;
            }
        }
        
        let askPx = input.market.asks[0].price;
        let askDepth = 0;
        for (let a of input.market.asks) {
            askDepth += a.size;
            if (askDepth >= depth) {
                break
            }
            else {
                askPx = a.price;
            }
        }
     
         return new StyleHelpers.GeneratedQuote(bidPx, size, askPx, size);
     };
};