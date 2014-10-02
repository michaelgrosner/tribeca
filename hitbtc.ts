/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

var crypto = require('crypto');
var ws = require('ws');
var https = require('https');
var Future = require('fibers/future');

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

interface HitBtcOrderBook {
    asks : Array<Array<string>>;
    bids : Array<Array<string>>;
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

        /*var order: NewOrder = {
            clientOrderId: new Date().getTime().toString(32),
            symbol: "BTCUSD",
            side: "sell",
            quantity: 10,
            type: "limit",
            price: 888.12,
            timeInForce: "IOC"
        };

        this.sendAuth("NewOrder", order);*/
    };

    private onMessage = (msg : any) => {
        this._log(JSON.parse(msg));
    };

    private getMarketData = () => {
        function getLevel(raw : HitBtcOrderBook, n : number) : MarketUpdate {
            return {bidPrice: parseFloat(raw.bids[n][0]),
                bidSize: parseFloat(raw.bids[n][1]),
                askPrice: parseFloat(raw.asks[n][0]),
                askSize: parseFloat(raw.asks[n][1]),
                time: new Date()};
        }

        https.get("https://api.hitbtc.com/api/1/public/BTCUSD/orderbook",
            e => e.on("data", bytes => {
                var raw : HitBtcOrderBook = JSON.parse(bytes);
                this._log({top: getLevel(raw, 0), second: getLevel(raw, 1), exchangeName: Exchange.HitBtc});
        }));
    };

    constructor() {
        this._ws = new ws('ws://demo-api.hitbtc.com:8080');
        this._ws.on('open', this.onOpen);
        this._ws.on('message', this.onMessage);
        this._ws.on("error", this.onMessage);

        //this.getMarketData();
        //setInterval(this.getMarketData, 5000);
    }
}