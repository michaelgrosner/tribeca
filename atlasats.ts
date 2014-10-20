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

    // example cancel ack:
    //      {"limit":350,"tif":"GTC","status":"DONE","ack":{"oref":"4696TJEGPKHMA0","time":"2014-10-16 01:08:30"},"urout":{"time":"2014-10-16 01:08:38"},"type":"LIMIT","currency":"USD","executed":0,"clid":"WEB","side":"BUY","oid":"1352-101614-011421-012","item":"BTC","account":1352,"quantity":0.0001,"left":0,"average":0}

    interface AtlasAtsExecutionReportReject {
        reason : string;
    }
    interface AtlasAtsAck {
        oref : string;
        time : string;
    }
    interface AtlasAtsExecutions {
        liquidity : string;
        time : string;
        price : number;
        quantity : number;
        venue : string;
        commission : string;
        eid : string;
    }
    interface AtlasAtsExecutionReport {
        limit : string;
        reject? : AtlasAtsExecutionReportReject;
        ack? : AtlasAtsAck;
        executions? : AtlasAtsExecutions[];
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

    class AtlasAtsSocket {
        _client : any;
        _secret : string = "d61eb29445f7a72a83fbc056b1693c962eb97524918f1e9e2d10b6965c16c8c7";
        _token : string = "0e48f9bd6f8dec728df2547b7a143e504a83cb2d";
        _nounce : number = 1;

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
        }

        private signMessage(channel : string, msg : any) {
            var inp : string = [this._token, this._nounce, channel, 'data' in msg ? JSON.stringify(msg['data']) : ''].join(":");
            var signature : string = crypto.createHmac('sha256', this._secret).update(inp).digest('hex').toString().toUpperCase();
            var sign = {ident: {key: this._token, signature: signature, nounce: this._nounce}};
            this._nounce += 1;
            return sign;
        }

        send = (msg : string) : void => {
            this._client.send(msg);
        };

        on = (channel : string, handler: () => void) => {
            this._client.on(channel, raw => handler());
        };

        subscribe<T>(channel : string, handler: (newMsg : T) => void) {
            this._client.subscribe(channel, raw => handler(JSON.parse(raw)));
        }
    }

    class AtlasAtsBaseGateway implements IGateway {
        ConnectChanged : Evt<ConnectivityStatus> = new Evt<ConnectivityStatus>();

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

        constructor(socket : AtlasAtsSocket) {
            socket.on('transport:up', () => this.ConnectChanged.trigger(ConnectivityStatus.Connected));
            socket.on('transport:down', () => this.ConnectChanged.trigger(ConnectivityStatus.Disconnected));
        }
    }

    class AtlasAtsOrderEntryGateway implements IOrderEntryGateway {
        _log : Logger = log("Hudson:Gateway:AtlasAtsOE");
        OrderUpdate : Evt<OrderStatusReport> = new Evt<OrderStatusReport>();
        _simpleToken : string = "9464b821cea0d62939688df750547593";
        _account : string = "1352";

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
                this.onExecRpt(body);
            });
        };

        replaceOrder = (replace : BrokeredReplace) => {
            this.cancelOrder(new BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
            this.sendOrder(replace);
        };

        cancelOrder = (cancel : BrokeredCancel) => {
            request({
                url: "https://atlasats.com/api/v1/orders/"+cancel.exchangeId,
                headers: {"Authorization": "Token token=\""+this._simpleToken+"\""},
                method: "DELETE"
            }, (err, resp, body) => {
                this._log("cxl-resp", err, body);
                var msg = JSON.parse(body);

                if (!err && msg.status !== "error") {
                    var rpt : OrderStatusReport = {
                        orderId: cancel.clientOrderId,
                        orderStatus: OrderStatus.Cancelled,
                        time: new Date()
                    };
                    this.OrderUpdate.trigger(rpt);
                } else {
                    var rpt : OrderStatusReport = {
                        orderId: cancel.clientOrderId,
                        orderStatus: OrderStatus.CancelRejected,
                        rejectMessage: msg.message,
                        time: new Date()
                    };
                    this.OrderUpdate.trigger(rpt);
                }
            });
        };

        private static getStatus = (raw : string) : OrderStatus => {
            switch (raw) {
                case "DONE":
                    // either cancelled or filled
                    return OrderStatus.Filled;
                case "REJECTED":
                    return OrderStatus.Rejected;
                case "PENDING":
                case "OPEN":
                    return OrderStatus.Working;
            }
        };

        private static getLiquidity = (raw : string) : Liquidity => {
            switch (raw) {
                case "A":
                    return Liquidity.Make;
                case "T":
                    return Liquidity.Take;
                default:
                    throw new Error("unknown liquidity " + raw);
            }
        };

        private onExecRpt = (msg : AtlasAtsExecutionReport) => {
            this._log("EXEC RPT", msg);

            var status : OrderStatusReport = {
                exchangeId: msg.oid,
                orderId: msg.clid,
                orderStatus: AtlasAtsOrderEntryGateway.getStatus(msg.status),
                time: new Date(), // doesnt give milliseconds??
                rejectMessage: msg.hasOwnProperty("reject") ? msg.reject.reason : null,
                leavesQuantity: msg.left,
                cumQuantity: msg.executed,
                averagePrice: msg.average,
                lastQuantity: msg.hasOwnProperty("executions") ? msg.executions[0].quantity : null,
                liquidity: msg.hasOwnProperty("executions") ? AtlasAtsOrderEntryGateway.getLiquidity(msg.executions[0].liquidity) : null
            };

            this.OrderUpdate.trigger(status);
        };

        constructor(socket : AtlasAtsSocket) {
            socket.subscribe("/account/"+this._account+"/orders", this.onExecRpt);
        }
    }

    class AtlasAtsMarketDataGateway implements IMarketDataGateway {
        MarketData : Evt<MarketBook> = new Evt<MarketBook>();

        private onMarketData = (msg : AtlasAtsMarketUpdate) => {
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

        constructor(socket : AtlasAtsSocket) {
            socket.subscribe("/market", this.onMarketData);

            request.get({
                url: "https://atlasats.com/api/v1/market/book",
                qs: {item: "BTC", currency: "USD"}
            }, (er, resp, body) => this.onMarketData(JSON.parse(body)));
        }
    }

    export class AtlasAts extends CombinedGateway {
        constructor() {
            var socket = new AtlasAtsSocket();
            super(
                new AtlasAtsMarketDataGateway(socket),
                new NullOrderGateway(), //new AtlasAtsOrderEntryGateway(socket),
                new AtlasAtsBaseGateway(socket));
        }
    }
}