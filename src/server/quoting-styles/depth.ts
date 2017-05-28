import StyleHelpers = require("./helpers");
import Models = require("../../share/models");

export class DepthQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Depth;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        const depth = input.params.widthPing;
        const latestPosition = input.position.latestReport;
        const buySize: number = (input.params.percentageValues && latestPosition != null)
            ? input.params.buySizePercentage * latestPosition.value / 100
            : input.params.buySize;
        const sellSize: number = (input.params.percentageValues && latestPosition != null)
            ? input.params.sellSizePercentage * latestPosition.value / 100
            : input.params.sellSize;

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

         return new StyleHelpers.GeneratedQuote(bidPx, buySize, askPx, sellSize);
     };
};