var autobahn = require('autobahn');
import crypto = require("crypto");
import request = require("request");
import url = require("url");
import Config = require("../config");
import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import Utils = require("../utils");
import util = require("util");
import Interfaces = require("../interfaces");
import * as Promises from '../promises';

interface OrderAck {
    result: boolean;
    order_id: number;
}

interface SignedMessage {
    command?: string;
    nonce?: number;
}

interface Order extends SignedMessage {
    symbol: string;
    type: string;
    price: string;
    amount: string;
}

interface Cancel extends SignedMessage {
    order_id: string;
    symbol: string;
}

interface PoloniexTradeRecord {
    averagePrice: string;
    completedTradeAmount: string;
    createdDate: string;
    id: string;
    orderId: string;
    sigTradeAmount: string;
    sigTradePrice: string;
    status: number;
    symbol: string;
    tradeAmount: string;
    tradePrice: string;
    tradeType: string;
    tradeUnitPrice: string;
    unTrade: string;
}

class PoloniexWebsocket {

  setHandler = <T>(channel : string, handler: (newMsg : Models.Timestamped<T>) => void) => {
    this._handlers[channel] = handler;
  }

  private onMessage = (msg: any) => {
    var t = new Date();
    try {
      if (typeof msg.type === "undefined") throw new Error("Unkown message from Poloniex socket: " + msg);

      // this._stillAlive = true;
      // if (!msg.data) {
        // console.log('pong',msg);
        // return;
      // }

      var handler = this._handlers[msg.type];

      if (typeof handler === "undefined") {
        console.warn(new Date().toISOString().slice(11, -1), 'poloniex', 'Got message on unknown topic', msg);
        return;
      }

      handler(new Models.Timestamped(msg.data, t));
    }
    catch (e) {
      console.error(new Date().toISOString().slice(11, -1), 'poloniex', e, 'Error parsing msg', msg);
      throw e;
    }
  };

  public connectWS = () => {
      var ws = new autobahn.Connection({ url: this.config.GetString("PoloniexWebsocketUrl"), realm: "realm1" });
      ws.onopen = (session: any) => {
        session.subscribe(this.symbolProvider.symbol, (args, kwargs) => {
          if (args.length) args.forEach(x => this.onMessage(x));
        });
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        console.info(new Date().toISOString().slice(11, -1), 'poloniex', 'Successfully connected to', this.symbolProvider.symbol);
      };
      ws.onclose = (reason, details) => {
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        console.info(new Date().toISOString().slice(11, -1), 'poloniex', this.symbolProvider.symbol, reason);
      };
      ws.open();
  };

  ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
  // private _stillAlive: boolean = true;
  private _handlers : { [channel : string] : (newMsg : Models.Timestamped<any>) => void} = {};
  constructor(
    private config: Config.ConfigProvider,
    private symbolProvider: PoloniexSymbolProvider
  ) {
    // setInterval(() => {
      // if (!this._stillAlive) {
        // console.warn(new Date().toISOString().slice(11, -1), 'poloniex', 'Heartbeat lost, reconnecting..');
        // this._stillAlive = true;
        // this.connectWS();
      // } else this._stillAlive = false;
    // }, 21000);
  }
}

class PoloniexMarketDataGateway implements Interfaces.IMarketDataGateway {
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
    private onTrade = (trade: Models.Timestamped<any>) => {
      this.MarketTrade.trigger(new Models.GatewayMarketTrade(
        parseFloat(trade.data.rate),
        parseFloat(trade.data.amount),
        trade.time,
        false,
        trade.data.type === "sell" ? Models.Side.Ask : Models.Side.Bid
      ));
    };

    MarketData = new Utils.Evt<Models.Market>();

    private static GetLevel = (n: [any, any]) : Models.MarketSide =>
        new Models.MarketSide(parseFloat(n[0]), parseFloat(n[1]));

    private mkt = new Models.Market([], [], null);

