/// <reference path="../../typings/tsd.d.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <amd-dependency path="ui.bootstrap"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Pair = require("./pair");

export class DisplayExchangeInformation {
    connected : boolean;
    name : string;
    pairs : Pair.DisplayPair[] = [];

    usdPosition : number;
    btcPosition : number;
    ltcPosition : number;

    private _positionSubscriber : Messaging.ISubscribe<Models.CurrencyPosition>;
    private _connectivitySubscriber : Messaging.ISubscribe<Models.ConnectivityStatus>;

    constructor(private _log : ng.ILogService,
                public exchange : Models.Exchange,
                private _io : any) {

        var makeSubscriber = <T>(topic : string) => {
            var wrappedTopic = Messaging.ExchangePairMessaging.wrapExchangeTopic(exchange, topic);
            return new Messaging.Subscriber<T>(wrappedTopic, _io, _log.info);
        };

        this._positionSubscriber = makeSubscriber(Messaging.Topics.Position)
            .registerDisconnectedHandler(this.clearPosition)
            .registerSubscriber(this.updatePosition, us => us.forEach(this.updatePosition));

        this._connectivitySubscriber = makeSubscriber(Messaging.Topics.ExchangeConnectivity)
            .registerSubscriber(this.setConnectStatus, cs => cs.forEach(this.setConnectStatus));

        this.name = Models.Exchange[exchange];
    }

    public dispose = () => {
        this._positionSubscriber.disconnect();
        this._connectivitySubscriber.disconnect();
        this.pairs.forEach(p => p.dispose());
        this.pairs.length = 0;
    };

    private setConnectStatus = (cs : Models.ConnectivityStatus) => {
        this.connected = cs == Models.ConnectivityStatus.Connected;
    };

    private clearPosition = () => {
        this.usdPosition = null;
        this.btcPosition = null;
        this.ltcPosition = null;
    };

    private updatePosition = (position : Models.CurrencyPosition) => {
        if (position === null) return;
        switch (position.currency) {
            case Models.Currency.BTC:
                this.btcPosition = position.amount;
                break;
            case Models.Currency.USD:
                this.usdPosition = position.amount;
                break;
            case Models.Currency.LTC:
                this.ltcPosition = position.amount;
                break;
        }
    };

    public getOrAddDisplayPair = (pair : Models.CurrencyPair) : Pair.DisplayPair => {
        for (var i = 0; i < this.pairs.length; i++) {
            var p = this.pairs[i];
            if (pair.base === p.pair.base && pair.quote === p.pair.quote) {
                return p;
            }
        }

        this._log.info("adding new pair, base:", Models.Currency[pair.base], "quote:",
            Models.Currency[pair.quote], "to exchange", Models.Exchange[this.exchange]);

        var newPair = new Pair.DisplayPair(this.exchange, pair, this._log, this._io);
        this.pairs.push(newPair);
        return newPair;
    };
}