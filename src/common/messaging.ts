/// <reference path="models.ts" />

import Models = require("./models");

module Prefixes {
    export var SUBSCRIBE = "u";
    export var SNAPSHOT = "n";
    export var MESSAGE = "m";
}

export interface IPublish<T> {
    publish : (msg : T) => void;
    registerSnapshot : (generator : () => T[]) => IPublish<T>;
}

export class Publisher<T> implements IPublish<T> {
    private _snapshot : () => T[] = null;
    constructor(private topic : string, 
                private _io : SocketIO.Server,
                snapshot : () => T[],
                private _log : (...args: any[]) => void) {
        this.registerSnapshot(snapshot || null);

        var onConnection = s => {
            this._log("socket", s.id, "connected for Publisher", topic);

            s.on("disconnect", () => {
                this._log("socket", s.id, "disconnected for Publisher", topic);
            });
            
            s.on(Prefixes.SUBSCRIBE + "-" + topic, () => {
                if (this._snapshot !== null) {
                    var snapshot = this._snapshot();
                    this._log("socket", s.id, "asking for snapshot on topic", topic);
                    s.emit(Prefixes.SNAPSHOT + "-" + topic, snapshot);
                }
            });
        };

        this._io.on("connection", onConnection);
        
        Object.keys(this._io.sockets.connected).forEach(s => {
            onConnection(this._io.sockets.connected[s]);
        });
    }

    public publish = (msg : T) => this._io.emit(Prefixes.MESSAGE + "-" + this.topic, msg);

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

export class NullPublisher<T> implements IPublish<T> {
    public publish = (msg : T) => {};
    public registerSnapshot = (generator : () => T[]) => this;
}

export interface ISubscribe<T> {
    registerSubscriber : (incrementalHandler : (msg : T) => void, snapshotHandler : (msgs : T[]) => void) => ISubscribe<T>;
    registerDisconnectedHandler : (handler : () => void) => ISubscribe<T>;
    registerConnectHandler : (handler : () => void) => ISubscribe<T>;
    connected: boolean;
    disconnect : () => void;
}

export class Subscriber<T> implements ISubscribe<T> {
    private _incrementalHandler : (msg : T) => void = null;
    private _snapshotHandler : (msgs : T[]) => void = null;
    private _disconnectHandler : () => void = null;
    private _connectHandler : () => void = null;
    private _socket : SocketIOClient.Socket;

    constructor(private topic : string, 
                io : SocketIOClient.Socket,
                private _log : (...args: any[]) => void) {
        this._socket = io;
        
        this._log("creating subscriber to", this.topic, "; connected?", this.connected);
        
        if (this.connected) 
            this.onConnect();
        
        this._socket.on("connect", this.onConnect)
                .on("disconnect", this.onDisconnect)
                .on(Prefixes.MESSAGE + "-" + topic, this.onIncremental)
                .on(Prefixes.SNAPSHOT + "-" + topic, this.onSnapshot);
    }
    
    public get connected() : boolean {
        return this._socket.connected;
    }

    private onConnect = () => {
        this._log("connect to", this.topic);
        if (this._connectHandler !== null) {
            this._connectHandler();
        }

        this._socket.emit(Prefixes.SUBSCRIBE + "-" + this.topic);
    };

    private onDisconnect = () => {
        this._log("disconnected from", this.topic);
        if (this._disconnectHandler !== null)
            this._disconnectHandler();
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
        this._socket.off("connect", this.onConnect);
        this._socket.off("disconnect", this.onDisconnect);
        this._socket.off(Prefixes.MESSAGE + "-" + this.topic, this.onIncremental);
        this._socket.off(Prefixes.SNAPSHOT + "-" + this.topic, this.onSnapshot);
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
    private _socket : SocketIOClient.Socket;

    constructor(private topic : string, io : SocketIOClient.Socket, _log : (...args: any[]) => void) {
        this._socket = io;
        this._socket.on("connect", () => _log("Fire connected to", this.topic))
                    .on("disconnect", () => _log("Fire disconnected to", this.topic));
    }

    public fire = (msg : T) : void => {
        this._socket.emit(Prefixes.MESSAGE + "-" + this.topic, msg);
    };
}

export interface IReceive<T> {
    registerReceiver(handler : (msg : T) => void) : void;
}

export class NullReceiver<T> implements IReceive<T> {
    registerReceiver = (handler : (msg : T) => void) => {};
}

export class Receiver<T> implements IReceive<T> {
    private _handler : (msg : T) => void = null;
    constructor(private topic : string, io : SocketIO.Server,
                private _log : (...args: any[]) => void) {
        var onConnection = (s : SocketIO.Socket) => {
            this._log("socket", s.id, "connected for Receiver", topic);
            s.on(Prefixes.MESSAGE + "-" + this.topic, msg => {
                if (this._handler !== null)
                    this._handler(msg);
            });
            s.on("error", e => {
                _log("error in Receiver", e.stack, e.message);
            });
        };
                    
        io.on("connection", onConnection);
        Object.keys(io.sockets.connected).forEach(s => {
            onConnection(io.sockets.connected[s]);
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
    static TargetBasePosition = "tbp";
    static TradeSafetyValue = "tsv";
    static CancelAllOrders = "cao";
}
