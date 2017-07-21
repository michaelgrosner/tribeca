import NullGateway = require("./nullgw");
import Models = require("../../share/models");
import util = require("util");
import Interfaces = require("../interfaces");
import fs = require('fs');
import uuid = require('uuid');
import Gdax = require('gdax');
import events = require('events');
import quickfix = require('node-quickfix-wrap');
import df = require('dateformat');
import path = require('path');
import crypto = require('crypto');

interface CoinbaseOrder {
    client_oid?: string;
    price?: string;
    side?: string;
    size: string;
    product_id: string;
    stp?: string;
    time_in_force?: string;
    post_only?: boolean;
    funds?: string;
    type?: string;
    reason?: string;
    trade_id?: number;
    maker_order_id?: string;
    taker_order_id?: string;
    order_id?: string;
    time?: string;
    new_size?: string;
    old_size?: string;
    remaining_size?: string;
}

interface CoinbaseOrderAck {
    id: string;
    error?: string;
    message?: string;
}

interface CoinbaseAccountInformation {
    id: string;
    balance: string;
    hold: string;
    available: string;
    currency: string;
}

interface CoinbaseRESTTrade {
    time: string;
    trade_id: number;
    price: string;
    size: string;
    side: string;
}

interface Product {
    id: string,
    base_currency: string,
    quote_currency: string,
    base_min_size: string,
    base_max_size: string,
    quote_increment: string,
}

class CoinbaseMarketDataGateway implements Interfaces.IMarketDataGateway {
    private onMessage = (data: CoinbaseOrder) => {
      if (data.type == 'match' || data.type == 'open') {
          this.onOrderBookChanged(data.side === "buy" ? Models.Side.Bid : Models.Side.Ask, parseFloat(data.price));
          if (data.type == 'match') {
            this._evUp('MarketTradeGateway', new Models.GatewayMarketTrade(
              parseFloat(data.price),
              parseFloat(data.size),
              data.side === "buy" ? Models.Side.Bid : Models.Side.Ask
            ));
          }
      }
    };

    private _cachedBids: Models.MarketSide[] = [];
    private _cachedAsks: Models.MarketSide[] = [];

    private reevalBook = () => {
        this._cachedBids = [];
        this._cachedAsks = [];
        const bidsIterator = this._client.book._bids.iterator();
        let item;
        while((item = bidsIterator.prev()) !== null && this._cachedBids.length < 13) {
            let size = 0;
            item.orders.forEach(x => size += parseFloat(x.size));
            this._cachedBids.push(new Models.MarketSide(parseFloat(item.price), size));
        };
        const asksIterator = this._client.book._asks.iterator();
        while((item = asksIterator.next()) !== null && this._cachedAsks.length < 13) {
            let size = 0;
            item.orders.forEach(x => size += parseFloat(x.size));
            this._cachedAsks.push(new Models.MarketSide(parseFloat(item.price), size));
        };
    };

    private onOrderBookChanged = (side: Models.Side, price: number) => {
        if ((side === Models.Side.Bid && this._cachedBids.length > 0 && price < this._cachedBids.slice(-1).pop().price)
          || (side === Models.Side.Ask && this._cachedAsks.length > 0 && price > this._cachedAsks.slice(-1).pop().price))
        return;

        this.raiseMarketData();
    };

    private onStateChange = () => {
        this._evUp('GatewayMarketConnect', this._client.socket ? Models.ConnectivityStatus.Connected : Models.ConnectivityStatus.Disconnected);
        this.raiseMarketData();
    };

