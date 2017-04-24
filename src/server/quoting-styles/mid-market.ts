import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        var widthPing = input.params.widthPing;

        let latestPosition = input.position.latestReport;
        let buySize: number = (input.params.percentageValues && latestPosition != null)
          ? input.params.buySizePercentage * latestPosition.value / 100
          : input.params.buySize;
        let sellSize: number = (input.params.percentageValues && latestPosition != null)
          ? input.params.sellSizePercentage * latestPosition.value / 100
          : input.params.sellSize;

        var bidPx = Math.max(input.fv.price - widthPing, 0);
        var askPx = input.fv.price + widthPing;

        return new StyleHelpers.GeneratedQuote(bidPx, buySize, askPx, sellSize);
    };
}
