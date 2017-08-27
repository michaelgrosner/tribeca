import Models = require("../../share/models");

export class GeneratedQuote {
  constructor(
    public bidPx: number,
    public bidSz: number,
    public askPx: number,
    public askSz: number,
    public isBidPong?: boolean,
    public isAskPong?: boolean
  ) { }
}

export function getQuoteAtTopOfMarket(input): GeneratedQuote {
  let topBid = input.market.bids[0].size > input.minTickIncrement ? input.market.bids[0] : input.market.bids[1];
  let topAsk = input.market.asks[0].size > input.minTickIncrement ? input.market.asks[0] : input.market.asks[1];
  if (typeof topBid === "undefined") topBid = input.market.bids[0];
  if (typeof topAsk === "undefined") topAsk = input.market.asks[0];
  return new GeneratedQuote(topBid.price, topBid.size, topAsk.price, topAsk.size);
}
