/// <reference path="utils.ts" />
/// <reference path="../common/models.ts" />
/// <reference path="../common/messaging.ts" />
/// <reference path="interfaces.ts"/>

import Models = require("../common/models");
import Messaging = require("../common/messaging");
import Utils = require("./utils");
import _ = require("lodash");
import Q = require("q");
import Interfaces = require("./interfaces");
import util = require("util");
import moment = require("moment");

export class ApplicationState {

    private _app_state : Models.ApplicationState = null;

    private onTick = () => {
      this._app_state = new Models.ApplicationState(
        process.memoryUsage().rss,
        moment.utc().hours()
      );
      this._appStatePublisher.publish(this._app_state);
    };

    constructor(private _timeProvider: Utils.ITimeProvider,
                private _appStatePublisher : Messaging.IPublish<Models.ApplicationState>) {
        _timeProvider.setInterval(this.onTick, moment.duration(1, "minutes"));
        this.onTick();

        _appStatePublisher.registerSnapshot(() => [this._app_state]);
    }
}