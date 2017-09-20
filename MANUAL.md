# What is Market Making?

Market making is a trading strategy where the trader simultaneously places both buy and sell orders in an attempt to profit from the bid-ask spread.  Market makers stand ready to both buy and sell from other traders, thus providing liquidity to the market.

The strategy is appealing to traders because it doesn't require traders to take a directional view of the market - there's money to be made when the market goes up and when the market goes down. It's also heavily incentivized by exchanges looking for liquidity and volume - many exchange operators will pay you to make markets on their exchanges.

## An example

Let's consider a simplified market. Let's say there are three traders: Alice, Bob, and Tim. Alice is looking to sell some of her Bitcoins and Bob is looking to convert some of his USD into Bitcoin. Neither one are savvy about markets or cryptocurrency - they use Bitcoin, but aren't going to lose sleep over trying to get the absolute best prices. Tim is operating K.js. Now lets say that the price BTC to USD is $100. Tim could configure K.js to send in a buy order for 1 BTC at $95 and a sell order for 1 BTC at $105. Tim would hope that Bob would come along and buy the offered sell order at $105 and Alice would come and sell BTC at $95 - netting Tim $10.

But what if that doesn't happen? What if the price of BTC/USD jumps to $103? Now Tim's buy order seems really uncompetitive at $95 - Alice doesn't want to sell for that little. And Bob could get a pretty good deal by getting the BTC at only two extra dollars. To prevent this scenario, Tim's K.js would readjust the orders by cancelling the $95-$105 orders and placing a new set of orders - also known as making a market - at $98-$108.

Like in any market these days, Bob and Alice probably aren't humans clicking buttons. They certainly aren't humans shouting on a floor in lower Manhattan. More likely, they are also computerized algorithms capable of placing orders in milliseconds. To survive as a market maker, you need to be faster than those algorithms to make a profit.

# So, how does K.js work?

As previously mentioned, market making is really the art of figuring out the price of something, then making a market around that price. So how do we know what is the real price of Bitcoin? Well... we don't. And of course the price of Bitcoin now might be radically different than the price in a day, in an hour, or even in a second from now. The best we can do is to build an estimate, or fair value, of the price of Bitcoin. In K.js, we consume the market data from the exchange we are sending orders into as a starting point. That includes the best bids and offers and most recent trades (aka market trades) by other participants in the market.

From our fair value, we then need to make a market around that price. Back to our hypothetical example with Bob and Alice: we could make our market as $95-$105, we could have also made it $99.99-$101.01, or we could have also done $47-$332.21. So how do we decide? That's where the quoting parameters come into play. Those parameters dictate how wide of a market to make (wide=askPrice-bidPrice) and what size we want our quotes in the market to be. The procedure for coming up with profitable parameters is both an art and a science - there is no one size fits all formula.

When K.js figures out a suitable market, K.js will then send in the buy and sell orders. Hopefully it's able to buy for less then sell for more, and repeat many times per day. Sometimes that's not always the case - sometimes there are genuinely more buyers than sellers for the prices you are setting. Often this comes when the market is moving very fast in one direction. Luckily, K.js will prevent you from selling too fast without finding corresponding buyers and will stop sending orders in the imbalanced side.

## Where does all this happen in the code?

The code is organized into 3 layers.

1) Engine layer - The brains of the application. Portion of the code responsible for synthesizing market data, open order status, position, fees, trades, safety information and converting that into a quote to send to the exchange. This is the portion of the code which calculates a fair value (`FairValueEngine`) and generates quotes (`QuotingEngine`)

2) Adapter layer - The engine layer should have no idea about the individual quirks of the exchanges. The engine layer uses the adapter layer to carry out its bidding. The adapter layer also has no idea that it is being used in a manner to make markets. In theory, the adapter layer code and the gateway layer code could be divorced from the Engine layer and we could build a technical analysis or latency arbitrage bot, instead. The adapter layer also contains all the state reported by the gateways.

