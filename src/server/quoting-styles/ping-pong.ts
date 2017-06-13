import StyleHelpers = require("./helpers");
import Models = require("../../share/models");
import TopJoin = require("./top-join");

export class PingPongQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.PingPong;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return TopJoin.computeTopJoinQuote(input);
    };
}

export class BoomerangQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.Boomerang;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return TopJoin.computeTopJoinQuote(input);
    };
}

export class AK47QuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.AK47;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return TopJoin.computeTopJoinQuote(input);
    };
}

export class HamelinRatQuoteStyle implements StyleHelpers.QuoteStyle {
    Mode = Models.QuotingMode.HamelinRat;

    GenerateQuote = (input: StyleHelpers.QuoteInput): StyleHelpers.GeneratedQuote => {
        return TopJoin.computeTopJoinQuote(input);
    };
}
