FROM overlordq/8-buster

RUN apt-get update \
 && apt-get install --no-install-recommends -y git sudo

# Feel free to choose the branch you want to build:
RUN git clone -b master https://github.com/ctubio/Krypto-trading-bot.git /K

WORKDIR /K

RUN make docker && rm -rf /var/lib/apt/lists/

EXPOSE 3000 5000

# See examples and descriptions of the
# following variables at etc/K.sh.dist.

ENV OPTIONAL_ARGUMENTS --colors

ENV UI_USERNAME NULL
ENV UI_PASSWORD NULL
ENV UI_OPENPORT 3000

ENV API_EXCHANGE NULL
ENV API_CURRENCY BTC/USD
ENV API_KEY NULL
ENV API_SECRET NULL
ENV API_PASSPHRASE NULL
ENV API_HTTP_ENDPOINT NULL
ENV API_WSS_ENDPOINT NULL
ENV K_BINARY_FILE K-trading-bot

CMD ["./K.sh", "--naked", "--without-ssl"]
