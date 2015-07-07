FROM node:0.12.4
RUN apt-get update

RUN apt-get install -y git 
RUN apt-get install -y libzmq3-dev

RUN git clone https://github.com/michaelgrosner/tribeca.git

WORKDIR tribeca

RUN npm install -g grunt-cli tsd
RUN npm install

RUN tsd reinstall -s
RUN grunt compile

EXPOSE 3000 5000

ENV EXCHANGE null
ENV TRIBECA_MODE dev

WORKDIR tribeca/service
RUN node main.js
