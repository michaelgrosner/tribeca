FROM node:7.9

WORKDIR /tribeca

COPY . /tribeca

RUN npm install -g grunt-cli forever
RUN npm install
RUN grunt compile

EXPOSE 3000 5000

WORKDIR tribeca/service

CMD ["forever", "main.js"]
