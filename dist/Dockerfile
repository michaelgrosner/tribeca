FROM node:8-stretch
RUN apt-get update

RUN apt-get install -y git sudo

# Feel free to choose the branch you want to build:
RUN git clone -b master https://github.com/ctubio/Krypto-trading-bot.git K

WORKDIR K

# Remove the ssl certificate (GUI accessible over plain HTTP, not recommended):
#RUN rm -rf dist/sslcert

RUN make docker

RUN make gdax && \
    mv build-x86_64-linux-gnu/local/etc/stunnel /usr/local/etc/stunnel

EXPOSE 3000 5000

# General config properties. Replace with your own exchange account information:
ENV BotIdentifier dev
ENV EXCHANGE NULL
ENV TradedPair BTC/USD
ENV WebClientUsername NULL
ENV WebClientPassword NULL
ENV WebClientListenPort 3000
ENV MatryoshkaUrl https://

# Set options only for your desired EXCHANGE:

## Coinbase GDAX
ENV CoinbaseRestUrl https://api.gdax.com
ENV CoinbaseWebsocketUrl wss://ws-feed.gdax.com
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

## Poloniex
ENV PoloniexHttpUrl https://poloniex.com
ENV PoloniexWebsocketUrl wss://api.poloniex.com
ENV PoloniexApiKey NULL
ENV PoloniexSecretKey NULL
ENV PoloniexOrderDestination Poloniex

## Korbit
ENV KorbitHttpUrl https://api.korbit.co.kr/v1
ENV KorbitApiKey NULL
ENV KorbitSecretKey NULL
ENV KorbitUsername NULL
ENV KorbitPassword NULL
ENV KorbitOrderDestination Korbit

## HitBtc
ENV HitBtcPullUrl http://api.hitbtc.com
ENV HitBtcOrderEntryUrl wss://api.hitbtc.com:8080
ENV HitBtcMarketDataUrl ws://api.hitbtc.com:80
ENV HitBtcApiKey NULL
ENV HitBtcSecret NULL
ENV HitBtcOrderDestination HitBtc

## Quoting Parameters
ENV delayUI 3

CMD ["node_modules/.bin/forever", "-c", "/bin/sh", "K.sh"]
