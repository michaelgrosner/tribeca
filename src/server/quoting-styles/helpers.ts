import Models = require("../../share/models");
import Broker = require("../broker");

export class GeneratedQuote {
  constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
}

export class QuoteInput {
    constructor(
        public market: Models.Market,
        public fv: Models.FairValue,
        public params: Models.QuotingParameters,
        public position: Broker.PositionBroker,
        public latestTargetPosition: Models.TargetBasePositionValue,
        public minTickIncrement: number,
        public minSizeIncrement: number = 0.01) {}
}

export interface QuoteStyle {
  Mode : Models.QuotingMode;
  GenerateQuote(input: QuoteInput) : GeneratedQuote;
}

export function getQuoteAtTopOfMarket(input: QuoteInput): GeneratedQuote {
  let topBid = (input.market.bids[0].size > input.minTickIncrement ? input.market.bids[0] : input.market.bids[1]);
  if (typeof topBid === "undefined") topBid = input.market.bids[0];
  let topAsk = (input.market.asks[0].size > input.minTickIncrement ? input.market.asks[0] : input.market.asks[1]);
  if (typeof topAsk === "undefined") topAsk = input.market.asks[0];
  return new GeneratedQuote(topBid.price, topBid.size, topAsk.price, topAsk.size);
}
