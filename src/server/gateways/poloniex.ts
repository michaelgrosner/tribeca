import autobahn = require('autobahn');
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

  public seq = 0;
  public connectWS = () => {
      const ws = new autobahn.Connection({ url: this.config.GetString("PoloniexWebsocketUrl"), realm: "realm1" });
      var queue = {};
      ws.onclose = (reason, details) => {
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        console.info(new Date().toISOString().slice(11, -1), 'poloniex', this.symbolProvider.symbol, reason);
      };
      ws.onopen = (session: any) => {
        session.subscribe(this.symbolProvider.symbol, (args, kwargs) => {
          if (args.length) {
            if (kwargs < this.seq)
              console.info(new Date().toISOString().slice(11, -1), 'poloniex', 'obsolete message received');
            else if (this.seq && this.seq + 1 < kwargs) {
              queue[kwargs] = args;
            } else {
              while (typeof queue[this.seq+1] !== 'undefined') {
                queue[this.seq+1].forEach(x => this.onMessage(x));
                delete queue[this.seq+1];
                this.seq++;
              }
              this.seq = kwargs;
              args.forEach(x => this.onMessage(x));
            }
          }
        });
        this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        console.info(new Date().toISOString().slice(11, -1), 'poloniex', 'Successfully connected to', this.symbolProvider.symbol);
      };
      ws.open();
  };

  ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
  private _handler: any[] = [];
  constructor(
    private config: Config.ConfigProvider,
    private symbolProvider: PoloniexSymbolProvider
  ) {
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

  private onDepth = (depth: Models.Timestamped<any>) => {
    const side = depth.data.type+'s';
    this.mkt[side] = this.mkt[side].filter(a => a.price != parseFloat(depth.data.rate));
    if (depth.data.amount) this.mkt[side].push(new Models.MarketSide(parseFloat(depth.data.rate), parseFloat(depth.data.amount)));
    this.mkt.bids = this.mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0, 27);
    this.mkt.asks = this.mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0, 27);
    const _bids = this.mkt.bids.slice(0, 13);
    const _asks = this.mkt.asks.slice(0, 13);
    if (_bids.length && _asks.length)
      this.MarketData.trigger(new Models.Market(_bids, _asks, depth.time));
  };

  constructor(
    socket: PoloniexWebsocket,
    symbol: PoloniexSymbolProvider,
    private mkt: Models.Market
  ) {
    socket.setHandler('newTrade', this.onTrade);
    socket.setHandler('orderBookModify', this.onDepth);
    socket.setHandler('orderBookRemove', this.onDepth);
    socket.ConnectChanged.on(cs => this.ConnectChanged.trigger(cs));
    setTimeout(()=>{
      const _bids = this.mkt.bids.slice(0, 13);
      const _asks = this.mkt.asks.slice(0, 13);
      this.MarketData.trigger(new Models.Market(_bids, _asks, mkt.time));
    }, 10);

  }
}

class PoloniexOrderEntryGateway implements Interfaces.IOrderEntryGateway {
  OrderUpdate = new Utils.Evt<Models.OrderStatusUpdate>();
  ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

  generateClientOrderId = (): string => new Date().valueOf().toString().substr(-11);

