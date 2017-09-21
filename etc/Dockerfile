FROM node:8-stretch
RUN apt-get update

RUN apt-get install -y git sudo

# Feel free to choose the branch you want to build:
RUN git clone -b master https://github.com/ctubio/Krypto-trading-bot.git K

WORKDIR K

# Remove the ssl certificate (GUI accessible over plain HTTP, not recommended):
#RUN rm -rf etc/sslcert/server.*

RUN make docker

EXPOSE 3000 5000

# See examples and descriptions of the
# following variables at etc/K.sh.dist.

ENV OPTIONAL_ARGUMENTS --colors

ENV UI_USERNAME NULL
ENV UI_PASSWORD NULL
ENV UI_OPENPORT 3000

ENV API_EXCHANGE NULL
ENV API_CURRENCY BTC/USD
ENV API_ORDERS_DESTINATION NULL
ENV API_KEY NULL
ENV API_SECRET NULL
ENV API_PASSPHRASE NULL
ENV API_USERNAME NULL
ENV API_HTTP_ENDPOINT NULL
ENV API_WSS_ENDPOINT NULL
ENV API_WS_ENDPOINT NULL

CMD ["K.sh"]
