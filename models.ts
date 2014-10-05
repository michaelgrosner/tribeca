/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />

interface MarketUpdate {
    bidPrice : number;
    bidSize : number;
    askPrice : number;
    askSize : number;
    time : Date;
}

enum ConnectivityStatus { Connected, Disconnected }
enum Exchange { Coinsetter, HitBtc, OkCoin }

interface MarketBook {
    top : MarketUpdate;
    second : MarketUpdate;
    exchangeName : Exchange;
}

interface IGateway {
    MarketData : Evt<MarketBook>;
    ConnectChanged : Evt<ConnectivityStatus>;
    name() : string;
}

interface IBroker {
    MarketData : Evt<MarketBook>;
    name() : string;
    currentBook() : MarketBook;
}

class ExchangeBroker implements IBroker {
    MarketData : Evt<MarketBook> = new Evt();

    name() : string {
        return this._gateway.name();
    }

    _currentBook : MarketBook = null;
    _gateway : IGateway;
    _log : Logger;

    public currentBook = () : MarketBook => {
        return this._currentBook;
    };

    private marketUpdatesEqual = (update1 : MarketUpdate, update2 : MarketUpdate) : boolean => {
        return update1.askPrice == update2.askPrice &&
               update1.bidPrice == update2.bidPrice &&
               update1.askSize == update2.askSize &&
               update1.askPrice == update2.askPrice;
    };

    private handleMarketData = (book : MarketBook) => {
        if (!this._currentBook !== null ||
            !this.marketUpdatesEqual(book.top, this._currentBook.top) ||
            !this.marketUpdatesEqual(book.second, this._currentBook.second)) {
            this._currentBook = book;
            this.MarketData.trigger(book);
            this._log(book);
        }
    };

    public onConnect = (cs : ConnectivityStatus) => {
        this._log("Connection status changed ", ConnectivityStatus[cs]);
    };

    constructor(gateway : IGateway) {
        this._log = log("ExchangeBroker:" + gateway.name());
        this._gateway = gateway;
        this._gateway.MarketData.on(this.handleMarketData);
        this._gateway.ConnectChanged.on(this.onConnect);
    }
}

class Agent {
    _brokers : Array<IBroker>;

    constructor(brokers : Array<IBroker>) {
        this._brokers = brokers;
        this._brokers.forEach(b => { b.MarketData.on(this.onNewMarketData) });
    }

    private onNewMarketData = () => {
        var books : Array<MarketBook> = [];

        for (var i = 0; i < this._brokers.length; i++) {
            var b = this._brokers[i];
            var bk = b.currentBook();
            if (bk == null) return;
            books.push(bk);
        }

        

    };
}