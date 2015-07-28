/// <reference path="../../../typings/tsd.d.ts" />
/// <reference path="../utils.ts" />
/// <reference path="../../common/models.ts" />
/// <reference path="nullgw.ts" />

import Config = require("../config");
import crypto = require('crypto');
import request = require('request');
import url = require("url");
import querystring = require("querystring");
import NullGateway = require("./nullgw");
import Models = require("../../common/models");
import Utils = require("../utils");
import Interfaces = require("../interfaces");
import moment = require("moment");
import util = require("util");
var shortId = require("shortid");

class BtcEMarketDataGateway implements Interfaces.IMarketDataGateway {
    MarketData = new Utils.Evt<Models.Market>();
    MarketTrade = new Utils.Evt<Models.MarketSide>();

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();
    
     _log : Utils.Logger = Utils.log("tribeca:gateway:BtcEMD");
    constructor(config : Config.IConfigProvider) {
        
    }
}

class BtcEOrderEntryGateway implements Interfaces.IOrderEntryGateway {
    OrderUpdate = new Utils.Evt<Models.OrderStatusReport>();
    
    public cancelsByClientOrderId = false;

    cancelOrder = (cancel : Models.BrokeredCancel) : Models.OrderGatewayActionReport => {
        
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    replaceOrder = (replace : Models.BrokeredReplace) : Models.OrderGatewayActionReport => {
        this.cancelOrder(new Models.BrokeredCancel(replace.origOrderId, replace.orderId, replace.side, replace.exchangeId));
        return this.sendOrder(replace);
    };

    sendOrder = (order : Models.BrokeredOrder) : Models.OrderGatewayActionReport => {
        
        return new Models.OrderGatewayActionReport(Utils.date());
    };

    ConnectChanged = new Utils.Evt<Models.ConnectivityStatus>();

    generateClientOrderId = () => {
        return shortId.generate();
    }

     _log : Utils.Logger = Utils.log("tribeca:gateway:BtcEOE");
    constructor(config : Config.IConfigProvider) {
    }
}

class BtcEPositionGateway implements Interfaces.IPositionGateway {
    _log : Utils.Logger = Utils.log("tribeca:gateway:BtcEPG");
    PositionUpdate = new Utils.Evt<Models.CurrencyPosition>();
	
    constructor(config : Config.IConfigProvider) {
    }
}

class BtcEBaseGateway implements Interfaces.IExchangeDetailsGateway {
    public get hasSelfTradePrevention() {
        return false;
    }

    exchange() : Models.Exchange {
        return Models.Exchange.BtcE;
    }

    makeFee() : number {
        return -0.0001;
    }

    takeFee() : number {
        return 0.001;
    }

    name() : string {
        return "BtcE";
    }
}

export class BtcE extends Interfaces.CombinedGateway {
    constructor(config : Config.IConfigProvider) {
        super(
            new BtcEMarketDataGateway(config),
            new BtcEOrderEntryGateway(config),
            new BtcEPositionGateway(config),
            new BtcEBaseGateway());
    }
}
