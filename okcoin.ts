/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

var ws = require('ws');
var crypto = require("crypto");

module OkCoin {

    interface OkCoinDepthMessage {
        asks : Array<Array<number>>;
        bids : Array<Array<number>>;
        timestamp : string;
    }

    export class OkCoin implements IGateway {
        OrderUpdate : Evt<GatewayOrderStatusReport> = new Evt<GatewayOrderStatusReport>();

        private signMsg = m => {
            var els : string[] = [];
            for (var key in m) {
                if (m.hasOwnProperty(key))
                    els.push(key + "=" + m[key]);
            }

            return crypto.createHmac('md5', this._secretKey).update(els.join("&"));
        };

        private sendSigned = msg => {
            msg.sign = this.signMsg(msg);
            this._ws.send(JSON.stringify(msg));
        };

        cancelOrder(cancel : BrokeredCancel) {
            var c = {partner: this._partner, order_id: cancel.requestId, symbol: "btc_usd", sign: null};
            this.sendSigned(c);

            var rpt : GatewayOrderStatusReport = {
                orderId: cancel.requestId,
                orderStatus: OrderStatus.PendingCancel,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        }

        sendOrder = (order : BrokeredOrder) => {
            // https://www.okcoin.com/api/trade.do
            var o = {partner: this._partner, symbol: "btc_usd", type: order.side == Side.Bid ? "buy" : "sell",
                     rate: order.price, amount: order.quantity, sign: null};

            this.sendSigned(o);

            var rpt : GatewayOrderStatusReport = {
                orderId: order.orderId,
                orderStatus: OrderStatus.New,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        };

        makeFee() : number {
            return -0.0005;
        }

        takeFee() : number {
            return 0.002;
        }

        name() : string {
            return "OkCoin";
        }

        ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();
        MarketData : Evt<MarketBook> = new Evt<MarketBook>();
        _ws : any;
        _log : Logger = log("OkCoin");

        _partner : number = 2013015;
        _secretKey : string = "75AB165AD31EB279A6EBEE709734A6C1";
        private onConnect = () => {
            this._ws.send(JSON.stringify({event: 'addChannel', channel: 'ok_btcusd_depth'}));
            this._ws.send(JSON.stringify({event: 'addChannel', channel: 'ok_usd_realtrades',
                    parameters: {partner: this._partner.toString(), secretkey: this._secretKey}}));
            this.ConnectChanged.trigger(ConnectivityStatus.Connected);
        };

        private onMessage = (m : any) => {
            var msg = JSON.parse(m)[0];
            if (msg.channel == "ok_btcusd_depth") {
                this.onDepth(msg.data);
            }
            else if (msg.channel == "ok_usd_realtrades") {
                this._log("ok_usd_realtrades", msg);
            }
            else {
                this._log("UNKNOWN", msg);
            }
        };

        private onDepth = (msg : OkCoinDepthMessage) => {
            function getLevel(n : number) : MarketUpdate {
                return {bidPrice: msg.bids[n][0], bidSize: msg.bids[n][1],
                    askPrice: msg.asks[n][0], askSize: msg.asks[n][1],
                    time: new Date(parseInt(msg.timestamp))};
            }

            var book : MarketBook = {top: getLevel(0), second: getLevel(1), exchangeName: Exchange.OkCoin};
            this.MarketData.trigger(book);
        };

        constructor() {
            this._ws = new ws("wss://real.okcoin.com:10440/websocket/okcoinapi");
            this._ws.on("open", this.onConnect);
            this._ws.on("message", this.onMessage);
        }
    }
}