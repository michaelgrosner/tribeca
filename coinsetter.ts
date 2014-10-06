/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

var request = require("request");
var io = require('socket.io-client');

module Coinsetter {

    interface CoinsetterDepthSide {
        price : number;
        size : number;
        exchangeId : string;
        timeStamp : number;
    }

    interface CoinsetterDepth {
        bid : CoinsetterDepthSide;
        ask : CoinsetterDepthSide;
    }

    interface CoinsetterNewOrder {
        customerUuid : string;
        accountUuid : string;
        symbol : string;
        side : string;
        orderType : string;
        requestedQuantity : number;
        routingMethod : number;
        requestedPrice : number;
        clientOrderId : string;
    }

    export class Coinsetter implements IGateway {
        OrderUpdate : Evt<GatewayOrderStatusReport> = new Evt<GatewayOrderStatusReport>();
        cancelOrder(cancel : BrokeredCancel) {
        }

        _customerUuid : string;
        _accountUuid : string;
        _clientSessionId : string;

        sendOrder = (order : BrokeredOrder) => {
            var getSide = (side : Side) => {
                switch (side) {
                    case Side.Bid:
                        return "BUY";
                    case Side.Ask:
                        return "SELL";
                    default:
                        throw new Error("Side " + Side[side] + " not supported in Coinsetter");
                }
            };

            var getType = (t : OrderType) => {
                switch (t) {
                    case OrderType.Limit:
                        return "LIMIT";
                    case OrderType.Market:
                        return "MARKET";
                    default:
                        throw new Error("OrderType " + OrderType[t] + " not supported in Coinsetter");
                }
            };

            var csOrder : CoinsetterNewOrder = {
                customerUuid: this._customerUuid,
                accountUuid: this._accountUuid,
                symbol: "BTCUSD",
                side: getSide(order.side),
                orderType: getType(order.type),
                requestedQuantity: order.quantity,
                routingMethod: 1,
                requestedPrice: order.price,
                clientOrderId: order.orderId
            };

            var options = {uri: "https://api.coinsetter.com/v1/order",
                headers: "coinsetter-client-session-id:" + this._clientSessionId,
                method: "POST",
                json: csOrder};
            request(options, (err, resp, body) => null);
        };

        makeFee() : number {
            return 0.0025;
        }

        takeFee() : number {
            return 0.0025;
        }

        name() : string {
            return "Coinsetter";
        }

        ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();
        MarketData : Evt<MarketBook> = new Evt<MarketBook>();
        _socket : any;
        _log : Logger = log("Coinsetter");

        private onConnect = () => {
            this._log("Connected to Coinsetter");
            this._socket.emit("depth room", "");
            this.ConnectChanged.trigger(ConnectivityStatus.Connected);
        };

        private onDepth = (msg : Array<CoinsetterDepth>) => {
            function getLevel(n : number) : MarketUpdate {
                return {bidPrice: msg[n].bid.price, bidSize: msg[n].bid.size,
                    askPrice: msg[n].ask.price, askSize: msg[n].ask.size,
                    time: new Date(msg[n].bid.timeStamp)};
            }

            var book : MarketBook = {top: getLevel(0), second: getLevel(1), exchangeName: Exchange.Coinsetter};
            this.MarketData.trigger(book);
        };

        constructor() {
            this._socket = io.connect('https://plug.coinsetter.com:3000');
            this._socket.on("connect", this.onConnect);
            this._socket.on("depth", this.onDepth);
        }
    }
}