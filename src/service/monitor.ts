import Models = require("../common/models");
import Publish = require("./publish");
import Utils = require("./utils");
import _ = require("lodash");
import Q = require("q");
import Interfaces = require("./interfaces");
import util = require("util");
import moment = require("moment");

export class ApplicationState {

    public io: SocketIO.Server;
    private _delayed: any[] = [];
    private _app_state: Models.ApplicationState = null;
    private _notepad: string = null;
    private _toggleConfigs: boolean = true;

    private onTick = () => {
      this._app_state = new Models.ApplicationState(
        process.memoryUsage().rss,
        moment.utc().hours(),
        this._tradesMinute
      );
      this._tradesMinute = 0;
      this._appStatePublisher.publish(this._app_state);
    };

    private _tick: number = 0;
    private onSnap = () => {
      if (++this._tick>=69) {
        this._tick = 0;
        this.onTick();
      }
      if (this.io === null) return;
      this._delayed.forEach(x => this.io.emit(x[0], x[1]));
      this._delayed = [];
    };

    private _tradesMinute: number = 0;

    public delay = (prefix: string, topic: string, msg: any) => {
      if (topic === Models.Topics.OrderStatusReports) {
        if (msg.data[1] === Models.OrderStatus.New) return ++this._tradesMinute;
        this._delayed = this._delayed.filter(x => x[0] !== prefix+topic || x[1].data[0] !== msg.data[0]);
      } else this._delayed = this._delayed.filter(x => x[0] !== prefix+topic);
      this._delayed.push([prefix+topic, msg]);
    };

    constructor(private _timeProvider: Utils.ITimeProvider,
                private _appStatePublisher : Publish.IPublish<Models.ApplicationState>,
                private _notepadPublisher : Publish.IPublish<string>,
                private _changeNotepadReciever : Publish.IReceive<string>,
                private _toggleConfigsPublisher : Publish.IPublish<boolean>,
                private _toggleConfigsReciever : Publish.IReceive<boolean>) {
        _timeProvider.setInterval(this.onSnap, moment.duration(1, "seconds"));
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