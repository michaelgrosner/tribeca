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
ENV CoinbaseWebsocketUrl wss://ws-feed.exchange.coinbase.com

WORKDIR tribeca/service
CMD ["forever", "main.js"]