    private onDepth = (depth: Models.Timestamped<any>) => {
        const side = depth.data.type+'s';
        this.mkt[side] = this.mkt[side].filter(a => a.price != parseFloat(depth.data.rate));
        if (depth.data.amount) this.mkt[side].push(new Models.MarketSide(parseFloat(depth.data.rate), parseFloat(depth.data.amount)));
        this.mkt.bids = this.mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0, 21);
        this.mkt.asks = this.mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0, 21);
        const _bids = this.mkt.bids.slice(0, 13);
        const _asks = this.mkt.asks.slice(0, 13);
        if (_bids.length && _asks.length)
          this.MarketData.trigger(new Models.Market(_bids, _asks, depth.time));
    };


    constructor(
      config: Config.ConfigProvider,
      symbol: PoloniexSymbolProvider
    ) {
        const socket = new PoloniexWebsocket(config, symbol);
        socket.setHandler('newTrade', this.onTrade);
        socket.setHandler('orderBookModify', this.onDepth);
        socket.setHandler('orderBookRemove', this.onDepth);
        socket.ConnectChanged.on(cs => this.ConnectChanged.trigger(cs));
        socket.connectWS();
    }
}

class PoloniexOrderEntryGateway implements Interfaces.IOrderEntryGateway {
  OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
  ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

  private chars: string = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  generateClientOrderId = (): string => {
    let id: string = '';
    for(let i=8;i--;) id += this.chars.charAt(Math.floor(Math.random() * this.chars.length));
    return id;
  };

  supportsCancelAllOpenOrders = () : boolean => { return false; };
  cancelAllOpenOrders = () : Promise<number> => {
      var d = Promises.defer<number>();
      // this._http.post("order_info.do", <Cancel>{order_id: '-1', symbol: this._symbolProvider.symbol }).then(msg => {
        // if (typeof (<any>msg.data).orders == "undefined"
          // || typeof (<any>msg.data).orders[0] == "undefined"
          // || typeof (<any>msg.data).orders[0].order_id == "undefined") { d.resolve(0); return; }
        // (<any>msg.data).orders.map((o) => {
            // this._http.post("cancel_order.do", <Cancel>{order_id: o.order_id.toString(), symbol: this._symbolProvider.symbol }).then(msg => {
                // if (typeof (<any>msg.data).result == "undefined") return;
                // if ((<any>msg.data).result) {
                    // this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
                      // exchangeId: (<any>msg.data).order_id.toString(),
                      // leavesQuantity: 0,
                      // time: msg.time,
                      // orderStatus: Models.OrderStatus.Cancelled
                    // });
                // }
            // });
        // });
        // d.resolve((<any>msg.data).orders.length);
      // });
      return d.promise;
  };

  public cancelsByClientOrderId = false;

  private static GetOrderType(side: Models.Side, type: Models.OrderType) : string {
      if (side === Models.Side.Bid) {
          if (type === Models.OrderType.Limit) return "buy";
          if (type === Models.OrderType.Market) return "buy_market";
      }
      if (side === Models.Side.Ask) {
          if (type === Models.OrderType.Limit) return "sell";
          if (type === Models.OrderType.Market) return "sell_market";
      }
      throw new Error("unable to convert " + Models.Side[side] + " and " + Models.OrderType[type]);
  }

  // let's really hope there's no race conditions on their end -- we're assuming here that orders sent first
  // will be acked first, so we can match up orders and their acks
  private _ordersWaitingForAckQueue = [];

  sendOrder = (order : Models.OrderStatusReport) => {
      // var o : Order = {
          // symbol: this._symbolProvider.symbol,
          // type: PoloniexOrderEntryGateway.GetOrderType(order.side, order.type),
          // price: order.price.toString(),
          // amount: order.quantity.toString()};

      // this._ordersWaitingForAckQueue.push([order.orderId, order.quantity]);

      // this._socket.send<OrderAck>("ok_spot" + this._symbolProvider.symbol + "_trade", this._signer.signMessage(o), () => {
          // this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              // orderId: order.orderId,
              // computationalLatency: new Date().valueOf() - order.time.valueOf()
          // });
      // });
  };

  private onOrderAck = (ts: Models.Timestamped<OrderAck>) => {
      // var order = this._ordersWaitingForAckQueue.shift();

      // var orderId = order[0];
      // if (typeof orderId === "undefined") {
          // console.error(new Date().toISOString().slice(11, -1), 'poloniex', 'got an order ack when there was no order queued!', util.format(ts.data));
          // return;
      // }

      // var osr : Models.OrderStatusUpdate = { orderId: orderId, time: ts.time };

      // if (typeof ts.data !== "undefined" && ts.data.result) {
          // osr.exchangeId = ts.data.order_id.toString();
          // osr.orderStatus = Models.OrderStatus.Working;
          // osr.leavesQuantity = order[1];
      // }
      // else {
          // osr.orderStatus = Models.OrderStatus.Rejected;
      // }

      // this.OrderUpdate.trigger(osr);
  };

