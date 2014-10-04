/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />

interface MarketUpdate {
    bidPrice : number;
    bidSize : number;
    askPrice : number;
    askSize : number;
    time : Date;
}

enum Exchange { Coinsetter, HitBtc, OkCoin }

interface MarketBook {
    top : MarketUpdate;
    second : MarketUpdate;
    exchangeName : Exchange;
}

interface IGateway {
    MarketData : Evt<MarketBook>;
    getSnapshot() : MarketBook;
    name() : string;
}

class ExchangeBroker {
    _currentBook : MarketBook = null;
    _gateway : IGateway;
    _log : Logger;

    private marketUpdatesEqual = (update1 : MarketUpdate, update2 : MarketUpdate) : boolean => {
        return update1.askPrice == update2.askPrice &&
               update1.bidPrice == update2.bidPrice &&
               update1.askSize == update2.askSize &&
               update1.askPrice == update2.askPrice;
    };

    public addBook = (book : MarketBook) => {
        if (!this._currentBook !== null ||
            !this.marketUpdatesEqual(book.top, this._currentBook.top) ||
            !this.marketUpdatesEqual(book.second, this._currentBook.second)) {
            this._currentBook = book;
            this._log(book);
        }
    };

    constructor(gateway : IGateway) {
        this._log = log("ExchangeBroker:" + gateway.name());
        this._gateway = gateway;
        this._gateway.MarketData.on(this.addBook);

        this._log(this._gateway.getSnapshot());
    }
}