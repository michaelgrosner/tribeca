# tribeca

![status](https://david-dm.org/ctubio/tribeca.svg)

`tribeca` is a very low latency cryptocurrency [market making](https://github.com/ctubio/tribeca/blob/master/HOWTO.md#what-is-market-making) trading bot with a full featured [web client](https://github.com/ctubio/tribeca#web-ui), [backtester](https://github.com/ctubio/tribeca/blob/master/HOWTO.md#how-can-i-test-new-trading-strategies), and supports direct connectivity to [several cryptocoin exchanges](https://github.com/ctubio/tribeca#configuration). On modern hardware, it can react to market data by placing and canceling orders in under a millisecond.

![Web UI Preview](https://raw.githubusercontent.com/ctubio/tribeca/master/dist/img/web_ui_preview.png)

Runs on the latest node.js (v6 or greater). Persistence is acheived using mongodb. Installation via Docker is supported, but manual installation in a dedicated fresh unix-like instance is recommended.

### Docker Installation

See [dist/Dockerfile](https://github.com/ctubio/tribeca/tree/master/dist#dockerfile) section if you use winy or mac.

### Manual Installation

1. Ensure your target machine has node v6 or greater (`nodejs -v`) and mongoDB v3 or greater (`mongo --version`).

2. Clone the repository somewhere with `git clone ssh://git@github.com/ctubio/tribeca`.

3. In the cloned repository directory, `npm install` to pull in all dependencies and also compile TypeScript to CommonJS.

4. (optional) Replace the certificate at `etc/sslcert` folder with your own. (see [web ui](https://github.com/ctubio/tribeca#web-ui) section).

5. (optional) Install forever globally (`npm i -g forever`) and setup the system daemon script `dist/tribeca-init.sh` (see [dist](https://github.com/ctubio/tribeca/tree/master/dist) folder).

6. Copy `etc/tribeca.json.dist` to `etc/tribeca.json`, and modify the config keys (see [configuration](https://github.com/ctubio/tribeca#configuration) section). Point the instance towards the running mongoDB instance (usually just `mongodb://localhost:27017/tribeca`).

7. Run `npm start` in the root folder to start the app, or `service tribeca start` anywhere if the init.d script `dist/tribeca-init.sh` is installed).

8. (optional) Set environment variable TRIBECA_CONFIG_FILE to full path of `tribeca.json` if you run the app manually from other locations with `nodejs path/to/tribeca.js` or with forever globally installed `forever start path/to/tribeca.js`. The environment variable is not needed if the working directory is the root folder where `tribeca.js` is located.

### Configuration

  * EXCHANGE

    1. `coinbase` - uses the WebSocket API. Ensure the Coinbase-specific properties have been set with your correct account information if you are using the sandbox or live-trading environment.

    2. `hitbtc` - WebSocket + socket.io API. Ensure the HitBtc-specific properties have been set with your correct account information if you are using the dev or prod environment.

    3. `okcoin` - Websocket.Ensure the OKCoin-specific properties have been set with your correct account information. Production environment only.

    4. `bitfinex` REST API only. Ensure the Bitfinex-specific properties have been filled out. REST API is not suitable to millisecond latency trading. Production environment only.

    5. `null` - Test in-memory exchange. No exchange-specific config needed.

  * TRIBECA_MODE

    1. `prod`

    2. `dev`

  * MongoDbUrl - If you are on OS X, change "tribeca-mongo" in the URL to the output of `boot2docker ip` on your host machine. If you are running an existing mongoDB instance, replace the URL with the existing instance's URL. If you are running from a Linux machine and set up mongo in step 1, you should not have to modify anything.

  * TradedPair - The following currency pairs are supported on these exchanges:

    1. `BTC/USD` - Coinbase, HitBtc, OkCoin, Null

    2. `BTC/EUR` - Coinbase, HitBtc, Null

    3. `BTC/GBP` - Coinbase, Null

  * WebClientUsername and WebClientPassword - Username and password for [web UI](https://github.com/ctubio/tribeca#web-ui) access. If kept as `NULL`, no the web client will not require authentication (Not recommended at all!!)

Input your exchange connectivity information, account information, and API keys in the config properties for the exchange you intend on trading on.

### Application Usage

1. Open your web browser to connect to port 3000 of the machine running tribeca. If you're running tribeca locally on Mac/Windows on Docker, replace "localhost" with the address returned by `boot2docker ip`.

2. Read up on how to use tribeca and market making in the [wiki](https://github.com/ctubio/tribeca/blob/master/HOWTO.md).

3. Set up trading parameters to your liking in the web UI. Click the "BTC/USD" button so it is green to start making markets.

### Web UI

Once `tribeca` is up and running, visit HTTPS port `3000` of the machine on which it is running to view the admin view. There are inputs for quoting parameters, grids to display market orders, market trades, your trades, your order history, your positions, and a big button with the currency pair you are trading. When you're ready, click that button green to begin sending out quotes. The UI uses a healthy mixture of socket.io and angularjs.

If you want to generate your own certificate see [SSL for internal usage](http://www.akadia.com/services/ssh_test_certificate.html).

### REST API

Tribeca also exposes a REST API of all it's data. It's all the same data you would get via the Web UI, just a bit easier to connect up to via other applications. Visit `http://localhost:3000/data/md` for the current market data, for instance.

### Grafana + InfluxDB + CollectD

Tribeca send metrics periodically to [StatsD plugin of CollectD](https://collectd.org/wiki/index.php/Plugin:StatsD) on default port 8125

You can setup a Grafana instance with a InfluxDB datasource to read the metrics from CollectD send by Tribeca.

The metrics send are:

 * Fair Value
 * Amount available in wallet for buy
 * Amount held in open trades for buy
 * Amount available in wallet for sell
 * Amount held in open trades for sell
 * Total amount available in wallet and held in open trades in BTC currency
 * Total amount available in wallet and held in open trades in Fiat currency

### TEST UNITS

Feel free to run `npm test` anytime.

### KNOWN BUGS:

1. Occasionnaly trades would not be correclty cancelled. (Coinbase seems to cancel orders always OK; what otherexchanges fail occasionnaly?)

2. Under EwmaBasic, when the TBP reaches 0 or the total size of the portfolio, it gets stuck there forever.

3. Orders do not last more than a few miliseconds on OkCoin.

4. Browserify does not minify es6 yet, so bundle.min.js is not really minified.

### TODO:

-1. concurrent exchanges, display charts, chat room

0. Migrate to angular 2 and typescript 2

1. Add new exchanges

2. Add new, smarter trading strategies (as always!)

3. Support for currency pairs which do not trade in $0.01 increments (LTC, DOGE)

4. More documentation

5. More performant UI

### Unreleased Changelog:

Added nodejs6, typescript2 and angular2.

Added npm install scripts instead of grunt tasks.

### Release 2.0 Changelog:

Added new quoting styles PingPong, Boomerang, AK-47.

Added cleanup of database records, memory usage and log recording.

Added audio notices, realtime wallet display, and grafana integration.

Added https, dark theme and new UI elements.

Added a bit of love to Kira.

### Release 1.0 Changelog:

see the upstream project [michaelgrosner/tribeca](https://github.com/michaelgrosner/tribeca).

### Donations

nope. but you can donate to your favorite developer today! (or tomorrow!)

or see the upstream project [michaelgrosner/tribeca](https://github.com/michaelgrosner/tribeca).