    private reevalPublicBook = async () => {
      return await new Promise<boolean>((resolve, reject) => {
        this._authClient.getProductOrderBook({level:2}, this._client.productID, (err, res, data) => {
            if (err) this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Disconnected);
            else {
              this._evUp('GatewayMarketConnect', Models.ConnectivityStatus.Connected);
              this._cachedBids = [];
              this._cachedAsks = [];
              data.bids.slice(0,13).forEach(x => this._cachedBids.push(new Models.MarketSide(parseFloat(x[0]), parseFloat(x[1]))));
              data.asks.slice(0,13).forEach(x => this._cachedAsks.push(new Models.MarketSide(parseFloat(x[0]), parseFloat(x[1]))));
            }
            if (!this._client.socket) setTimeout(this.raiseMarketData, 1210);
            resolve(true);
        });
      });
    };

    private raiseMarketData = () => {
        if (this._client.socket)
          this.reevalBook();
        else this.reevalPublicBook();
        if (this._cachedBids.length && this._cachedAsks.length)
            this._evUp('MarketDataGateway', new Models.Market(this._cachedBids, this._cachedAsks));
    };

    constructor(
      private _evUp,
      private _client: Gdax.OrderbookSync,
      private _authClient: Gdax.AuthenticatedClient
    ) {
        this._client.on("open", data => this.onStateChange());
        this._client.on("close", data => this.onStateChange());
        this._client.on("message", data => this.onMessage(data));
    }
}

class CoinbaseOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    supportsCancelAllOpenOrders = () : boolean => { return false; };
    cancelAllOpenOrders = () : Promise<number> => {
      return new Promise<number>((resolve, reject) => {
        this._authClient.cancelAllOrders((err, resp, data) => {
            data.forEach(cxl_id => {
                this._evUp('OrderUpdateGateway', {
                    exchangeId: cxl_id,
                    time: new Date().getTime(),
                    orderStatus: Models.OrderStatus.Cancelled,
                    leavesQuantity: 0
                });
            });

            resolve(data.length);
        });
      });
    };

    generateClientOrderId = (): string => { return uuid(); }

    cancelOrder = (cancel: Models.OrderStatusReport) => {
        if (this._FIXHeader) return this.cancelFIXOrder(cancel);

        this._authClient.cancelOrder(cancel.exchangeId, (err?: Error, resp?: any, ack?: CoinbaseOrderAck) => {
            var msg = null;
            if (err) {
                if (err.message) msg = err.message;
            }
            else if (ack != null) {
                if (ack.message) msg = ack.message;
                if (ack.error) msg = ack.error;
            }
            if (msg !== null) {
                this._evUp('OrderUpdateGateway', <Models.OrderStatusUpdate>{
                    orderId: cancel.orderId,
                    rejectMessage: msg,
                    orderStatus: Models.OrderStatus.Cancelled,
                    time: new Date().getTime(),
                    leavesQuantity: 0
                });

                if (msg === "You have exceeded your request rate of 5 r/s." || msg === "BadRequest") {
                    setTimeout(() => this.cancelOrder(cancel), 500);
                }
            }
        });
    };

    private cancelFIXOrder = (cancel: Models.OrderStatusReport) => {
      this._FIXClient.send({
        header: this._FIXHeader,
        tags: {
          11: cancel.orderId,
          37: cancel.exchangeId,
          41: cancel.orderId,
          55: this._symbolProvider.symbol,
          35: 'F'
        }
      });
    };

    replaceOrder = (replace: Models.OrderStatusReport) => {
        this.cancelOrder(replace);
        this.sendOrder(replace);
    };
    sendOrder = (order: Models.OrderStatusReport) => {
        if (this._FIXHeader) return this.sendFIXOrder(order);

        var cb = (err?: Error, resp?: any, ack?: CoinbaseOrderAck) => {
            if (ack == null || typeof ack.id === "undefined") {
              if (ack==null || (ack.message && ack.message!='Insufficient funds'))
                console.warn(new Date().toISOString().slice(11, -1), 'coinbase', 'WARNING FROM GATEWAY:', order.orderId, err, ack);
            }
            var msg = null;
            if (err) {
                if (err.message) msg = err.message;
            }
            else if (ack != null) {
                if (ack.message) msg = ack.message;
                if (ack.error) msg = ack.error;
            }
            else if (ack == null) {
                msg = "No ack provided!!";
            }

            if (msg !== null) {
              this._evUp('OrderUpdateGateway', {
                  orderId: order.orderId,
                  rejectMessage: msg,
                  orderStatus: Models.OrderStatus.Cancelled,
                  time: new Date().getTime()
              });
            }
        };

        var o: CoinbaseOrder = {
            client_oid: order.orderId,
            size: order.quantity.toFixed(8),
            product_id: this._symbolProvider.symbol
        };

        if (order.type === Models.OrderType.Limit) {
            o.price = order.price.toFixed(this._fixedPrecision);

            if (order.preferPostOnly)
                o.post_only = true;

            switch (order.timeInForce) {
                case Models.TimeInForce.GTC:
                    break;
                case Models.TimeInForce.FOK:
                    o.time_in_force = "FOK";
                    break;
                case Models.TimeInForce.IOC:
                    o.time_in_force = "IOC";
                    break;
            }
        }
        else if (order.type === Models.OrderType.Market) {
            o.type = "market";
        }

        if (order.side === Models.Side.Bid)
            this._authClient.buy(o, cb);
        else if (order.side === Models.Side.Ask)
            this._authClient.sell(o, cb);

        this._evUp('OrderUpdateGateway', {
            orderId: order.orderId,
            computationalLatency: (new Date()).getTime() - order.time
        });
    };

    private sendFIXOrder = (order: Models.OrderStatusReport) => {
        var o = {
          header: this._FIXHeader,
          tags: {
            21: 1,
            11: order.orderId,
            38: order.quantity.toFixed(8),
            54: (order.side === Models.Side.Bid) ? 1 : 2,
            55: this._symbolProvider.symbol,
            35: 'D',
            7928: 'D'
          }
        };

        if (order.type === Models.OrderType.Limit) {
            o.tags[40] = 2;
            o.tags[44] = order.price.toFixed(this._fixedPrecision);

            if (order.preferPostOnly)
                o.tags[59] = 'P';

            switch (order.timeInForce) {
                case Models.TimeInForce.GTC:
                    break;
                case Models.TimeInForce.FOK:
                    o.tags[59] = 4;
                    break;
                case Models.TimeInForce.IOC:
                    o.tags[59] = 3;
                    break;
            }
        }
        else if (order.type === Models.OrderType.Market) {
            o.tags[40] = 1;
        }

        this._FIXClient.send(o, () => {
          this._evUp('OrderUpdateGateway', {
              orderId: order.orderId,
              computationalLatency: (new Date()).getTime() - order.time
          });
        });
    };

    public cancelsByClientOrderId = false;

    private onStateChange = () => {
      let state = Models.ConnectivityStatus.Disconnected;
      if (this._FIXHeader) {
        if (this._FIXClient.isLoggedOn()) state = Models.ConnectivityStatus.Connected;
        console.log(new Date().toISOString().slice(11, -1), 'coinbase', 'FIX Logon', Models.ConnectivityStatus[state])
      } else if (this._client.socket) state = Models.ConnectivityStatus.Connected;
      this._evUp('GatewayOrderConnect', state);
    };

    private onMessage = (data: CoinbaseOrder) => {
      let status: Models.OrderStatusUpdate;
      if (data.type == 'open') {
        status = {
            exchangeId: data.order_id,
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            leavesQuantity: parseFloat(data.remaining_size)
        };
      } else if (data.type == 'received') {
        status = {
            exchangeId: data.order_id,
            orderId: data.client_oid,
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            leavesQuantity: parseFloat(data.size)
        };
      } else if (data.type == 'change') {
        status = {
            exchangeId: data.order_id,
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            quantity: parseFloat(data.new_size)
        };

      } else if (data.type == 'match') {
        status = {
            exchangeId: data.maker_order_id,
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            lastQuantity: parseFloat(data.size),
            lastPrice: parseFloat(data.price),
            liquidity: Models.Liquidity.Make
        };
        this._evUp('OrderUpdateGateway', status);
        status.exchangeId = data.taker_order_id;
        status.liquidity = Models.Liquidity.Take;
      } else if (data.type == 'done') {
        status = {
            exchangeId: data.order_id,
            orderStatus: data.reason === "filled"
              ? Models.OrderStatus.Complete
              : Models.OrderStatus.Cancelled,
            time: new Date().getTime(),
            leavesQuantity: 0
        };
      }

      this._evUp('OrderUpdateGateway', status);
    };

    private onFIXMessage = (tags) => {
      let status: Models.OrderStatusUpdate;
      if (tags[150] == '0') {
        status = {
            exchangeId: tags[37],
            orderId: tags[11],
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            leavesQuantity: parseFloat(tags[38])
        };
      } else if (tags[150] == 'D') {
        status = {
            exchangeId: tags[37],
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            quantity: parseFloat(tags[38])
        };

      } else if (tags[150] == '1') {
        status = {
            exchangeId: tags[37],
            orderStatus: Models.OrderStatus.Working,
            time: new Date().getTime(),
            lastQuantity: parseFloat(tags[32]),
            lastPrice: parseFloat(tags[44]),
            liquidity: Models.Liquidity.Make
        };
      } else if (['3','4','7','8'].indexOf(tags[150])>-1 || tags[434]) {
        status = {
            orderId: tags[11],
            exchangeId: tags[37],
            orderStatus: tags[150] == '3'
              ? Models.OrderStatus.Complete
              : Models.OrderStatus.Cancelled,
            time: new Date().getTime(),
            leavesQuantity: tags[151] ? parseFloat(tags[151]) : 0
        };
      }

      this._evUp('OrderUpdateGateway', status);
    };

    private _fixedPrecision: number = 0;
    private _FIXClient: any = null;
    private _FIXHeader:any = null;

    constructor(
        private _evUp,
        cfString,
        minTick: number,
        private _client: Gdax.OrderbookSync,
        private _authClient: Gdax.AuthenticatedClient,
        private _symbolProvider: CoinbaseSymbolProvider
    ) {
        this._fixedPrecision = -Math.floor(Math.log10(minTick));
        var initiator = quickfix.initiator;
        if (typeof initiator !== 'undefined') {
          this._FIXHeader = {
            8: 'FIX.4.2',
            49: cfString("CoinbaseApiKey"),
            56: cfString("CoinbaseOrderDestination")
          };
          util.inherits(initiator, events.EventEmitter);
          let now;
          this._FIXClient = new initiator({
            onLogon: (sessionID) => this.onStateChange(),
            onLogout: (sessionID) => this.onStateChange(),
            fromApp: (message, sessionID) => {
              if (['8','9'].indexOf(message.header[35])>-1) this.onFIXMessage(message.tags);
              else if (message.header[35]=='3') console.log(new Date().toISOString().slice(11, -1), 'coinbase', 'FIX message rejected:', message);
            }
          }, {
            credentials: {
              username: '',
              password: cfString("CoinbasePassphrase"),
              rawdata: ((what, secret) => crypto.createHmac('sha256', new Buffer(secret, 'base64')).update(what).digest('base64'))([
                now=df(new Date(), "yyyymmdd-HH:MM:ss.l"), 'A', '1',
                cfString("CoinbaseApiKey"),
                cfString("CoinbaseOrderDestination"),
                cfString("CoinbasePassphrase")
              ].join('\x01'), cfString("CoinbaseSecret")),
              sendingtime: now,
              cancelordersondisconnect: 'Y'
            },
            settings: `[DEFAULT]
ReconnectInterval=21
PersistMessages=N
FileStorePath=/var/tmp/quickfix/store
FileLogPath=/var/tmp/quickfix/log
[SESSION]
ConnectionType=initiator
EncryptMethod=0
SenderCompID=${cfString("CoinbaseApiKey")}
TargetCompID=${cfString("CoinbaseOrderDestination")}
BeginString=FIX.4.2
StartTime=00:00:00
EndTime=23:59:59
HeartBtInt=30
SocketConnectPort=4199
SocketConnectHost=127.0.0.1
UseDataDictionary=N
ResetOnLogon=Y`
          });
          this._FIXClient.start(() => {
            console.log(new Date().toISOString().slice(11, -1), 'coinbase', 'FIX Initiator Start');
            setTimeout(() => ((path) => {
              if (path.indexOf('/var/tmp/quickfix/log')===0 && fs.existsSync(path))
                fs.readdirSync(path).forEach(function(file,index) {
                  const curPath = path + "/" + file;
                  if(!fs.lstatSync(curPath).isDirectory() && curPath.indexOf('/var/tmp/quickfix/log')===0) fs.unlinkSync(curPath);
                });
            })(path.resolve('/var/tmp/quickfix/log')), 21000);
          });
        } else {
          this._client.on("open", data => this.onStateChange());
          this._client.on("close", data => this.onStateChange());
          this._client.on("message", data => this.onMessage(data));
        }
    }
}

