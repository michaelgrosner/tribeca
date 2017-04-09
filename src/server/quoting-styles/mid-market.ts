import StyleHelpers = require("./helpers");
import Models = require("../../share/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : StyleHelpers.GeneratedQuote => {
        var widthPing = params.widthPing;
        var buySize = params.buySize;
        var sellSize = params.sellSize;

        var bidPx = Math.max(fv.price - widthPing, 0);
        var askPx = fv.price + widthPing;

        return new StyleHelpers.GeneratedQuote(bidPx, buySize, askPx, sellSize);
    };
}
