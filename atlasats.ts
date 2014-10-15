/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

module AtlasAts {

    var Faye = require('faye');
    var request = require("request");
    var crypto = require('crypto');

    interface AtlasAtsQuote {
        id : string;
        mm : string;
        price : number;
        symbol : string;
        side : string;
        size : number;
        currency : string
    }

    interface AtlasAtsMarketUpdate {
        symbol : string;
        currency : string;
        bidsize : number;
        bid : number;
        asksize : number;
        ask : number;
        quotes : Array<AtlasAtsQuote>;
    }

    interface AtlasAtsOrder {
        action: string;
        item: string;
        currency: string;
        side: string;
        quantity: number;
        type: string;
        price: number;
        clid: string;
    }

    interface AtlasAtsCancelOrder {
        action: string;
        oid: string;
    }

    export class AtlasAts implements IGateway {
        MarketData : Evt<MarketBook> = new Evt<MarketBook>();
        ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();
        OrderUpdate : Evt<GatewayOrderStatusReport> = new Evt<GatewayOrderStatusReport>();

        name() : string {
            return "AtlasAts";
        }

        makeFee() : number {
            return -0.001;
        }

        takeFee() : number {
            return 0.002;
        }

        exchange() : Exchange {
            return Exchange.AtlasAts;
        }

        _account : string = "1352";
        _secret : string = "4f3b057d936f6363fd4e021795f7681e2107cbb99175a0bbf94b25ec9b125fa9";
        _token : string = "7446ed9575be15cd41f08905282d2935d38ca2d4";
        _nounce : number = 1;
        private signMessage(channel : string, msg : any) {
            var inp : string = [this._token, this._nounce, channel, JSON.stringify(msg)].join(":");
            var signature : string = crypto.createHmac('sha256', this._secret).update(inp).digest('hex');
            var sign = {ident: {key: this._token, signature: signature, nounce: this._nounce}};
            this._nounce += 1;
            return sign;
        }

        sendOrder(order : BrokeredOrder) {
            var o : AtlasAtsOrder = {
                action: "order:create",
                item: "BTC",
                currency: "USD",
                side: order.side == Side.Bid ? "BUY" : "SELL",
                quantity: order.quantity,
                type: order.type == OrderType.Limit ? "limit" : "market",
                price: order.price,
                clid: order.orderId
            };

            this._client.publish("/actions", o);

            var rpt : GatewayOrderStatusReport = {
                orderId: order.orderId,
                orderStatus: OrderStatus.New,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        }

        replaceOrder(replace : BrokeredReplace) {
        }

        cancelOrder(cancel : BrokeredCancel) {
            var c : AtlasAtsCancelOrder = {
                action: "order:cancel",
                oid: cancel.requestId
            };

            this._client.publish("/actions", c);

            var rpt : GatewayOrderStatusReport = {
                orderId: cancel.requestId,
                orderStatus: OrderStatus.PendingCancel,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        }

        private onMarketData = (rawMsg : string) => {
            var msg : AtlasAtsMarketUpdate = JSON.parse(rawMsg);
            if (msg.symbol != "BTC" || msg.currency != "USD") return;

            var bids : AtlasAtsQuote[] = [];
            var asks : AtlasAtsQuote[] = [];
            for (var i = 0; i < msg.quotes.length; i++) {
                var qt = msg.quotes[i];
                if (bids.length > 2 && qt.side == "BUY") continue;
                if (bids.length > 2 && asks.length > 2) break;
                if (qt.side == "BUY") bids.push(qt);
                if (qt.side == "SELL") asks.push(qt);
            }

            var getUpdate = (n : number) => {
                var bid = new MarketSide(bids[n].price, bids[n].size);
                var ask = new MarketSide(asks[n].price, asks[n].size);
                return new MarketUpdate(bid, ask, new Date());
            };

            var b = new MarketBook(getUpdate(0), getUpdate(1), Exchange.AtlasAts);
            this.MarketData.trigger(b);
        };

        _log : Logger = log("Hudson:Gateway:AtlasAts");
        _client : any;

        constructor() {
            this._client = new Faye.Client('https://atlasats.com/api/v1/streaming');
            this._client.addExtension({
                outgoing: (msg, cb) => {
                    msg.ext = this.signMessage("/actions", msg);
                    this._log("sending outgoing", msg);
                    cb(msg);
                }
            });

            this._client.subscribe("/account/"+this._account+"/orders", this._log);
            this._client.subscribe("/market", this.onMarketData);
            this._client.on('transport:up', () => this.ConnectChanged.trigger(ConnectivityStatus.Connected));
            this._client.on('transport:down', () => this.ConnectChanged.trigger(ConnectivityStatus.Disconnected));

            request.get({
                url: "https://atlasats.com/api/v1/market/book",
                qs: {item: "BTC", currency: "USD"}
            }, (er, resp, body) => this.onMarketData(body));
        }
    }
}