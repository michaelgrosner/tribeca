# tribeca

`tribeca` is a very low latency cryptocurrency market making trading bot with a full featured web client, backtester, and supports direct connectivity to several large cryptocoin exchanges. On modern hardware, it can react to market data by placing and canceling orders in under a millisecond. It runs on v0.12 nodejs or the latest io.js. Persistence is acheived using mongodb.

`tribeca` can be installed "by-hand" or via Docker

### Manual Installation

#### Pre-reqs

1) nodejs v0.12 or latest io.js

2) MongoDB

3) Typescript 1.5 with [tsd](https://github.com/DefinitelyTyped/tsd)

4) Grunt

#### Steps

0) Make sure your machine has all listed pre-reqs 

1) `npm install` and then `tsd reinstall -s`

2) Compile typescript to javascript via `grunt compile`

3) Copy `sample-prod-tribeca.json` or `sample-dev-tribeca.json` into `src/service/tribeca.json`. Modify the various parameters to include API keys and point it to your running mongo instance.

4) Run tribeca via `node main.js`. It may also be prudent to run via a tool like forever.js or supervisor to keep tribeca running after any unexpected crashes.

5) Set up parameters to your trading liking in the web admin (http://localhost:3000)

6) Click the "BTC/USD" button in the web admin so it is green to start making markets

### Docker Installation

COMING SOON

### Environment variables

#### EXCHANGES

1) Coinbase - uses the WebSocket API

2) HitBtc - WebSocket + socket.io API

3) Null - Test in-memory exchange

#### Quoting Parameters

Note: only a few parameters are listed currently. Peek at src/common/models.ts:QuotingParameters for the full list
of parameters

1) `width`: sell orders will always be placed for this many dollars more than buy orders. Defaults to $0.30

2) `size`: amount of coins to make each side of the quote. Defaults to 0.05 BTC

3) `mode`: quoting style to use. Some modes will look at other orders in the market, the midpoint of the spread, etc.

4) `tradesPerMinute` and `tradeRateSeconds`: only allow `tradesPerMinute` (disregard the minutes portion of the name...) trades in every `tradeRateSeconds` window.

### Web UI

![Web UI Preview](http://i.imgur.com/FY4hhp2.png)

Once `tribeca` is up and running, visit port `3000` of the machine on which it is running to view the admin view. There are inputs for quoting parameters, grids to display market orders, market trades, your trades, your order history, your positions, and a big button with the currency pair you are trading. When you're ready, click that button green to begin sending out quotes. The UI uses a healthy mixture of socket.io and angularjs.

### REST API

Tribeca also exposes a REST API of all it's data. It's all the same data you would get via the Web UI, just a bit easier to connect up to via other applications. Visit `http://localhost:3000/data/md` for the current market data, for instance.


### TODO

TODO:

1) Create a VM image or something to make deployment much much easier

2) Write up descriptions of all the parameters and their functionality

3) More configurable currency pairs. Not everyone trades BTC/USD

### Donations

If you would like to support this project, please consider donating to 1BDpAKp8qqTA1ASXxK2ekk8b8metHcdTxj
