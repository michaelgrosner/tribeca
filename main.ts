/// <reference path="coinsetter.ts" />
/// <reference path="hitbtc.ts" />
/// <reference path="okcoin.ts" />
/// <reference path="models.ts" />

var gateways : Array<IGateway> = [new HitBtc.HitBtc(), new OkCoin.OkCoin()];
var brokers : Array<IBroker> = gateways.map(g => {
    return new ExchangeBroker(g);
});
var agent : Agent = new Agent(brokers);

var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var path = require("path");

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, "index.html"));
});

http.listen(3000, () => console.log('listening on *:3000'));

io.on('connection', sock => {
    sock.emit("hello");
    console.log('a user connected');
});