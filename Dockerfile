FROM node:5
RUN apt-get update

RUN apt-get install -y git

RUN git clone https://github.com/michaelgrosner/tribeca.git

WORKDIR tribeca

# Chose the branch you want to build. Leave as it is if you want to build the master branch (recommanded).
# RUN git checkout -B YOUR-NAME-OF-THE-BRANCH --track remotes/origin/NAME-OF-THE-BRANCH-IN-GITHUB

RUN npm install -g grunt-cli typings@0.8.1 forever
RUN npm install
RUN typings install
RUN grunt compile

EXPOSE 3000 5000

# General config properties. Properties with `NULL` should be replaced with your own exchange account information.
ENV TRIBECA_MODE dev
ENV EXCHANGE null
ENV TradedPair BTC/USD
ENV WebClientUsername NULL
ENV WebClientPassword NULL
ENV WebClientListenPort 3000
# IP to access mongo instance. If you are on a mac, run `boot2docker ip` and replace `tribeca-mongo`.
ENV MongoDbUrl mongodb://tribeca-mongo:27017/tribeca

# DEV
## HitBtc
ENV HitBtcPullUrl http://demo-api.hitbtc.com
ENV HitBtcOrderEntryUrl ws://demo-api.hitbtc.com:8080
ENV HitBtcMarketDataUrl ws://demo-api.hitbtc.com:80
ENV HitBtcSocketIoUrl https://demo-api.hitbtc.com:8081
ENV HitBtcApiKey NULL
ENV HitBtcSecret NULL
ENV HitBtcOrderDestination HitBtc
## Coinbase
## Use GDAX keys
ENV CoinbaseRestUrl https://api-public.sandbox.gdax.com
ENV CoinbaseWebsocketUrl wss://ws-feed-public.sandbox.gdax.com
ENV CoinbasePassphrase NULL
ENV CoinbaseApiKey NULL
ENV CoinbaseSecret NULL
ENV CoinbaseOrderDestination Coinbase
## OkCoin
ENV OkCoinWsUrl wss://real.okcoin.com:10440/websocket/okcoinapi
ENV OkCoinHttpUrl https://www.okcoin.com/api/v1/
ENV OkCoinApiKey NULL
ENV OkCoinSecretKey NULL
ENV OkCoinOrderDestination OkCoin
## Bitfinex
ENV BitfinexHttpUrl https://api.bitfinex.com/v1
ENV BitfinexKey NULL
ENV BitfinexSecret NULL
ENV BitfinexOrderDestination Bitfinex

# PROD - values provided for reference.
## HitBtc
#ENV HitBtcPullUrl http://api.hitbtc.com
#ENV HitBtcOrderEntryUrl wss://api.hitbtc.com:8080
#ENV HitBtcMarketDataUrl ws://api.hitbtc.com:80
#ENV HitBtcSocketIoUrl https://api.hitbtc.com:8081
## Coinbase
#ENV CoinbaseRestUrl https://api.gdax.com
#ENV CoinbaseWebsocketUrl wss://ws-feed.gdax.com

WORKDIR tribeca/service
CMD ["forever", "main.js"]
