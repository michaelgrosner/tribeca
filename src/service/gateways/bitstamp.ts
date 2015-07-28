/// <reference path="../../../typings/tsd.d.ts" />
/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("../config");
import crypto = require('crypto');
import ws = require('ws');
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
	bids : [number, number][];
	asks : [number, number][];
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
    datetime: string,
    id: string,
    type: number, //(0 - deposit; 1 - withdrawal; 2 - market trade)
    usd: number,
    btc: number;
    fee: number;
    order_id: number;
}

class PusherClient<T> {
	private _ws;
	private _log : Utils.Logger;
	
	ConnectionState : Models.ConnectivityStatus = Models.ConnectivityStatus.Disconnected;
	ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
	private onConnectionStatusChange = () => {
        if (this._ws.readyState === WebSocket.OPEN) {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
			this.ConnectionState = Models.ConnectivityStatus.Connected;
        }
        else {
            this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
			this.ConnectionState = Models.ConnectivityStatus.Disconnected;
        }
    };
	
	Message = new Utils.Evt<Models.Timestamped<T>>();
	private onMessage = (data) => {
		try {
			var first = JSON.parse(data);
			
			if (!first.hasOwnProperty("data"))
				return;
			
			var t = Utils.date();
			this.Message.trigger(new Models.Timestamped(JSON.parse(first['data']), t));
		} catch (error) {
			this._log("Error parsing data", data, error);
		}
	};
	
	constructor(url: string, endpoint: string) {
		this._log = Utils.log("tribeca:gateway:"+endpoint);
		this._ws = new WebSocket(url);
		
		this._ws.on('open', () => {
			this.onConnectionStatusChange();
			
			this._ws.send(JSON.stringify({'data': {'channel': endpoint}, 'event': 'pusher:subscribe'}));
		});
        this._ws.on('message', this.onMessage);
        this._ws.on("close", (code, msg) => {
            this.onConnectionStatusChange();
            this._log("close code=%d msg=%s", code, msg);
        });
        this._ws.on("error", err => {
            this.onConnectionStatusChange();
            this._log("error %s", err);
            throw err;
        });
	}
}

class BitstampMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.GatewayMarketTrade>();
	
	private _orderBookClient : PusherClient<OrderBook>;
	private _tickerWsClient : PusherClient<Ticker>;
	
	ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
	private onConnectChanged = () => {
		if (this._orderBookClient.ConnectionState === Models.ConnectivityStatus.Connected 
				&& this._tickerWsClient.ConnectionState === Models.ConnectivityStatus.Connected) {
			this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected);
		}
		else {
			this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected);
		}
	};
	
	private static ConvertToMarketSide = (input : [number, number]) => new Models.MarketSide(input[0], input[1]);
	private onOrderBookMessage = (message : Models.Timestamped<OrderBook>) => {
		var bids = message.data.bids.map(BitstampMarketDataGateway.ConvertToMarketSide);
        var asks = message.data.asks.map(BitstampMarketDataGateway.ConvertToMarketSide);
		this.MarketData.trigger(new Models.Market(bids, asks, message.time));
	};
    
    private onTicker = (message : Models.Timestamped<Ticker>) => {
        var trd = new Models.GatewayMarketTrade(message.data.price, message.data.amount,  message.time, false, null);
        this.MarketTrade.trigger(trd);
    };
	
     _log : Utils.Logger = Utils.log("tribeca:gateway:BitstampMD");
    constructor(config : Config.IConfigProvider) {
		var url = config.GetString("BitstampPusherUrl");
		this._orderBookClient = new PusherClient(url, "order_book");
		this._orderBookClient.ConnectChanged.on(this.onConnectChanged);
		this._orderBookClient.Message.on(this.onOrderBookMessage);
		
		this._tickerWsClient = new PusherClient(url, "live_trades");
		this._tickerWsClient.ConnectChanged.on(this.onConnectChanged);
        this._tickerWsClient.Message.on(this.onTicker);
    }
}

class BitstampAuthenticatedClient {
    private _nonce;
    
    constructor(private baseUrl: string, private _apiKey: string, private _secret: string) {
        
    }
    
    public sendAuthenticated = <TRequest, TResponse>(endpoint: string, req : TRequest) : Q.Promise<TResponse> => {
        var defer = Q.defer<TResponse>();
        request.post(this.baseUrl + "/" + endpoint, {}, (err, resp, body) => {
            if (err) defer.reject(err);
            else defer.resolve(<TResponse>(<any>resp));
        });
        return defer.promise;
    }; 
}

class BitstampEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
    public cancelsByClientOrderId = false;

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        this._client.sendAuthenticated<{}, boolean>("cancel_order", {id: cancel.exchangeId}).then(ack => {
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
        var side = order.side === Models.Side.Bid ? "buy" : "sell";
        this._client.sendAuthenticated<{}, NewOrderAck>(side, {amount: order.quantity, price: order.price}).then(ack => {
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
        this._client.sendAuthenticated("order_status", )
    };

     _log : Utils.Logger = Utils.log("tribeca:gateway:BitstampOE");
    
    constructor(private _client : BitstampAuthenticatedClient, config : Config.IConfigProvider) {
        
    }
}

class BitstampPositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:BitstampPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    constructor(client : BitstampAuthenticatedClient, config : Config.IConfigProvider) {
        
    }
}

class BitstampBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.Bitstamp;
    }

    makeFee() : number {
        return 0.0025;
    }

    takeFee() : number {
        return 0.0025;
    }

    name() : string {
        return "Bitstamp";
    }
}

export class Bitstamp extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var client = new BitstampAuthenticatedClient(config);
        var orderGateway = new BitstampEntryGateway(client, config);
        var positionGateway = new BitstampPositionGateway(client, config);
        var marketDataGateway = new BitstampMarketDataGateway(config);
        super(
            marketDataGateway,
            orderGateway,
            positionGateway,
            new BitstampBaseGateway());
    }
}
