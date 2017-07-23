import autobahn = require('autobahn');
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import util = require("util");
import Interfaces = require("../interfaces");

interface SignedMessage {
    command?: string;
    nonce?: number;
    currencyPair?: string;
    orderNumber?: string;
    rate?: number;
    amount?: number;
    fillOrKill?: number;
    immediateOrCancel?: number;
    postOnly?: number;
    start?: number;
    end?: number;
}

class PoloniexWebsocket {
  setHandler = <T>(channel: string, handler: (newMsg : Models.Timestamped<T>) => void) => {
    if (typeof this._handler[channel] == 'undefined') this._handler[channel] = [];
    this._handler[channel].push(handler);
  }

  private onMessage = (msg: any) => {
    try {
      if (typeof msg.type === "undefined") throw new Error("Unkown message from Poloniex socket: " + msg);

      if (typeof this._handler[msg.type] === "undefined") {
        console.warn(new Date().toISOString().slice(11, -1), 'poloniex', 'Got message on unknown topic', msg);
        return;
      }

      this._handler[msg.type].forEach(x => x(new Models.Timestamped(msg.data, new Date())));
    }
    catch (e) {
      console.error(new Date().toISOString().slice(11, -1), 'poloniex', e, 'Error parsing msg', msg);
      throw e;
    }
  };

  public connectWS = () => {
      // const ws = new autobahn.Connection({ url: this.cfString("PoloniexWebsocketUrl"), realm: "realm1" });

      // ws.onclose = (reason, details) => {
        // this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        // console.info(new Date().toISOString().slice(11, -1), 'poloniex', this._gwSymbol, reason);
      // };
      // ws.onopen = (session: any) => {
        // session.subscribe(this._gwSymbol, this.seqQueueMsg);
        // this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        // console.info(new Date().toISOString().slice(11, -1), 'poloniex', 'Successfully connected to', this._gwSymbol);
      // };
      // ws.open();
  };

  public seq = 0;
  public queue = {};
  public seqQueueMsg = (args, kwargs) => {
    if (!args.length) return;
    if (kwargs.seq < this.seq) console.info(new Date().toISOString().slice(11, -1), 'poloniex', 'obsolete message received');
    // while (typeof this.queue[this.seq+1] !== 'undefined') {
      // this.queue[this.seq+1].forEach(x => this.onMessage(x));
      // delete this.queue[this.seq+1];
      // this.seq++;
    // }
    // if (this.seq && this.seq + 1 < kwargs.seq) this.queue[kwargs.seq] = args;
    // else {
      this.seq = kwargs.seq;
      args.forEach(x => this.onMessage(x));
    // }
  };

  private _handler: any[] = [];
  constructor(
    private cfString,
    private _gwSymbol
  ) {
  }
}

class PoloniexMarketDataGateway implements Interfaces.IMarketDataGateway {
  private onTrade = (trade: Models.Timestamped<any>) => {
    this._evUp('MarketTradeGateway', new Models.GatewayMarketTrade(
      parseFloat(trade.data.rate),
      parseFloat(trade.data.amount),
      trade.data.type === "sell" ? Models.Side.Ask : Models.Side.Bid
    ));
  };

  private mkt: Models.Market = new Models.Market([], []);

  private onDepth = (depth: Models.Timestamped<any>) => {
    const side = depth.data.type+'s';
    this.mkt[side] = this.mkt[side].filter(a => a.price != parseFloat(depth.data.rate));
    if (typeof depth.data.amount !== 'undefined')
      this.mkt[side].push(new Models.MarketSide(parseFloat(depth.data.rate), parseFloat(depth.data.amount)));
    this.mkt.bids = this.mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0, 27);
    this.mkt.asks = this.mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0, 27);
    const _bids = this.mkt.bids.slice(0, 13);
    const _asks = this.mkt.asks.slice(0, 13);
    if (_bids.length && _asks.length)
      this._evUp('MarketDataGateway', new Models.Market(_bids, _asks));
  };

  private onDepthSnapshot = (mkt: Models.Timestamped<any>) => {
    this.mkt = mkt.data;
    const _bids = this.mkt.bids.slice(0, 13);
    const _asks = this.mkt.asks.slice(0, 13);
    if (_bids.length && _asks.length)
      this._evUp('MarketDataGateway', new Models.Market(_bids, _asks));
  };

  constructor(
    private _evUp,
    socket: PoloniexWebsocket,
    http: PoloniexHttp,
    gwSymbol
  ) {
    // socket.setHandler('newTrade', this.onTrade);
    // socket.setHandler('orderBookModify', this.onDepth);
    // socket.setHandler('orderBookRemove', this.onDepth);
    socket.setHandler('orderBookSnapshot', this.onDepthSnapshot);
    // socket.ConnectChanged.on(cs => this.ConnectChanged.trigger(cs));
    setTimeout(()=>this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Connected), 10);
    setInterval(async ()=>{
      await new Promise<number>((resolve, reject) => {
        http.get('returnOrderBook&depth=13&currencyPair='+gwSymbol).then(msg => {
          if (!(<any>msg.data).seq) return reject(0);
          var kwargs = parseFloat((<any>msg.data).seq);
          const _mkt = new Models.Market([], []);
          (<any>msg.data).bids.forEach(x => _mkt.bids.push(new Models.MarketSide(parseFloat(x[0]), parseFloat(x[1]))));
          (<any>msg.data).asks.forEach(x => _mkt.asks.push(new Models.MarketSide(parseFloat(x[0]), parseFloat(x[1]))));
          _mkt.bids = _mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0, 27);
          _mkt.asks = _mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0, 27);
          var args = [{type:'orderBookSnapshot',data:_mkt}];
          socket.seqQueueMsg(args, kwargs);
          resolve(1);
        });
      });
    }, 2222);
  }
}

