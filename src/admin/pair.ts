/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="shared_directives.ts"/>

import angular = require("angular");
import Models = require("../common/models");
import io = require("socket.io-client");
import moment = require("moment");
import Messaging = require("../common/messaging");
import Shared = require("./shared_directives");

class FormViewModel<T> {
    master: T;
    display: T;
    pending: boolean = false;
    connected: boolean = false;

    constructor(defaultParameter: T,
        private _sub: Messaging.ISubscribe<T>,
        private _fire: Messaging.IFire<T>,
        private _submitConverter: (disp: T) => T = null) {
        if (this._submitConverter === null)
            this._submitConverter = d => d;
            
        _sub.registerConnectHandler(() => this.connected = true)
            .registerDisconnectedHandler(() => this.connected = false)
            .registerSubscriber(this.update, us => us.forEach(this.update));
            
        this.connected = _sub.connected;
        this.master = angular.copy(defaultParameter);
        this.display = angular.copy(defaultParameter);
    }

    public reset = () => {
        this.display = angular.copy(this.master);
    };

    public update = (p: T) => {
        console.log("updating parameters", p);
        this.master = angular.copy(p);
        this.display = angular.copy(p);
        this.pending = false;
    };

    public submit = () => {
        this.pending = true;
        this._fire.fire(this._submitConverter(this.display));
    };
}

class QuotingButtonViewModel extends FormViewModel<boolean> {
    constructor(sub: Messaging.ISubscribe<boolean>,
        fire: Messaging.IFire<boolean>) {
        super(false, sub, fire, d => !d);
    }

    public getClass = () => {
        if (this.pending) return "btn btn-warning";
        if (this.display) return "btn btn-success";
        return "btn btn-danger";
    }
}

class DisplayQuotingParameters extends FormViewModel<Models.QuotingParameters> {
    availableQuotingModes = [];
    availableFvModels = [];
    availableAutoPositionModes = [];

    constructor(sub: Messaging.ISubscribe<Models.QuotingParameters>,
        fire: Messaging.IFire<Models.QuotingParameters>) {
        super(new Models.QuotingParameters(null, null, null, null, null, null, null, null, null, null, null, null, null, null, null, null), sub, fire);

        this.availableQuotingModes = DisplayQuotingParameters.getMapping(Models.QuotingMode);
        this.availableFvModels = DisplayQuotingParameters.getMapping(Models.FairValueModel);
        this.availableAutoPositionModes = DisplayQuotingParameters.getMapping(Models.AutoPositionMode);
    }

    private static getMapping<T>(enumObject: T) {
        var names = [];
        for (var mem in enumObject) {
            if (!enumObject.hasOwnProperty(mem)) continue;
            var val = parseInt(mem, 10);
            if (val >= 0) {
                names.push({ 'str': enumObject[mem], 'val': val });
            }
        }
        return names;
    }
}

export class DisplayPair {
    name: string;
    connected: boolean;

    active: QuotingButtonViewModel;
    quotingParameters: DisplayQuotingParameters;

    private _subscribers: Messaging.ISubscribe<any>[] = [];

    constructor(public scope: ng.IScope,
        subscriberFactory: Shared.SubscriberFactory,
        fireFactory: Shared.FireFactory) {

        var setConnectStatus = (cs: Models.ConnectivityStatus) => {
            this.connected = cs == Models.ConnectivityStatus.Connected;
        };

        var connectivitySubscriber = subscriberFactory.getSubscriber(scope, Messaging.Topics.ExchangeConnectivity)
            .registerSubscriber(setConnectStatus, cs => cs.forEach(setConnectStatus));
        this._subscribers.push(connectivitySubscriber);

        var activeSub = subscriberFactory.getSubscriber(scope, Messaging.Topics.ActiveChange);
        this.active = new QuotingButtonViewModel(
            activeSub,
            fireFactory.getFire(Messaging.Topics.ActiveChange)
            );
        this._subscribers.push(activeSub);

        var qpSub = subscriberFactory.getSubscriber(scope, Messaging.Topics.QuotingParametersChange);
        this.quotingParameters = new DisplayQuotingParameters(
            qpSub,
            fireFactory.getFire(Messaging.Topics.QuotingParametersChange)
            );
        this._subscribers.push(qpSub);
    }

    public dispose = () => {
        console.log("dispose client");
        this._subscribers.forEach(s => s.disconnect());
    };

    public updateParameters = (p: Models.QuotingParameters) => {
        this.quotingParameters.update(p);
    };
}