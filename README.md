[***REFUGEES WELCOME!***](http://www.refugeesaid.eu/rab-campaign/)

[![Release](https://img.shields.io/github/release/ctubio/tribeca.svg)](https://github.com/ctubio/tribeca/releases)
[![Platform](https://img.shields.io/badge/platform-unix--like-lightgray.svg)](https://www.gnu.org/)
[![Software License](https://img.shields.io/badge/license-ISC-111111.svg)](https://raw.githubusercontent.com/ctubio/tribeca/master/LICENSE)
[![Software License](https://img.shields.io/badge/license-MIT-111111.svg)](https://raw.githubusercontent.com/ctubio/tribeca/master/COPYING)

`tribeca.js` is a very low latency cryptocurrency [market making](https://github.com/ctubio/tribeca/blob/master/MANUAL.md#what-is-market-making) trading bot with a full featured [web client](https://github.com/ctubio/tribeca#web-ui), [backtester](https://github.com/ctubio/tribeca/blob/master/MANUAL.md#how-can-i-test-new-trading-strategies), and supports direct connectivity to [several cryptocoin exchanges](https://github.com/ctubio/tribeca/tree/master/etc#configuration-options). On modern hardware, it can react to market data by placing and canceling orders in under a millisecond.

[![Build Status](https://img.shields.io/travis/ctubio/tribeca/master.svg?label=test%20build)](https://travis-ci.org/ctubio/tribeca)
[![Coverage Status](https://img.shields.io/coveralls/ctubio/tribeca/master.svg?label=code%20coverage)](https://coveralls.io/r/ctubio/tribeca?branch=master)
[![Quality Status](https://img.shields.io/codacy/grade/21564745dbb0449ca05912f77d484b0c/master.svg)](https://www.codacy.com/app/ctubio/tribeca)
[![Dependency Status](https://img.shields.io/david/ctubio/tribeca.svg)](https://david-dm.org/ctubio/tribeca)
[![Open Issues](https://img.shields.io/github/issues/ctubio/tribeca.svg)](https://github.com/ctubio/tribeca/issues)

Runs on the latest node.js (v6 or v7). Persistence is achieved using mongodb. Installation via Docker is supported, but manual installation in a dedicated fresh unix-like instance is recommended.

![Web UI Preview](https://raw.githubusercontent.com/ctubio/tribeca/master/dist/img/web_ui_preview.png)

### Compatible Exchanges

with Post-Only Orders support:

1. Coinbase GDAX

2. Bitfinex

without:

3. OKCoin.com OKCoin.cn

4. HitBTC

All currency pairs are supported, otherwise please open a [new issue](https://github.com/ctubio/tribeca/issues/new?title=Missing%20currency%20pair) to easily include any missing currency that you would like.

### Docker Installation

See [dist/Dockerfile](https://github.com/ctubio/tribeca/tree/master/dist#dockerfile) section if you use winy or mac.

### Manual Installation

1. Ensure your target machine has installed g++ compiler (`g++ -v`), node v6 or v7 (`nodejs -v`) and mongoDB v3 or greater (`mongo --version`).

2. Run `git clone ssh://git@github.com/ctubio/tribeca` in any location that you wish.

3. Copy `etc/tribeca.json.dist` to `etc/tribeca.json` and modify the configuration options, see [configuration](https://github.com/ctubio/tribeca/tree/master/etc#configuration-options) section. Point the instance towards the running mongoDB instance (usually just `mongodb://localhost:27017/tribeca`).

4. Run `npm start` in the toplevel path of the git cloned repository. This will run `tribeca.js` in the background using [forever](https://www.npmjs.com/package/forever). But before, it will install all local dependencies in `node_modules` folder and will compile TypeScript in `src` folder to CommonJS in `app` folder if it was not already done before.

Optional:

1. Install the system daemon script `dist/tribeca-init.sh` (to make use of `service tribeca start` from anywhere instead of `cd path/to/tribeca && npm start`) see [dist](https://github.com/ctubio/tribeca/tree/master/dist) folder.

2. Replace the certificate at `etc/sslcert` folder with your own, see [web ui](https://github.com/ctubio/tribeca#web-ui) section. But, the certificate provided is a fully featured default openssl, that you may just need to authorise in your browser.

3. Set environment variable TRIBECA_CONFIG_FILE to full path of `tribeca.json` if you run the app manually from other locations with `nodejs path/to/tribeca.js`. The environment variable is not needed if the working directory is the root folder where `tribeca.js` is located.

### Configuration

See [etc](https://github.com/ctubio/tribeca/tree/master/etc) folder.

### Upgrade to the latest commit

Feel free anytime to check if there are new modifications with `npm run diff`.

Once you decide that is time to upgrade, execute `npm run latest` to download and install the latest modifications in your remote branch (or directly `npm run reinstall` to skip the display of the new commit messages).

If you are running multiple instances, this will stop all instances and only restart `tribeca.json`.

### Multiple instances party time

Please note, an "instance" is in fact a config file under `etc` folder; using a single machine and the same source folder, you can run as many instances as config files you have in `etc` folder (limited by the available free RAM).

You can list the current instances running anytime with `npm run list`.

Simple commands like `npm start`, `npm stop` or `npm restart` (without any config file defined) will use the default config file `etc/tribeca.json`.

To run multiple instances using a collection of config files:

1. Create a new config file with `cp etc/tribeca.json etc/Xibeca_party_time.json` (with any name but `.json` extension).

    1. Edit the value of `WebClientListenPort` in the new config file to set a new port, so all applications have a unique port to display the UI.

    2. Edit the value of `MongoDbUrl` in the new config file to set a new database name, so all applications have a unique database to save the data. You dont need to modify the host:port because a single database host can have multiple databases inside.

    3. Edit the values of `TRIBECA_MODE`, `EXCHANGE` and `TradedPair` in the new config file as you alternatively desire.

2. Run the new instance with `npm start --tribeca:config=Xibeca_party_time.json`, also the commands `npm stop` and `npm restart` allow the parameter `--tribeca:config=`, the value is simply the filename of the config file under `etc` folder that you want to run; this value will also be used as the `uid` of the process executed by `forever`.

3. Open in the web browser the different pages of the ports of the different running instances, or display the UI of all instances together in a single page using the MATRYOSHKA link in the footer and the config option `MatryoshkaUrl`.

### Application Usage

1. Open your web browser to connect to HTTPS port `3000` (or value of `WebClientListenPort`) of the machine running tribeca. If you're running tribeca locally on Mac/Windows on Docker, replace "localhost" with the address returned by `boot2docker ip`.

2. Read up on how to use tribeca and market making in the [manual](https://github.com/ctubio/tribeca/blob/master/MANUAL.md).

3. Set up trading parameters to your liking in the web UI. Click the "BTC/USD" button so it is green to start making markets.

### Web UI

Once `tribeca` is up and running, visit HTTPS port `3000` (or value of `WebClientListenPort`) of the machine on which it is running to view the admin view. There are inputs for quoting parameters, grids to display market orders, market trades, your trades, your order history, your positions, and a big button with the currency pair you are trading. When you're ready, click that button green to begin sending out quotes. The UI uses a healthy mixture of socket.io and angularjs observed with reactivexjs.

If you want to generate your own certificate see [SSL for internal usage](http://www.akadia.com/services/ssh_test_certificate.html).

In case you really want to use plain HTTP, remove the files `server.crt` and `server.key` inside `etc/sslcert` folder.

### REST API

Tribeca also exposes a REST API of all it's data. It's all the same data you would get via the Web UI, just a bit easier to connect up to via other applications. Visit `http://localhost:3000/data/md` for the current market data, for instance.

### Charts

The metrics are not saved anywhere, is just UI data collected with a visibility retention of 6 hours, to display over time:

 * Market Fair Value with High and Low Prices
 * Trades Complete
 * Target Position for BTC currency (TBP)
 * Target Position for Fiat currency
 * EWMA values for Quote Protection and APR
 * Amount available in wallet for buy
 * Amount held in open trades for buy
 * Amount available in wallet for sell
 * Amount held in open trades for sell
 * Total amount available and held at both sides in BTC currency
 * Total amount available and held at both sides in Fiat currency

### Test units and Build notes

Feel free to run `npm test` anytime.

To rebuild the application with your modifications, please run `npm install` or directly `npm run postinstall`.

To pipe the output to stdout, execute the application in the foreground with `nodejs tribeca.js`.

To ignore the output, execute the application in the background with `forever start tribeca.js` or with the alias `npm start`.

To debug the server code with chrome-devtools, attach the node debugger with `nodejs --inspect tribeca.js` (from your local, you can open a ssh tunnel to access it with `ssh -N -L 9229:127.0.0.1:9229 user@host`).

Another way to debug is to use some `if (log.debug())` blocks here and there, then execute it passing `debug` parameter after `tribeca.js`.

Passing a config filename as a parameter after `tribeca.js` is also allowed.


### Unreleased Changelog:

Added nodejs7, typescript2, angular4 and reactivexjs.

Added cleanup of bandwidth, source code, dependencies and installation steps.

Added support for multiple instances/config files with nested matryoshka UI.

Added npm scripts, david-dm, travis-ci, coveralls and codacy.

Added historical charts to replace grafana.

Updated OKCoin API (since https://www.okcoin.com/t-354.html).

Added Bitfinex API v2.

### Release 2.0 Changelog:.

Added new quoting styles PingPong, Boomerang, AK-47.

Added cleanup of database records, memory usage and log recording.

Added audio notices, realtime wallet display, and grafana integration.

Added https, dark theme and new UI elements.

Added a bit of love to Kira.

### Release 1.0 Changelog:

see the upstream project [michaelgrosner/tribeca](https://github.com/michaelgrosner/tribeca).

### Donations

nope, this project doesn't have maintenance costs. but you can donate to your favorite developer today! (or tomorrow!)

or see the upstream project [michaelgrosner/tribeca](https://github.com/michaelgrosner/tribeca).

or donate your time with programming or financial suggestions in the topical IRC channel **##tradingBot** at irc.domirc.net on port 6697 (SSL), or 6667 (plain) or feel free to make any question, but questions technically are not donations.