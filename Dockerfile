FROM node:0.12.7
RUN apt-get update

RUN apt-get install -y git

RUN git clone https://github.com/michaelgrosner/tribeca.git

WORKDIR tribeca

RUN npm install -g grunt-cli tsd forever
RUN npm install

RUN tsd reinstall -s
RUN grunt compile

EXPOSE 3000 5000

# General config properties. Properties with `NULL` should be replaced with your own exchange account information.
ENV TRIBECA_MODE dev
ENV EXCHANGE null
# IP to access mongo instance. If you are on a mac, run `boot2docker ip` and replace `tribeca-mongo`.
ENV MongoDbUrl mongodb://tribeca-mongo:27017/tribeca
ENV CoinbaseOrderDestination Coinbase
ENV HitBtcOrderDestination HitBtc

# DEV
## HitBtc
ENV HitBtcPullUrl http://demo-api.hitbtc.com 
ENV HitBtcOrderEntryUrl ws://demo-api.hitbtc.com:8080
ENV HitBtcMarketDataUrl ws://demo-api.hitbtc.com:80
ENV HitBtcSocketIoUrl https://demo-api.hitbtc.com:8081
ENV HitBtcApiKey NULL
ENV HitBtcSecret NULL
## Coinbase
ENV CoinbaseRestUrl https://api-public.sandbox.exchange.coinbase.com
ENV CoinbaseWebsocketUrl https://ws-feed-public.sandbox.exchange.coinbase.com
ENV CoinbasePassphrase NULL
ENV CoinbaseApiKey NULL
ENV CoinbaseSecret NULL
ENV CoinbaseOrderDestination Coinbase
ENV HitBtcOrderDestination HitBtc

# PROD - values provided for reference.
## HitBtc
#ENV HitBtcPullUrl http://api.hitbtc.com 
#ENV HitBtcOrderEntryUrl wss://api.hitbtc.com:8080
#ENV HitBtcMarketDataUrl ws://api.hitbtc.com:80
#ENV HitBtcSocketIoUrl https://api.hitbtc.com:8081
## Coinbase
#ENV CoinbaseRestUrl https://api.exchange.coinbase.com
#ENV CoinbaseWebsocketUrl wss://ws-feed.exchange.coinbase.com

WORKDIR tribeca/service
CMD ["forever", "main.js"]