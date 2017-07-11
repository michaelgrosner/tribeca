import Models = require("../share/models");
import Publish = require("./publish");
import QuotingParameters = require("./quoting-parameters");
import path = require('path');
import fs = require('fs');

export class ApplicationState {

  private _delayed: any[] = [];
  private _app_state: Models.ApplicationState;
  private _notepad: string;
  private _toggleConfigs: boolean = true;
  private _newOrderMinute: number = 0;
  private _tick: number = 0;
  private _ioDelay: number = 0;
  private _interval = null;

  private onTick = () => {
    this._tick = 0;
    this._app_state = new Models.ApplicationState(
      process.memoryUsage().rss,
      (new Date()).getHours(),
      this._newOrderMinute / 2,
      this._sqlite.size()
    );
    this._newOrderMinute = 0;
    this._publisher.publish(Models.Topics.ApplicationState, this._app_state);
  };

  private onDelay = () => {
    this._tick += this._ioDelay;
    if (this._tick>=6e1) this.onTick();
    let orders: any[] = this._delayed.filter(x => x[0]===Models.Prefixes.MESSAGE+Models.Topics.OrderStatusReports);
    this._delayed = this._delayed.filter(x => x[0]!==Models.Prefixes.MESSAGE+Models.Topics.OrderStatusReports);
    if (orders.length) this._delayed.push([Models.Prefixes.MESSAGE+Models.Topics.OrderStatusReports, {data: orders.map(x => x[1])}]);
    this._delayed.forEach(x => this._publisher.send(x[0], x[1]));
    this._delayed = orders.filter(x => x[1].data[1]===Models.OrderStatus.Working);
  };

  public delay = (prefix: string, topic: string, msg: any) => {
    const isOSR: boolean = topic === Models.Topics.OrderStatusReports;
    if (isOSR && msg.data[1] === Models.OrderStatus.New) return ++this._newOrderMinute;
    if (!this._ioDelay) return this._publisher.send(prefix + topic, msg);
    this._delayed = this._delayed.filter(x => x[0] !== prefix+topic || (isOSR?x[1].data[0] !== msg.data[0]:false));
    this._delayed.push([prefix+topic, msg]);
  };

  private setDelay = () => {
    this._ioDelay = this._qlParamRepo.latest.delayUI;
  };

  private setTick = () => {
    if (this._interval) clearInterval(this._interval);
    if (this._ioDelay<1) this._ioDelay = 0;
    this._delayed = [];
    this._interval = setInterval(
      this._ioDelay ? this.onDelay : this.onTick,
      (this._ioDelay || 6e1) * 1e3
    );
    this.onTick();
  };

  constructor(
    private _sqlite,
    private _qlParamRepo: QuotingParameters.QuotingParametersRepository,
    private _publisher: Publish.Publisher,
    private _evOn
  ) {
    this.setDelay();
    this.setTick();
    this._evOn('QuotingParameters', () => {
      this.setDelay();
      this.setTick();
    });

    _publisher.registerSnapshot(Models.Topics.ApplicationState, () => [this._app_state]);

    _publisher.registerSnapshot(Models.Topics.Notepad, () => [this._notepad]);

    _publisher.registerSnapshot(Models.Topics.ToggleConfigs, () => [this._toggleConfigs]);

    _publisher.registerReceiver(Models.Topics.Notepad, (notepad: string) => {
      this._notepad = notepad;
    });

    _publisher.registerReceiver(Models.Topics.ToggleConfigs, (toggleConfigs: boolean) => {
      this._toggleConfigs = toggleConfigs;
    });
  }
}