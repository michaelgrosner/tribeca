/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="models.ts" />

import Models = require("./models");

module Prefixes {
    export var SUBSCRIBE = "subscribe";
    export var SNAPSHOT = "snapshot";
    export var MESSAGE = "message";
}

export interface IPublish<T> {
    publish : (msg : T) => void;
    registerSnapshot : (generator : () => T[]) => IPublish<T>;
}

export class Publisher<T> implements IPublish<T> {
    private _io : any;
    private _snapshot : () => T[] = null;
    constructor(private topic : string, io : any,
                snapshot : () => T[] = null,
                private _log : (...args: any[]) => void = console.log) {
        this.registerSnapshot(snapshot);

        this._io = io.of("/"+this.topic);
        this._io.on("connection", s => {
            this._log("socket", s.id, "connected for Publisher", topic);

            s.on("disconnect", () => {
                this._log("socket", s.id, "disconnected for Publisher", topic);
            });

            s.on(Prefixes.SUBSCRIBE, () => {
                if (this._snapshot !== null) {
                    var snapshot = this._snapshot();
                    this._log("socket", s.id, "asking for snapshot on topic", topic);
                    s.emit(Prefixes.SNAPSHOT, snapshot);
                }
            });
        });
    }

    public publish = (msg : T) => this._io.emit(Prefixes.MESSAGE, msg);

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
    private _io : any;

    constructor(private topic : string, io : any,
                private _log : (...args: any[]) => void = console.log) {
        this._log("creating subscriber to", this.topic);
        this._io = io("/"+this.topic, {forceNew: true});
        this._io.on("connect", this.onConnect);
        this._io.on("disconnect", this.onDisconnect);
        this._io.on(Prefixes.MESSAGE, this.onIncremental);
        this._io.on(Prefixes.SNAPSHOT, this.onSnapshot);
    }

    private onConnect = () => {
        this._log("connect to", this.topic);
        if (this._connectHandler !== null) {
            this._connectHandler();
        }

        this._io.emit(Prefixes.SUBSCRIBE);
    };

    private onDisconnect = () => {
        this._log("disconnected from", this.topic);
        if (this._disconnectHandler !== null)
            this._disconnectHandler();
        this._io.disconnect();
    };

    private onIncremental = (m : T) => {
        if (this._incrementalHandler !== null)
            this._incrementalHandler(m);
    };

    private onSnapshot = (msgs : T[]) => {
        this._log("handling snapshot for", this.topic, "nMsgs:", msgs.length);
        if (this._snapshotHandler !== null)
            this._snapshotHandler(msgs);
    };

    public disconnect = () => {
        this._log("forcing disconnection from ", this.topic);
        this._io.off("connect", this.onConnect);
        this._io.off("disconnect", this.onDisconnect);
        this._io.off(Prefixes.MESSAGE, this.onIncremental);
        this._io.off(Prefixes.SNAPSHOT, this.onSnapshot);
        this._io.disconnect();
    };

    public registerSubscriber = (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => {
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
    private _io : any;

    constructor(private topic : string, io : any, _log : (...args: any[]) => void) {
        this._io = io("/"+this.topic, {forceNew: true, transports: ['websocket']});
        this._io.on("connect", () => _log("Fire connected to", this.topic));
        this._io.on("disconnect", () => _log("Fire disconnected to", this.topic));
    }

    public fire = (msg : T) : void => {
        this._io.emit(Prefixes.MESSAGE, msg);
    };
}

export interface IReceive<T> {
    registerReceiver(handler : (msg : T) => void) : void;
}

export class Receiver<T> implements IReceive<T> {
    private _handler : (msg : T) => void = null;
    constructor(private topic : string, io : any,
                private _log : (...args: any[]) => void = console.log) {
        io.of("/"+this.topic).on("connection", (s : any) => {
            this._log("socket", s.id, "connected for Receiver", topic);
            s.on(Prefixes.MESSAGE, msg => {
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
    static Position = "pos";
    static ExchangeConnectivity = "ec";
    static SubmitNewOrder = "sno";
    static CancelOrder = "cxl";
    static MarketTrade = "mt";
    static Trades = "t";
    static Message = "msg";
    static ExternalValuation = "ev";
    static QuoteStatus = "qs";
}
