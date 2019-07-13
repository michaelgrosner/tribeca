import { Observable } from 'rxjs';

import * as Models from './models';

var events = {};
var socket;

export class KSocket {
  public ws;
  constructor() {
    socket = this;
    this.ws = new WebSocket(location.origin.replace('http', 'ws'));
    for (const ev in events) events[ev].forEach(cb => this.ws.addEventListener(ev, cb));
    this.ws.addEventListener('close', () => {
      setTimeout(() => { new KSocket(); }, 5000);
    });
  }
  public setEventListener = (ev, cb) => {
    if (typeof events[ev] == 'undefined') events[ev] = [];
    events[ev].push(cb);
    this.ws.addEventListener(ev, cb);
  }
}


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
    private _topic: string
  ) {
    super(observer => {
      if (socket.ws.readyState==1) this.onConnect();

      socket.setEventListener('open', this.onConnect);
      socket.setEventListener('close', this.onDisconnect);
      socket.setEventListener('message', (msg) => {
        const topic = msg.data.substr(0,2);
        const data = JSON.parse(msg.data.substr(2));
        if (Models.Prefixes.MESSAGE+this._topic == topic) setTimeout(() => observer.next(data), 0);
        else if (Models.Prefixes.SNAPSHOT+this._topic == topic)
          data.forEach(item => setTimeout(() => observer.next(item), 0));
      });

      return () => {};
    });
  }

  public get connected(): boolean {
    return socket.ws.readyState == 1;
  }

  private onConnect = () => {
      if (this._connectHandler !== null)
          this._connectHandler();

      socket.ws.send(Models.Prefixes.SNAPSHOT + this._topic);
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
    else throw new Error("already registered incremental handler for topic " + this._topic);
    return this;
  };

  public registerDisconnectedHandler = (handler : () => void) => {
    if (this._disconnectHandler === null) this._disconnectHandler = handler;
    else throw new Error("already registered disconnect handler for topic " + this._topic);
    return this;
  };

  public registerConnectHandler = (handler : () => void) => {
      if (this._connectHandler === null) this._connectHandler = handler;
      else throw new Error("already registered connect handler for topic " + this._topic);
      return this;
  };
}

export interface IFire<T> {
  fire(msg?: T): void;
}

export class Fire<T> implements IFire<T> {
    constructor(private _topic: string) {}

    public fire = (msg?: T) : void => {
        socket.ws.send(Models.Prefixes.MESSAGE + this._topic + (typeof msg == 'object' ? JSON.stringify(msg) : msg));
    };
}
