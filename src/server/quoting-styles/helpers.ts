import Interfaces = require("../interfaces");
import Models = require("../../share/models");

export class GeneratedQuote {
  constructor(public bidPx: number, public bidSz: number, public askPx: number, public askSz: number) { }
}

export interface QuoteStyle {
  Mode : Models.QuotingMode;
  GenerateQuote(market: Models.Market, fv: Models.FairValue, params: Models.QuotingParameters, position:Interfaces.IPositionBroker) : GeneratedQuote;
}

export function getQuoteAtTopOfMarket(filteredMkt: Models.Market, params: Models.QuotingParameters): GeneratedQuote {
  let topBid = (filteredMkt.bids[0].size > params.stepOverSize ? filteredMkt.bids[0] : filteredMkt.bids[1]);
  if (typeof topBid === "undefined") topBid = filteredMkt.bids[0];
  let topAsk = (filteredMkt.asks[0].size > params.stepOverSize ? filteredMkt.asks[0] : filteredMkt.asks[1]);
  if (typeof topAsk === "undefined") topAsk = filteredMkt.asks[0];
  return new GeneratedQuote(topBid.price, topBid.size, topAsk.price, topAsk.size);
}
