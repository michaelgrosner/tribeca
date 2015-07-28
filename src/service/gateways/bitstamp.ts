/// <reference path="../../../typings/tsd.d.ts" />
/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("../config");
import crypto = require('crypto');
import WebSocket = require('ws');
import request = require('request');
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import moment = require("moment");
import util = require("util");
import _ = require('lodash');
import Q = require("q");

var shortId = require("shortid");

interface OrderBook {
    timestamp?: string;        // only present in the http get
	bids : [string, string][];
	asks : [string, string][];
}

interface Ticker {
	id : string;
	amount : number;
	price : number;
}

interface NewOrderAck {
    id: string;
    datetime: string;
    type: number; //  buy or sell (0 - buy; 1 - sell)
    price: number;
    amount: number;
}

interface Transaction {
    datetime: string;
    id: string;
    type: number; //(0 - deposit; 1 - withdrawal; 2 - market trade)
    usd: number;
    btc: number;
    fee: number;
    order_id: number;
}

interface MarketTransaction {
    date: string;
    tid: string;
    price: string;
    type: number;
    amount: string;
}

interface AccountBalance {
    usd_balance: number;
    btc_balance: number;
    usd_reserved: number;
    btc_reserved: number;
    usd_available: number;
    btc_available: number;
    fee: number;
}

class PusherClient<T> {
	private _ws;
	private _log : Utils.Logger;
    private _subscribedEvents = [];
	
	ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
	private onConnectionStatusChange = () => {
        if (this._ws.readyState === WebSocket.OPEN) {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
        }
        else {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
        }
    };
	
	Message = new Utils.Evt<Models.Timestamped<T>>();
	private onMessage = (data) => {
		try {
			var first = JSON.parse(data);
			
			if (!first.hasOwnProperty("event") || !_.contains(this._subscribedEvents, first.event))
				return;
			
			var t = Utils.date();
			this.Message.trigger(new Models.Timestamped(JSON.parse(first['data']), t));
		} catch (error) {
			this._log("Error parsing data", data, error);
		}
	};
	
	constructor(url: string, name: string, endpointsAndEvents: [string, string][]) {
		this._log = Utils.log("tribeca:gateway:"+name);
        
		this._ws = new WebSocket(url);
        
        this._ws
            .on('open', () => {
    			this.onConnectionStatusChange();
                
                var getSubscribeMsg = (channel: string) => {
                    return {'data': {'channel': channel}, 'event': 'pusher:subscribe'};
                }
    			
                endpointsAndEvents.forEach(e =>  {
                    this._subscribedEvents.push(e[1]);
                    this._ws.send(JSON.stringify(getSubscribeMsg(e[0])));
                }); 
    		})
            .on('message', this.onMessage)
            .on("close", (code, msg) => {
                this.onConnectionStatusChange();
                this._log("close code=%d msg=%s", code, msg);
            })
            .on("error", err => {
                this.onConnectionStatusChange();
                this._log("error %s", err);
                throw err;
            });
	}
}

class BitstampMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
	
	private _client : PusherClient<OrderBook | Ticker>;
	
	ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    private onMessage = (msg : Models.Timestamped<OrderBook | Ticker>) => {
        if (msg.data.hasOwnProperty("id")) {
            this.onTicker(<Models.Timestamped<Ticker>>msg);
        }
        else {
            this.onOrderBookMessage(<Models.Timestamped<OrderBook>>msg);
        }
    };
	
	private static ConvertToMarketSide = (input : [string, string]) => 
        new Models.MarketSide(parseFloat(input[0]), parseFloat(input[1]));
        
    private static ConvertToMarketSideList = (input : [string, string][]) => 
        _(input).slice(0, 5).map(BitstampMarketDataGateway.ConvertToMarketSide).value();
        
	private onOrderBookMessage = (message : Models.Timestamped<OrderBook>) => {
		var bids = BitstampMarketDataGateway.ConvertToMarketSideList(message.data.bids);
        var asks = BitstampMarketDataGateway.ConvertToMarketSideList(message.data.asks);
		this.MarketData.trigger(new Models.Market(bids, asks, message.time));
	};
    
    private onTicker = (message : Models.Timestamped<Ticker>) => {
        var trd = new Models.GatewayMarketTrade(message.data.price, message.data.amount,  message.time, false, null);
        this.MarketTrade.trigger(trd);
    };
	
     _log : Utils.Logger = Utils.log("tribeca:gateway:BitstampMD");
    constructor(restClient: BitstampAuthenticatedClient, url: string) {
		this._client = new PusherClient(url, "BitstampPusherClient", [["order_book", "data"], ["live_trades", "trade"]]);
		this._client.ConnectChanged.on(c => this.ConnectChanged.trigger(c));
		this._client.Message.on(this.onMessage);
        
        restClient.getFromEndpoint<OrderBook>("order_book/").then(b => 
            this.onOrderBookMessage(new Models.Timestamped(b, moment.utc())))
                .done();
            
        restClient.getFromEndpoint<MarketTransaction[]>("transactions/").then(ts => {
            ts.forEach(t => {
                var side = t.type === 0 ? Models.Side.Bid : Models.Side.Ask; // is this really the make side?
                var trd = new Models.GatewayMarketTrade(parseFloat(t.price), parseFloat(t.amount), moment(t.date, "X"), true, side);
                this.MarketTrade.trigger(trd);
            });
        }).done();
    }
}

class BitstampAuthenticatedClient {
    private static UserAgent = 'Mozilla/4.0 (compatible; Bitstamp node.js client)';
    
