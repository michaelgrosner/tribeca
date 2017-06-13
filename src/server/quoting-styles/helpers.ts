import Models = require("../../share/models");

export class GeneratedQuote {
  constructor(
    public bidPx: number,
    public bidSz: number,
    public askPx: number,
    public askSz: number
  ) { }
}

export class QuoteInput {
  constructor(
    public market: Models.Market,
    public fvPrice: number,
    public widthPing: number,
    public buySize: number,
    public sellSize: number,
    public mode: Models.QuotingMode,
    public minTickIncrement: number
  ) {}
}

export function getQuoteAtTopOfMarket(input: QuoteInput): GeneratedQuote {
  let topBid = input.market.bids[0].size > input.minTickIncrement ? input.market.bids[0] : input.market.bids[1];
  let topAsk = input.market.asks[0].size > input.minTickIncrement ? input.market.asks[0] : input.market.asks[1];
  if (typeof topBid === "undefined") topBid = input.market.bids[0];
  if (typeof topAsk === "undefined") topAsk = input.market.asks[0];
  return new GeneratedQuote(topBid.price, topBid.size, topAsk.price, topAsk.size);
}
