import Models = require("./models");

module Prefixes {
    export var SUBSCRIBE = "subscribe-";
    export var SNAPSHOT = "-snapshot";
    export var CONNECTION_ACK = "hello";
}

export interface IPublish<T> {
    publish : (msg : T) => void;
}

export class Publisher<T> implements IPublish<T> {
    constructor(private topic : string,
                private _io : any,
                private _snapshot : () => T[]) {
        _io.on(Prefixes.SUBSCRIBE+topic, sock => {
            _snapshot().forEach(msg => {
                sock.emit(topic+Prefixes.SNAPSHOT, msg);
            });
        });
    }

    public publish = (msg : T) => {
        this._io.emit(this.topic, msg);
    }
}

export interface ISubscribe<T> {
    registerSubscriber : (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => void;
}

export class Subscriber<T> implements ISubscribe<T> {
    private _incrementalHandler : (msg : T) => void = null;
    private _snapshotHandler : (msgs : T[]) => void = null;

    constructor(private topic : string,
                private _io : any) {
        var subscribe = () => _io.emit(Prefixes.SUBSCRIBE+topic);
        _io.on(Prefixes.CONNECTION_ACK, subscribe);
        subscribe();

        _io.on(topic, (m : T) => {
            if (this._incrementalHandler !== null)
                this._incrementalHandler(m);
        });

        _io.on(topic+Prefixes.SNAPSHOT, (msgs : T[]) => {
            if (this._snapshotHandler !== null)
                this._snapshotHandler(msgs);
        });
    }

    public registerSubscriber(incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) {
        if (this._incrementalHandler === null) {
            this._incrementalHandler = incrementalHandler;
        }
        else {
            throw new Error("already registered incremental handler for topic " + this.topic);
        }

        if (this._snapshotHandler === null) {
            this._snapshotHandler = snapshotHandler;
        }
        else {
            throw new Error("already registered snapshot handler for topic " + this.topic);
        }
    }
}


export class Topics {
    static FairValue = "fv";
    static Quote = "q";
    static TradingDecision = "td";
    static ActiveSubscription = "a";
    static ActiveChange = "ac";
    static MarketData = "md";
    static QuotingParametersSubscription = "qp-sub";
    static QuotingParametersChange = "qp-chg";
}

export module ExchangePairPubSub {
    function wrapTopic(exch : Models.Exchange, pair : Models.CurrencyPair, topic : string) {
        return exch + "." + pair + "." + topic;
    }

    export class ExchangePairPublisher<T> implements IPublish<T> {
        private _wrapped : IPublish<T>;
        constructor(exch : Models.Exchange,
                    pair : Models.CurrencyPair,
                    topic : string,
                    snapshot : () => T[],
                    io : any) {
            this._wrapped = new Publisher<T>(wrapTopic(exch, pair, topic), io, snapshot);
        }

        public publish = (msg : T) => {
            this._wrapped.publish(msg);
        };
    }

    export class ExchangePairSubscriber<T> implements ISubscribe<T> {
        private _wrapped : ISubscribe<T>;
        constructor(exch : Models.Exchange,
                    pair : Models.CurrencyPair,
                    topic : string,
                    io : any) {
            this._wrapped = new Subscriber<T>(wrapTopic(exch, pair, topic), io);
        }

        public registerSubscriber = (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => {
            this._wrapped.registerSubscriber(incrementalHandler, snapshotHandler);
        };
    }
}