    constructor(
        private baseUrl: string, 
        private _customerId: string, 
        private _apiKey: string, 
        private _secret: string) { }
        
    private _lastTimeMs : number = 0;
    private getNonce = () => {
        var t = new Date().getTime();
        if (t === this._lastTimeMs) {
            this._lastTimeMs++;
        }
        else {
            this._lastTimeMs = t*100;
        }
        
        return this._lastTimeMs;
    };
    
    private addAuthentication = (req: {}) => {
        var nonce = this.getNonce();
        var message = nonce + this._customerId + this._apiKey;
        var signer = crypto.createHmac('sha256', new Buffer(this._secret, 'utf8'));
        var signature = signer.update(message).digest('hex').toUpperCase();
        
        return _.extend({
            key: this._apiKey,
            signature: signature,
            nonce: nonce}, req);
    }; 
    
    // the URLs must end with slashes in their API
    private makeUrl = (endpoint: string) => this.baseUrl + "/" + endpoint;
    
    public postToEndpoint = <TRequest, TResponse>(endpoint: string, req : TRequest) : Q.Promise<TResponse> => {
        return this.requestFromEndpoint<TResponse>({
            url: this.makeUrl(endpoint),
            form: this.addAuthentication(req),
            method: 'POST',
            timeout: 5000,
            headers: { 'User-Agent': BitstampAuthenticatedClient.UserAgent }
        });
    }; 
    
    public getFromEndpoint = <TResponse>(endpoint: string) => {
        return this.requestFromEndpoint<TResponse>({
            url: this.makeUrl(endpoint),
            method: 'GET',
            timeout: 5000,
            headers: { 'User-Agent': BitstampAuthenticatedClient.UserAgent }
        });
    };
    
    private requestFromEndpoint = <TResponse>(options: request.Options) => {
        var defer = Q.defer<TResponse>();
        request(options, (err, resp, body) => {
            if (err) defer.reject(err);
            else defer.resolve(JSON.parse(body));
        });
        return defer.promise;
    };
}

class BitstampEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    public cancelsByClientOrderId = false;

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this._client.postToEndpoint<{}, boolean>("cancel_order/", {id: cancel.exchangeId}).then(ack => {
            if (ack) this.OrderUpdate.trigger({
                exchangeId: cancel.exchangeId, 
                orderStatus: Models.OrderStatus.Cancelled
            });
            else this.OrderUpdate.trigger({
                exchangeId: cancel.exchangeId, 
                orderStatus: Models.OrderStatus.Rejected, 
                cancelRejected: true
            });
        }).done();
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        var side = order.side === Models.Side.Bid ? "buy/" : "sell/";
        this._client.postToEndpoint<{}, NewOrderAck>(side, {amount: order.quantity, price: order.price}).then(ack => {
            this.OrderUpdate.trigger({
                orderId: order.orderId, 
                exchangeId: ack.id, 
                orderStatus: Models.OrderStatus.Working
            });
        }).done();
        return new Models.OrderGatewayActionReport(Utils.date());
    };
    
    generateClientOrderId = () : string => shortId.generate();

    private downloadOrderStatuses = () => {
        // I really have no idea what to do here.
        //this._client.postToEndpoint("order_status/", )
    };

     _log : Utils.Logger = Utils.log("tribeca:gateway:BitstampOE");
    
    constructor(timeProvider : Utils.ITimeProvider, private _client : BitstampAuthenticatedClient) {
        timeProvider.setInterval(this.downloadOrderStatuses, moment.duration(10, "seconds"));
        timeProvider.setTimeout(() => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected), moment.duration(10));
    }
}

class BitstampPositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:BitstampPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
    
    private onRefresh = () => {
        this._client.postToEndpoint<{}, AccountBalance>("balance/", {}).then(ack => {
            this.PositionUpdate.trigger(new Models.CurrencyPosition(ack.usd_balance, ack.usd_reserved, Models.Currency.USD));
            this.PositionUpdate.trigger(new Models.CurrencyPosition(ack.btc_balance, ack.btc_reserved, Models.Currency.BTC));
        });
    };

    constructor(timeProvider : Utils.ITimeProvider, private _client : BitstampAuthenticatedClient) {
        timeProvider.setInterval(this.onRefresh, moment.duration(20, "seconds"));
    }
}

class BitstampBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.Bitstamp;
    }

    // this is provided dynamically via the `balance` request
    makeFee() : number {
        return 0.0025;
    }

    // this is provided dynamically via the `balance` request
    takeFee() : number {
        return 0.0025;
    }

    name() : string {
        return "Bitstamp";
    }
}

export class Bitstamp extends Interfaces.CombinedGateway {
    constructor(timeProvider: Utils.ITimeProvider, config : Config.IConfigProvider) {
        var marketDataUrl = config.GetString("BitstampPusherUrl");
        var httpUrl = config.GetString("BitstampHttpUrl");
        var customerId = config.GetString("BitstampCustomerId");
        var apiKey = config.GetString("BitstampApiKey");
        var secret = config.GetString("BitstampSecret");
        
        var client = new BitstampAuthenticatedClient(httpUrl, customerId, apiKey, secret);
        var orderGateway = new BitstampEntryGateway(timeProvider, client);
        var positionGateway = new BitstampPositionGateway(timeProvider, client);
        var marketDataGateway = new BitstampMarketDataGateway(client, marketDataUrl);
        super(
            marketDataGateway,
            orderGateway,
            positionGateway,
            new BitstampBaseGateway());
    }
}
