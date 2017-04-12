import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;

    GenerateQuote = (market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : StyleHelpers.GeneratedQuote => {
        var widthPing = params.widthPing;

        let latestPosition = position.latestReport;
        let buySize: number = (params.percentageValues && latestPosition != null)
          ? params.buySizePercentage * latestPosition.value / 100
          : params.buySize;
        let sellSize: number = (params.percentageValues && latestPosition != null)
          ? params.sellSizePercentage * latestPosition.value / 100
          : params.sellSize;

        var bidPx = Math.max(fv.price - widthPing, 0);
        var askPx = fv.price + widthPing;

        return new StyleHelpers.GeneratedQuote(bidPx, buySize, askPx, sellSize);
    };
}
