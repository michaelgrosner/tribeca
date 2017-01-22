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

    private _app_state: Models.ApplicationState = null;
    private _notepad: string = null;
    private _toggleConfigs: boolean = true;

    private onTick = () => {
      this._app_state = new Models.ApplicationState(
        process.memoryUsage().rss,
        moment.utc().hours()
      );
      this._appStatePublisher.publish(this._app_state);
    };

    constructor(private _timeProvider: Utils.ITimeProvider,
                private _appStatePublisher : Messaging.IPublish<Models.ApplicationState>,
                private _notepadPublisher : Messaging.IPublish<string>,
                private _changeNotepadReciever : Messaging.IReceive<string>,
                private _toggleConfigsPublisher : Messaging.IPublish<boolean>,
                private _toggleConfigsReciever : Messaging.IReceive<boolean>) {
        _timeProvider.setInterval(this.onTick, moment.duration(69, "seconds"));
        this.onTick();

        _appStatePublisher.registerSnapshot(() => [this._app_state]);

        _notepadPublisher.registerSnapshot(() => [this._notepad]);

        _toggleConfigsPublisher.registerSnapshot(() => [this._toggleConfigs]);

        _changeNotepadReciever.registerReceiver((notepad: string) => {
            this._notepad = notepad;
        });

        _toggleConfigsReciever.registerReceiver((toggleConfigs: boolean) => {
            this._toggleConfigs = toggleConfigs;
        });
    }
}