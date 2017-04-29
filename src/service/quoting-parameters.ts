/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="config.ts" />
/// <reference path="utils.ts" />
/// <reference path="interfaces.ts"/>

import Models = require("../common/models");
import Interfaces = require("./interfaces");
import Messaging = require("../common/messaging");
import _ = require("lodash");
import Utils = require("./utils");
import log from "./logging";

class Repository<T> implements Interfaces.IRepository<T> {
    private _log = log("tribeca:" + this._name);

    NewParameters = new Utils.Evt();

    constructor(
        private _name: string,
        private _validator: (a: T) => boolean,
        private _paramsEqual: (a: T, b: T) => boolean,
        defaultParameter: T,
        private _rec: Messaging.IReceive<T>,
        private _pub: Messaging.IPublish<T>) {
            
        this._log.info("Starting parameter:", defaultParameter);
        _pub.registerSnapshot(() => [this.latest]);
        _rec.registerReceiver(this.updateParameters);
        this._latest = defaultParameter;
    }

    private _latest: T;
    public get latest(): T {
        return this._latest;
    }

    public updateParameters = (newParams: T) => {
        if (this._validator(newParams) && this._paramsEqual(newParams, this._latest)) {
            this._latest = newParams;
            this._log.info("Changed parameters", this.latest);
            this.NewParameters.trigger();
        }

        this._pub.publish(this.latest);
    };
}

export class QuotingParametersRepository extends Repository<Models.QuotingParameters> {
    constructor(pub: Messaging.IPublish<Models.QuotingParameters>,
        rec: Messaging.IReceive<Models.QuotingParameters>,
        initParam: Models.QuotingParameters) {
        super("qpr",
            (p: Models.QuotingParameters) => p.size > 0 || p.width > 0,
            (a: Models.QuotingParameters, b: Models.QuotingParameters) => !_.isEqual(a, b),
            initParam, rec, pub);

    }
}