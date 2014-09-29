/// <reference path="typings/tsd.d.ts" />

var crypto = require('crypto');
var ws = require('ws');
var io = require('socket.io-client');
var log = require('debug');

interface Logger { (arg : any) : void; }

var apikey = '004ee1065d6c7a6ac556bea221cd6338';
var secretkey = "aa14d615df5d47cb19a13ffe4ea638eb";

interface NoncePayload<T> {
    nonce: number;
    payload: T;
}

interface AuthorizedHitBtcMessage<T> {
    apikey : string;
    signature : string;
    message : NoncePayload<T>;
}

interface HitBtcPayload {}

interface Login extends HitBtcPayload {}

interface NewOrder extends HitBtcPayload {
    clientOrderId : string;
    symbol : string;
    side : string;
    quantity : number;
    type : string;
    price : number;
    timeInForce : string;
}

function authMsg<T>(payload : T) : AuthorizedHitBtcMessage<T> {
    var msg = {nonce: new Date().getTime(), payload: payload};

    var signMsg = function(m) : string {
        return crypto.createHmac('sha512', secretkey)
            .update(JSON.stringify(m))
            .digest('base64');
    };

    return {apikey: apikey, signature: signMsg(msg), message: msg};
}

interface CoinsetterDepthSide {
    price : number;
    size : number;
    exchangeId : string;
    timeStamp : number;
}

interface CoinsetterDepth {
    bid : CoinsetterDepthSide;
    ask : CoinsetterDepthSide;
}

interface MarketUpdate {
    bidPrice : number;
    bidSize : number;
    askPrice : number;
    askSize : number;
    time : Date;
}

enum Exchange { Coinsetter, HitBtc }

interface MarketBook {
    top : MarketUpdate;
    second : MarketUpdate;
    exchangeName : Exchange;
}

class Coinsetter {
    _socket : any;
    _log : Logger = log("Coinsetter");

    private onConnect = () => {
        this._log("Connected to Coinsetter");
        this._socket.emit("depth room", "");
    };

    private onDepth = (msg : Array<CoinsetterDepth>) => {
        function getLevel(n: number) : MarketUpdate {
            return {bidPrice: msg[n].bid.price, bidSize: msg[n].bid.size,
                    askPrice: msg[n].ask.price, askSize: msg[n].ask.size,
                    time: new Date(msg[n].bid.timeStamp)};
        }
        var book : MarketBook = {top: getLevel(0), second: getLevel(1), exchangeName: Exchange.Coinsetter};
        this._log(book);
    };

    constructor() {
        this._socket = io.connect('https://plug.coinsetter.com:3000');
        this._socket.on("connect", this.onConnect);
        this._socket.on("depth", this.onDepth);
    }
}

class HitBtc {
    _ws : any;
    _log : Logger = log("HitBtc");

    private sendAuth = <T extends HitBtcPayload>(msgType : string, msg : T) => {
        var v = {}; v[msgType] = msg;
        var readyMsg = authMsg(v);
        this._log(readyMsg);
        this._ws.send(JSON.stringify(readyMsg));
    };

    private onOpen = () => {
        this.sendAuth("Login", {});

        var order: NewOrder = {
            clientOrderId: new Date().getTime().toString(32),
            symbol: "BTCUSD",
            side: "sell",
            quantity: 10,
            type: "limit",
            price: 888.12,
            timeInForce: "IOC"
        };

        this.sendAuth("NewOrder", order);
    };

    private onMessage = (msg : any) => {
        this._log(JSON.parse(msg));
    };

    constructor() {
        this._ws = new ws('ws://demo-api.hitbtc.com:8080');
        this._ws.on('open', this.onOpen);
        this._ws.on('message', this.onMessage);
    }
}

new Coinsetter();
//new HitBtc();


