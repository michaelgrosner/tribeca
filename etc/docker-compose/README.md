# Using Krypto-trading-bot with SSL enabled integrated with NGINX proxy and autorenew LetsEncrypt certificates

![Krypto-trading-bot-docker-letsencrypt](https://github.com/evertramos/images/raw/master/webproxy.jpg)

This docker-compose should be used with WebProxy (the NGINX Proxy):

[https://github.com/evertramos/docker-compose-letsencrypt-nginx-proxy-companion](https://github.com/evertramos/docker-compose-letsencrypt-nginx-proxy-companion)


## Usage

After everything is settle, and you have your three containers running (_proxy, generator and letsencrypt_) you do the following:

1. Copy `docker-compose.yml` and `Dockerfile` files to your computer.

2. Copy `.env.sample` file and rename it to `.env`. Then, update this file with your preferences.

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
