/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

var io = require('socket.io-client');

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

class Coinsetter {
    _socket : any;
    _log : Logger = log("Coinsetter");
    _broker : MarketDataBroker;

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
        this._broker.addBook(book);
    };

    constructor(broker : MarketDataBroker) {
        this._broker = broker;
        this._socket = io.connect('https://plug.coinsetter.com:3000');
        this._socket.on("connect", this.onConnect);
        this._socket.on("depth", this.onDepth);
    }
}