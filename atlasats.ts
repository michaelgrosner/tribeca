/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

module AtlasAts {

    var Faye = require('faye');
    var request = require("request");
    var crypto = require('crypto');

    // example order reject:
    //      {"limit":0.01,"reject":{"reason":"risk_buying_power"},"tif":"GTC","status":"REJECTED","type":"LIMIT","currency":"USD","executed":0,"clid":"WEB","side":"BUY","oid":"1352-101614-000056-004","item":"BTC","account":1352,"quantity":0.01,"left":0,"average":0}

    // example order fill:
    //      {"limit":390,"tif":"GTC","status":"OPEN","ack":{"oref":"4696TJEGPJYZA0","time":"2014-10-16 00:12:35"},"type":"LIMIT","currency":"USD","executed":0,"clid":"WEB","side":"SELL","oid":"1352-101614-001825-009","item":"BTC","account":1352,"quantity":0.0001,"left":0.0001,"average":0}
    //      {"executions":[{"liquidity":"R","time":"2014-10-16 00:12:35","price":392.47,"quantity":0.0001,"venue":"CROX","commission":-0.00007849400000000001,"eid":"4696TJEGPJYZA02_4696TJEGPJZ0"}],"limit":390,"tif":"GTC","status":"DONE","ack":{"oref":"4696TJEGPJYZA0","time":"2014-10-16 00:12:35"},"type":"LIMIT","currency":"USD","executed":0.0001,"clid":"WEB","side":"SELL","oid":"1352-101614-001825-009","item":"BTC","account":1352,"quantity":0.0001,"left":0,"average":392.47}

    interface AtlasAtsExecutionReportReject {
        reason : string;
    }
    interface AtlasAtsExecutionReport {
        limit : string;
        reject? : AtlasAtsExecutionReportReject;
        tif : string;
        status : string;
        type : string;
        currency : string;
        executed : number;
        clid: string;
        side : string;
        oid : string;
        item : string;
        account : string;
        quantity : number;
        left : number;
        average : number;
    }

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
        _secret : string = "d61eb29445f7a72a83fbc056b1693c962eb97524918f1e9e2d10b6965c16c8c7";
        _token : string = "0e48f9bd6f8dec728df2547b7a143e504a83cb2d";
        _simpleToken : string = "9464b821cea0d62939688df750547593";
        _nounce : number = 1;
        private signMessage(channel : string, msg : any) {
            var inp : string = [this._token, this._nounce, channel, 'data' in msg ? JSON.stringify(msg['data']) : ''].join(":");
            var signature : string = crypto.createHmac('sha256', this._secret).update(inp).digest('hex').toString().toUpperCase();
            var sign = {ident: {key: this._token, signature: signature, nounce: this._nounce}};
            this._log(inp);
            this._nounce += 1;
            return sign;
        }

        sendOrder = (order : BrokeredOrder) => {
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

            request({
                url: "https://atlasats.com/api/v1/orders",
                body: JSON.stringify(o),
                headers: {"Authorization": "Token token=\""+this._simpleToken+"\"", "Content-Type": "application/json"},
                method: "POST"
            }, (err, resp, body) => {
                this._log("Status", resp.statusCode);
                this._log(body);
            });

            //this._client.publish("/actions", o);

            var rpt : GatewayOrderStatusReport = {
                orderId: order.orderId,
                orderStatus: OrderStatus.New,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        };

        replaceOrder = (replace : BrokeredReplace) => {
            this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side));
            this.sendOrder(replace);
        };

        cancelOrder = (cancel : BrokeredCancel) => {
            var c : AtlasAtsCancelOrder = {
                action: "order:cancel",
                oid: cancel.requestId
            };

            request({
                url: "https://atlasats.com/api/v1/orders/"+cancel.requestId,
                headers: {"Authorization": "Token token=\""+this._simpleToken+"\""},
                method: "DELETE"
            }, (err, resp, body) => {
                this._log("Status", resp.statusCode);
                this._log(body);
            });

            //this._client.publish("/actions", c);

            var rpt : GatewayOrderStatusReport = {
                orderId: cancel.requestId,
                orderStatus: OrderStatus.PendingCancel,
                time: new Date()
            };
            this.OrderUpdate.trigger(rpt);
        };

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

        private onExecRpt = (rawMsg : string) => {
            var msg
        };

        _log : Logger = log("Hudson:Gateway:AtlasAts");
        _client : any;

        constructor() {
            this._client = new Faye.Client('https://atlasats.com/api/v1/streaming', {
                endpoints: {
                    websocket: 'wss://atlasats.com/api/v1/streaming'
                }
            });

            this._client.addExtension({
                outgoing: (msg, cb) => {
                    if (msg.channel != '/meta/handshake') {
                        msg.ext = this.signMessage(msg.channel, msg);
                    }
                    cb(msg);
                }
            });

            this._client.subscribe("/account/"+this._account+"/orders", m => this._log("acct", m));
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