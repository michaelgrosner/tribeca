/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="models.ts" />

import Models = require("./models");

module Prefixes {
    export var SUBSCRIBE = "subscribe-";
    export var SNAPSHOT = "-snapshot";
}

export interface IPublish<T> {
    publish : (msg : T) => void;
}

export class Publisher<T> implements IPublish<T> {
    constructor(private topic : string,
                private _io : any,
                private _snapshot : () => T[],
                private _log : (...args: any[]) => void = console.log) {
        this._io.on("connection", s => {
            this._log("socket", s.id, "connected");

            s.on("disconnect", () => {
                this._log("socket", s.id, "disconnected");
            });

            this._log("awaiting client snapshot requests on topic", Prefixes.SUBSCRIBE+topic);
            s.on(Prefixes.SUBSCRIBE+topic, () => {
                var snapshot = this._snapshot();
                this._log("socket", s.id, "asking for snapshot on topic", Prefixes.SUBSCRIBE+topic);
                s.emit(topic+Prefixes.SNAPSHOT, snapshot);
            });
        });
    }

    public publish = (msg : T) => {
        this._io.emit(this.topic, msg);
    }
}

export interface ISubscribe<T> {
    registerSubscriber : (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => ISubscribe<T>;
    registerDisconnectedHandler : (handler : () => void) => ISubscribe<T>;
    registerConnectHandler : (handler : () => void) => ISubscribe<T>;
}

export class Subscriber<T> implements ISubscribe<T> {
    private _incrementalHandler : (msg : T) => void = null;
    private _snapshotHandler : (msgs : T[]) => void = null;
    private _disconnectHandler : () => void = null;
    private _connectHandler : () => void = null;
    private _needsSnapshot = true;

    constructor(private topic : string,
                private _io : any,
                private _log : (...args: any[]) => void = console.log) {
        var onConnect = () => {
            if (this._needsSnapshot) {
                this._log("requesting snapshot via ", Prefixes.SUBSCRIBE+topic);
                _io.emit(Prefixes.SUBSCRIBE+topic);
                this._needsSnapshot = false;
                if (this._connectHandler !== null)
                    this._connectHandler();
            }
            else {
                this._log("already have snapshot to ", topic);
            }
        };

        _io.on("connect", onConnect);

        _io.on("disconnect", () => {
            this._needsSnapshot = true;
            this._log("disconnected from ", topic);
            if (this._disconnectHandler !== null)
                this._disconnectHandler();
        });

        _io.on(topic, (m : T) => {
            if (this._incrementalHandler !== null)
                this._incrementalHandler(m);
        });

        _io.on(topic+Prefixes.SNAPSHOT, (msgs : T[]) => {
            if (this._snapshotHandler !== null)
                this._snapshotHandler(msgs);
        });

        onConnect();
    }

    public registerSubscriber = (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => {
        this._log("registered subscriber for topic", this.topic);

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

        return this;
    };

    public registerDisconnectedHandler = (handler : () => void) => {
        if (this._disconnectHandler === null) {
            this._disconnectHandler = handler;
        }
        else {
            throw new Error("already registered disconnect handler for topic " + this.topic);
        }

        return this;
    };

    public registerConnectHandler = (handler : () => void) => {
        if (this._connectHandler === null) {
            this._connectHandler = handler;
        }
        else {
            throw new Error("already registered connect handler for topic " + this.topic);
        }

        return this;
    };
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
        return exch + "." + pair.base + "/" + pair.quote + "." + topic;
    }

    export class ExchangePairPublisher<T> implements IPublish<T> {
        private _wrapped : IPublish<T>;
        constructor(exch : Models.Exchange,
                    pair : Models.CurrencyPair,
                    topic : string,
                    snapshot : () => T[],
                    io : any,
                    log : (...args: any[]) => void = console.log) {
            this._wrapped = new Publisher<T>(wrapTopic(exch, pair, topic), io, snapshot, log);
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
                    io : any,
                    log : (...args: any[]) => void = console.log) {
            this._wrapped = new Subscriber<T>(wrapTopic(exch, pair, topic), io, log);
        }

        public registerSubscriber = (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => {
            return this._wrapped.registerSubscriber(incrementalHandler, snapshotHandler);
        };

        public registerDisconnectedHandler = (handler : () => void) => {
            return this._wrapped.registerDisconnectedHandler(handler);
        };

        public registerConnectHandler = (handler : () => void) => {
            return this._wrapped.registerConnectHandler(handler);
        };
    }
}