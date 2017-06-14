### K.json.dist
Sample configuration file, must be located inside `etc` folder, to initialize your configurations:
```
 $ cd /path/to/K
 $ cp etc/K.json.dist etc/K.json
 $ vim etc/K.json
```

You must end up with a customized `etc/K.json` file, see all option details bellow.

### Configuration options

  * EXCHANGE - must be one of:

    1. `coinbase` - REST + WebSocket + FIX Protocol. Ensure the Coinbase-specific properties have been set with your correct account information if you are using the sandbox or live-trading environment.

    2. `hitbtc` - REST + WebSocket. Ensure the HitBtc-specific properties have been set with your correct account information if you are using the dev or prod environment.

    3. `okcoin` - REST + Websocket. Ensure the OKCoin-specific properties have been set with your correct account information. Production environment only.

    4. `bitfinex` - REST + WebSocket. Ensure the Bitfinex-specific properties have been filled out. REST API is not suitable to millisecond latency trading. Production environment only.

    5. `korbit` - REST only. Ensure the Bitfinex-specific properties have been filled out. REST API is not suitable to millisecond latency trading. Production environment only.

    6. `null` - Test in-memory exchange. No exchange-specific config needed.

  * BotIdentifier - Any value is valid, and optionally can be prefixed with `auto` to start trading on boot, for example:

    1. `bot #1` - Shows `bot #1` in the title.

    2. `autobot #1` - Shows `bot #1` in the title and auto start trading on boot.

    3. `My Bot` - Shows `My Bot` in the title.

    4. `autoMy Bot` - Shows `My Bot` in the title and auto start trading on boot.


  * MongoDbUrl - If you are on macOS, change "K-mongo" in the URL to the output of `boot2docker ip` on your host machine. If you are running an existing mongoDB server, replace the URL with the existing server's URL. If you are running from a Linux machine, you should not have to modify anything (usually is just `mongodb://localhost:27017/K`).

  * TradedPair - Any combination of the following currencies are supported, if the target EXCHANGE supports trading the currency pair:

    - USD
    - BTC
    - LTC
    - EUR
    - GBP
    - CNY
    - CAD
    - ETH
    - ETC
    - BFX
    - RRT
    - ZEC
    - BCN
    - DASH
    - DOGE
    - DSH
    - EMC
    - FCN
    - LSK
    - NXT
    - QCN
    - SDB
    - SCB
    - STEEM
    - XDN
    - XEM
    - XMR
    - ARDR
    - WAVES
    - BTU
    - MAID
    - AMP
    - XRP
    - KRW
    - IOT

  * WebClientUsername and WebClientPassword - Username and password for [web UI](https://github.com/ctubio/Krypto-trading-bot#web-ui) access. If kept as `NULL`, the web client will not require authentication (not recommended)

  * MatryoshkaUrl - The URL of the next instance, it can be used to link one after another all running instances (see [Multiple instances party time](https://github.com/ctubio/Krypto-trading-bot#multiple-instances-party-time)); it will only autofill the prompt to avoid to type it everytime.

Input your exchange connectivity information, account information, and API keys in the config properties for the exchange you intend on trading on (Coinbase GDAX needs one single different API key for each different market, because users of FIX Protocol can't have multiple sessions opened at the same time [so just use multiple API keys]).

If you set in your exchange the `OrderDestination` value as `Null`, all API calls will be redirect to NullGateway (testing mode).

These options are also valid environment variables for the [dist/Dockerfile](https://github.com/ctubio/Krypto-trading-bot/tree/master/dist#dockerfile) file.

As additional non-mandatory options, all quoting parameters (or some of them) can be added to the config file too (in case you need to preconfigure multiple instances, or if you have database issues, or simply if you want to easily persist your desired quoting parameters [see the valid option names at [defaultQuotingParameters](https://github.com/ctubio/Krypto-trading-bot/blob/master/src/server/main.ts#L45)]). Once parameters are saved into the database, the database values will be used instead of these default options.