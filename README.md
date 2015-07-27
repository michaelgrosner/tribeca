# tribeca

`tribeca` is a very low latency cryptocurrency market making trading bot with a full featured web client, backtester, and supports direct connectivity to several large cryptocoin exchanges. On modern hardware, it can react to market data by placing and canceling orders in under a millisecond. It runs on v0.12 nodejs or the latest io.js. Persistence is acheived using mongodb.

### Installation

0) Installation is acheived using [docker](https://www.docker.com/). Please install docker for your system before preceeding. Mac only: Ensure boot2docker is already running. See [the docs](https://docs.docker.com/installation/mac/) for more help.

1) Set up mongodb. If you do not have a mongo container already running: `docker run -p 27017:27017 --name tribeca-mongo -d mongo` will get you started.

2) Copy the repository [Dockerfile](https://raw.githubusercontent.com/michaelgrosner/tribeca/master/Dockerfile) into a text editor. Change the environment variables to input your exchange connectivity information, account information, and mongoDB credentials.

  * EXCHANGE
  
    1) `coinbase` - uses the WebSocket API. Ensure the Coinbase-specific properties have been set with your correct account information if you are using the sandbox or live-trading environment.
    
    2) `hitbtc` - WebSocket + socket.io API. Ensure the HitBtc-specific properties have been set with your correct account information if you are using the dev or prod environment.
    
    3) `null` - Test in-memory exchange. No exchange-specific config needed.
    
  * TRIBECA_MODE
  
    1) `prod`
    
    2) `dev`
    
  * MongoDbUrl - If you are on OS X, change "tribeca-mongo" in the URL to the output of `boot2docker ip` on your host machine. If you are running an existing mongoDB instance, replace the URL with the existing instance's URL.

3) Save the Dockerfile, preferably in a secure location and in an empty directory. Build the image from the Dockerfile `docker build -t tribeca .`

4) Run the container `docker run -p 3000:3000 --name tribeca -d tribeca`. If you run `docker ps`, you should see tribeca and mongo containers running.

5) Open your browser to http://localhost:3000. Set up parameters to your trading liking.

6) Click the "BTC/USD" button in the web admin so it is green to start making markets

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

1) Write up descriptions of all the parameters and their functionality

2) More configurable currency pairs. Not everyone trades BTC/USD

### Donations

If you would like to support this project, please consider donating to 1BDpAKp8qqTA1ASXxK2ekk8b8metHcdTxj
