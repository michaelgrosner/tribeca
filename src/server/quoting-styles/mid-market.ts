import StyleHelpers = require("./helpers");
import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        var widthPing = (input.params.percentageValues)
            ? input.params.widthPingPercentage * input.fv.price / 100
            : input.params.widthPing;

        var bidPx = Math.max(input.fv.price - widthPing, 0);
        var askPx = input.fv.price + widthPing;

        const latestPosition = input.position.latestReport;
        let buySize: number = (input.params.percentageValues && latestPosition != null)
            ? input.params.buySizePercentage * latestPosition.value / 100
            : input.params.buySize;
        let sellSize: number = (input.params.percentageValues && latestPosition != null)
            ? input.params.sellSizePercentage * latestPosition.value / 100
            : input.params.sellSize;
        const tbp = input.latestTargetPosition;
        if (tbp !== null) {
          const targetBasePosition = tbp.data;
          const totalBasePosition = latestPosition.baseAmount + latestPosition.baseHeldAmount;
          if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
            buySize = Math.max(buySize, targetBasePosition - totalBasePosition);
           if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
            sellSize = Math.max(sellSize, totalBasePosition - targetBasePosition);
        }

        return new StyleHelpers.GeneratedQuote(bidPx, buySize, askPx, sellSize);
    };
}