  supportsCancelAllOpenOrders = () : boolean => { return false; };
  cancelAllOpenOrders = () : Promise<number> => {
    var d = Promises.defer<number>();
    this._http.post("returnOpenOrders", {currencyPair: this._symbolProvider.symbol }).then(msg => {
      if (!msg.data || !(<any>msg.data).length) { d.resolve(0); return; }
      (<any>msg.data).forEach((o) => {
        this._http.post("cancelOrder", {orderNumber: o.orderNumber }).then(msg => {
          if (typeof (<any>msg.data).success == "undefined") return;
          if ((<any>msg.data).success=='1') {
            this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
              exchangeId: o.orderNumber,
              leavesQuantity: 0,
              time: msg.time,
              orderStatus: Models.OrderStatus.Cancelled
            });
          }
        });
      });
      d.resolve((<any>msg.data).length);
    });
    return d.promise;
  };

  public cancelsByClientOrderId = false;
  sendOrder = (order: Models.OrderStatusReport) => {
    this._http.post(order.side === Models.Side.Bid ? 'buy' : 'sell', {
      currencyPair: this._symbolProvider.symbol,
      rate: order.price,
      amount: order.quantity,
      fillOrKill: order.timeInForce === Models.TimeInForce.FOK ? 1 : 0,
      immediateOrCancel: order.timeInForce === Models.TimeInForce.IOC ? 1 : 0,
      postOnly: order.preferPostOnly ? 1 : 0
    }).then(msg => {
      if (typeof (<any>msg.data).orderNumber !== 'undefined')
        this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
          exchangeId: (<any>msg.data).orderNumber,
          exchangeTradeId: (<any>msg.data).resultingTrades[0].tradeID,
          orderId: order.orderId,
          leavesQuantity: parseFloat((<any>msg.data).resultingTrades[0].rate),
          time: msg.time,
          orderStatus: Models.OrderStatus.Working,
          computationalLatency: new Date().valueOf() - order.time.valueOf()
        });
      else
        this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
          orderId: order.orderId,
          leavesQuantity: 0,
          time: msg.time,
          orderStatus: Models.OrderStatus.Cancelled
        });
    });
  };

  cancelOrder = (cancel: Models.OrderStatusReport) => {
    this._http.post("cancelOrder", {orderNumber: cancel.exchangeId }).then(msg => {
      if (typeof (<any>msg.data).success == "undefined") return;
      if ((<any>msg.data).success=='1') {
        this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
          exchangeId: cancel.exchangeId,
          orderId: cancel.orderId,
          leavesQuantity: 0,
          time: msg.time,
          orderStatus: Models.OrderStatus.Cancelled
        });
      }
    });
  };

  replaceOrder = (replace : Models.OrderStatusReport) => {
      this.cancelOrder(replace);
      this.sendOrder(replace);
  };

  private onTrade = (trade: Models.Timestamped<any>) => {
    this.OrderUpdate.trigger(<Models.OrderStatusUpdate>{
      exchangeTradeId: trade.data.tradeID,
      orderStatus: Models.OrderStatus.Complete,
      time: trade.time,
      side: trade.data.type === "sell" ? Models.Side.Ask : Models.Side.Bid,
      lastQuantity: parseFloat(trade.data.amount),
      lastPrice: parseFloat(trade.data.price),
      averagePrice: parseFloat(trade.data.price)
    });
  };

  constructor(
    private _http: PoloniexHttp,
    private _symbolProvider: PoloniexSymbolProvider,
    socket: PoloniexWebsocket
  ) {
    socket.setHandler('newTrade', this.onTrade);
    socket.ConnectChanged.on(cs => this.ConnectChanged.trigger(cs));
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

  constructor(config : Config.ConfigProvider) {
    this._api_key = config.GetString("PoloniexApiKey");
    this._secretKey = config.GetString("PoloniexSecretKey");
  }
}

class PoloniexHttp {
  private _freq: number = 0;
  post = async <T>(actionUrl: string, msg: SignedMessage): Promise<Models.Timestamped<T>> => {
    while (this._freq+125>Date.now()) {}
    this._freq = Date.now();
    var d = Promises.defer<Models.Timestamped<T>>();
    request(this._signer.signMessage(this._baseUrl, actionUrl, msg), (err, resp, body) => {
      if (err) d.reject(err);
      else {
        try {
          var t = new Date();
          var data = JSON.parse(body);
          if (typeof data.error !== 'undefined')
            console.error(new Date().toISOString().slice(11, -1), 'poloniex', 'Error', actionUrl, data.error);
          else d.resolve(new Models.Timestamped(data, t));
        }
        catch (e) {
          console.error(new Date().toISOString().slice(11, -1), 'poloniex', err, 'url:', actionUrl, 'err:', err, 'body:', body);
          d.reject(e);
        }
      }
    });

    return await d.promise;
  };

