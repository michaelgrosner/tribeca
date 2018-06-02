# Using Krypto-trading-bot with SSL enabled integrated with NGINX proxy and autorenew LetsEncrypt certificates

![Krypto-trading-bot-docker-letsencrypt](https://github.com/evertramos/images/raw/master/webproxy.jpg)

This docker-compose should be used with WebProxy (the NGINX Proxy):

[https://github.com/evertramos/docker-compose-letsencrypt-nginx-proxy-companion](https://github.com/evertramos/docker-compose-letsencrypt-nginx-proxy-companion)


## Usage

After everything is settle, and you have your three containers running (_proxy, generator and letsencrypt_) you do the following:

1. Clone this folder:

```bash
git clone https://github.com/ctubio/Krypto-trading-bot/tree/master/etc/docker-compose
```

Or just copy the content of `docker-compose.yml` and the `Dockerfile`, as of below:

```bash
version: '3'

services:
  tribeca:
    container_name: ${APP_CONTAINER_NAME}
    build:
      context: ./
    image: tribeca
    env_file:
      - ./.env
    tty: true
    restart: unless-stopped
    volumes:
        - ${LOCAL_DB_DIR}:/data/db
#    links:
#      - tribeca-mongo
#    depends_on:
#        - tribeca-mongo
#  tribeca-mongo:
#    container_name: ${DB_CONTAINER_NAME}
#    image: mongo
#    restart: unless-stopped
#    volumes:
#        - ${LOCAL_DB_DIR}:/data/db
#     environment:
#       MONGO_INITDB_DATABASE: ${MONGO_DATABASE}
#       MONGO_INITDB_ROOT_USERNAME: ${MONGO_USER}
#       MONGO_INITDB_ROOT_PASSWORD: ${MONGO_PASSWORD}

networks:
    default:
       external:
        name: ${NETWORK}
```

```bash
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
```

2. Make a copy of our .env.sample and rename it to .env:

Update this file with your preferences.

```bash
#
# Configuration for Krypto-trading-bot using NGINX WebProxy
#

# Containers name
APP_CONTAINER_NAME=Krypto-trading-bot

# Krypto-trading-bot data path
LOCAL_DB_DIR=/my/dir/

# Host
VIRTUAL_HOST=cloud.yourdomain.com
LETSENCRYPT_HOST=cloud.yourdomain.com
LETSENCRYPT_EMAIL=your_email@yourdomain.com

#
# Configuration of Krypto-trading-bot
# See examples and descriptions of the following variables 
# at https://github.com/ctubio/Krypto-trading-bot/blob/master/etc/K.sh.dist
OPTIONAL_ARGUMENTS=--colors

UI_USERNAME=NULL
UI_PASSWORD=NULL

API_EXCHANGE=NULL
API_CURRENCY=BTC/USD
API_KEY=NULL
API_SECRET=NULL
API_PASSPHRASE=NULL
API_USERNAME=NULL
API_HTTP_ENDPOINT=NULL
API_WSS_ENDPOINT=NULL


#
# Network name
#
# Your container app must use a network conencted to your webproxy
# https://github.com/evertramos/docker-compose-letsencrypt-nginx-proxy-companion
#
NETWORK=webproxy
```

>This container must use a network connected to your webproxy or the same network of your webproxy.

3. Start your project

```bash
docker-compose up -d --build
```

**Be patient** - when you first run a container to get new certificates, it may take a few minutes.

## WebProxy

[WebProxy - docker-compose-letsencrypt-nginx-proxy-companion](https://github.com/evertramos/docker-compose-letsencrypt-nginx-proxy-companion)

----

## Issues

Please be advised that if are running docker on azure servers you must mount your database in your disks partitions (exemple: `/mnt/data/`) so your db container can work. This is a some kind of issue regarding Hyper-V sharing drivers... not really sure why.


## Full Source

1. [@jwilder](https://github.com/jwilder/nginx-proxy)
2. [@jwilder](https://github.com/jwilder/docker-gen)
3. [@JrCs](https://github.com/JrCs/docker-letsencrypt-nginx-proxy-companion).
