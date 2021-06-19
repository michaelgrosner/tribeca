"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Fire = exports.Subscriber = exports.Client = void 0;
const rxjs_1 = require("rxjs");
const Models = require("./models");
var socket;
var events = {};
class Client {
    constructor() {
        this.setEventListener = (ev, cb) => {
            if (typeof events[ev] == 'undefined')
                events[ev] = [];
            events[ev].push(cb);
            this.ws.addEventListener(ev, cb);
        };
        socket = this;
        this.ws = new WebSocket(location.origin.replace('http', 'ws'));
        for (const ev in events)
            events[ev].forEach(cb => this.ws.addEventListener(ev, cb));
        this.ws.addEventListener('close', () => {
            setTimeout(() => { new Client(); }, 5000);
        });
    }
}
exports.Client = Client;
class Subscriber extends rxjs_1.Observable {
    constructor(_topic) {
        super(observer => {
            if (socket.ws.readyState == 1)
                this.onConnect();
            socket.setEventListener('open', this.onConnect);
            socket.setEventListener('close', this.onDisconnect);
            socket.setEventListener('message', (msg) => {
                const topic = msg.data.substr(0, 2);
                const data = JSON.parse(msg.data.substr(2));
                if (Models.Prefixes.MESSAGE + this._topic == topic)
                    setTimeout(() => observer.next(data), 0);
                else if (Models.Prefixes.SNAPSHOT + this._topic == topic)
                    data.forEach(item => setTimeout(() => observer.next(item), 0));
            });
            return () => { };
        });
        this._topic = _topic;
        this._connectHandler = null;
        this._disconnectHandler = null;
        this.onConnect = () => {
            if (this._connectHandler !== null)
                this._connectHandler();
            socket.ws.send(Models.Prefixes.SNAPSHOT + this._topic);
        };
        this.onDisconnect = () => {
            if (this._disconnectHandler !== null)
                this._disconnectHandler();
        };
        this.registerSubscriber = (incrementalHandler) => {
            if (!this._incrementalHandler) {
                this.subscribe(incrementalHandler);
                this._incrementalHandler = true;
            }
            else
                throw new Error("already registered incremental handler for topic " + this._topic);
            return this;
        };
        this.registerDisconnectedHandler = (handler) => {
            if (this._disconnectHandler === null)
                this._disconnectHandler = handler;
            else
                throw new Error("already registered disconnect handler for topic " + this._topic);
            return this;
        };
        this.registerConnectHandler = (handler) => {
            if (this._connectHandler === null)
                this._connectHandler = handler;
            else
                throw new Error("already registered connect handler for topic " + this._topic);
            return this;
        };
    }
    get connected() {
        return socket.ws.readyState == 1;
    }
}
exports.Subscriber = Subscriber;
class Fire {
    constructor(_topic) {
        this._topic = _topic;
        this.fire = (msg) => {
            socket.ws.send(Models.Prefixes.MESSAGE + this._topic + (typeof msg == 'object' ? JSON.stringify(msg) : msg));
        };
    }
}
exports.Fire = Fire;