class PoloniexOrderEntryGateway implements Interfaces.IOrderEntryGateway {
  generateClientOrderId = (): string => new Date().valueOf().toString().substr(-11);

  supportsCancelAllOpenOrders = () : boolean => { return false; };
  cancelAllOpenOrders = () : Promise<number> => {
    return new Promise<number>((resolve, reject) => {
      this._http.post("returnOpenOrders", {currencyPair: this._gwSymbol }).then(msg => {
        if (!msg.data || !(<any>msg.data).length) { resolve(0); return; }
        (<any>msg.data).forEach(async (o) => {
          await new Promise<number>(async (_resolve, _reject) => {
            this._http.post("cancelOrder", {orderNumber: o.orderNumber }).then(msg => {
              if (typeof (<any>msg.data).success == "undefined") return _reject(0);
              if ((<any>msg.data).success=='1') {
                this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
                  exchangeId: o.orderNumber,
                  leavesQuantity: 0,
                  time: new Date().getTime(),
                  orderStatus: Models.OrderStatus.Cancelled
                });
              }
              _resolve(1);
            });
          });
        });
        resolve((<any>msg.data).length);
      });
    });
  };

  public cancelsByClientOrderId = false;
  sendOrder = (order: Models.OrderStatusReport) => {
    (async ()=>{
      await new Promise<number>((resolve, reject) => {
        this._http.post(order.side === Models.Side.Bid ? 'buy' : 'sell', {
          currencyPair: this._gwSymbol,
          rate: order.price,
          amount: order.quantity,
          fillOrKill: order.timeInForce === Models.TimeInForce.FOK ? 1 : 0,
          immediateOrCancel: order.timeInForce === Models.TimeInForce.IOC ? 1 : 0,
          postOnly: order.preferPostOnly ? 1 : 0
        }).then(msg => {
          if (typeof (<any>msg.data).orderNumber !== 'undefined')
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
              exchangeId: (<any>msg.data).orderNumber,
              orderId: order.orderId,
              leavesQuantity: order.quantity,
              time: new Date().getTime(),
              orderStatus: Models.OrderStatus.Working,
              computationalLatency: new Date().valueOf() - order.time
            });
          else
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
              orderId: order.orderId,
              leavesQuantity: 0,
              time: new Date().getTime(),
              orderStatus: Models.OrderStatus.Cancelled
            });
          resolve(1);
        });
      });
    })();
  };

  cancelOrder = (cancel: Models.OrderStatusReport) => {
    (async ()=>{
      await new Promise<number>((resolve, reject) => {
        this._http.post("cancelOrder", {orderNumber: cancel.exchangeId }).then(msg => {
          if (typeof (<any>msg.data).success == "undefined") return reject(0);
          if ((<any>msg.data).success=='1') {
            this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
              exchangeId: cancel.exchangeId,
              orderId: cancel.orderId,
              leavesQuantity: 0,
              time: new Date().getTime(),
              orderStatus: Models.OrderStatus.Cancelled
            });
          }
          resolve(1);
        });
      });
    })();
  };

  replaceOrder = (replace : Models.OrderStatusReport) => {
      this.cancelOrder(replace);
      this.sendOrder(replace);
  };

  private onTrade = (trade: Models.Timestamped<any>) => {
    this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
      exchangeId: trade.data.orderNumber,
      orderStatus: Models.OrderStatus.Complete,
      time: new Date().getTime(),
      side: trade.data.type === "sell" ? Models.Side.Ask : Models.Side.Bid,
      lastQuantity: parseFloat(trade.data.amount),
      lastPrice: parseFloat(trade.data.rate),
      averagePrice: parseFloat(trade.data.rate)
    });
  };

  private _startTrade: number = Math.floor(new Date().getTime() / 1000);
  constructor(
    private _evUp,
    private _http: PoloniexHttp,
    private _gwSymbol,
    socket: PoloniexWebsocket
  ) {
    // socket.setHandler('newTrade', this.onTrade);
    // socket.ConnectChanged.on(cs => this.ConnectChanged.trigger(cs));
    setTimeout(()=>this._evUp('GatewayOrderConnect', Models.ConnectivityStatus.Connected), 10);
    setInterval(async ()=> {
      var endTrade = Math.floor(new Date().getTime() / 1000);
      var startTrade = this._startTrade;
      this._startTrade = endTrade + 1;
      await new Promise<number>((resolve, reject) => {
        this._http.post('returnTradeHistory', {
          currencyPair: this._gwSymbol,
          start: startTrade,
          end: endTrade
        }).then(msg => {
          if (!(<any>msg.data).length) return;
          (<any>msg.data).forEach(x => this.onTrade(new Models.Timestamped(x, new Date())));
        });
      });
    }, 3333);
  }
}

