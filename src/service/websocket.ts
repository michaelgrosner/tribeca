import * as ws from "ws";
import * as Util from "./utils";
import * as Models from "../common/models";
import * as bunyan from "bunyan";
import * as moment from "moment";
import log from "./logging";

export default class WebSocket {
    private readonly _log : bunyan;
    private _ws : ws = null;

    constructor(private _url: string, 
                private _reconnectInterval = 5000,
                private _onData: (msgs : Models.Timestamped<string>) => void = null,
                private _onConnected: () => void = null, 
                private _onDisconnected: () => void = null) {
        this._log = log(`ws:${this._url}`);
        this._onData = this._onData || (_ => {});
        this._onConnected = this._onConnected || (() => {});
        this._onDisconnected = this._onDisconnected || (() => {});
    }

    public get isConnected() : boolean { return this._ws.readyState === ws.OPEN; }    

    public connect = () => {
        if (this._ws !== null) return;

        try {
            this.createSocket();
        } catch (error) {
            this._log.error("unhandled exception creating websocket!", error);
            throw(error);
        }
    };

    private _failureCount = 0;
    private createSocket = () => {
        this._ws = new ws(this._url);

        this._ws = this._ws
            .on("open", () => {
                try {
                    this._failureCount = 0;
                    this._log.info("connected");
                    this._onConnected();
                } catch (e) {
                    this._log.error("error handling websocket open!");
                }
            })
            .on("message", data => {
                try {
                    const t = new Date();
                    this._onData(new Models.Timestamped<string>(data, t));
                } catch (e) {
                    this._log.error("error handling websocket message!", {data: data, error: e});
                }
            })
            .on("close", (code, message) => {
                try {
                    this._log.info("disconnected", {code: code, message: message});
                    this._onDisconnected();
                    this.closeAndReconnect();
                } catch (e) {
                    this._log.error("error handling websocket disconnect!", {code: code, message: message});
                }
            })
            .on("error", err => {
                this._log.info("socket error", err);
                this._onDisconnected();
                this.closeAndReconnect();
            });
    };

    private closeAndReconnect = () => {
        if (this._ws === null) return;
        
        this._failureCount += 1;
        this._ws.close();
        this._ws = null;

        const interval = this._failureCount == 1 ? 10 : this._reconnectInterval;
        this._log.info(`will try a reconnect in ${interval}ms, failed ${this._failureCount} times`);

        setTimeout(() => {
            this._log.info("reconnection begun");
            this.connect();
        }, interval);
    };

    public send = (data: string, callback?: () => void) => {
        if (this._ws !== null) {
            this._ws.send(data, (e: Error) => { 
                if (!e && callback) callback();
                else if (e) this._log.error(e, "error during websocket send!");
            });
        }
        else {
            this._log.warn(data, "cannot send because socket is not connected!");
        }
    }
}