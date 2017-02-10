import Models = require("../share/models");
import Publish = require("./publish");
import Utils = require("./utils");
import _ = require("lodash");
import Q = require("q");
import Interfaces = require("./interfaces");
import QuotingParameters = require("./quoting-parameters");
import util = require("util");
import moment = require("moment");

export class ApplicationState {

  public io: SocketIO.Server;
  private _delayed: any[] = [];
  private _app_state: Models.ApplicationState = null;
  private _notepad: string = null;
  private _toggleConfigs: boolean = true;
  private _tradesMinute: number = 0;
  private _tick: number = 0;
  private _ioDelay: number = 0;
  private _interval = null;

  private onTick = () => {
    this._tick = 0;
    this._app_state = new Models.ApplicationState(
      process.memoryUsage().rss,
      moment.utc().hours(),
      this._tradesMinute
    );
    this._tradesMinute = 0;
    this._appStatePublisher.publish(this._app_state);
  };

  private onDelay = () => {
    this._tick += this._ioDelay;
    if (this._tick>=6e1) this.onTick();
    if (this.io === null) return;
    let orders: any[] = this._delayed.filter(x => x[0]===Models.Prefixes.MESSAGE+Models.Topics.OrderStatusReports);
    if (orders.length) this._delayed.push([Models.Prefixes.MESSAGE+Models.Topics.OrderStatusReports, {data:_.map(orders, x => x[1])}]);
    this._delayed.forEach(x => this.io.emit(x[0], x[1]));
    this._delayed = orders;
  };

  public delay = (prefix: string, topic: string, msg: any) => {
    let isOSR: boolean = topic === Models.Topics.OrderStatusReports;
    if (isOSR && msg.data[1] === Models.OrderStatus.New) return ++this._tradesMinute;
    if (!this._ioDelay) return this.io ? this.io.emit(prefix + topic, msg) : null;
    this._delayed = this._delayed.filter(x => x[0] !== prefix+topic || (isOSR?x[1].data[0] !== msg.data[0]:false));
    if (!isOSR || msg.data[1] === Models.OrderStatus.Working) this._delayed.push([prefix+topic, msg]);
  };

  private setDelay = () => {
    this._ioDelay = this._qlParamRepo.latest.delayUI;
  };

  private setTick = () => {
    if (this._interval) clearInterval(this._interval);
    if (this._ioDelay<1) this._ioDelay = 0;
    this._delayed = [];
    this._interval = this._timeProvider.setInterval(
      this._ioDelay ? this.onDelay : this.onTick,
      moment.duration(this._ioDelay || 6e1, "seconds")
    );
    this.onTick();
  };

  constructor(
    private _timeProvider: Utils.ITimeProvider,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
    private _appStatePublisher : Publish.IPublish<Models.ApplicationState>,
    private _notepadPublisher : Publish.IPublish<string>,
    private _changeNotepadReciever : Publish.IReceive<string>,
    private _toggleConfigsPublisher : Publish.IPublish<boolean>,
    private _toggleConfigsReciever : Publish.IReceive<boolean>
  ) {
    this.setDelay();
    this.setTick();
    _qlParamRepo.NewParameters.on(() => {
      this.setDelay();
      this.setTick();
    });

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