3) Gateway layer - Each exchange has their own API for interacting with the exchange. All of that business is hidden behind 4 different interfaces:

  * `IMarketDataGateway`: Handles order book updates and market trade updates.
  * `IOrderEntryGateway`: Send and cancel orders and handle updates to the orders.
  * `IPositionGateway`: Pulls in the latest position (how much BTC and USD do I have?) information
  * `IExchangeDetailsGateway`: Read-only information describing naming and exchange fee structure.

Gateways are ideally stateless (some state may be needed in order to perform exchange-specific functionality) and are mostly translation layers between exchange APIs and the K.js API.

# How do I see what's going on?

Navigate to the Web UI as described in the install process. You should see a screen like:

![](https://github.com/ctubio/Krypto-trading-bot/raw/master/etc/img/K.png)

* Market Data and Quotes - this is perhaps the most important screen in the app.

  * The top row in blue with "q" is the quote that you generate via the Quoting Parameters supplied. If the text is grey, the generated quotes are not in the market, and green when they are.

  * "FV" is the fair value price calculated by K.js as the starting point for the generated quote.

  * The "mkt" rows are the best bids and offers on the exchange you are connected to.

* Trades - Trades done by your exchange account. "side" is the side which your order was sent as. "val" is the total value of the trade, which is price * size +/- total exchange fee

* Market Trades - Trades done by all participants in the market. "ms" is the side that was the make side (provided liquidity). The columns starting with "q" are your quotes at the time of the trade, the columns starting with "m" are the best bid and offer information at the time of the trade.

* Quoting Parameters - All of the parameters needed to generate a quote. See the section "How do I control K.js' quotes?" for each field's description.

* Positions - Shows holdings of each currency pair you are using.

* Order List - Shows order statuses of each order sent to the exchange.

  * "Cxl" - clicking the red button will attempt to cancel the order.

# How do I control K.js' quotes?

In the web UI, there are three rows of panels with cryptic looking names and editable textboxes. Those are the quoting parameters, the knobs which we can turn to affect how K.js will trade.

* `%` - If enabled, the values of `bidSize`, `askSize`, `tbp` and `pDiv` will be a percentage related to the total funds (available + held in both sides); useful when the very same funds are used in multiple markets, so the quantity of the funds is highly variable, then may be useful to work with percentages.

* `mode` - Sets the quoting mode

  * `Join` - Sets our quote to be at the best bid and the best offered price, If the BBO is narrower than `width`, set our bid quote at `FV - width / 2` and ask quote at `FV + width / 2`.

  * `Top` - Same as `Join`, but if the code can better the best bid or offer by a penny while respecting the `width`, set that as the quote so we will then be at the top of the market.

  * `Mid` - Set our bid quote at `FV - width / 2` and ask quote at `FV + width / 2`

  * `Inverse Join` - Set the quote at the BBO if the BBO is narrower than `width`, otherwise make the quote so wide that no one will trade with it.

  * `Inverse Top` - Same as `Inverse Join` but make our orders jump to the very top of the order book.

  * `PingPong` - Same as `Top` but always respect the calculated `widthPong` from the last sold or bought `size`, if any.

  * `Boomerang` - Same as `PingPong` but the calculated `widthPong` for new Pongs is based on any best matching previous sold or bought `size`, if any.

  * `AK-47` - Same as `Boomerang` but allows multiple orders at the same time in both sides. To avoid old trades, on every new trade K.js will cancel all previous trades if those are worst.

  * `HamelinRat` - Same as `Boomerang` for now (under development).

  * `Depth` - Use `width` as `depth`. Unlike all other modes, it does not calculate the quote spread based on fair value, instead it walks over all current open orders in the book and places the quote right after `depth` quantity, at both sides.

* `bullets` - Maximum amount of trades placed in each side (only affects `AK-47`).

* `range` - Minimum width between `bullets` in USD (ex. a value of .3 is 30 cents; only affects `AK-47`).

* `pingAt` (Pongs are always placed in both sides, only affects `PingPong`, `Boomerang` and `AK-47`)

  * `BothSides` - Place new Pings in both sides.

  * `BidSide` - Place new Pings only in Bid side, and therefore in Ask side only Pongs will be placed.

  * `AskSide` - Place new Pings only in Ask side, and therefore in Bid side only Pongs will be placed.

  * `DepletedSide` - Place new Pings only in the opposite side with not enough funds to continue trading.

  * `DepletedBidSide` - Place new Pings only in the Ask side if there are not enough funds to continue trading in the Bid side.

  * `DepletedAskSide` - Place new Pings only in the Bid side if there are not enough funds to continue trading in the Ask side.

  * `StopPings` - Only place new Pongs based on the history of Pings, without placing new Pings.

* `pongAt` (only affects `PingPong`, `Boomerang` and `AK-47`)

  * `ShortPingFair` - Place new Pongs based on the lowest margin Ping in history respecting the `widthPong` from the `fair value`.

  * `LongPingFair` - Place new Pongs based on the highest margin Ping in history respecting the `widthPong` from the `fair value`.

  * `ShortPingAggresive` - Place new Pongs based on the lowest margin Ping in history without respecting the `widthPong` from the `fair value`.

  * `LongPingAggresive` - Place new Pongs based on the highest margin Ping in history without respecting the `widthPong` from the `fair value`.

* `bw?` - Enable Best Width to place orders avoiding "hollows" in the book, while accomodating new orders right near to existent orders in the book, without leaving "hollows" in between.

* `%w?` - If enabled, the values of `width` or `widthPing` and `widthPong` will be a percentage related to the `fair value`; useful when calculating profits subtracting exchange's fees (that usually are percentages too).

* `width` and `widthPing` - Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). With the exception for when `apr` is checked and the system is aggressively rebalancing positions after they get out of whack, `width` will always be respected.

