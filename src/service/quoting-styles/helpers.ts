/// <reference path="../../common/models.ts" />

import Models = require("../../common/models");

export class GeneratedQuote {
    constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
}

export interface QuoteStyle {
    Mode : Models.QuotingMode;
    GenerateQuote(market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters) : GeneratedQuote;
}