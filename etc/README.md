### tribeca.json.dist
Sample configuration file, must be located inside `etc` folder, to initialize your configurations:
```
 $ cd /path/to/tribeca
 $ cp etc/tribeca.json.dist etc/tribeca.json
 $ vim etc/tribeca.json
```

You must end up with a customized `etc/tribeca.json` file, see all option details bellow.

### Configuration options

  * EXCHANGE

    1. `coinbase` - uses the WebSocket API. Ensure the Coinbase-specific properties have been set with your correct account information if you are using the sandbox or live-trading environment.

    2. `hitbtc` - WebSocket + socket.io API. Ensure the HitBtc-specific properties have been set with your correct account information if you are using the dev or prod environment.

    3. `okcoin` - Websocket.Ensure the OKCoin-specific properties have been set with your correct account information. Production environment only.

    4. `bitfinex` REST API only. Ensure the Bitfinex-specific properties have been filled out. REST API is not suitable to millisecond latency trading. Production environment only.

    5. `null` - Test in-memory exchange. No exchange-specific config needed.

  * TRIBECA_MODE - Any value is valid, and optionally can be prefixed with `auto` to start trading on boot, for example:

    1. `bot #1` - Shows `bot #1` in the title.

    2. `autobot #1` - Shows `bot #1` in the title and auto start trading on boot.

    1. `My Bot` - Shows `My Bot` in the title.

    2. `autoMy Bot` - Shows `My Bot` in the title and auto start trading on boot.


  * MongoDbUrl - If you are on OS X, change "tribeca-mongo" in the URL to the output of `boot2docker ip` on your host machine. If you are running an existing mongoDB instance, replace the URL with the existing instance's URL. If you are running from a Linux machine and set up mongo in step 1, you should not have to modify anything.

  * TradedPair - The following currency pairs are supported on these exchanges:

    1. `BTC/USD` - Coinbase, HitBtc, OkCoin, Null

    2. `BTC/EUR` - Coinbase, HitBtc, Null

    3. `BTC/GBP` - Coinbase, Null

  * WebClientUsername and WebClientPassword - Username and password for [web UI](https://github.com/ctubio/tribeca#web-ui) access. If kept as `NULL`, no the web client will not require authentication (Not recommended at all!!)

Input your exchange connectivity information, account information, and API keys in the config properties for the exchange you intend on trading on.

If you set in your exchange the `OrderDestination` value as `Null`, all API calls will be redirect to NullGateway (testing mode).

These options are also valid environment variables for the [dist/Dockerfile](https://github.com/ctubio/tribeca/tree/master/dist#dockerfile) file.