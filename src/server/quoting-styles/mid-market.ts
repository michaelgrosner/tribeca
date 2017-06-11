import StyleHelpers = require("./helpers");
import Models = require("../../share/models");

export class MidMarketQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Mid;

    GenerateQuote = (input: StyleHelpers.QuoteInput) : StyleHelpers.GeneratedQuote => {
        const widthPing = (input.params.percentageValues)
            ? input.params.widthPingPercentage * input.fv.price / 100
            : input.params.widthPing;

        const bidPx = Math.max(input.fv.price - widthPing, 0);
        const askPx = input.fv.price + widthPing;

        let buySize: number = (input.params.percentageValues && input.latestPosition != null)
            ? input.params.buySizePercentage * input.latestPosition.value / 100
            : input.params.buySize;
        let sellSize: number = (input.params.percentageValues && input.latestPosition != null)
            ? input.params.sellSizePercentage * input.latestPosition.value / 100
            : input.params.sellSize;
        const tbp = input.latestTargetPosition;
        if (tbp !== null) {
          const targetBasePosition = tbp.data;
          const totalBasePosition = input.latestPosition.baseAmount + input.latestPosition.baseHeldAmount;
          if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.buySizeMax)
            buySize = Math.max(buySize, targetBasePosition - totalBasePosition);
           if (input.params.aggressivePositionRebalancing != Models.APR.Off && input.params.sellSizeMax)
            sellSize = Math.max(sellSize, totalBasePosition - targetBasePosition);
        }

        return new StyleHelpers.GeneratedQuote(bidPx, buySize, askPx, sellSize);
    };
}
