FROM node:10-stretch
RUN apt-get update

RUN apt-get install -y git sudo

# Feel free to choose the branch you want to build:
RUN git clone -b master https://github.com/ctubio/Krypto-trading-bot.git K

WORKDIR K

# Remove the ssl certificate (GUI accessible over plain HTTP, not recommended):
RUN rm -rf etc/sslcert/server.*

RUN make docker

EXPOSE 80 5000
ENV UI_OPENPORT 80

CMD ["./K.sh", "--naked", "--without-ssl"]