* `widthPong` - Minimum width (spread) of our quote in USD (ex. a value of .3 is 30 cents). Used only if previous Pings exists in the opposite side.

* `bidSize` - Maximum bid size of our quote in BTC (ex. a value of 1.5 is 1.5 bitcoins). With the exception for when `apr` is checked and the system is aggressively rebalancing positions after they get out of whack.

* `askSize` - Maximum ask size of our quote in BTC (ex. a value of 1.5 is 1.5 bitcoins). With the exception for when `apr` is checked and the system is aggressively rebalancing positions after they get out of whack.

* `maxBidSize?` and `maxAskSize?` - Use `bidSize` and `askSize` as minimums and automatically find the maximum possible `size` based on the current "Target Base Position" (just as having enabled `apr` on `Size` but even before your position diverges more than `pDiv`).

* `fv` - Sets the fair value calculation mode

  * `BBO` - `FV = ([best bid price] + [best ask price])/2.0`

  * `wBBO` - `FV = ([best bid price]*[best ask size] + [best ask price]*[best bid size])/([best ask size] + [best bid size])`

* `apMode`

  * `Manual` - K.js will not try to automatically manage positions, instead you will need to manually set `tbp`.

  * `EWMA_LS` - K.js will use a `long` minute and `short` minute exponential weighted moving average calculation to buy up BTC when the `short` minute line crosses over the `long` minute line, and sell BTC when the reverse happens. The EWMA values are currently exposed in the stats.

  * `EWMA_LMS` - K.js will use a `long` minute, `medium` minute and `short` minute exponential weighted moving average calculation, together with the simple moving average of the last 3 `fair value` values, to buy up BTC when the `short` minute line crosses over the `long` minute line, and sell BTC when the reverse happens.

* `long` - Only used when `apMode` is `EWMA`. Sets the periods of EWMA Long to automatically manage positions.

* `medium` - Not used yet. Sets the periods of EWMA Medium.

* `short` - Only used when `apMode` is `EWMA`. Sets the periods of EWMA Short to automatically manage positions.

* `sensibility` - Threshold removed from each period, affects EWMA Long, Medium and Short. The decimal value of `sensibility` must be betweem 0 and 1.

