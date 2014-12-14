/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("./config");
import crypto = require('crypto');
import io = require("socket.io-client");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../common/models");
import Utils = require("./utils");
import Interfaces = require("./interfaces");

interface GroupOrderLevel {
    price : number;
    type : string;
    totalamount : number;
}

interface GroupOrderPayload {
    market : string;
    ask : GroupOrderLevel[];
    bid : GroupOrderLevel[];
}

interface GroupOrderWrapper {
    grouporder : GroupOrderPayload;
}

class BtcChinaOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    sendOrder(order : Models.BrokeredOrder) : Models.OrderGatewayActionReport {
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    cancelOrder(cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport {
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    replaceOrder(replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport {
        return new Models.OrderGatewayActionReport(Utils.date());
    }

    constructor(socket : any) {
        //socket.emit('subscribe', 'grouporder_btcltc');
    }
}

class BtcChinaPositionGateway implements Interfaces.IPositionGateway {
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();

    constructor(config : Config.IConfigProvider) {}
}

class BtcChinaMarketDataGateway implements Interfaces.IMarketDataGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:BtcChinaMD");

    MarketData = new Utils.Evt<Models.MarketUpdate>();
    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    private static convertGroupOrderToMarketSide(go : GroupOrderLevel) {
        return new Models.MarketSide(go.price, go.totalamount);
    }

    private onGroupOrderData = (data : GroupOrderWrapper) => {
        var t = Utils.date();
        try {
            var topAsk = BtcChinaMarketDataGateway.convertGroupOrderToMarketSide(data.grouporder.ask[0]);
            var topBid = BtcChinaMarketDataGateway.convertGroupOrderToMarketSide(data.grouporder.bid[0]);
            this.MarketData.trigger(new Models.MarketUpdate(topBid, topAsk, t));
        }
        catch (e) {
            this._log("Caught exception (%s) on data %j", e, data);
        }
    };

    constructor(socket : any) {
        socket.emit('subscribe', 'grouporder_btcltc');
        socket.on('grouporder', this.onGroupOrderData);

        socket.on('connect', () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Connected));
        socket.on('disconnect', () => this.ConnectChanged.trigger(Models.ConnectivityStatus.Disconnected));
    }
}

class BtcChinaBaseGateway implements Interfaces.IExchangeDetailsGateway {
    name() : string {
        return "BtcChina";
    }

    makeFee() : number {
        return 0;
    }

    takeFee() : number {
        return 0;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.BtcChina;
    }
}

export class BtcChina extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        var socket = io('https://websocket.btcchina.com/');

        var orderGateway = config.GetString("BtcChinaOrderDestination") == "BtcChina" ?
            <Interfaces.IOrderEntryGateway>new BtcChinaOrderEntryGateway(socket)
            : new NullGateway.NullOrderGateway();

        var positionGateway = config.environment() == Config.Environment.Dev ?
            new NullGateway.NullPositionGateway() :
            new BtcChinaPositionGateway(config);

        super(
            new BtcChinaMarketDataGateway(socket),
            orderGateway,
            positionGateway,
            new BtcChinaBaseGateway());
    }
}