class PoloniexMessageSigner {
  private _secretKey : string;
  private _api_key : string;

  public signMessage = (baseUrl: string, actionUrl: string, m: SignedMessage): any => {
    var els : string[] = [];

    m.command = actionUrl;
    m.nonce = Date.now();

    var keys = [];
    for (var key in m) {
      if (m.hasOwnProperty(key))
        keys.push(key);
    }
    keys.sort();

    for (var i = 0; i < keys.length; i++) {
      const k = keys[i];
      if (m.hasOwnProperty(k))
        els.push(k + "=" + m[k]);
    }

    return {
      url: url.resolve(baseUrl, 'tradingApi'),
      body: els.join("&"),
      headers: {
        "Content-Type": "application/x-www-form-urlencoded",
        "Key": this._api_key,
        "Sign": crypto.createHmac("sha512", this._secretKey).update(els.join("&")).digest("hex")
      },
      method: "POST"
    };
  };

  constructor(cfString) {
    this._api_key = cfString("PoloniexApiKey");
    this._secretKey = cfString("PoloniexSecretKey");
  }
}

class PoloniexHttp {
  private _freq: number = 0;
  post = <T>(actionUrl: string, msg: SignedMessage): Promise<Models.Timestamped<T>> => {
    while (this._freq+125>Date.now()) {}
    this._freq = Date.now();
    return (async ()=> await new Promise<Models.Timestamped<T>>((resolve, reject) => {
      request(this._signer.signMessage(this._baseUrl, actionUrl, msg), (err, resp, body) => {
        if (err) reject(err);
        else {
          try {
            var t = new Date();
            var data = JSON.parse(body);
            if (typeof data.error !== 'undefined') {
              if (data.error.indexOf('Nonce must be greater than')>-1) {
                resolve(this.post(actionUrl, msg));
              } else console.error(new Date().toISOString().slice(11, -1), 'poloniex', 'Error', actionUrl, data.error);
            }
            else resolve(new Models.Timestamped(data, t));
          }
          catch (e) {
            console.error(new Date().toISOString().slice(11, -1), 'poloniex', err, 'url:', actionUrl, 'err:', err, 'body:', body);
            reject(e);
          }
        }
      });
    }))();
  };

  get = <T>(actionUrl: string) : Promise<Models.Timestamped<T>> => {
    while (this._freq+125>Date.now()) {}
    this._freq = Date.now();
    return (async ()=> await new Promise<Models.Timestamped<T>>((resolve, reject) => {
      request({
        url: url.resolve(this._baseUrl, 'public?command='+actionUrl),
        headers: {},
        method: "GET"
      }, (err, resp, body) => {
        if (err) reject(err);
        else {
          try {
            var t = new Date();
            var data = JSON.parse(body);
            resolve(new Models.Timestamped(data, t));
          }
          catch (e) {
            console.error(new Date().toISOString().slice(11, -1), 'poloniex', err, 'url:', actionUrl, 'err:', err, 'body:', body);
            reject(e);
          }
        }
      });
    }))();
  };

  private _baseUrl : string;
  constructor(cfString, private _signer: PoloniexMessageSigner) {
    this._baseUrl = cfString("PoloniexHttpUrl");
  }
}

export class Poloniex extends Interfaces.CombinedGateway {
  constructor(
    gwSymbol,
    cfString,
    _evUp
  ) {
    const socket = new PoloniexWebsocket(cfString, gwSymbol);
    const http = new PoloniexHttp(cfString, new PoloniexMessageSigner(cfString));
    new PoloniexMarketDataGateway(_evUp, socket, http, gwSymbol);
    super(
      cfString("PoloniexOrderDestination") == "Poloniex"
        ? <Interfaces.IOrderEntryGateway>new PoloniexOrderEntryGateway(_evUp, http, gwSymbol, socket)
        : new NullGateway.NullOrderGateway(_evUp)
    );
    socket.connectWS();
  }
}

