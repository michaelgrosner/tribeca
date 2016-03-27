/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
///<reference path="interfaces.ts"/>
///<reference path="persister.ts"/>

import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import Interfaces = require("./interfaces");
import Persister = require("./persister");
import util = require("util");
import Models = require("../common/models");

export class MessagesPubisher implements Interfaces.IPublishMessages {
    private _storedMessages : Models.Message[] = [];

    constructor(private _timeProvider: Utils.ITimeProvider,
                private _persister : Persister.IPersist<Models.Message>,
                initMsgs : Models.Message[],
                private _wrapped : Messaging.IPublish<Models.Message>) {
        _.forEach(initMsgs, m => this._storedMessages.push(m));
        _wrapped.registerSnapshot(() => _.takeRight(this._storedMessages, 50));
    }

    public publish = (text : string) => {
        var message = new Models.Message(text, this._timeProvider.utcNow());
        this._wrapped.publish(message);
        this._persister.persist(message);
        this._storedMessages.push(message);
    };
}