* `tbp` - Only used when `apMode` is `Manual`. Sets a static "Target Base Position" for K.js to stay near. In manual position mode, K.js will still try to respect `pDiv` and not make your position fluctuate by more than that value. So if you have 10 BTC to trade, set `tbp = 3`, set `apMode = Manual`, and `pDiv = 1`, your holding of BTC will never be less than 2 or greater than 4.

* `pDiv` - If your "Target Base Position" diverges more from this value, K.js will stop sending orders to stop too much directional trading. So if you have 10 BTC to trade, "Target Base Position" is reporting 5, and `pDiv` is set to 3, your holding of BTC will never be less than 2 or greater than 8.

* `apr` - If you're in a state where K.js has stopped sending orders because your position has diverged too far from Target Base Position, this setting will much more aggressively try to fix that discrepancy by placing orders much larger than `size` and at prices much more aggressive than `width` normally allows (see `pongAt` option). It's a bit risky to use this setting.

  * `Off` - K.js will not try to aggressively try to stabilize the target based position.

  * `Size` - K.js will aggressively make use of bigger `size` values (x3 `size` or half of the diverged target base position, whatever is smaller).

  * `SizeWidth` - Same as `Size` but also will aggressively make use of smaller `width` values (respecting always aggressive `pongAt` option and `widthPong`).

* `sop` - Super opportunities, if enabled and if the market width is x3 times bigger than the `width` set, it multiplies (x2 or x3) the `trades` and/or the `size`, in both sides at the same time.

* `trades` - Often, only buying or selling many times in a short timeframe indicates that there is going to be a price swing. `trades` and `/sec` are highly related: If you successfully complete more orders than `trades` in `/sec` seconds, K.js will stop sending more buy orders until either `/sec` seconds has passed, or you have sold enough at a higher cost to make all those buy orders profitable. The number of trades is reported by side in the UI; "BuyTS", "SellTS", and "TotTS". If "BuyTS" goes above `trades`, K.js will stop sending buy orders, and the same for sells. For example, if `trades` is 2 and `/sec` is 1800 (half an hour):

Time     | Side | Price | Size | BuyTS | SellTS | Notes
-------- |------|------:|-----:|------:|-------:|------------------------------------------------:
12:00:01 | Buy  | 10    | 1    | 1     | 0      |
12:00:02 | Buy  | 10    | 0.5  | 1.5   | 0      | Partial fills of `size` get counted fractionally
12:00:03 | Sell | 11    | 0.75 | 0.75  | 0      | Sell for more decrements the imbalance
12:00:05 | Sell | 5     | 0.75 | 0.75  | 0      | Sell for less than the other buys doesn't help
12:00:06 | Buy  | 10    | 0.5  | 1.75  | 0      |
12:00:07 | Buy  | 10    | 0.5  | 2.75  | 0      | Stop sending buy orders until 12:30:07!

* `/sec` - see `trades`.

* `ewma?` - Use a quote protection of `periods` smoothed line of the price to limit the price while sending new orders.

* `periodsᵉʷᵐᵃ` - Maximum amount of values collected in the sequences used to calculate the `ewma?` quote protection. After collect sequentially every 1 minute the value of the `fair value`, and before place new orders, a limit will be always applied to the new orders price using a `ewma` calculation, taking into account only the last `periods` periods in each sequence.

* `stdev`

  * `Off` - Do not limit the price of new orders.

  * `OnFV` - Use a quote protection of STDEV, calculated from a sequence of `fair value` values during `periods` periods of 1 second, to limit the price equally on both sides while sending new orders.

  * `OnFVAPROff` - Same as `OnFV` when `apr` is `Off` or when the system is not aggressively rebalancing positions; otherwise if is rebalancing, is same as `Off`.

  * `OnTops` - Use a quote protection STDEV, calculated from a unique sequence of both `best bid` and `best ask` values in the market order book during `periods * 2` periods of 1 second, to limit the price equally on both sides while sending new orders.

  * `OnTopsAPROff` - Same as `OnTops` when `apr` is `Off` or when the system is not aggressively rebalancing positions; otherwise if one side is rebalancing, is same as `Off` for that side.

  * `OnTop` - Use a quote protection STDEV, calculated from two sequences of the `best bid` (first sequence) and also of the `best ask` (second sequence) value in the market order book during `periods` periods of 1 second, to limit the price independently on each side while sending new orders.

  * `OnTopAPROff` - Same as `OnTop` when `apr` is `Off` or when the system is not aggressively rebalancing positions; otherwise if one side is rebalancing, is same as `Off` for that side.

