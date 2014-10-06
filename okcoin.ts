/// <reference path="typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="models.ts" />

var ws = require('ws');

module OkCoin {

    interface OkCoinDepthMessage {
        asks : Array<Array<number>>;
        bids : Array<Array<number>>;
        timestamp : string;
    }

    export class OkCoin implements IGateway {
        cancelOrder(cancel : BrokeredCancel) {
        }

        sendOrder = (order : BrokeredOrder) => {
            // not yet implemented
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

        private onConnect = () => {
            this._ws.send(JSON.stringify({event: 'addChannel', channel: 'ok_btcusd_depth'}));
            //this._ws.send(JSON.stringify({event: 'addChannel', channel: 'ok_usd_realtrades',
            //    parameters: {partner: "partner", secretkey: "secretkey"}}));
            this.ConnectChanged.trigger(ConnectivityStatus.Connected);
        };

        private onMessage = (m : any) => {
            var msg = JSON.parse(m)[0];
            if (msg.channel == "ok_btcusd_depth") {
                this.onDepth(msg.data);
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