FROM node:0.12.4
RUN apt-get update

RUN apt-get install -y git 
RUN apt-get install -y libzmq3-dev

ADD package.json /tmp/package.json
RUN cd /tmp && npm install
RUN mkdir -p /opt/app && cp -a /tmp/node_modules /opt/app/
RUN npm install -g forever grunt-cli tsd
 
WORKDIR /opt/app
ADD src /opt/app/src
ADD Gruntfile.js /opt/app/
ADD package.json /opt/app/
ADD tsd.json /opt/app/

RUN tsd reinstall -s
RUN grunt compile

EXPOSE 3000 5000

CMD bash 
