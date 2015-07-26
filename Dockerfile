FROM node:0.12.4
RUN apt-get update

RUN apt-get install -y git

RUN git clone https://github.com/michaelgrosner/tribeca.git

WORKDIR tribeca

RUN npm install -g grunt-cli tsd
RUN npm install

RUN tsd reinstall -s
RUN grunt compile

EXPOSE 3000 5000

WORKDIR tribeca/service
CMD ["node", "main.js"]