* `periodsˢᵗᵈᶜᵛ` - Maximum amount of values collected in the sequences used to calculate the STDEV, each side may have its own STDEV calculation with the same amount of `periods`. After collect sequentially every 1 second the values of the `fair value`, `last bid` and also of the `last ask` from the market order book, and before place new orders, a limit will be always applied to the new orders price using a calculation of the STDEV, taking into account only the last `periods` periods in each sequence.

* `factor` - Multiplier used to increase or decrease the value of the selected `stdev` calculation, a `factor` of 1 does effectively nothing.

* `BB?` - Enable Bollinger Bands with upper and lower bands calculated from the result of the selected `stdev` above or below its own moving average of `periods`.

* `delayAPI` - Relax the new orders send to the exchange API by `delayAPI` orders/60seconds, for example `delayAPI=0` does not apply any delay but `delayAPI=75` will send a new order once every 800 milliseconds, or `delayAPI=3` will send a new order once every 20 seconds.

* `cxl?` - Enable a timeout of 5 minutes to cancel all orders that exist as open in the exchange (in case you found yourself with zombie orders in the exchange, because the API integration have bugs or because the connection is interrupted).

* `profit` - Interval in hours to recalculate the display of Profit (under wallet values), for example a `profit` of 0.5 will compare the current wallet values and the values from half hour ago to display the +/- % of increment between both.

* `Kmemory` - Timeout in days for Pings (yet unmatched trades) and/or Pongs (K trades) to remain in memory, a value of `0` keeps the history in memory forever; a positive value remove only Pongs after `Kmemory` days; but a negative value remove both Pings and Pongs after `Kmemory` days (for example a value of `-2` will keep a history of trades no longer than 2 days without matter if Pings are not matched by Pongs).

* `delayUI` - Relax the display of UI data by `delayUI` seconds. Set a value of 0 (zero) to display UI data in realtime, but this may penalize the communication with the exchange if you end up sending too much frequent UI data (like in low latency environments with super fast market data updates; at home is OK in realtime because the latency of K.js with the exchange tends to be higher than the latency of K.js with your browser).

* `audio?` - plays a sound for each new trade (ping-pong modes have 2 sounds for each type of trade).


# How can I make up my own trading strategies?

Lets say that you have a great idea about how to profitably make markets and are comfortable getting your hands dirty in some TypeScript.  The steps are quite easy

1. Create a class implementing `QuoteStyle` found in `src/server/quoting-styles/helpers.ts`. As part of implementing the interface, you'll need to create a new enum member for `QuotingMode` in models.ts. Inside that class you can create any sort of logic to create a two-sided market, or return null to signify that there should be no quote in the market. The interface gives you:

  a. The latest order book depth of the market. On most exchanges, this is the best 3 price levels. Individual orders are not supported since not all exchanges support that functionality. Last trades in the market are also not yet provided (though reasonably should in the future).

  b. The calculated fair value.

  c. The current quoting parameters, viewable in the web UI.

2. Install the new class alongside the provided list of modes in the `QuotingStyleRegistry` in `main.js`.

3. Rebuild with `npm install` and restart with `npm restart`.

Alternatively, you can always open a [new issue](https://github.com/ctubio/Krypto-trading-bot/issues/new?title=Missing%20trading%20strategy) to request a new feature.

# How can I test new trading strategies?

K.js is packaged with a test unit mode.

To create your own test runs, you can duplicate at your convenience the mocha test units at `test/*.ts`.
