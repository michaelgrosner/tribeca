import Models = require("../share/models");
import Interfaces = require("./interfaces");
import Broker = require("./broker");
import QuotingEngine = require("./quoting-engine");
import * as moment from "moment";

export class MarketTradeBroker {
    private _marketTrades: Models.MarketTrade[] = [];
    private handleNewMarketTrade = (u: Models.GatewayMarketTrade) => {
        var t = new Models.MarketTrade(this._exchange, this._pair, u.price, u.size, new Date().getTime(), u.make_side);

        this._marketTrades.push(t);
        this._marketTrades = this._marketTrades.slice(-69);

        this._evUp('MarketTrade');
        this._uiSend(Models.Topics.MarketTrade, t);
    };

    constructor(
      private _uiSnap,
      private _uiSend,
      private _pair,
      private _exchange,
      private _evOn,
      private _evUp
    ) {
      _uiSnap(Models.Topics.MarketTrade, () => this._marketTrades);
      this._evOn('MarketTradeGateway', this.handleNewMarketTrade);
    }
}