class CoinbasePositionGateway implements Interfaces.IPositionGateway {
    private onTick = () => {
        this._authClient.getAccounts((err?: Error, resp?: any, data?: CoinbaseAccountInformation[]|{message: string}) => {
            try {
              if (Array.isArray(data)) {
                    data.forEach(d => this._evUp('PositionGateway', new Models.CurrencyPosition(
                      parseFloat(d.available),
                      parseFloat(d.hold),
                      Models.toCurrency(d.currency)
                    )));
                }
                else console.warn(new Date().toISOString().slice(11, -1), 'coinbase', 'Unable to read Coinbase positions', data);
            } catch (error) {
                console.error(new Date().toISOString().slice(11, -1), 'coinbase', error, 'Exception while reading Coinbase positions', data)
            }
        });
    };

    constructor(
      private _evUp,
      private _authClient: Gdax.AuthenticatedClient
    ) {
        setInterval(this.onTick, 7500);
        this.onTick();
    }
}

class CoinbaseSymbolProvider {
    public symbol: string;

    constructor(cfPair) {
        this.symbol = Models.fromCurrency(cfPair.base) + "-" + Models.fromCurrency(cfPair.quote);
    }
}

class Coinbase extends Interfaces.CombinedGateway {
    constructor(
      authClient: Gdax.AuthenticatedClient,
      cfString,
      symbolProvider: CoinbaseSymbolProvider,
      quoteIncrement: number,
      _evOn,
      _evUp
    ) {
        const orderEventEmitter: Gdax.OrderbookSync = new Gdax.OrderbookSync(symbolProvider.symbol, cfString("CoinbaseRestUrl"), cfString("CoinbaseWebsocketUrl"), authClient);

        new CoinbaseMarketDataGateway(_evUp, orderEventEmitter, authClient);
        new CoinbasePositionGateway(_evUp, authClient);

        super(
          cfString("CoinbaseOrderDestination") == "Coinbase"
            ? <Interfaces.IOrderEntryGateway>new CoinbaseOrderEntryGateway(_evUp, cfString, quoteIncrement, orderEventEmitter, authClient, symbolProvider)
            : new NullGateway.NullOrderGateway(_evUp)
        );
    }
};

export async function createCoinbase(setMinTick, setMinSize, cfString, cfPair, _evOn, _evUp): Promise<Interfaces.CombinedGateway> {
    const authClient: Gdax.AuthenticatedClient = new Gdax.AuthenticatedClient(
      cfString("CoinbaseApiKey"),
      cfString("CoinbaseSecret"),
      cfString("CoinbasePassphrase"),
      cfString("CoinbaseRestUrl")
    );

    const products = await new Promise<Product[]>((resolve, reject) => {
      authClient.getProducts((err, res, data) => {
          if (err) reject(err);
          else resolve(data);
      });
    });

    if (!products)
      throw new Error("Unable to connect to Coinbase, seems currently offline. Please retry once is online.");

    const symbolProvider = new CoinbaseSymbolProvider(cfPair);

    for (let p of products) {
        if (p.id === symbolProvider.symbol) {
            setMinTick(parseFloat(p.quote_increment));
            setMinSize(parseFloat(p.base_min_size));
            return new Coinbase(authClient, cfString, symbolProvider, parseFloat(p.quote_increment), _evOn, _evUp);
        }
    }

    throw new Error("Unable to match pair to a coinbase symbol " + Models.Currency[cfPair.base]+'/'+Models.Currency[cfPair.quote]);
}