  cancelOrder = (cancel : Models.OrderStatusReport) => {
      // var c : Cancel = {order_id: cancel.exchangeId, symbol: this._symbolProvider.symbol };
      // this._socket.send<OrderAck>("ok_spot" + this._symbolProvider.symbol + "_cancel_order", this._signer.signMessage(c), () => {
          // this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              // orderId: cancel.orderId,
              // leavesQuantity: 0,
              // time: cancel.time,
              // orderStatus: Models.OrderStatus.Cancelled
          // });
      // });
  };

  private onCancel = (ts: Models.Timestamped<OrderAck>) => {
      // if (typeof ts.data.order_id == "undefined") return;
      // var osr : Models.OrderStatusUpdate = {
        // exchangeId: ts.data.order_id.toString(),
        // time: ts.time,
        // leavesQuantity: 0
      // };

      // if (ts.data.result) {
          // osr.orderStatus = Models.OrderStatus.Cancelled;
      // }
      // else {
          // osr.orderStatus = Models.OrderStatus.Rejected;
          // osr.cancelRejected = true;
      // }

      // this.OrderUpdate.trigger(osr);
  };

  replaceOrder = (replace : Models.OrderStatusReport) => {
      this.cancelOrder(replace);
      this.sendOrder(replace);
  };

  private static getStatus(status: number) : Models.OrderStatus {
      // status: -1: cancelled, 0: pending, 1: partially filled, 2: fully filled, 4: cancel request in process
      switch (status) {
          case -1: return Models.OrderStatus.Cancelled;
          case 0: return Models.OrderStatus.Working;
          case 1: return Models.OrderStatus.Working;
          case 2: return Models.OrderStatus.Complete;
          case 4: return Models.OrderStatus.Working;
          default: return Models.OrderStatus.Other;
      }
  }

  private onTrade = (tsMsg : Models.Timestamped<PoloniexTradeRecord>) => {
      // var t = tsMsg.time;
      // var msg : PoloniexTradeRecord = tsMsg.data;
      // var avgPx = parseFloat(msg.averagePrice);
      // var lastQty = parseFloat(msg.sigTradeAmount);
      // var lastPx = parseFloat(msg.sigTradePrice);

      // var status : Models.OrderStatusUpdate = {
          // exchangeId: msg.orderId.toString(),
          // orderStatus: PoloniexOrderEntryGateway.getStatus(msg.status),
          // time: t,
          // side: msg.tradeType.indexOf('buy')>-1 ? Models.Side.Bid : Models.Side.Ask,
          // lastQuantity: lastQty > 0 ? lastQty : undefined,
          // lastPrice: lastPx > 0 ? lastPx : undefined,
          // averagePrice: avgPx > 0 ? avgPx : undefined,
          // pendingCancel: msg.status === 4,
          // partiallyFilled: msg.status === 1
      // };

      // this.OrderUpdate.trigger(status);
  };

  constructor(
    private _http: PoloniexHttp,
    private _symbolProvider: PoloniexSymbolProvider
  ) {
    this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
    // _socket.setHandler("ok_sub_spot" + _symbolProvider.symbol + "_trades", this.onTrade);
    // _socket.setHandler("ok_spot" + _symbolProvider.symbol + "_trade", this.onOrderAck);
    // _socket.setHandler("ok_spot" + _symbolProvider.symbol + "_cancel_order", this.onCancel);
  }
}

class PoloniexMessageSigner {
  private _secretKey : string;
  private _api_key : string;

  public signMessage = (baseUrl: string, actionUrl: string, m : SignedMessage) : any => {
    var els : string[] = [];

    m.command = 'return'+actionUrl;
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

  constructor(config : Config.ConfigProvider) {
    this._api_key = config.GetString("PoloniexApiKey");
    this._secretKey = config.GetString("PoloniexSecretKey");
  }
}

class PoloniexHttp {
  post = <T>(actionUrl: string, msg : SignedMessage) : Promise<Models.Timestamped<T>> => {
    var d = Promises.defer<Models.Timestamped<T>>();

    request(this._signer.signMessage(this._baseUrl, actionUrl, msg), (err, resp, body) => {
      if (err) d.reject(err);
      else {
        try {
          var t = new Date();
          var data = JSON.parse(body);
          d.resolve(new Models.Timestamped(data, t));
        }
        catch (e) {
          console.error(new Date().toISOString().slice(11, -1), 'poloniex', err, 'url:', actionUrl, 'err:', err, 'body:', body);
          d.reject(e);
        }
      }
    });

    return d.promise;
  };

