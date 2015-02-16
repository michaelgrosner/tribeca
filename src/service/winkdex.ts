/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="../../typings/tsd.d.ts" />

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import Interfaces = require("./interfaces");
import _ = require("lodash");
import util = require("util");
import express = require("express");
import request = require('request');

export interface IValuationGateway {
    Source : Models.ExternalValuationSource;
    ValueChanged : Utils.Evt<number>;
}

export class WinkdexGateway implements IValuationGateway {
    private _log : Utils.Logger = Utils.log("tribeca:winkdex");

    public get Source() { return Models.ExternalValuationSource.Winkdex }
    ValueChanged = new Utils.Evt<number>();

    private poll = () => {
        request.get(
            {url: "https://winkdex.com/api/v0/price"},
            (err, body, res) => {
                if (err) {
                    this._log("error trying to get winkdex value %s", err);
                }
                else {
                    try {
                        var price = JSON.parse(res).price / 100.0;
                        this.ValueChanged.trigger(price);
                    }
                    catch (e) {
                        this._log("error handling winkdex value %s %j %j", e.stack, body, res);
                    }
                }
            });
    };

    constructor() {
        setInterval(this.poll, 60*1000);
        this.poll();
    }
}

export class ExternalValuationSource {
    private _log : Utils.Logger = Utils.log("tribeca:evs");

    ValueChanged = new Utils.Evt<Models.ExternalValuationUpdate>();
    private _value : Models.ExternalValuationUpdate = null;
    public get Value() { return this._value; }

    private handleValueChanged = (v : number) => {
        if (this.Value === null || this._value.value !== v) {
            var update = new Models.ExternalValuationUpdate(v, Utils.date(), this._gateway.Source);
            this._value = update;
            this.ValueChanged.trigger(update);
            this._pub.publish(update);
            this._log("%s value changed: %j", Models.ExternalValuationSource[this._gateway.Source], this.Value)
        }
    };

    constructor(private _gateway : IValuationGateway,
                private _pub : Messaging.IPublish<Models.ExternalValuationUpdate>) {
        _gateway.ValueChanged.on(this.handleValueChanged);
    }
}