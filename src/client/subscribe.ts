import { Observable } from 'rxjs/Observable';

import Models = require("../share/models");

export interface ISubscribe<T> {
  registerSubscriber: (incrementalHandler: (msg: T) => void) => ISubscribe<T>;
  registerConnectHandler: (handler: () => void) => ISubscribe<T>;
  registerDisconnectedHandler: (handler: () => void) => ISubscribe<T>;
  connected: boolean;
}

export class Subscriber<T> extends Observable<T> implements ISubscribe<T> {
  private _connectHandler: () => void = null;
  private _disconnectHandler: () => void = null;
  private _incrementalHandler: boolean;

  constructor(
    private topic: string,
    private _socket: WebSocket
  ) {
    super(observer => {
      console.log(this._socket);
      if (this._socket.readyState==1) this.onConnect();

      this._socket.addEventListener('open', this.onConnect);
      this._socket.addEventListener('close', this.onDisconnect);
      this._socket.addEventListener('message', (msg) => {
        console.log('MSG:', msg[0], msg);
        // .on(Models.Prefixes.MESSAGE + topic, (data) => observer.next(data))
        // .on(Models.Prefixes.SNAPSHOT + topic, (data) => data.forEach(item => setTimeout(() => observer.next(item), 0)));
      });

      return () => {};
    });
  }

  public get connected(): boolean {
    return this._socket.readyState == 1;
  }

  private onConnect = () => {
      if (this._connectHandler !== null)
          this._connectHandler();

      this._socket.send(Models.Prefixes.SUBSCRIBE + this.topic);
  };

  private onDisconnect = () => {
    if (this._disconnectHandler !== null)
      this._disconnectHandler();
  };

  public registerSubscriber = (incrementalHandler: (msg: T) => void) => {
    if (!this._incrementalHandler) {
      this.subscribe(incrementalHandler);
      this._incrementalHandler = true;
    }
    else throw new Error("already registered incremental handler for topic " + this.topic);
    return this;
  };

  public registerDisconnectedHandler = (handler : () => void) => {
    if (this._disconnectHandler === null) this._disconnectHandler = handler;
    else throw new Error("already registered disconnect handler for topic " + this.topic);
    return this;
  };

  public registerConnectHandler = (handler : () => void) => {
      if (this._connectHandler === null) this._connectHandler = handler;
      else throw new Error("already registered connect handler for topic " + this.topic);
      return this;
  };
}

export interface IFire<T> {
  fire(msg?: T): void;
}

export class Fire<T> implements IFire<T> {
    constructor(private topic: string, private _socket: WebSocket) {
        // this._socket.on("connect", () => _log("Fire connected to", this.topic))
                    // .on("disconnect", () => _log("Fire disconnected to", this.topic));
    }

    public fire = (msg?: T) : void => {
        this._socket.send(JSON.stringify([Models.Prefixes.MESSAGE + this.topic, msg || null]));
    };
}
