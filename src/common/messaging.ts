/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="models.ts" />

import Models = require("./models");

module Prefixes {
    export var SUBSCRIBE = "subscribe-";
    export var SNAPSHOT = "-snapshot";
}

export interface IPublish<T> {
    publish : (msg : T) => void;
    registerSnapshot : (generator : () => T[]) => IPublish<T>;
}

export class Publisher<T> implements IPublish<T> {
    constructor(private topic : string,
                private _io : any,
                private _snapshot : () => T[] = null,
                private _log : (...args: any[]) => void = console.log) {
        this._io.on("connection", s => {
            this._log("socket", s.id, "connected for Publisher", topic);

            s.on("disconnect", () => {
                this._log("socket", s.id, "disconnected for Publisher", topic);
            });

            this._log("awaiting client snapshot requests on topic", Prefixes.SUBSCRIBE+topic);
            s.on(Prefixes.SUBSCRIBE+topic, () => {
                var snapshot = this._snapshot();
                this._log("socket", s.id, "asking for snapshot on topic", Prefixes.SUBSCRIBE+topic);
                s.emit(topic+Prefixes.SNAPSHOT, snapshot);
            });
        });
    }

    public publish = (msg : T) => this._io.emit(this.topic, msg);

    public registerSnapshot = (generator : () => T[]) => {
        if (this._snapshot === null) {
            this._snapshot = generator;
        }
        else {
            throw new Error("already registered snapshot generator for topic " + this.topic);
        }

        return this;
    }
}

export interface ISubscribe<T> {
    registerSubscriber : (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => ISubscribe<T>;
    registerDisconnectedHandler : (handler : () => void) => ISubscribe<T>;
    registerConnectHandler : (handler : () => void) => ISubscribe<T>;
    disconnect : () => void;
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
        _io.on("connect", this.onConnect);
        _io.on("disconnect", this.onDisconnect);
        _io.on(topic, this.onIncremental);
        _io.on(topic+Prefixes.SNAPSHOT, this.onSnapshot);

        this.onConnect();
    }

    private onConnect = () => {
        if (this._needsSnapshot) {
            this._log("requesting snapshot via ", Prefixes.SUBSCRIBE+this.topic);
            this._io.emit(Prefixes.SUBSCRIBE+this.topic);
            this._needsSnapshot = false;
            if (this._connectHandler !== null)
                this._connectHandler();
        }
        else {
            this._log("already have snapshot to ", this.topic);
        }
    };

    private onDisconnect = () => {
        this._needsSnapshot = true;
        this._log("disconnected from ", this.topic);
        if (this._disconnectHandler !== null)
            this._disconnectHandler();
    };

    private onIncremental = (m : T) => {
        if (this._incrementalHandler !== null)
            this._incrementalHandler(m);
    };

    private onSnapshot = (msgs : T[]) => {
        if (this._snapshotHandler !== null)
            this._snapshotHandler(msgs);
    };

    public disconnect = () => {
        this._log("disconnection from ", this.topic);
        this._io.off("connect", this.onConnect);
        this._io.off("disconnect", this.onDisconnect);
        this._io.off(this.topic, this.onIncremental);
        this._io.off(this.topic+Prefixes.SNAPSHOT, this.onSnapshot);
    };

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

export interface IFire<T> {
    fire(msg : T) : void;
}

export class Fire<T> implements IFire<T> {
    constructor(private topic : string,
                private _io : any) {}

    public fire = (msg : T) : void => {
        this._io.emit(this.topic, msg);
    };
}

export interface IReceive<T> {
    registerReceiver(handler : (msg : T) => void) : void;
}

export class Receiver<T> implements IReceive<T> {
    private _handler : (msg : T) => void = null;
    constructor(private topic : string,
                private _io : any,
                private _log : (...args: any[]) => void = console.log) {
        _io.on("connection", s => {
            this._log("socket", s.id, "connected for Receiver", topic);
            s.on(topic, msg => {
                if (this._handler !== null)
                    this._handler(msg);
            });
        });
    }

    registerReceiver = (handler : (msg : T) => void) => {
        if (this._handler === null) {
            this._handler = handler;
        }
        else {
            throw new Error("already registered receive handler for topic " + this.topic);
        }
    };
}

export class Topics {
    static FairValue = "fv";
    static Quote = "q";
    static ActiveSubscription = "a";
    static ActiveChange = "ac";
    static MarketData = "md";
    static QuotingParametersChange = "qp-sub";
    static SafetySettings = "ss";
    static Product = "p";
    static OrderStatusReports = "osr";
    static ProductAdvertisement = "pa";
}

export module ExchangePairMessaging {
    export function wrapExchangePairTopic(exch : Models.Exchange, pair : Models.CurrencyPair, topic : string) {
        return exch + "." + pair.base + "/" + pair.quote + "." + topic;
    }

    export function wrapExchangeTopic(exch : Models.Exchange, topic : string) {
        return exch + "." + topic;
    }
}