import * as ws from "ws";
import * as Util from "./utils";
import * as Models from "../common/models";
import * as bunyan from "bunyan";
import * as moment from "moment";

export class WebSocket {
    private readonly _log : bunyan;
    private _ws : ws = null;

    constructor(private _url: string, 
                private _reconnectInterval = 5000,
                private _onData: (msgs : Models.Timestamped<string>) => void = null,
                private _onConnected: () => void = null, 
                private _onDisconnected: () => void = null) {
        this._log = Util.log(`ws:${this._url}`);
        this._onData = this._onData || (_ => {});
        this._onConnected = this._onConnected || (() => {});
        this._onDisconnected = this._onDisconnected || (() => {});
    }

    public connect = () => {
        try {
            this.createSocket();
        } catch (error) {
            this._log.error("unhandled exception creating websocket!", error);
            throw(error);
        }
    };

    private createSocket = () => {
        this._ws = new ws(this._url)
            .on("open", () => {
                try {
                    this._log.info("connected");
                    this._onConnected();
                } catch (e) {
                    this._log.error("error handling websocket open!");
                }
            })
            .on("message", data => {
                try {
                    const t = moment.utc();
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
        this._ws.close();
        this._ws = null;
        this._log.info(`will try a reconnect in ${this._reconnectInterval}ms`);

        setTimeout(() => {
            this._log.info("reconnection begun");
            this.connect();
        }, this._reconnectInterval);
    };
}