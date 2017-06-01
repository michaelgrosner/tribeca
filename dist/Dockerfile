FROM node:7
RUN apt-get update

RUN apt-get install -y git sudo

# Feel free to choose the branch you want to build:
RUN git clone -b master https://github.com/ctubio/tribeca.git

WORKDIR tribeca

# Remove the ssl certificate (GUI accessible over plain HTTP, not recommended):
#RUN rm -rf etc/sslcert

RUN npm install --unsafe-perm

EXPOSE 3000 5000

# General config properties. Replace with your own exchange account information:
ENV TRIBECA_MODE dev
ENV EXCHANGE NULL
ENV TradedPair BTC/USD
ENV WebClientUsername NULL
ENV WebClientPassword NULL
ENV WebClientListenPort 3000
ENV MongoDbUrl mongodb://tribeca-mongo:27017/tribeca
ENV MatryoshkaUrl https://

# Set options only for your desired EXCHANGE:

## HitBtc
ENV HitBtcPullUrl http://demo-api.hitbtc.com
ENV HitBtcOrderEntryUrl ws://demo-api.hitbtc.com:8080
ENV HitBtcMarketDataUrl ws://demo-api.hitbtc.com:80
ENV HitBtcSocketIoUrl https://demo-api.hitbtc.com:8081
ENV HitBtcApiKey NULL
ENV HitBtcSecret NULL
ENV HitBtcOrderDestination HitBtc

## Coinbase (GDAX keys)
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
ENV BitfinexWebsocketUrl wss://api.bitfinex.com/ws/2
ENV BitfinexKey NULL
ENV BitfinexSecret NULL
ENV BitfinexOrderDestination Bitfinex

ENV KorbitHttpUrl https://api.korbit.co.kr/v1
ENV KorbitKey NULL
ENV KorbitSecret NULL
ENV KorbitUsername NULL
ENV KorbitPassword NULL
ENV KorbitOrderDestination Korbit

CMD ["node_modules/.bin/forever", "tribeca.js"]
