/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import util = require("util");
import express = require("express");
import request = require('request');

export interface IValuationGateway {
    ValueChanged : Utils.Evt<number>;
}

export class WinkdexGateway implements IValuationGateway {
    ValueChanged = new Utils.Evt<number>();

    private poll = () => {
        request.get(
            {url: "https://winkdex.com/api/v0/price"},
            (err, body, resp) => {
                var price = JSON.parse(body).price / 100.0;
                this.ValueChanged.trigger(price);
            });
    };

    constructor() {
        setInterval(this.poll, 60*1000);
    }
}

export class ExternalValuationSource {
    private _log : Utils.Logger = Utils.log("tribeca:evs");

    ValueChanged = new Utils.Evt<number>();
    private _value : number = null;
    public get Value() { return this._value; }

    private handleValueChanged = (v : number) => {
        if (this._value !== v) {
            this._value = v;
            this.ValueChanged.trigger(v);
            this._pub.publish(new Models.ExternalValuationUpdate(this.Value, Utils.date(), this._source));
            this._log("%s value changed: %d", Models.ExternalValuationSource[this._source], this.Value)
        }
    };

    constructor(private _gateway : IValuationGateway,
                private _source : Models.ExternalValuationSource,
                private _pub : Messaging.IPublish<Models.ExternalValuationUpdate>) {
        _gateway.ValueChanged.on(this.handleValueChanged);
    }
}