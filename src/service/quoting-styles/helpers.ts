/// <reference path="../../common/models.ts" />

import Models = require("../../common/models");

export class GeneratedQuote {
    constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
}

export class QuoteInput {
    constructor(
        public market: Models.Market, 
        public fv: Models.FairValue, 
        public params: Models.QuotingParameters,
        public minTickIncrement: number,
        public minSizeIncrement: number = 0.01) {}
}

export interface QuoteStyle {
    Mode : Models.QuotingMode;
    GenerateQuote(input: QuoteInput) : GeneratedQuote;
}