  get = <T>(actionUrl: string) : Promise<Models.Timestamped<T>> => {
    var d = Promises.defer<Models.Timestamped<T>>();

    request({
      url: url.resolve(this._baseUrl, 'public?command=return'+actionUrl),
      headers: {},
      method: "GET"
    }, (err, resp, body) => {
      if (err) d.reject(err);
      else {
        try {
          var t = new Date();
          var data = JSON.parse(body);
          d.resolve(new Models.Timestamped(data, t));
        }
        catch (e) {
          console.error(new Date().toISOString().slice(11, -1), 'poloniex', err, 'url:', actionUrl, 'err:', err, 'body:', body);
          d.reject(e);
        }
      }
    });

    return d.promise;
  };

  private _baseUrl : string;
  constructor(config : Config.ConfigProvider, private _signer: PoloniexMessageSigner) {
    this._baseUrl = config.GetString("PoloniexHttpUrl");
  }
}

class PoloniexPositionGateway implements Interfaces.IPositionGateway {
  PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

  private trigger = () => {
    this._http.post("CompleteBalances", {}).then(msg => {
      const symbols: string[] = this._symbolProvider.symbol.split('_');
      for (var i = symbols.length;i--;) {
        if (!(<any>msg.data) || !(<any>msg.data)[symbols[i]])
          console.error(new Date().toISOString().slice(11, -1), 'poloniex', 'Please change the API Key or contact support team of Poloniex, your API Key does not work because was not possible to retrieve your real wallet position; the application will probably crash now.');
        this.PositionUpdate.trigger(new Models.CurrencyPosition(parseFloat((<any>msg.data)[symbols[i]].available), parseFloat((<any>msg.data)[symbols[i]].onOrders), Models.toCurrency(symbols[i])));
      }
    });
  };

  constructor(private _http : PoloniexHttp, private _symbolProvider: PoloniexSymbolProvider) {
    setInterval(this.trigger, 15000);
    setTimeout(this.trigger, 10);
  }
}

class PoloniexBaseGateway implements Interfaces.IExchangeDetailsGateway {
  public get hasSelfTradePrevention() {
    return false;
  }

  name() : string {
    return "Poloniex";
  }

  makeFee() : number {
    return 0.001;
  }

  takeFee() : number {
    return 0.002;
  }

  exchange() : Models.Exchange {
    return Models.Exchange.Poloniex;
  }

  constructor(public minTickIncrement: number, public minSize: number) {}
}

class PoloniexSymbolProvider {
  public symbol: string;

  constructor(pair: Models.CurrencyPair) {
    this.symbol = Models.fromCurrency(pair.quote) + "_" + Models.fromCurrency(pair.base);
  }
}

class Poloniex extends Interfaces.CombinedGateway {
  constructor(
    config: Config.ConfigProvider,
    symbol: PoloniexSymbolProvider,
    http: PoloniexHttp,
    minTick: number
  ) {
    super(
      new PoloniexMarketDataGateway(config, symbol),
      config.GetString("PoloniexOrderDestination") == "Poloniex"
        ? <Interfaces.IOrderEntryGateway>new PoloniexOrderEntryGateway(http, symbol)
        : new NullGateway.NullOrderGateway(),
      new PoloniexPositionGateway(http, symbol),
      new PoloniexBaseGateway(minTick, 0.01)
    );
  }
}

export async function createPoloniex(config: Config.ConfigProvider, pair: Models.CurrencyPair): Promise<Interfaces.CombinedGateway> {
  var symbol = new PoloniexSymbolProvider(pair);
  var signer = new PoloniexMessageSigner(config);
  var http = new PoloniexHttp(config, signer);

  const minTick = await new Promise<number>((resolve, reject) => {
    http.get('Ticker').then(msg => {
      if (!(<any>msg.data)[symbol.symbol]) return reject('Unable to get Poloniex Ticker for symbol '+symbol.symbol);
      console.warn(new Date().toISOString().slice(11, -1), 'poloniex', 'client IP allowed');
      const precisePrice = parseFloat((<any>msg.data)[symbol.symbol].last).toPrecision(6).toString();
      resolve(parseFloat('1e-'+precisePrice.substr(0, precisePrice.length-1).concat('1').replace(/^-?\d*\.?|0+$/g, '').length));
    });
  });

  return new Poloniex(config, symbol, http, minTick);
}