  get = <T>(actionUrl: string) : Promise<Models.Timestamped<T>> => {
    var d = Promises.defer<Models.Timestamped<T>>();

    request({
      url: url.resolve(this._baseUrl, 'public?command='+actionUrl),
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
    this._http.post("returnCompleteBalances", {}).then(msg => {
      const symbols: string[] = this._symbolProvider.symbol.split('_');
      for (var i = symbols.length;i--;) {
        if (!(<any>msg.data) || !(<any>msg.data)[symbols[i]])
          console.error(new Date().toISOString().slice(11, -1), 'poloniex', 'Missing symbol', symbols[i]);
        else this.PositionUpdate.trigger(new Models.CurrencyPosition(parseFloat((<any>msg.data)[symbols[i]].available), parseFloat((<any>msg.data)[symbols[i]].onOrders), Models.toCurrency(symbols[i])));
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
    socket: PoloniexWebsocket,
    mkt: Models.Market,
    minTick: number
  ) {
    super(
      new PoloniexMarketDataGateway(socket, symbol, mkt),
      config.GetString("PoloniexOrderDestination") == "Poloniex"
        ? <Interfaces.IOrderEntryGateway>new PoloniexOrderEntryGateway(http, symbol, socket)
        : new NullGateway.NullOrderGateway(),
      new PoloniexPositionGateway(http, symbol),
      new PoloniexBaseGateway(minTick, 0.01)
    );
    socket.connectWS();
  }
}

export async function createPoloniex(config: Config.ConfigProvider, pair: Models.CurrencyPair): Promise<Interfaces.CombinedGateway> {
  const symbol = new PoloniexSymbolProvider(pair);
  const signer = new PoloniexMessageSigner(config);
  const http = new PoloniexHttp(config, signer);
  const socket = new PoloniexWebsocket(config, symbol);

  const minTick = await new Promise<number>((resolve, reject) => {
    http.get('returnTicker').then(msg => {
      if (!(<any>msg.data)[symbol.symbol]) return reject('Unable to get Poloniex Ticker for symbol '+symbol.symbol);
      console.warn(new Date().toISOString().slice(11, -1), 'poloniex', 'client IP allowed');
      const precisePrice = parseFloat((<any>msg.data)[symbol.symbol].last).toPrecision(6).toString();
      resolve(parseFloat('1e-'+precisePrice.substr(0, precisePrice.length-1).concat('1').replace(/^-?\d*\.?|0+$/g, '').length));
    });
  });

  const mkt = await new Promise<Models.Market>((resolve, reject) => {
    http.get('returnOrderBook&depth=21&currencyPair='+symbol.symbol).then(msg => {
      if (!(<any>msg.data).seq) return reject('Unable to get Poloniex OrderBook for symbol '+symbol.symbol);
      const _mkt = new Models.Market([], [], msg.time);
      (<any>msg.data).bids.forEach(x => _mkt.bids.push(new Models.MarketSide(parseFloat(x[0]), parseFloat(x[1]))));
      (<any>msg.data).asks.forEach(x => _mkt.asks.push(new Models.MarketSide(parseFloat(x[0]), parseFloat(x[1]))));
      _mkt.bids = _mkt.bids.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price < b.price ? 1 : (a.price > b.price ? -1 : 0)).slice(0, 27);
      _mkt.asks = _mkt.asks.sort((a: Models.MarketSide, b: Models.MarketSide) => a.price > b.price ? 1 : (a.price < b.price ? -1 : 0)).slice(0, 27);
      socket.seq = parseFloat((<any>msg.data).seq);
      resolve(_mkt);
    });
  });
  return new Poloniex(config, symbol, http, socket, mkt, minTick);
}
